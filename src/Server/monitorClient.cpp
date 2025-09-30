#include "monitorClient.hpp"
#include "../Socket/socket.hpp"

#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <ctime>
#include <sstream>


#define CHUNK_SIZE 8192

monitorClient::monitorClient(sock serverSockets) : ServerConfig(serverSockets.getConfig()) {

    std::vector<int> serverFDs = serverSockets.getFDs();

    pollfd serverPollFd;
    for (size_t i = 0; i < serverFDs.size(); i++) {
        memset(&serverPollFd, 0, sizeof(serverPollFd));
        serverPollFd.fd = serverFDs[i];
        serverPollFd.events = POLLIN;
        fds.push_back(serverPollFd);
        // Informational log
        std::ostringstream ss;
        ss << "[INFO] " << "Server: listening socket added (fd=" << serverFDs[i] << ")";
        std::cout << ss.str() << std::endl;
    }
    this->numberOfServers = serverFDs.size();
}

void monitorClient::acceptNewClient(int serverFD) {
    int clientFd = accept(serverFD, NULL, NULL);
    if (clientFd == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        throw monitorexception("[ERROR] accept fail: " + std::string(strerror(errno)));
    }
    
    if (clientFd < 0) {
        std::cerr << "Invalid client file descriptor: " << clientFd << std::endl;
        return;
    }
    
    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
        close(clientFd);
        throw monitorexception("[ERROR]: fcntl fail: " + std::string(strerror(errno)));
    }

    pollfd serverPollFd;
    memset(&serverPollFd, 0, sizeof(serverPollFd));
    serverPollFd.fd = clientFd; 
    serverPollFd.events = POLLIN;
    serverPollFd.revents = 0;
    
    try {
        fds.push_back(serverPollFd);
        SocketTracker st;
        std::pair<TrackerIt, bool> result = this->fdsTracker.insert(
            std::pair<int, SocketTracker>(clientFd, st)
        );
        
        if (!result.second) {
            std::cerr << "Warning: Client " << clientFd << " already exists in tracker" << std::endl;
            fds.pop_back();
            close(clientFd);
            return;
        }
        
    // Log accepted connection with client FD and server FD
    std::ostringstream ss;
    ss << "[INFO] " << "Server (listen_fd=" << serverFD << ") accepted new connection (client_fd=" << clientFd << ")";
    std::cout << ss.str() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception while adding client " << clientFd << ": " << e.what() << std::endl;
        close(clientFd);
        throw;
    }
}

void monitorClient::removeClient(int index) {
    if (index < 0 || index >= static_cast<int>(fds.size()) || index < static_cast<int>(numberOfServers)) {
        std::cerr << "Invalid index " << index << " for removeClient (size: " << fds.size() << ", servers: " << numberOfServers << ")" << std::endl;
        return;
    }
    
    int clientFd = fds[index].fd;
    close(clientFd);
    fds.erase(fds.begin() + index);
    fdsTracker.erase(clientFd);
    
    std::cout << "Client " << clientFd << " removed from index " << index << std::endl;
}

void monitorClient::startEventLoop() {
    int ready = 0;
    initializeTimeouts();
    
    while (1) {   
        time_t now = time(NULL);
        if (shouldCheckTimeouts(now)) {
            checkTimeouts();
            lastTimeoutCheck = now;
        }
        
        ready = poll(fds.data(), fds.size(), 100);
        if (ready == -1) {
            if (errno == EINTR) continue;
            throw monitorexception("[ERROR] poll fail: " + std::string(strerror(errno)));
        }
        
        for (size_t i = 0; i < numberOfServers; i++) {
            if (fds[i].revents & POLLIN) {
                acceptNewClient(fds[i].fd);
            }
        }
        
        for (int i = static_cast<int>(fds.size()) - 1; i >= static_cast<int>(numberOfServers); i--) {
            if (i >= static_cast<int>(fds.size())) continue;
            
            pollfd& currentFd = fds[i];
            
            // Handle incoming data (POLLIN)
            if (currentFd.revents & POLLIN) {
                // If there's already a fully-parsed request and a pending response,
                // avoid reading further from this socket until the response is sent.
                // This prevents the reactor from reading pipelined bytes while the
                // response is not yet written out.
                std::map<int, SocketTracker>::iterator it_check = fdsTracker.find(currentFd.fd);
                if (it_check != fdsTracker.end()) {
                    if (it_check->second.request_obj.isComplete() && !it_check->second.response.empty()) {
                        // Ensure POLLOUT is enabled so we can continue writing the response
                        currentFd.events |= POLLOUT;
                        // Skip reading for now
                        continue;
                    }
                }

                (void)readClientRequest(currentFd.fd);
                updateClientActivity(currentFd.fd);

                std::map<int, SocketTracker>::iterator it = fdsTracker.find(currentFd.fd);
                if (it != fdsTracker.end()) {
                    // Generate response if request complete and none queued yet
                    if (it->second.request_obj.isComplete()) {
                        if (!it->second.error.empty()) {
                            generateErrorResponse(it->second);
                        } else {
                            generateSuccessResponse(it->second);
                        }
                    }
                    // Arm writer if we have any response data queued (pipelining supported)
                    if (!it->second.response.empty()) {
                        currentFd.events |= POLLOUT;
                    }
                }
            }
            
            // Handle outgoing data (POLLOUT) - WRITE RESPONSE HERE
            if (currentFd.revents & POLLOUT) {
                int wr = writeClientResponse(currentFd.fd);

                // If write is still pending (partial or EAGAIN), keep POLLOUT enabled
                if (wr == 1) {
                    currentFd.events |= POLLOUT;
                    continue;
                }

                // On fatal write error, remove client
                if (wr == -1) {
                    removeClient(i);
                    continue;
                }

                // wr == 0 -> response fully written
                std::map<int, SocketTracker>::iterator it = fdsTracker.find(currentFd.fd);
                if (it == fdsTracker.end()) continue;

                // Determine Connection header (case-insensitive value check)
                const std::string connHdr = it->second.request_obj.getHeader("connection");
                std::string connVal = connHdr;
                for (size_t k = 0; k < connVal.size(); ++k) connVal[k] = std::tolower(connVal[k]);

                if (connVal == "close" || it->second.WError || it->second.RError) {
                    removeClient(i);
                } else {
                    // Keep-alive: clear request-specific state and return to POLLIN
                    it->second.response.clear();
                    it->second.raw_buffer.clear();
                    // keep any pipelined bytes in raw_buffer so next request can be parsed
                    it->second.request_obj.reset();
                    it->second.headersParsed = false;
                    it->second.consumedBytes = 0;
                    it->second.error.clear();
                    it->second.WError = 0;
                    it->second.RError = 0;
                    currentFd.events = POLLIN;
                }
            }
        }
    }
}

monitorClient::~monitorClient() {
    std::cout << "close all fds \n";
    for (PollFdsIt it = fds.begin(); it != fds.end(); it++) {
        if (it->fd > 0)
            close(it->fd);
    }
}

monitorClient::monitorexception::monitorexception(std::string msg) {
    this->ErrorMsg = msg;
}

const char *monitorClient::monitorexception::what() const throw() {
    return ErrorMsg.c_str();
}

monitorClient::monitorexception::~monitorexception() throw() {}

monitorClient::SocketTracker::SocketTracker() 
    : headersParsed(false), consumedBytes(0), WError(0), RError(0), lastActive(time(NULL)) {
    raw_buffer = "";
    response = "";
    error = "";
}


bool monitorClient::shouldCheckTimeouts(time_t currentTime) {
    return (currentTime - lastTimeoutCheck >= TIMEOUT_CHECK_INTERVAL);
}

void monitorClient::SocketTracker::updateActivity() {
    lastActive = time(NULL);
}

bool monitorClient::SocketTracker::hasTimedOut(time_t currentTime, time_t timeoutSeconds) const {
    return (currentTime - lastActive) > timeoutSeconds;
}

void monitorClient::checkTimeouts() {
    time_t now = time(NULL);
    // Print a per-client debug line including the client FD and current time
    for (size_t i = numberOfServers; i < fds.size(); i++) {
        int clientFd = fds[i].fd;
        std::ostringstream ss;
        ss << "[DEBUG] " << "Checking client fd=" << clientFd << " for timeout at " << std::ctime(&now);
        std::cout << ss.str();

        TrackerIt it = fdsTracker.find(clientFd);

        if (it != fdsTracker.end() && it->second.hasTimedOut(now, CLIENT_TIMEOUT)) {
            std::ostringstream ss2;
            ss2 << "[WARN] " << "Client fd=" << clientFd << " timed out after " << (now - it->second.lastActive) << " seconds";
            std::cout << ss2.str() << std::endl;
            
            it->second.RError = 408;
            it->second.WError = 1;
            std::string timeoutHtml = "<html><body><h1>408 Request Timeout</h1>"
                                     "<p>The server timed out waiting for the Request.</p></body></html>";
            it->second.response = "HTTP/1.1 408 Request Timeout\r\n";
            it->second.response += "Connection: close\r\n";
            it->second.response += "Content-Type: text/html\r\n";
            {
                std::ostringstream tmp;
                tmp << timeoutHtml.length();
                it->second.response += "Content-Length: " + tmp.str() + "\r\n\r\n";
            }
            it->second.response += timeoutHtml;
            fds[i].events = POLLOUT;
        }
    }
    lastTimeoutCheck = now;
}

void monitorClient::initializeTimeouts() {
    lastTimeoutCheck = time(NULL);
}

void monitorClient::updateClientActivity(int clientFd) {
    TrackerIt it = fdsTracker.find(clientFd);
    if (it != fdsTracker.end()) {
        it->second.updateActivity();
    }
}



#include "monitorClient.hpp"
#include "../Socket/socket.hpp"
#include "../HTTP/DirectoryListing.hpp"
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <fcntl.h>


#define CHUNK_SIZE 8192

monitorClient::monitorClient(sock serverSockets) : ServerConfig(serverSockets.getConfig()) {

    std::vector<int> serverFDs = serverSockets.getFDs();

    pollfd serverPollFd;
    for (size_t i = 0; i < serverFDs.size(); i++) {
        memset(&serverPollFd, 0, sizeof(serverPollFd));
        std::cout << "socket add == [ " << serverFDs[i] << " ]\n";
        serverPollFd.fd = serverFDs[i];
        serverPollFd.events = POLLIN;
        fds.push_back(serverPollFd);
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
        
        std::cout << "Server " << serverFD << " accepted new connection " << clientFd << std::endl;
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
                int keepAlive = readClientRequest(currentFd.fd);
                updateClientActivity(currentFd.fd);
                
                std::map<int, SocketTracker>::iterator it = fdsTracker.find(currentFd.fd);
                if (it != fdsTracker.end()) {
                    if (it->second.request_obj.isComplete() && it->second.response.empty()) {
                        // Request is complete, generate response based on matched server config
                        if (!it->second.error.empty()) {
                            generateErrorResponse(it->second);
                        } else {
                            generateSuccessResponse(it->second);
                        }
                        
                        // Switch to write mode
                        currentFd.events |= POLLOUT;
                    }
                }
                
                if (keepAlive == 0) {
                    removeClient(i);
                    continue;
                }
            }
            
            // Handle outgoing data (POLLOUT) - WRITE RESPONSE HERE
            if (currentFd.revents & POLLOUT) {
                writeClientResponse(currentFd.fd);
                
                // Check if response is complete
                std::map<int, SocketTracker>::iterator it = fdsTracker.find(currentFd.fd);
                if (it != fdsTracker.end() && it->second.response.empty()) {
                    // Response sent completely, switch back to reading
                    currentFd.events = POLLIN;
                    
                    // If connection should be closed, remove client
                    if (it->second.WError || it->second.RError) {
                        removeClient(i);
                    }
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
    : WError(0), RError(0), lastActive(time(NULL)) {
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
    std::cout << "Checking for timed out clients..." << std::endl;
    time_t now = time(NULL);
    
    for (size_t i = numberOfServers; i < fds.size(); i++) {
        int clientFd = fds[i].fd;
        TrackerIt it = fdsTracker.find(clientFd);

        if (it != fdsTracker.end() && it->second.hasTimedOut(now, CLIENT_TIMEOUT)) {
            std::cout << "Client " << clientFd << " timed out after " 
                      << (now - it->second.lastActive) << " seconds" << std::endl;
            
            it->second.RError = 408;
            it->second.WError = 1;
            std::string timeoutHtml = "<html><body><h1>408 Request Timeout</h1>"
                                     "<p>The server timed out waiting for the Request.</p></body></html>";
            it->second.response = "HTTP/1.1 408 Request Timeout\r\n";
            it->second.response += "Connection: close\r\n";
            it->second.response += "Content-Type: text/html\r\n";
            it->second.response += "Content-Length: " + std::to_string(timeoutHtml.length()) + "\r\n\r\n";
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



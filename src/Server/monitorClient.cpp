#include "monitorClient.hpp"
#include "../Socket/socket.hpp"
#include "../CGI/CGIHandler.hpp"

#include <unistd.h>
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
        std::cerr << "[ERROR] accept failed for serverFD=" << serverFD << std::endl;
        return;
    }
    
    if (clientFd < 0) {
        std::cerr << "Invalid client file descriptor: " << clientFd << std::endl;
        return;
    }
    
    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
        std::cerr << "[ERROR] fcntl failed for clientFd=" << clientFd << std::endl;
        close(clientFd);
        return;
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
        return;
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
            perror("[ERROR] poll fail");
            throw monitorexception("[ERROR] poll fail");
        }
        
        for (size_t i = 0; i < numberOfServers; i++) {
            if (fds[i].revents & POLLIN) {
                acceptNewClient(fds[i].fd);
            }
        }
        
        for (int i = static_cast<int>(fds.size()) - 1; i >= static_cast<int>(numberOfServers); i--) {
            if (i >= static_cast<int>(fds.size())) continue;
            
            pollfd& currentFd = fds[i];
            
            // If this client is currently waiting on a CGI, skip reading from it
            TrackerIt waitCgiIt = fdsTracker.find(currentFd.fd);
            if (waitCgiIt != fdsTracker.end() && waitCgiIt->second.isCgiRequest && waitCgiIt->second.cgiOutputFd >= 0) {
                // Ensure we don't read from this socket while CGI is running
                currentFd.events &= ~POLLIN;
            }
            
            // Check if this is a CGI output fd
            bool isCgiFd = false;
            int clientFdForCgi = -1;
            for (TrackerIt tit = fdsTracker.begin(); tit != fdsTracker.end(); ++tit) {
                if (tit->second.isCgiRequest && tit->second.cgiOutputFd == currentFd.fd) {
                    isCgiFd = true;
                    clientFdForCgi = tit->first;
                    break;
                }
            }
            
            // Also handle POLLHUP/POLLERR to finalize short-lived scripts that close quickly
            if (isCgiFd && (currentFd.revents & (POLLIN | POLLHUP | POLLERR))) {
                // Handle CGI output
                TrackerIt cgiTracker = fdsTracker.find(clientFdForCgi);
                if (cgiTracker != fdsTracker.end() && cgiTracker->second.cgiHandler) {
                    // Any activity from the CGI should keep the client connection alive
                    updateClientActivity(clientFdForCgi);
                    int cgiStatus = cgiTracker->second.cgiHandler->processCGIOutput();
                    
                    if (cgiStatus == 0) {
                        // CGI completed successfully
                        std::cout << "[CGI] CGI completed for client " << clientFdForCgi << std::endl;
                        
                        // Remove CGI fd from poll
                        fds.erase(fds.begin() + i);
                        
                        // Generate response from CGI result
                        CGIHandler::Result result = cgiTracker->second.cgiHandler->getResult();
                        std::ostringstream resp;
                        resp << "HTTP/1.1 " << result.status_code << " " << result.status_text << "\r\n";
                        bool hasConn = false;
                        size_t bodyLen = result.body.size();
                        for (std::map<std::string,std::string>::const_iterator hit = result.headers.begin(); 
                             hit != result.headers.end(); ++hit) {
                            if (hit->first == "Connection") hasConn = true;
                            resp << hit->first << ": " << hit->second << "\r\n";
                        }
                        // Add Content-Length if CGI didn't provide it (so we can close immediately)
                        if (result.headers.find("Content-Length") == result.headers.end()) {
                            resp << "Content-Length: " << bodyLen << "\r\n";
                        }
                        // Force close to avoid 60s keep-alive wait after CGI
                        if (!hasConn) resp << "Connection: close\r\n";
                        resp << "\r\n" << result.body;
                        cgiTracker->second.response = resp.str();
                        
                        // Clean up CGI state
                        delete cgiTracker->second.cgiHandler;
                        cgiTracker->second.cgiHandler = NULL;
                        cgiTracker->second.isCgiRequest = false;
                        cgiTracker->second.cgiOutputFd = -1;
                        // Mark activity on completion to avoid any race with timeout checker
                        updateClientActivity(clientFdForCgi);
                        
                        // Enable POLLOUT for client to send response
                        for (size_t j = numberOfServers; j < fds.size(); j++) {
                            if (fds[j].fd == clientFdForCgi) {
                                fds[j].events |= POLLOUT;
                                break;
                            }
                        }
                    } else if (cgiStatus == -1) {
                        // CGI error or timeout
                        std::cout << "[CGI] CGI failed for client " << clientFdForCgi << std::endl;
                        
                        // Remove CGI fd from poll
                        fds.erase(fds.begin() + i);
                        
                        // Generate error response
                        CGIHandler::Result result = cgiTracker->second.cgiHandler->getResult();
                        std::ostringstream resp;
                        resp << "HTTP/1.1 " << result.status_code << " " << result.status_text << "\r\n";
                        // Ensure minimal headers
                        resp << "Content-Type: text/html\r\n";
                        resp << "Content-Length: " << result.body.size() << "\r\n";
                        resp << "Connection: close\r\n\r\n";
                        resp << result.body;
                        cgiTracker->second.response = resp.str();
                        
                        // Clean up CGI state
                        delete cgiTracker->second.cgiHandler;
                        cgiTracker->second.cgiHandler = NULL;
                        cgiTracker->second.isCgiRequest = false;
                        cgiTracker->second.cgiOutputFd = -1;
                        // Mark activity to reflect we produced a response
                        updateClientActivity(clientFdForCgi);
                        
                        // Enable POLLOUT for client to send response
                        for (size_t j = numberOfServers; j < fds.size(); j++) {
                            if (fds[j].fd == clientFdForCgi) {
                                fds[j].events |= POLLOUT;
                                // Mark write error so loop will close client after sending
                                cgiTracker->second.WError = 1;
                                break;
                            }
                        }
                    }
                    // cgiStatus == 1 means still reading, continue polling
                }
                continue;
            }
            
            // Handle incoming data (POLLIN) - regular client socket
            if (currentFd.revents & POLLIN) {
                // If there's already a fully-parsed request and a pending response,
                // avoid reading further from this socket until the response is sent.
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
                            // Detect CGI before generating normal response
                            std::string scriptPath, interpreterPath;
                            if (shouldHandleAsCGI(it->second, scriptPath, interpreterPath)) {
                                if (!it->second.isCgiRequest) {
                                    it->second.isCgiRequest = true;
                                    startAsyncCGI(it->second, currentFd.fd, scriptPath, interpreterPath);
                                    // Stop reading from client while CGI is running
                                    currentFd.events &= ~POLLIN;
                                }
                                // Do not generate response now; wait for CGI completion
                            } else {
                                generateSuccessResponse(it->second);
                            }
                        }
                    }
                    // Arm writer if we have any response data queued (pipelining supported)
                    if (!it->second.response.empty()) {
                        currentFd.events |= POLLOUT;
                    } else {
                        // Ensure we are not arming POLLOUT when CGI is running
                        if (it->second.isCgiRequest) {
                            // Keep events as POLLIN; CGI pipe fd will trigger POLLIN when ready
                            currentFd.events &= ~POLLOUT;
                        }
                    }
                }
            }
            
            // Handle outgoing data (POLLOUT) - WRITE RESPONSE HERE
            if (currentFd.revents & POLLOUT) {
                int wr = writeClientResponse(currentFd.fd);

                // If write is still pending (partial write), keep POLLOUT enabled
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
    : headersParsed(false), consumedBytes(0), WError(0), RError(0), lastActive(time(NULL)),
      isCgiRequest(false), cgiOutputFd(-1), cgiHandler(NULL) {
    raw_buffer = "";
    response = "";
    error = "";
}

monitorClient::SocketTracker::~SocketTracker() {
    if (cgiHandler) {
        delete cgiHandler;
        cgiHandler = NULL;
    }
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

        // Skip timeout enforcement while a CGI is actively running for this client
        if (it != fdsTracker.end() && it->second.isCgiRequest) {
            continue;
        }

        if (it != fdsTracker.end() && it->second.hasTimedOut(now, CLIENT_TIMEOUT)) {
            std::ostringstream ss2;
            ss2 << "[WARN] " << "Client fd=" << clientFd << " timed out after " << (now - it->second.lastActive) << " seconds";
            std::cout << ss2.str() << std::endl;
            
            // Decide whether to emit 408 or silently close idle keep-alive
            bool hasPartialRequest = (!it->second.headersParsed && !it->second.raw_buffer.empty())
                                   || (it->second.headersParsed && !it->second.request_obj.isComplete());

            if (hasPartialRequest) {
                // Incomplete request -> send 408
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
            } else {
                // Idle keep-alive connection -> close without sending 408
                removeClient(static_cast<int>(i));
                if (i > numberOfServers) {
                    // Adjust index after removal so we don't skip next fd
                    --i;
                }
            }
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



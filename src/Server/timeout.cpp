#include "monitorClient.hpp"
#include "../HTTP/Request.hpp"
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <iostream> 
#include <fcntl.h>  

#define CHUNK_SIZE 8192 // Define the chunk size for reading data
monitorClient::SocketTracker::SocketTracker() : WError(0), RError(0), lastActive(time(NULL)) {
}
bool monitorClient::shouldCheckTimeouts(time_t currentTime) {
    // Determine if enough time has passed to check for timeouts
    return (currentTime - lastTimeoutCheck >= TIMEOUT_CHECK_INTERVAL);
}

void monitorClient::SocketTracker::updateActivity() {
    lastActive = time(NULL); // Update the last active time to the current time
}

bool monitorClient::SocketTracker::hasTimedOut(time_t currentTime, time_t timeoutSeconds) const {
    // Check if client has exceeded the timeout threshold
    return (currentTime - lastActive) > timeoutSeconds;
}

void monitorClient::checkTimeouts() {
    std::cout << "Checking for timed out clients..." << std::endl;
    time_t now = time(NULL);
    
    for (size_t i = numberOfServers; i < fds.size(); i++) {
        int clientFd = fds[i].fd;
        Miterator it = fdsTracker.find(clientFd);

        if (it != fdsTracker.end() && it->second.hasTimedOut(now, CLIENT_TIMEOUT)) {
            std::cout << "Client " << clientFd << " timed out after " 
                      << (now - it->second.lastActive) << " seconds" << std::endl;
            
            it->second.RError = 408;
            it->second.WError = 1;
            
            std::string timeoutHtml = "<html><body><h1>408 Request Timeout</h1>"
                                     "<p>The server timed out waiting for the request.</p></body></html>";
            
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
    // Called during monitorClient initialization
    lastTimeoutCheck = time(NULL);
}

// Handle client activity to prevent timeout
void monitorClient::updateClientActivity(int clientFd) {
    Miterator it = fdsTracker.find(clientFd);
    if (it != fdsTracker.end()) {
        it->second.updateActivity();
    }
}



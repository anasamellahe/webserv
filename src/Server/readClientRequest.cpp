#include "monitorClient.hpp"
#include "../HTTP/Request.hpp"
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <iostream>
#include <fcntl.h>

int monitorClient::readChunkFromClient(int clientFd, std::string& buffer) {
    char chunk[CHUNK_SIZE];
    ssize_t bytesRead = read(clientFd, chunk, CHUNK_SIZE);

    if (bytesRead > 0) {
        buffer.append(chunk, bytesRead);
        //std::cout << "Read " << bytesRead << " bytes from client " << clientFd << std::endl;
        return bytesRead;
    } else if (bytesRead == 0) {
        // Client closed the connection
        //std::cout << "Client " << clientFd << " closed connection" << std::endl;
        return 0;
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No data available, try again later
        //std::cout << "No data available from client " << clientFd << std::endl;
        return -1;
    } else {
        // Error occurred
        //std::cerr << "Error reading from client " << clientFd << ": " << strerror(errno) << std::endl;
        return -2;
    }
}

int monitorClient::readClientRequest(int clientFd) {
    std::map<int, SocketTracker>::iterator it = fdsTracker.find(clientFd);
    if (it == fdsTracker.end()) {
        std::cerr << "Client " << clientFd << " not found in tracker" << std::endl;
        return -1;
    }

    SocketTracker& tracker = it->second;
    
    // Read chunk (up to 8KB)
    std::string buffer;
    int readResult = readChunkFromClient(clientFd, buffer);
    
    // Handle read status
    if (readResult <= 0) {
        if (readResult == 0) {  // Client closed connection
            std::cout << "Client " << clientFd << " closed connection" << std::endl;
            return 0;
        }
        if (readResult == -2) { // Read error
            std::cerr << "Error reading from client " << clientFd << std::endl;
            return -1;
        }
        return 1; // No data but keep alive
    }

    // Append to request buffer
    tracker.request += buffer;

    // Process complete requests in buffer
    while (true) {
        request req(clientFd);
        size_t prev_size = tracker.request.size();
        
        // Try parsing (even if incomplete)
        bool parseSuccess = req.parseFromSocket(clientFd, tracker.request, tracker.request.size());
        
        // Handle parsing outcomes
        if (!req.isValid()) {
            if (req.getErrorCode() == "431" || req.getErrorCode() == "400") {
                // Fatal error - close connection
                tracker.error = req.getErrorCode();
                // generateErrorResponse(tracker);
                return 0;
            }
            // Incomplete request - keep waiting
            break;
        }

        // Complete request processing
        if (req.isComplete()) {
            // Update tracker with request data
            tracker.method = req.getMethod();
            tracker.path = req.getPath();
            tracker.headers = req.getAllHeaders();
            tracker.queryParams = req.getQueryParams();
            tracker.cookies = req.getCookies();
            
            // Generate response
            // generateResponse(req, tracker);
            
            // Remove processed data from buffer
            size_t processed_size = prev_size - tracker.request.size();
            tracker.request.erase(0, processed_size);
            
            // Check keep-alive
            std::string connection = req.getHeader("Connection");
            bool keepAlive = (connection == "keep-alive" || 
                            (req.getVersion() == "HTTP/1.1" && connection != "close"));
            
            if (!keepAlive) return 0;
        } else {
            // Partial request - need more data
            break;
        }
    }

    return 1; // Keep connection alive
}
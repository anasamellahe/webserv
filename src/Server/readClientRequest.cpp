#include "monitorClient.hpp"
#include "../HTTP/Request.hpp"
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <iostream>
#include <fcntl.h>
#define MAX_HEADER_SIZE 8192 // Define the maximum header size

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
// int getMaxBodySizeOfServer(const std::string& host) {
//     ConfigParser configParser;
//     Config config;

//     // Parse the configuration file
//     if (!configParser.parseConfigFile("/path/to/config/file", config)) {
//         std::cerr << "Failed to parse configuration file" << std::endl;
//         return -1; // Return an error code
//     }

//     // Extract host and port from the input string
//     std::string serverHost, serverPort;
//     size_t colonPos = host.find(':');
//     if (colonPos != std::string::npos) {
//         serverHost = host.substr(0, colonPos);
//         serverPort = host.substr(colonPos + 1);
//     } else {
//         serverHost = host;
//         serverPort = "80"; // Default to port 80 if not specified
//     }

//     // Search for the server block matching the host, port, or server name
//     for (const auto& server : config.servers) {
//         if ((server.host == serverHost && server.port == serverPort) || 
//             (std::find(server.serverNames.begin(), server.serverNames.end(), serverHost) != server.serverNames.end())) {
//             return server.maxBodySize; // Return the max body size for the matched server
//         }
//     }

//     // If no matching server block is found, return a default value
//     return 1024 * 1024; // Default max body size (1MB)
// }

int monitorClient::readClientRequest(int clientFd) {
    // Find client in tracker
    auto it = fdsTracker.find(clientFd);
    if (it == fdsTracker.end()) {
        std::cerr << "Client " << clientFd << " not found in tracker" << std::endl;
        return -1;
    }

    SocketTracker& tracker = it->second;
    
    Request req(clientFd);
    
    // Read chunk (up to 8KB)
    std::string buffer;
    int readResult = readChunkFromClient(clientFd, buffer);
    
    // Handle read status
    if (readResult <= 0) {
        if (readResult == 0) {  // Client closed connection
            req.setErrorCode("400 Bad Request");
            std::cout << "Client " << clientFd << " closed connection" << std::endl;
            return 0;
        }
        if (readResult == -2) {  // Error occurred
            std::cerr << "Error reading from client " << clientFd << std::endl;
            req.setErrorCode("500 Internal Server Error");
            return -1;
        }
        return 1; // No data but keep connection alive (EAGAIN/EWOULDBLOCK)
    }

    // Append to request buffer
    tracker.request += buffer;
    
    // Check if headers exceed maximum size
    if (tracker.request.size() > MAX_HEADER_SIZE && tracker.request.find("\r\n\r\n") == std::string::npos) {
        req.setErrorCode("431 Request Header Fields Too Large");
        return 0;
    }

    // Check for chunked transfer or content-length
    size_t header_end = tracker.request.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        // Headers not fully received yet, continue reading
        return 1;
    }

    std::string headers_section = tracker.request.substr(0, header_end);
    if (!req.parseHeaders(headers_section)) {
        return 0;
    }
    if (!tracker.headers.empty()) {
        if (req.isChunked()) {
            // Check if chunked transfer is complete
            // For chunked transfers, check if the last chunk is received
            size_t chunk_end = tracker.request.find("0\r\n\r\n");
            if (chunk_end != std::string::npos) {
                req.setComplete(true);
            }
        } else if (req.getContentLength() > 0) {
            size_t header_end = tracker.request.find("\r\n\r\n");
            if (header_end != std::string::npos) {
                size_t body_start = header_end + 4;
                size_t body_received = tracker.request.size() - body_start;
                
                if (body_received >= req.getContentLength()) {
                    // We have the complete body
                    req.setComplete(true);
                    
                    // Check if payload is too large
                    if (body_received > req.getContentLength()) {
                        req.setErrorCode("413 Payload Too Large");
                        return 0;
                    }
                }
            }
        }
    }

    bool processed_data = false;
    size_t prev_size;
    
    // Process complete requests in buffer
    do {
        prev_size = tracker.request.size();
        
        // Try parsing (even if incomplete)
        bool validity = req.parseFromSocket(clientFd, tracker.request.c_str(), tracker.request.size());
        
        // Handle parsing outcomes
        if (!req.isValid()) {
            if (!req.getErrorCode().empty()) {
                // Fatal error - close connection
                tracker.error = req.getErrorCode();
                return 0;
            }
            // Incomplete request - stop processing and wait for more data
            break;
        }

        // Complete request processing
        if (req.isComplete()) {
            processed_data = true;
            
            // Update tracker with request data
            tracker.method = req.getMethod();
            tracker.path = req.getPath();
            tracker.headers = req.getAllHeaders();
            tracker.queryParams = req.getQueryParams();
            tracker.cookies = req.getCookies();
            
            // Remove processed data from buffer
            size_t processed_size = prev_size - tracker.request.size();
            if (processed_size > 0) {
                tracker.request.erase(0, processed_size);
            }
        }
    } while (!tracker.request.empty() && prev_size != tracker.request.size());

    if (processed_data) {
        tracker.updateActivity();
    }
        tracker.updateActivity();
             return 1; // Continue reading
    }


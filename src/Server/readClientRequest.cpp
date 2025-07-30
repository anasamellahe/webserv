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
    auto it = fdsTracker.find(clientFd);
    if (it == fdsTracker.end()) {
        std::cerr << "Client " << clientFd << " not found in tracker" << std::endl;
        return -1;
    }

    SocketTracker& tracker = it->second;
    
    // Set clientFD in the request object if not already set
    if (tracker.request_obj->getClientFD() != clientFd) {
        tracker.request_obj->setClientFD(clientFd);
    }
    
    // Read chunk
    std::string buffer;
    int readResult = readChunkFromClient(clientFd, buffer);
    
    if (readResult <= 0) {
        if (readResult == 0) {
            std::cout << "Client " << clientFd << " closed connection" << std::endl;
            return 0;
        }
        if (readResult == -2) {
            std::cerr << "Error reading from client " << clientFd << std::endl;
            return -1;
        }
        return 1;
    }

    // Append to raw buffer
    tracker.raw_buffer += buffer;
    
    // Check header size limit
    if (tracker.raw_buffer.size() > MAX_HEADER_SIZE && 
        tracker.raw_buffer.find("\r\n\r\n") == std::string::npos) {
        tracker.error = "431 Request Header Fields Too Large";
        return 0;
    }

    // Try to parse the accumulated data
    bool validity = tracker.request_obj->parseFromSocket(clientFd, tracker.raw_buffer, tracker.raw_buffer.size());

    if (validity && tracker.request_obj->isValid()) {
        // Request is complete and valid
        tracker.updateActivity();
        
        // Clear the buffer since request is complete
        tracker.raw_buffer.clear();
        
        // Print debug info
        
        return 1; // Ready for response generation
    } else if (!tracker.request_obj->getErrorCode().empty() && 
               tracker.request_obj->getErrorCode() != "200 OK") {
        // Fatal parsing error
        tracker.error = tracker.request_obj->getErrorCode();
        return 0;
    }
    
    // Incomplete request - continue reading
    return 1;
}

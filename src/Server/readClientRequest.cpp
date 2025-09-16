#include "monitorClient.hpp"
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <iostream>
#include <fcntl.h>
#include <sys/socket.h>

// Response handlers
#include "../methods/ResponseGet.hpp"
#include "../methods/ResponsePost.hpp"
#include "../methods/ResponseDelete.hpp"

#define MAX_HEADER_SIZE 8192

int monitorClient::readChunkFromClient(int clientFd, std::string& buffer) {
    char chunk[CHUNK_SIZE];
    ssize_t bytesRead = read(clientFd, chunk, CHUNK_SIZE);

    if (bytesRead > 0) {
        buffer.append(chunk, bytesRead);
        return bytesRead;
    } else if (bytesRead == 0) {
        return 0;
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return -1;
    } else {
        return -2;
    }
}

int monitorClient::readClientRequest(int clientFd) {
    TrackerIt it = fdsTracker.find(clientFd);
    if (it == fdsTracker.end()) {
        std::cerr << "Client " << clientFd << " not found in tracker" << std::endl;
        return -1;
    }

    SocketTracker& tracker = it->second;
    if (tracker.request_obj.getClientFD() != clientFd) {
        tracker.request_obj.setClientFD(clientFd);
    }
 
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

    tracker.raw_buffer += buffer;
    
    // Check for oversized headers
    if (tracker.raw_buffer.size() > MAX_HEADER_SIZE && 
        tracker.raw_buffer.find("\r\n\r\n") == std::string::npos) {
        tracker.error = "431 Request Header Fields Too Large";
        tracker.request_obj.setComplete(false);
        return 0;
    }

    // Try to parse the request - this handles ALL validation internally
    bool parseSuccess = tracker.request_obj.parseFromSocket(clientFd, tracker.raw_buffer, tracker.raw_buffer.size());
    
    // Check if we have complete headers first
    size_t headerEnd = tracker.raw_buffer.find("\r\n\r\n");
    
    // Set the global config in request object BEFORE parsing so server matching can work
    if (headerEnd != std::string::npos) {
        Config globalConfig = ServerConfig.getConfigs();
        tracker.request_obj.setServerConfig(globalConfig);
        
        // Now re-parse with config available for server matching
        parseSuccess = tracker.request_obj.parseFromSocket(clientFd, tracker.raw_buffer, tracker.raw_buffer.size());
    }

    // If parsing failed and we have some data, log a truncated view of the raw request to aid debugging
    if (!parseSuccess && !tracker.raw_buffer.empty()) {
        std::string preview = tracker.raw_buffer.substr(0, std::min<size_t>(tracker.raw_buffer.size(), 512));
        std::cerr << "[DEBUG] Raw request (truncated):\n" << preview << "\n--- end preview ---" << std::endl;
    }
    
    // Check if we have complete headers first
    headerEnd = tracker.raw_buffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        // Headers not complete yet
        tracker.request_obj.setComplete(false);
        return 1;
    }
    
    // Headers are complete - now check if request parsing succeeded and is valid
    if (parseSuccess && tracker.request_obj.isValid()) {
        // Additional check: verify body is complete if expected
        if (isRequestBodyComplete(tracker, headerEnd)) {
            // Request is fully complete and valid
            tracker.request_obj.setComplete(true);
            tracker.updateActivity();
            tracker.raw_buffer.clear();
            
            std::cout << "Request from client " << clientFd << " is complete and valid" << std::endl;
            return 1;
        } else {
            // Headers complete but body still incoming
            tracker.request_obj.setComplete(false);
            return 1;
        }
    } else if (!tracker.request_obj.getErrorCode().empty() && 
               tracker.request_obj.getErrorCode() != "200 OK") {
        // Request has parsing error
        tracker.request_obj.setComplete(false);
        tracker.error = tracker.request_obj.getErrorCode();
        
        std::cout << "Request from client " << clientFd << " has error: " << tracker.error << std::endl;
        return 0;
    }
    
    // Request headers complete but parsing not finished (incomplete body)
    tracker.request_obj.setComplete(false);
    return 1;
}

bool monitorClient::isRequestBodyComplete(SocketTracker& tracker, size_t headerEnd) {
    // Calculate body start position (after "\r\n\r\n")
    size_t bodyStart = headerEnd + 4;
    size_t bodyReceived = 0;
    
    if (bodyStart < tracker.raw_buffer.size()) {
        bodyReceived = tracker.raw_buffer.size() - bodyStart;
    }
    
    // Check Content-Length header
    const std::string& contentLengthHeader = tracker.request_obj.getHeader("content-length");
    if (contentLengthHeader != "content-length") { // getHeader returns key if not found
        char *endptr = NULL;
        unsigned long expectedLength = strtoul(contentLengthHeader.c_str(), &endptr, 10);
        if (endptr == contentLengthHeader.c_str()) {
            // Invalid Content-Length, assume no body expected
            return true;
        }
        return bodyReceived >= expectedLength;
    }
    
    // Check for chunked transfer encoding
    const std::string& transferEncoding = tracker.request_obj.getHeader("transfer-encoding");
    if (transferEncoding != "transfer-encoding" && 
        transferEncoding.find("chunked") != std::string::npos) {
        // For chunked encoding, look for the final chunk (0\r\n\r\n)
        std::string bodyData = tracker.raw_buffer.substr(bodyStart);
        return bodyData.find("0\r\n\r\n") != std::string::npos;
    }
    
    // For GET/DELETE requests or requests without body indicators, consider complete
    const std::string& method = tracker.request_obj.getMethod();
    if (method == "GET" || method == "DELETE") {
        return true;
    }
    
    // For POST without Content-Length or Transfer-Encoding, assume no body
    return true;
}

void monitorClient::writeClientResponse(int clientFd) {
    TrackerIt it = fdsTracker.find(clientFd);
    if (it == fdsTracker.end()) return;
    SocketTracker& tracker = it->second;

    if (tracker.response.empty()) return;

    ssize_t w = write(clientFd, tracker.response.c_str(), tracker.response.size());
    if (w > 0) {
        tracker.response.erase(0, w);
    } else if (w == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Try later
            return;
        }
        // fatal write error
        tracker.WError = 1;
        std::cerr << "[ERROR] write to client " << clientFd << " failed: " << strerror(errno) << std::endl;
        return;
    }

    // If response fully sent and connection should be closed, mark for removal
    if (tracker.response.empty()) {
        const std::string& connHdr = tracker.request_obj.getHeader("connection");
        if (connHdr == "close") {
            tracker.WError = 1; // will be removed by caller
        }
    }
}

void monitorClient::generateErrorResponse(SocketTracker& tracker) {
    // Build a simple error response from tracker.error (like "400 Bad Request")
    std::string err = tracker.error;
    if (err.empty()) err = "500 Internal Server Error";
    // split code and text
    std::istringstream ss(err);
    std::string codeStr, rest;
    ss >> codeStr;
    std::getline(ss, rest);
    if (!rest.empty() && rest[0] == ' ') rest.erase(0,1);
    if (codeStr.empty()) codeStr = "500";
    if (rest.empty()) rest = "Error";

    std::ostringstream body;
    body << "<html><head><title>" << codeStr << "</title></head>"
        << "<body><h1>" << codeStr << "</h1><p>" << rest << "</p></body></html>";

    std::ostringstream resp;
    resp << "HTTP/1.1 " << codeStr << " " << rest << "\r\n";
    resp << "Content-Type: text/html; charset=utf-8\r\n";
    resp << "Connection: close\r\n";
    resp << "Content-Length: " << body.str().size() << "\r\n\r\n";
    resp << body.str();

    tracker.response = resp.str();
    tracker.WError = 0;
}

void monitorClient::generateSuccessResponse(SocketTracker& tracker) {
    Request &req = tracker.request_obj;
    const std::string &method = req.getMethod();

    try {
        if (method == "GET"){
            ResponseGet handler(req);
            tracker.response = handler.generate();
        } else if (method == "POST"){
            ResponsePost handler(req);
            tracker.response = handler.generate();
        } else if (method == "DELETE"){
            ResponseDelete handler(req);
            tracker.response = handler.generate();
        } else {
            // 501 Not Implemented
            std::ostringstream b;
            b << "<html><body><h1>501 Not Implemented</h1><p>Method " << method << " not implemented.</p></body></html>";
            std::ostringstream resp;
            resp << "HTTP/1.1 501 Not Implemented\r\n";
            resp << "Content-Type: text/html; charset=utf-8\r\n";
            resp << "Content-Length: " << b.str().size() << "\r\n\r\n";
            resp << b.str();
            tracker.response = resp.str();
        }
    } catch (...) {
        tracker.error = "500 Internal Server Error";
        generateErrorResponse(tracker);
    }

}

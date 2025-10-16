#include "monitorClient.hpp"
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iostream>
#include <fcntl.h>
#include <sys/socket.h>
#include <ctime>

// Response handlers
#include "../methods/ResponseGet.hpp"
#include "../methods/ResponsePost.hpp"
#include "../methods/ResponseDelete.hpp"

#define MAX_HEADER_SIZE 8192

static size_t findHeadersEnd(const std::string &buf) {
    return buf.find("\r\n\r\n");
}

// (replaced by checkChunkedCompleteAndSize)

// Return: 1 = complete, 0 = incomplete, -1 = malformed/too large
static int checkChunkedCompleteAndSize(const std::string &body, size_t maxAllowedSize) {
    // Quick size check: if body already exceeds maxAllowedSize -> error
    if (maxAllowedSize > 0 && body.size() > maxAllowedSize) return -1;

    // Look for the final 0-length chunk sequence "\r\n0\r\n\r\n"
    if (body.find("\r\n0\r\n\r\n") != std::string::npos) return 1;

    // Also accept if body ends exactly with "0\r\n\r\n"
    if (body.size() >= 5) {
        const std::string tail = body.substr(body.size() - 5);
        if (tail == "0\r\n\r\n") return 1;
    }

    // Not complete yet
    return 0;
}

int monitorClient::readChunkFromClient(int clientFd, std::string& buffer) {
    char chunk[CHUNK_SIZE];
    ssize_t bytesRead = read(clientFd, chunk, CHUNK_SIZE);

    if (bytesRead > 0) {
        buffer.append(chunk, bytesRead);
        return bytesRead;
    } else if (bytesRead == 0) {
        return 0;
    } else if (bytesRead == -1) {
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
    // 1) Drain reads until no more data available
    std::string chunk;
    while (true) {
        int rr = readChunkFromClient(clientFd, chunk);
        if (rr > 0) {
            tracker.raw_buffer.append(chunk);
            chunk.clear();
            continue;
        } else if (rr == 0) {
            // peer closed; if no pending data, close, else process what we have
            if (tracker.raw_buffer.empty()) return 0;
            break;
        } else if (rr == -1) {
            // No more data available
            break;
        } else {
            // fatal read error
            std::cerr << "Error reading from client " << clientFd << std::endl;
            return -1;
        }
    }

    // 2) Enforce header-size cap pre-CRLFCRLF
    size_t hdrEnd = findHeadersEnd(tracker.raw_buffer);
    if (hdrEnd == std::string::npos) {
        if (tracker.raw_buffer.size() > MAX_HEADER_SIZE) {
            tracker.error = "431 Request Header Fields Too Large";
            tracker.request_obj.setComplete(false);
            return 0; // signal close
        }
        return 1; // need more data
    }

    // 3) Parse headers once
    if (!tracker.headersParsed) {
        // ensure Request is clean for this message only
        tracker.request_obj.reset();
        tracker.request_obj.setClientFD(clientFd);
        // set full server config for matching
        Config globalConfig = ServerConfig.getConfigs();
        tracker.request_obj.setServerConfig(globalConfig);
        std::string headersSection = tracker.raw_buffer.substr(0, hdrEnd);
        if (!tracker.request_obj.parseHeadersSection(headersSection)) {
            tracker.error = tracker.request_obj.getErrorCode();
            tracker.request_obj.setComplete(false);
            return 0;
        }
        // Match and store the server configuration now that Host/Port are known
        tracker.request_obj.matchServerConfiguration();
        tracker.headersParsed = true;
        tracker.consumedBytes = hdrEnd + 4; // include CRLFCRLF
    }



    // 4) Determine body completeness and enforce per-server max body size
    const bool isChunked = tracker.request_obj.hasChunkedEncoding();
    if (isChunked)
        tracker.request_obj.setChunked(true);
    size_t bodyAvail = (tracker.raw_buffer.size() > tracker.consumedBytes)
        ? (tracker.raw_buffer.size() - tracker.consumedBytes) : 0;
    size_t need = 0;

    // Determine server max body size (fallback to 0 = unlimited)
    size_t maxBody = 0;
    if (tracker.request_obj.hasServerConfig()) {
        maxBody = tracker.request_obj.getCurrentServer()->client_max_body_size;
    }

    if (isChunked) {
        std::string bodyView = tracker.raw_buffer.substr(tracker.consumedBytes);
        int chk = checkChunkedCompleteAndSize(bodyView, maxBody);
        if (chk == -1) {
            tracker.error = "413 Request Entity Too Large";
            tracker.request_obj.setErrorCode("413 Request Entity Too Large");
            tracker.request_obj.setComplete(false);
            return 0; // signal error
        } else if (chk == 0) {
            tracker.request_obj.setComplete(false);
            return 1; // need more data
        }
        // else chk == 1 -> complete
    } else {
        need = tracker.request_obj.expectedContentLength();
        if (need > 0) {
            if (maxBody > 0 && need > maxBody) {
                tracker.error = "413 Request Entity Too Large";
                tracker.request_obj.setErrorCode("413 Request Entity Too Large");
                tracker.request_obj.setComplete(false);
                return 0; // error
            }
            if (bodyAvail < need) {
                tracker.request_obj.setComplete(false);
                return 1; // wait for more
            }
        }
    }


    // 5) We have a complete request; parse body with exactly the slice
    std::string bodySlice;
    if (isChunked) {
        bodySlice = tracker.raw_buffer.substr(tracker.consumedBytes);
    } else {
        bodySlice = tracker.raw_buffer.substr(tracker.consumedBytes, need);
    }
    if (!tracker.request_obj.parseBodySection(bodySlice)) {
        tracker.error = tracker.request_obj.getErrorCode();
        tracker.request_obj.setComplete(false);
        return 0;
    }
    tracker.request_obj.setComplete(true);

    // // Debug: print the fully-parsed request (headers + body and derived fields)
    // std::cout << "[DEBUG] Full parsed request for client " << clientFd << ":\n";
    // tracker.request_obj.debugPrint();

     // Log that the request was fully parsed (include HTTP method)
     time_t tnow = time(NULL);
     std::ostringstream ss;
     ss << "[INFO] " << "Parsed request from client_fd=" << clientFd
         << " method=" << tracker.request_obj.getMethod()
         << " at " << std::ctime(&tnow);
     std::cout << ss.str();
     
     // Print detailed request information for debugging
     std::cout << "[DEBUG] Request Details:" << std::endl;
     std::cout << "  Method: " << tracker.request_obj.getMethod() << std::endl;
     std::cout << "  Path: " << tracker.request_obj.getPath() << std::endl;
     std::cout << "  Version: " << tracker.request_obj.getVersion() << std::endl;
     std::cout << "  Content-Length: " << tracker.request_obj.getHeader("content-length") << std::endl;
     std::cout << "  Content-Type: " << tracker.request_obj.getHeader("content-type") << std::endl;
     std::cout << "  Host: " << tracker.request_obj.getHeader("host") << std::endl;
     std::cout << "  User-Agent: " << tracker.request_obj.getHeader("user-agent") << std::endl;
     std::cout << "  Connection: " << tracker.request_obj.getHeader("connection") << std::endl;
     if (!tracker.request_obj.getBody().empty()) {
         std::cout << "  Body length: " << tracker.request_obj.getBody().length() << " bytes" << std::endl;
         std::cout << "  Body preview: " << tracker.request_obj.getBody().substr(0, 100) << std::endl;
     }
     std::cout << "[DEBUG] End request details" << std::endl;
    return 1;
}

// Provide out-of-line definitions to satisfy linker if needed
void monitorClient::generateErrorResponse(SocketTracker& tracker) {
    // Build basic error from tracker.error
    std::string err = tracker.error;
    if (err.empty()) err = "500 Internal Server Error";
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

              std::cout << "[INFO] Generating response for method: " << method << std::endl;
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
            std::cerr << "[ERROR] Unsupported HTTP method: " << method << std::endl;
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

int monitorClient::writeClientResponse(int clientFd) {
    TrackerIt it = fdsTracker.find(clientFd);
    if (it == fdsTracker.end()) return -1;
    SocketTracker& tracker = it->second;

    if (tracker.response.empty()) return 0; // nothing to send

    // Attempt to write as much as possible (non-blocking)
    ssize_t w = write(clientFd, tracker.response.c_str(), tracker.response.size());
    if (w > 0) {
        tracker.response.erase(0, static_cast<size_t>(w));
        // If there's still data remaining, ask caller to keep POLLOUT enabled
        if (!tracker.response.empty()) return 1; // partial remain
        // else fall-through: all data sent
    } else if (w == -1) {
        // Use fcntl to check if socket is non-blocking and would block
        int flags = fcntl(clientFd, F_GETFL, 0);
        if (flags != -1 && (flags & O_NONBLOCK)) {
            // Assume would block if non-blocking
            return 1; // still pending
        }
        // fatal write error
        tracker.WError = 1;
        return -1;
    }

    // If we reach here, the full response was sent (tracker.response empty)
    // Check Connection header: HTTP/1.1 defaults to keep-alive unless "close"
    const std::string& connHdr = tracker.request_obj.getHeader("connection");
    std::string connVal = connHdr;
    for (size_t k = 0; k < connVal.size(); ++k) connVal[k] = std::tolower(connVal[k]);
    if (connVal == "close") {
        tracker.WError = 1; // signal caller to remove client
        return 0; // complete, but marked for close
    }

    return 0; // complete, keep-alive
}

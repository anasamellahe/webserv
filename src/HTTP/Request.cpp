#include "Request.hpp"
#include <unistd.h>
#include <string>
#include <sstream>
#include <iostream>



request::request(int clientFD)
{
    this->clientFD = clientFD;

}
// request::~request()
// {

// }




bool request::parseFromSocket(int socket_fd) {
    char buffer[4096];
    ssize_t bytesRead = read(socket_fd, buffer, sizeof(buffer) - 1);

    if (bytesRead <= 0) {
        if (bytesRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // No data available yet
            return false;
        } else {
            // Error or connection closed
            setErrorCode("400 Bad Request");
            return false;
        }
    }

    buffer[bytesRead] = '\0';
    raw_request.append(buffer, bytesRead);

    // Detect and parse the status line
    if (status_line.empty()) {
        size_t firstLineEnd = raw_request.find("\r\n");
        if (firstLineEnd != std::string::npos) {
            status_line = raw_request.substr(0, firstLineEnd);
            if (!parseRequestLine(status_line)) {
                setErrorCode("400 Bad Request");
                return false;
            }
            raw_request.erase(0, firstLineEnd + 2); // Remove the parsed status line
        } else {
            // Status line is incomplete
            return false;
        }
    }

    // Detect headers and send them to parseHeaders
    size_t headerEnd = raw_request.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        std::string headers_part = raw_request.substr(0, headerEnd);
        if (!parseHeaders(headers_part)) {
            setErrorCode("400 Bad Request");
            return false;
        }
        raw_request.erase(0, headerEnd + 4); // Remove the parsed headers
        headers_parsed = true;
    }

    return true;
}



bool request::parse(const std::string& raw_request)
{
    std::istringstream request_stream(raw_request);
    std::string line;
    
    // Get the first line (request line)
    if (!std::getline(request_stream, line)) {
        setErrorCode("400 Bad Request");
        return false;
    }
    
    // Remove trailing \r if present
    if (line[line.length() - 1] == '\r') {
        line.erase(line.length() - 1);
    }
    
    // Parse request line
    if (!parseRequestLine(line)) {
        return false;
    }
    
    // Find the end of headers
    size_t header_end = raw_request.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        setErrorCode("400 Bad Request");
        return false;
    }
    
    // Extract headers section
    std::string headers_section = raw_request.substr(0, header_end);
    
    // Parse headers
    if (!parseHeaders(headers_section)) {
        return false;
    }
    
    // Process body if present
    if (header_end + 4 < raw_request.length()) {
        std::string body_content = raw_request.substr(header_end + 4);
        
        // Handle chunked transfer encoding
        if (is_chunked) {
            if (!parseChunkedTransfer(body_content)) {
                return false;
            }
        } else {
            // Regular body
            if (!parseBody(body_content)) {
                return false;
            }
        }
    }
    
    // Extract CGI info if needed
    extractCgiInfo();
    
    is_valid = true;
    return true;
}




// void request::setMethod(const std::string& method)
// {

// }
// void request::setPath(const std::string& path)
// {

// }
// void request::setVersion(const std::string& version)
// {

// }
// void request::setHeader(const std::string& key, const std::string& value)
// {

// }
// void request::setBody(const std::string& body)
// {

// }
// void request::setClientFD(int fd)
// {

// }
// void request::setClientAddr(const sockaddr_in& addr)
// {

// }

// std::string request::getMethod() const
// {

// }
// std::string request::getPath() const
// {

// }
// std::string request::getVersion() const
// {

// }
// std::string request::getHeader(const std::string& key) const
// {

// }
// std::string request::getBody() const
// {

// }
// int request::getClientFD() const
// {

// }
// sockaddr_in request::getClientAddr() const
// {

// }

// void request::validateRequest()
// {

// }
// void request::setErrorCode(const std::string& error_code)
// {

// }
// std::string request::getErrorCode() const
// {

// }
// std::map<std::string, std::string> request::getQueryParams() const
// {

// }
// std::map<std::string, FilePart> request::getUploads() const
// {

// }
// std::map<std::string, std::string> request::getCGIEnv() const
// {

// }
// std::string request::getCGIExtension() const
// {

// }
// std::map<std::string, std::string> request::getCookies() const
// {

// }
// bool request::isChunked() const
// {

// }
// std::vector<std::string> request::getChunks() const
// {

// }
// void request::addQueryParam(const std::string& key, const std::string& value)
// {

// }
// void request::addUpload(const std::string& key, const FilePart& file_part)
// {

// }

// bool request::isValid()
// {

// }


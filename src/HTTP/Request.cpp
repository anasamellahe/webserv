#include "Request.hpp"
#include <unistd.h>
#include <string>
#include <sstream>
#include <iostream>
#define MAX_HEADER_SIZE 8192 // Define the maximum header size



request::request(int clientFD) : clientFD(clientFD), is_valid(false), is_chunked(false), headers_parsed(false), complete(false) {
}





bool request::parseFromSocket(int socket_fd, const std::string& existing_data, int total_bytes_read) {
    raw_request = existing_data;
    size_t header_end = raw_request.find("\r\n\r\n");

    // Check for incomplete headers
    if (header_end == std::string::npos) {
        if (raw_request.size() > MAX_HEADER_SIZE) {
            setErrorCode("431 Request Header Fields Too Large");
            return false;
        }
        return false; // Need more data
    }

    // Parse complete request
    return parse(raw_request);
}
bool request::parse(const std::string& raw_request) {
    try {
        std::istringstream request_stream(raw_request);
        std::string line;

        if (!std::getline(request_stream, line)) {
            setErrorCode("400 Bad Request");
            return false;
        }

        if (!line.empty() && line.back() == '\r') {
            line.erase(line.size() - 1);
        }
        
        if (!parseRequestLine(line)) {
            return false;
        }

        // --- Header Section Parsing ---
        size_t header_end = raw_request.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            setErrorCode("400 Bad Request");
            return false;
        }

        std::string headers_section = raw_request.substr(0, header_end);
        if (parseHeaders(headers_section)) { // Returns true on error
            return false;
        }

        

        // --- Body Processing ---
        std::string body_content;
        if (header_end + 4 < raw_request.length()) {
            body_content = raw_request.substr(header_end + 4);
        }

        // --- Chunked Transfer Encoding ---

        // --- Chunked Transfer Handling ---
        if (is_chunked) {
            if (!parseChunkedTransfer(body_content)) {
                return false;
            }
        } else if (!body_content.empty()) {
            if (!parseBody(body_content)) {
                return false;
            }
        }

        // --- Post-Processing ---
        if (!extractCgiInfo()) {  // Ensure this returns false on errors
            setErrorCode("500 Internal Server Error");
            return false;
        }

        // --- Final Validation ---
        if (!validateMethod() || !validatePath() || !validateVersion()) {
            setErrorCode("400 Bad Request");
            return false;
        }

        is_valid = true;
        return true;

    } catch (const std::exception& e) {
        setErrorCode("500 Internal Server Error");
        return false;
    }
}


std::string request::getHeader(const std::string& key) const
{
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    if (it != headers.end()) {
        return it->second;
    }
    return "";
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


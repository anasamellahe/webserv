#include "Request.hpp"
#include "../Config/ConfigParser.hpp" // Include Config definition here
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <stdexcept>
#include <algorithm>
#include <cstddef>
#include <iostream>

// Debug helpers
#define DEBUG_MODE 1
#define DEBUG(msg) if (DEBUG_MODE) { std::cout << "\033[33m[DEBUG Request]\033[0m " << msg << std::endl; }
#define DEBUG_ERROR(msg) if (DEBUG_MODE) { std::cout << "\033[31m[ERROR Request]\033[0m " << msg << std::endl; }
#define DEBUG_FUNC() if (DEBUG_MODE) { std::cout << "\033[36m[FUNC Request]\033[0m " << __func__ << " called" << std::endl; }
#define DEBUG_VAR(var) if (DEBUG_MODE) { std::cout << "\033[32m[VAR Request]\033[0m " << #var << " = " << var << std::endl; }
#define DEBUG_START() if (DEBUG_MODE) { std::cout << "\033[34m[START Request]\033[0m " << __func__ << " ----------------------" << std::endl; }
#define DEBUG_END() if (DEBUG_MODE) { std::cout << "\033[34m[END Request]\033[0m " << __func__ << " ----------------------" << std::endl; }

// Default constructor
Request::Request() 
{
    DEBUG_FUNC();
    this->clientFD = -1;
    this->is_valid = false;
    this->is_chunked = false;
    this->Port = 0;
    this->isIp = false;
    this->error_code.clear();
    DEBUG("Default constructor initialized");
}

// Constructor with client FD
Request::Request(int clientFD) 
{
    DEBUG_FUNC();
    DEBUG_VAR(clientFD);
    this->clientFD = clientFD;
    this->is_valid = false;
    this->is_chunked = false;
    this->Port = 0;
    this->isIp = false;
    this->error_code.clear();
    DEBUG("Constructor with clientFD initialized");
}

// Destructor
Request::~Request() 
{
    DEBUG_FUNC();
    DEBUG_VAR(clientFD);
    // Destructor logic if needed
}

void Request::reset() {
    this->method.clear();
    this->path.clear();
    this->version.clear();
    this->headers.clear();
    this->body.clear();
    this->error_code.clear();
    this->Host.clear();
    this->is_Complete = false;
    this->Port = 0;
    this->isIp = false;
    this->query_params.clear();
    this->uploads.clear();
    this->cgi_extension.clear();
    this->cgi_env.clear();
    this->cookies.clear();
    this->is_chunked = false;
    this->chunks.clear();
    this->is_valid = false;
    // Do not clear clientFD or client_addr, as they are connection-specific
}

bool Request::parseFromSocket(int clientFD, const std::string& buffer, size_t size)
{
    DEBUG_START();
    DEBUG_VAR(clientFD);
    DEBUG_VAR(size);
    
    this->clientFD = clientFD;
    this->requestContent = buffer.substr(0, size);
    
    try {
        // Find the end of headers (double CRLF)
        size_t header_end = this->requestContent.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            DEBUG("Incomplete request, no double CRLF found");
            return false; // Incomplete request, need more data
        }
        
        DEBUG_VAR(header_end);
        
        // Extract header section
        std::string headers_section = this->requestContent.substr(0, header_end);
        DEBUG("Headers section extracted, length: " << headers_section.length());
        
        // Split headers into lines
        std::vector<std::string> header_lines;
        size_t pos = 0;
        size_t prev = 0;
        while ((pos = headers_section.find("\r\n", prev)) != std::string::npos) {
            header_lines.push_back(headers_section.substr(prev, pos - prev));
            prev = pos + 2;
        }
        
        if (header_lines.empty()) {
            DEBUG_ERROR("No header lines found");
            this->error_code = BAD_REQ;
            return false;
        }
        
        DEBUG("Number of header lines: " << header_lines.size());
        DEBUG("First line: " << header_lines[0]);
        
        // Parse the request line (first line)
        try {
            DEBUG("Parsing start line: " << header_lines[0]);
            parseStartLine(header_lines[0]);
            DEBUG("Start line parsed successfully");
        } catch (int e) {
            DEBUG_ERROR("Error parsing start line: " << e);
            this->is_valid = false;
            return false;
        }
        
        // Parse the rest of the headers
        DEBUG("Parsing header lines");
        for (size_t i = 1; i < header_lines.size(); i++) {
            try {
                DEBUG("*****Parsing header line " << i << ": " << header_lines[i]);
                parseHeaders(header_lines[i]);
            } catch (int e) {
                
                DEBUG_ERROR("Error parsing header line " << i << ": " << e);
                this->is_valid = false;
                return false;
            }
        }
        
        // Parse query string if present in the path
        size_t query_pos = this->path.find('?');
        if (query_pos != std::string::npos) {
            DEBUG("Query string found in path at position " << query_pos);
            std::string query_string = this->path.substr(query_pos + 1);
            this->path = this->path.substr(0, query_pos);
            DEBUG("Path after removing query: " << this->path);
            DEBUG("Query string: " << query_string);
            parseQueryString();
        }
        
        // Parse body if present
        if (header_end + 4 < this->requestContent.length()) {
            DEBUG("Body present, starting at position " << (header_end + 4));
            std::string body_data = this->requestContent.substr(header_end + 4);
            DEBUG("Body length: " << body_data.length());
            
            // Check for chunked encoding
            ConstHeaderIterator transfer_encoding = this->headers.find("transfer-encoding");
            if (transfer_encoding != this->headers.cend() && 
                transfer_encoding->second.find("chunked") != std::string::npos) {
                DEBUG("Chunked transfer encoding detected");
                parseChunkedTransfer(body_data);
            } else {
                DEBUG("Normal body parsing");
                parseBody(body_data);
            }
        } else {
            DEBUG("No body in request");
        }
        
        // Extract cookies if present
        DEBUG("Parsing cookies");
        parseCookies();
        
        // Check for CGI requests
        DEBUG("Extracting CGI info");
        extractCgiInfo();
        
        // Validate the request
        DEBUG("Validating request");
        validateRequest();
        
        DEBUG("Request parsing complete, is_valid: " << (this->is_valid ? "true" : "false"));
        DEBUG_END();
        return this->is_valid;
    }
    catch (int error_code) {
        DEBUG_ERROR("Exception caught in parseFromSocket: " << error_code);
        this->is_valid = false;
        DEBUG_END();
        return false;
    }
    catch (const std::exception& e) {
        DEBUG_ERROR("Standard exception caught in parseFromSocket: " << e.what());
        this->is_valid = false;
        DEBUG_END();
        return false;
    }
    catch (...) {
        DEBUG_ERROR("Unknown exception caught in parseFromSocket");
        this->is_valid = false;
        DEBUG_END();
        return false;
    }
}

void Request::setMethod(const std::string& method)
{
    DEBUG_FUNC();
    DEBUG_VAR(method);
    
    if (this->method.empty() && !method.empty()) {
        this->method = method;
        DEBUG("Method set successfully");
    }
    else if (error_code.empty()) {
        this->error_code = BAD_REQ;
        DEBUG_ERROR("Method already set or empty method provided, setting error code: " << BAD_REQ);
    }
}

void Request::setPath(const std::string& path)
{
    DEBUG_FUNC();
    DEBUG_VAR(path);
    
    if (path.size() > MAX_PATH_SIZE && error_code.empty()) {
        this->error_code = URI_T_LONG;
        DEBUG_ERROR("Path too long, setting error code: " << URI_T_LONG);
    }
    else if (this->path.empty() && !path.empty()) {
        this->path = path;
        DEBUG("Path set successfully");
    }
    else if (error_code.empty()) {
        this->error_code = BAD_REQ;
        DEBUG_ERROR("Path already set or empty path provided, setting error code: " << BAD_REQ);
    }
}

void Request::setVersion(const std::string& version)
{
    DEBUG_FUNC();
    DEBUG_VAR(version);
    
    if (this->version.empty() && !version.empty() && version.compare("HTTP/1.1") == 0) {
        this->version = version;
        DEBUG("Version set successfully");
    }
    else if (this->error_code.empty()) {
        this->error_code = VERSION_ERR;
        DEBUG_ERROR("Version not HTTP/1.1 or already set, setting error code: " << VERSION_ERR);
    }
}

void Request::parseStartLine(const std::string& line)
{
    DEBUG_START();
    DEBUG_VAR(line);
    
    size_t pos = std::string::npos;

    const char* methods[] = {"GET", "POST", "DELETE", NULL}; // Changed to const char*
    
    for (size_t i = 0; methods[i]; i++) {
        DEBUG("Checking for method: " << methods[i]);
        if ((pos = line.find(methods[i], 0)) != std::string::npos) {
            DEBUG("Method found: " << methods[i] << " at position " << pos);
            break;
        }
    }
    
    if (pos == std::string::npos) {
        DEBUG_ERROR("No valid HTTP method found in the request line");
        throw -1;
    }
    
    try {
        std::string str;
        std::stringstream ss(line);
        
        // Extract method
        ss >> str;
        DEBUG("Extracted method: " << str);
        setMethod(str);
        
        // Extract path
        ss >> str;
        DEBUG("Extracted path: " << str);
        setPath(str);
        
        // Extract version
        ss >> str;
        DEBUG("Extracted version: " << str);
        setVersion(str);
        
        if (!this->error_code.empty()) {
            DEBUG_ERROR("Error code set during parsing: " << this->error_code);
            throw 1;
        }
        
        DEBUG("Start line parsed successfully");
        DEBUG_END();
    }
    catch (const std::exception& e) {
        DEBUG_ERROR("Exception in parseStartLine: " << e.what());
        throw 1;
    }
}

bool Request::parseHeaders(const std::string& headers_text)
{
    DEBUG_START();
    // DEBUG_VAR(headers_text);
     std::cout << "====================================================== start\n" ;
    std::cout << headers_text <<std::endl ;
    std::cout << "======================================================end\n" ;
    std::string key, value;
    size_t pos = headers_text.find(":", 0);
    
    if (pos == std::string::npos) {
        this->error_code = BAD_REQ;
        throw 1;
    }
    
    
    key = headers_text.substr(0, pos);
    
    stringToLower(key);
    
    size_t valueStartIndex = headers_text.find_first_not_of(": \r\t\v\f", pos);
    if (valueStartIndex == std::string::npos) {
        valueStartIndex = pos;
    }
    
    value = headers_text.substr(valueStartIndex, headers_text.rfind("\r\n") - valueStartIndex);
    
    
    stringToLower(value);
    
    // Validate key and value
    if (keyValidationNot(key) || valueValidationNot(value)) {

        if (this->error_code.empty())
            this->error_code = BAD_REQ;

        throw 1;
    }

    if (key == "host" && value.empty()) {
        if (this->error_code.empty())
            this->error_code = BAD_REQ;
        throw 1;
    }
    
    this->headers.insert(std::pair<std::string, std::string>(key, value));
    return true;
}



const std::string& Request::getMethod() const
{
    DEBUG_FUNC();
    DEBUG("Method: " << this->method);
    return this->method;
}

const std::string& Request::getPath() const
{
    DEBUG_FUNC();
    DEBUG("Path: " << this->path);
    return this->path;
}

const std::string& Request::getVersion() const
{
    DEBUG_FUNC();
    DEBUG("Version: " << this->version);
    return this->version;
}

const std::string& Request::getHeader(const std::string& key) const
{
    DEBUG_FUNC();
    DEBUG_VAR(key);
    
    ConstHeaderIterator it = this->headers.find(key);
    if (it != headers.cend()) {
        DEBUG("Header found: " << it->second);
        return it->second;
    }
    
    DEBUG("Header not found, returning key");
    return key;
}

const std::string& Request::getBody() const
{
    DEBUG_FUNC();
    DEBUG("Body length: " << this->body.length());
    return this->body;
}

int Request::getClientFD() const
{
    DEBUG_FUNC();
    DEBUG_VAR(this->clientFD);
    return this->clientFD;
}

sockaddr_in Request::getClientAddr() const
{
    DEBUG_FUNC();
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(this->client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    DEBUG("Client IP: " << client_ip);
    return this->client_addr;
}

void Request::setHeader(const std::string& key, const std::string& value)
{
    if (!key.empty() && !value.empty())
        this->headers[key] = value;
}

void Request::setBody(const std::string& body)
{
    this->body = body;
}

void Request::setClientFD(int fd)
{
    this->clientFD = fd;
}

void Request::setClientAddr(const sockaddr_in& addr)
{
    this->client_addr = addr;
}

void Request::setErrorCode(const std::string& error_code)
{
    this->error_code = error_code;
}

const std::string& Request::getErrorCode() const
{
    if (!this->error_code.empty())
        return this->error_code;
    static std::string default_ok = "200 OK"; // Use static variable to avoid returning reference to temporary
    return default_ok;
}

void Request::addQueryParam(const std::string& key, const std::string& value)
{
    if (!key.empty())
        this->query_params.insert(std::pair<std::string, std::string>(key, value));
}

void Request::parseQueryPair(const std::string &query, size_t start, size_t end) 
{

    size_t pos = query.find("=", start);
    if (pos != std::string::npos || pos <= end)
    {
        std::string key = query.substr(start, pos - start);
        std::string value = query.substr(pos + 1, end - pos);
        addQueryParam(key, value);
    }
}

bool Request::parseQueryString()
{

    size_t pos = path.find("?", 0);

    if (pos == std::string::npos)
        return false;
    std::string queryParams;
    queryParams = path.substr(pos + 1, std::string::npos);
    path.erase(pos);
    size_t start    = 0;
    size_t end      = 0;
    while (start < queryParams.size())
    {
        end = queryParams.find("&", start);
        if (end != std::string::npos)
        {
            parseQueryPair(queryParams, start, end - 1);
            start = end + 1;
        }
        else
        {
            parseQueryPair(queryParams, start, queryParams.size() - 1);
            break;
        }
    }
    return true;
}
const std::map<std::string, std::string>& Request::getQueryParams() const
{
    return this->query_params;
}

bool Request::isValidHost(const std::string& Host)
{
    size_t pos = Host.find_first_not_of("0123456789.:");
    int num1, num2, num3, num4, num5, consumed;
    if (pos == std::string::npos)
    {
        if (sscanf(Host.c_str(), "%d.%d.%d.%d:%d%n",  &num1, &num2, &num3, &num4, &num5, &consumed) == 5)
        {
            if ((num1 >= 0 && num1 <= 255) &&
                (num2 >= 0 && num2 <= 255) &&
                (num3 >= 0 && num3 <= 255) &&
                (num4 >= 0 && num4 <= 255) &&
                (num5 >= 0 && num5 <= 65535) &&
                (static_cast<size_t>(consumed) == Host.length()) // Cast to fix signedness comparison
            )
            {
                this->isIp = true;
                return true;
            }
            else
                this->isIp = false;
        }
    }
    for (size_t i = 0; i < Host.length(); i++)
    {
        if (!isalnum(Host[i]) && Host[i] != '.' && Host[i] != '-' && Host[i] != ':')
            return false;
        if (i == 0 && (Host[i] == '.' || Host[i] == '-' || Host[i] == ':'))
            return false;
    }
    this->isIp = false;
    return true;
}

bool Request::validateHost(const std::string& Host)
{
    // Simplified implementation
    this->Host = Host;
    this->Port = 8080;
    return true;
}

const std::map<std::string, std::string>& Request::getAllHeaders() const
{
    return this->headers;
}

bool Request::isComplete() const
{
    return this->is_valid;
}

bool Request::setComplete(bool complete)
{
    this->is_valid = complete;
    return this->is_valid;
}

double Request::getContentLength() 
{
    ConstHeaderIterator it = this->headers.find("content-length");
    if (it != this->headers.cend())
    {
        try {
            return std::stod(it->second);
        } catch (const std::invalid_argument&) {
            return 0.0; // Invalid content length
        } catch (const std::out_of_range&) {
            return 0.0; // Out of range content length
        }                   
    }
    return 0.0; // No content-length header
}

// Implementation for getserverConfig method

bool Request::isValid()
{
    return this->is_valid;
}

const std::map<std::string, FilePart>& Request::getUploads() const
{
    return this->uploads;
}

const std::map<std::string, std::string>& Request::getCGIEnv() const
{
    return this->cgi_env;
}

const std::string& Request::getCGIExtension() const
{
    return this->cgi_extension;
}

const std::map<std::string, std::string>& Request::getCookies() const
{
    return this->cookies;
}

const std::vector<std::string>& Request::getChunks() const
{
    return this->chunks;
}

bool Request::isChunked() const
{
    return this->is_chunked;
}

void Request::addUpload(const std::string& key, const FilePart& file_part)
{
    if (!key.empty())
        this->uploads[key] = file_part;
}

// Implementation for parseBody method
bool Request::parseBody(const std::string& body_data)
{
    // Suppress unused parameter warning
    (void)body_data;
    
    // Check if this is a multipart form submission
    ConstHeaderIterator it = this->headers.find("content-type");
    if (it != this->headers.cend())
    {
        std::string content_type = it->second;
        if (content_type.find("multipart/form-data") != std::string::npos)
        {
            // Extract boundary
            size_t boundary_pos = content_type.find("boundary=");
            if (boundary_pos != std::string::npos)
            {
                std::string boundary = content_type.substr(boundary_pos + 9); // 9 is the length of "boundary="
                return parseMultipartBody(boundary);
            }
        }
    }
    
    // For regular body handling, just store the body
    this->body = body_data;
    return true;
}

// Implementation for parseMultipartBody method
bool Request::parseMultipartBody(const std::string& boundary)
{
    // Suppress unused parameter warning
    (void)boundary;
    
    // TODO: Implement multipart body parsing
    // This is a complex task that involves:
    // 1. Splitting the body by boundary
    // 2. Parsing each part's headers
    // 3. Storing file uploads or form fields accordingly
    
    return true;
}

// Implementation for parseChunkedTransfer method
bool Request::parseChunkedTransfer(const std::string& chunked_data)
{
    // Suppress unused parameter warning
    (void)chunked_data;
    
    this->is_chunked = true;
    
    // TODO: Implement chunked transfer parsing
    // This involves:
    // 1. Reading chunk size (in hex)
    // 2. Reading chunk data
    // 3. Repeating until a zero-sized chunk is found
    
    return true;
}

// Implementation for extractCgiInfo method
bool Request::extractCgiInfo()
{
    // Check if the path has a CGI extension (e.g., .php, .cgi, .py)
    size_t dot_pos = this->path.find_last_of('.');
    if (dot_pos != std::string::npos)
    {
        std::string extension = this->path.substr(dot_pos);
        if (extension == ".php" || extension == ".cgi" || extension == ".py")
        {
            this->cgi_extension = extension;
            
            // Set up basic CGI environment variables
            this->cgi_env["QUERY_STRING"] = ""; // Set from query params if any
            this->cgi_env["REQUEST_METHOD"] = this->method;
            this->cgi_env["CONTENT_TYPE"] = this->getHeader("content-type");
            this->cgi_env["CONTENT_LENGTH"] = this->getHeader("content-length");
            this->cgi_env["SCRIPT_NAME"] = this->path;
            
            // If we have query params, add them to QUERY_STRING
            if (!this->query_params.empty())
            {
                std::stringstream ss;
                for (std::map<std::string, std::string>::const_iterator it = this->query_params.begin();
                     it != this->query_params.end(); ++it)
                {
                    if (it != this->query_params.begin())
                        ss << "&";
                    ss << it->first << "=" << it->second;
                }
                this->cgi_env["QUERY_STRING"] = ss.str();
            }
            
            return true;
        }
    }
    
    return false;
}

// Implementation for parseCookies method
bool Request::parseCookies()
{
    ConstHeaderIterator it = this->headers.find("cookie");
    if (it != this->headers.cend())
    {
        std::string cookie_str = it->second;
        std::string::size_type pos = 0;
        std::string::size_type prev = 0;
        
        while ((pos = cookie_str.find(';', prev)) != std::string::npos)
        {
            std::string cookie = cookie_str.substr(prev, pos - prev);
            std::string::size_type equals_pos = cookie.find('=');
            
            if (equals_pos != std::string::npos)
            {
                std::string key = cookie.substr(0, equals_pos);
                std::string value = cookie.substr(equals_pos + 1);
                
                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                this->cookies[key] = value;
            }
            
            prev = pos + 1;
        }
        
        // Handle the last cookie
        std::string cookie = cookie_str.substr(prev);
        std::string::size_type equals_pos = cookie.find('=');
        
        if (equals_pos != std::string::npos)
        {
            std::string key = cookie.substr(0, equals_pos);
            std::string value = cookie.substr(equals_pos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            this->cookies[key] = value;
        }
        
        return true;
    }
    
    return false;
}

// Implementation for validateMethod method
bool Request::validateMethod() const
{
    // Check if the method is one of the supported ones (GET, POST, DELETE)
    return (this->method == "GET" || this->method == "POST" || this->method == "DELETE");
}

// Implementation for validatePath method
bool Request::validatePath() const
{
    // Basic path validation
    if (this->path.empty())
        return false;
    
    // Check if the path is too long
    if (this->path.length() > MAX_PATH_SIZE)
        return false;
    
    // Make sure the path starts with '/'
    if (this->path[0] != '/')
        return false;
    
    return true;
}

// Implementation for validateRequest method
void Request::validateRequest()
{
    // Check if the request has the required components
    if (this->method.empty() || this->path.empty() || this->version.empty())
    {
        this->error_code = BAD_REQ;
        this->is_valid = false;
        return;
    }
    
    // Validate method
    if (!validateMethod())
    {
        this->error_code = BAD_REQ;
        this->is_valid = false;
        return;
    }
    
    // Validate path
    if (!validatePath())
    {
        this->error_code = URI_T_LONG;
        this->is_valid = false;
        return;
    }
    
    // Validate version
    if (this->version != "HTTP/1.1")
    {
        this->error_code = VERSION_ERR;
        this->is_valid = false;
        return;
    }
    
    // Validate host header (required for HTTP/1.1)
    ConstHeaderIterator host_it = this->headers.find("host");
    if (host_it == this->headers.cend())
    {
        this->error_code = BAD_REQ;
        this->is_valid = false;
        return;
    }
    
    // Validate content-length if present
    ConstHeaderIterator cl_it = this->headers.find("content-length");
    if (cl_it != this->headers.cend())
    {
        try {
            double content_length = std::stod(cl_it->second);
            if (content_length < 0)
            {
                this->error_code = BAD_REQ;
                this->is_valid = false;
                return;
            }
        } catch (const std::exception&) {
            this->error_code = BAD_REQ;
            this->is_valid = false;
            return;
        }
    }
    
    // All validations passed
    this->is_valid = true;
}
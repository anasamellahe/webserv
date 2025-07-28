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

// Default constructor
Request::Request() 
{
    this->clientFD = -1;
    this->is_valid = false;
    this->is_chunked = false;
    this->Port = 0;
    this->isIp = false;
    this->error_code.clear();
}

// Constructor with client FD
Request::Request(int clientFD) 
{
    this->clientFD = clientFD;
    this->is_valid = false;
    this->is_chunked = false;
    this->Port = 0;
    this->isIp = false;
    this->error_code.clear();
}

// Destructor
Request::~Request() 
{
    // Destructor logic if needed
}

bool Request::parseFromSocket(int clientFD, const std::string& buffer, size_t size)
{
    this->clientFD = clientFD;
    this->requestContent = buffer.substr(0, size);
    
    try {
        // Find the end of headers (double CRLF)
        size_t header_end = this->requestContent.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            return false; // Incomplete request, need more data
        }
        
        // Extract header section
        std::string headers_section = this->requestContent.substr(0, header_end + 2);
        
        // Split headers into lines
        std::vector<std::string> header_lines;
        size_t pos = 0;
        size_t prev = 0;
        while ((pos = headers_section.find("\r\n", prev)) != std::string::npos) {
            header_lines.push_back(headers_section.substr(prev, pos - prev));
            prev = pos + 2;
        }
        
        if (header_lines.empty()) {
            this->error_code = BAD_REQ;
            return false;
        }
        
        // Parse the request line (first line)
        parseStartLine(header_lines[0]);
        
        // Parse the rest of the headers
        for (size_t i = 1; i < header_lines.size(); i++) {
            parseHeaders(header_lines[i]);
        }
        
        // Parse query string if present in the path
        size_t query_pos = this->path.find('?');
        if (query_pos != std::string::npos) {
            std::string query_string = this->path.substr(query_pos + 1);
            this->path = this->path.substr(0, query_pos);
            parseQueryString();
        }
        
        // Parse body if present
        if (header_end + 4 < this->requestContent.length()) {
            std::string body_data = this->requestContent.substr(header_end + 4);
            
            // Check for chunked encoding
            ConstHeaderIterator transfer_encoding = this->headers.find("transfer-encoding");
            if (transfer_encoding != this->headers.cend() && 
                transfer_encoding->second.find("chunked") != std::string::npos) {
                parseChunkedTransfer(body_data);
            } else {
                parseBody(body_data);
            }
        }
        
        // Extract cookies if present
        parseCookies();
        
        // Check for CGI requests
        extractCgiInfo();
        
        // Validate the request
        validateRequest();
        
        return this->is_valid;
    }
    catch (int error_code) {
        this->is_valid = false;
        return false;
    }
}

void Request::setMethod(const std::string& method)
{
    if (this->method.empty() && !method.empty())
        this->method = method;
    else if (error_code.empty())
        this->error_code = BAD_REQ;
}

void Request::setPath(const std::string& path)
{
    if (path.size() > MAX_PATH_SIZE && error_code.empty())
        this->error_code = URI_T_LONG;
    else if (this->path.empty() && !path.empty())
        this->path = path;
    else if (error_code.empty())
        this->error_code = BAD_REQ;
}

void Request::setVersion(const std::string& version)
{
    if (this->version.empty() && !version.empty() && version.compare("HTTP/1.1") == 0)
        this->version = version;
    else if (this->error_code.empty())
        this->error_code = VERSION_ERR;
}

void Request::parseStartLine(const std::string& line)
{
    size_t pos = std::string::npos;

    const char* methods[] = {"GET", "POST", "DELETE", NULL}; // Changed to const char*
    for (size_t i = 0; methods[i]; i++)
    {
        if ((pos = line.find(methods[i], 0)) != std::string::npos)
            break;
    }
    if (pos == std::string::npos)
        throw -1;
        
    std::string str;
    std::stringstream ss(line);
    ss >> str;
    setMethod(str);
    ss >> str;
    setPath(str);
    ss >> str;
    setVersion(str);
    if (!this->error_code.empty())
        throw 1;
}

bool Request::parseHeaders(const std::string& headers_text)
{
    std::cout << headers_text<< std::endl;
    std::string key, value;
    size_t pos = headers_text.find(":", 0);
    if (pos == std::string::npos)
    {
        this->error_code = BAD_REQ;
        throw 1;
    }
    key = headers_text.substr(0, pos);
    stringToLower(key);
    size_t valueStartIndex = headers_text.find_first_not_of(": \r\t\v\f", pos);
    if (valueStartIndex == std::string::npos)
        valueStartIndex = pos;
    value = headers_text.substr(valueStartIndex, headers_text.rfind("\r\n") - valueStartIndex);
    stringToLower(value);
    if (keyValidationNot(key) || valueValidationNot(value))
    {
        if (this->error_code.empty())
            this->error_code = BAD_REQ;
        throw 1;
    }

    if (key == "host" && value.empty())
    {
        if (this->error_code.empty())
            this->error_code = BAD_REQ;
        throw 1;
    }
    this->headers.insert(std::pair<std::string, std::string>(key, value));
    return true;
}



const std::string& Request::getMethod() const
{
    return this->method;
}

const std::string& Request::getPath() const
{
    return this->path;
}

const std::string& Request::getVersion() const
{
    return this->version;
}

const std::string& Request::getHeader(const std::string& key) const
{
    ConstHeaderIterator it = this->headers.find(key);
    if (it != headers.cend())
        return it->second;
    return key;
}

const std::string& Request::getBody() const
{
    return this->body;
}

int Request::getClientFD() const
{
    return this->clientFD;
}

sockaddr_in Request::getClientAddr() const
{
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
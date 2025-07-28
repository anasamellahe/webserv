#include "Request.hpp"

Request::Request(int clientFD) 
{
    this->clientFD = clientFD;
    this->is_valid = false;
    this->is_chunked = false;
    this->Port = 0;
    this->isIp = false;
    this->error_code.clear();
}

Request::Request() 
{
    this->clientFD = -1; // Default value for testing
    this->is_valid = false;
    this->is_chunked = false;
    this->Port = 0;
    this->isIp = false;
    this->error_code.clear();
}

Request::~Request() 
{
    // Destructor logic if needed
}

bool Request::parseFromSocket(int clientFD, const std::string& buffer, size_t size)
{
    this->clientFD = clientFD;

    std::string requestData(buffer, size);
    
    // Find the end of headers
    size_t headerEnd = requestData.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
    {
        this->error_code = BAD_REQ;
        return false;
    }
    
    // Parse Request line and headers
    std::string headers = requestData.substr(0, headerEnd);
    if (!parseRequestLine(headers))
    {
        return false;
    }
    
    // Parse query parameters if any
    parseQueryString();
    
    // Validate Host header
    std::string host = getHeader("host");
    if (host.empty() || !validateHost(host))
    {
        if (this->error_code.empty())
            this->error_code = BAD_REQ;
        return false;
    }
    
    // Handle body if present
    if (headerEnd + 4 < requestData.size())
    {
        this->body = requestData.substr(headerEnd + 4);
    }
    
    this->is_valid = true;
    return true;
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
    const char* methods[] = {"GET", "POST", "DELETE", NULL};
    
    for (size_t i = 0; methods[i]; i++)
    {
        pos = line.find(methods[i], 0);
        if (pos != std::string::npos)
            break;
    }
    
    if (pos == std::string::npos)
    {
        this->error_code = BAD_REQ;
        return;
    }
    
    std::string str;
    std::stringstream ss(line);
    ss >> str;
    setMethod(str);
    ss >> str;
    setPath(str);
    ss >> str;
    setVersion(str);
}

bool Request::parseHeaders(const std::string& headers_text)
{
    std::istringstream iss(headers_text);
    std::string line;
    
    // Parse the first line (Request line)
    if (std::getline(iss, line))
    {
        if (line.back() == '\r')
            line.pop_back();
        parseStartLine(line);
    }
    
    // Parse the rest of the headers
    while (std::getline(iss, line))
    {
        if (line.back() == '\r')
            line.pop_back();
        
        if (line.empty())
            break;
        
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos)
        {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Convert key to lowercase for case-insensitive comparisons
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            
            headers[key] = value;
        }
    }
    
    return true;
}

bool Request::parseSingleLineHeader(const std::string line)
{
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos)
        return false;
    
    std::string key = line.substr(0, colonPos);
    std::string value = line.substr(colonPos + 1);
    
    // Trim whitespace
    key.erase(0, key.find_first_not_of(" \t"));
    key.erase(key.find_last_not_of(" \t") + 1);
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t") + 1);
    
    // Convert key to lowercase for case-insensitive comparisons
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    
    headers[key] = value;
    return true;
}

bool Request::parseRequestLine(const std::string& line)
{
    std::istringstream iss(line);
    std::string method, path, version;
    
    iss >> method >> path >> version;
    
    if (method.empty() || path.empty() || version.empty())
    {
        this->error_code = BAD_REQ;
        return false;
    }
    
    setMethod(method);
    setPath(path);
    setVersion(version);
    
    return this->error_code.empty();
}

bool Request::parseQueryString()
{
    size_t questionPos = this->path.find('?');
    if (questionPos == std::string::npos)
        return true; // No query string, which is fine
    
    std::string queryStr = this->path.substr(questionPos + 1);
    std::string actualPath = this->path.substr(0, questionPos);
    this->path = actualPath; // Update path to remove query string
    
    std::istringstream queryStream(queryStr);
    std::string pair;
    
    while (std::getline(queryStream, pair, '&'))
    {
        size_t equalsPos = pair.find('=');
        if (equalsPos != std::string::npos)
        {
            std::string key = pair.substr(0, equalsPos);
            std::string value = pair.substr(equalsPos + 1);
            query_params[key] = value;
        }
        else
        {
            // Key with no value
            query_params[pair] = "";
        }
    }
    
    return true;
}

bool Request::validateHost(const std::string& host)
{
    // Basic validation - could be expanded
    return !host.empty();
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
    static const std::string empty_string;
    
    std::string lowercase_key = key;
    std::transform(lowercase_key.begin(), lowercase_key.end(), lowercase_key.begin(), ::tolower);
    
    auto it = headers.find(lowercase_key);
    if (it != headers.end())
        return it->second;
    
    return empty_string;
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

void Request::setErrorCode(const std::string& error_code)
{
    this->error_code = error_code;
}

const std::string& Request::getErrorCode() const
{
    return this->error_code;
}

const std::map<std::string, std::string>& Request::getQueryParams() const
{
    return this->query_params;
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

const std::map<std::string, std::string>& Request::getAllHeaders() const
{
    return this->headers;
}

bool Request::isComplete() const
{
    // For simplicity, assume the Request is complete if it's valid
    return this->is_valid;
}

bool Request::setComplete(bool complete)
{
    this->is_valid = complete;
    return this->is_valid;
}

double Request::getContentLength()
{
    const std::string& contentLengthStr = getHeader("content-length");
    if (contentLengthStr.empty())
        return 0;
    
    try {
        return std::stod(contentLengthStr);
    } catch (const std::exception&) {
        return 0;
    }
}

void Request::addUpload(const std::string& key, const FilePart& file_part)
{
    this->uploads[key] = file_part;
}

bool Request::isChunked() const
{
    const std::string& transferEncoding = getHeader("transfer-encoding");
    return transferEncoding.find("chunked") != std::string::npos;
}

bool Request::isValid()
{
    return this->is_valid && this->error_code.empty();
}

Config Request::getserverConfig(std::string serverToFind, const Config& serversConfigs)
{
    // This is a placeholder implementation
    return serversConfigs;
}

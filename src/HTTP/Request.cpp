#include "Request.hpp"
#include "../Config/ConfigParser.hpp"

Request::Request() {
    this->clientFD = -1;
    this->is_valid = false;
    this->is_chunked = false;
    this->Port = -1;
    this->isIp = false;
    this->is_Complete = false;
    this->configSet = false;  // Initialize server config flag
    this->error_code.clear();
}

Request::Request(int clientFD) {
    this->clientFD = clientFD;
    this->is_valid = false;
    this->is_chunked = false;
    this->Port = -1;
    this->isIp = false;
    this->is_Complete = false;
    this->configSet = false;  // Initialize server config flag
    this->error_code.clear();
}

Request::~Request() {}

void Request::reset() {
    this->method.clear();
    this->path.clear();
    this->version.clear();
    this->headers.clear();
    this->body.clear();
    this->error_code.clear();
    this->Host.clear();
    this->is_Complete = false;
    this->Port = -1;
    this->isIp = false;
    this->query_params.clear();
    this->uploads.clear();
    this->cgi_extension.clear();
    this->cgi_env.clear();
    this->cookies.clear();
    this->is_chunked = false;
    this->chunks.clear();
    this->is_valid = false;
    this->configSet = false;  // Reset server config flag
}

bool Request::parseFromSocket(int clientFD, const std::string& buffer, size_t size) {
    // Back-compat helper: delegate to split parsing
    this->reset();
    this->clientFD = clientFD;
    this->requestContent = buffer.substr(0, size);
    try {
        size_t header_end = this->requestContent.find("\r\n\r\n");
        if (header_end == std::string::npos) return false;
        if (!parseHeadersSection(this->requestContent.substr(0, header_end))) return false;
        std::string body = (header_end + 4 < this->requestContent.size()) ? this->requestContent.substr(header_end + 4) : std::string();
        if (!parseBodySection(body)) return false;
        return this->is_valid;
    } catch (...) {
        this->is_valid = false;
        return false;
    }
}

bool Request::parseHeadersSection(const std::string& headers_section) {
    std::vector<std::string> header_lines;
    size_t pos = 0, prev = 0;
    while ((pos = headers_section.find("\r\n", prev)) != std::string::npos) {
        header_lines.push_back(headers_section.substr(prev, pos - prev));
        prev = pos + 2;
    }
    if (prev < headers_section.size()) header_lines.push_back(headers_section.substr(prev));
    if (header_lines.empty()) { this->error_code = BAD_REQ; return false; }

    std::vector<std::string> merged;
    for (size_t i = 0; i < header_lines.size(); ++i) {
        const std::string &ln = header_lines[i];
        if (!ln.empty() && (ln[0] == ' ' || ln[0] == '\t')) {
            if (!merged.empty()) {
                size_t cont = ln.find_first_not_of(" \t");
                std::string part = (cont == std::string::npos) ? std::string() : ln.substr(cont);
                if (!part.empty()) merged.back() += std::string(" ") + part;
            } else {
                merged.push_back(ln);
            }
        } else {
            merged.push_back(ln);
        }
    }
    header_lines.swap(merged);

    try { parseStartLine(header_lines[0]); }
    catch (...) { this->is_valid = false; return false; }

    for (size_t i = 1; i < header_lines.size(); ++i) {
        try { parseHeaders(header_lines[i]); }
        catch (...) { this->is_valid = false; return false; }
    }

    size_t qpos = this->path.find('?');
    if (qpos != std::string::npos) { this->path = this->path.substr(0, qpos); parseQueryString(); }
    return true;
}

void Request::debugPrint() const {
    std::cout << "---- Request debug dump ----\n";
    std::cout << "clientFD: " << this->clientFD << "\n";
    std::cout << "configSet: " << (this->configSet ? "true" : "false") << "\n";
    std::cout << "requestContent(size): " << this->requestContent.size() << "\n";
    std::cout << "Method: " << this->method << "\n";
    std::cout << "Path: " << this->path << "\n";
    std::cout << "Version: " << this->version << "\n";
    std::cout << "Host: " << this->Host << "\n";
    std::cout << "is_Complete: " << (this->is_Complete ? "true" : "false") << "\n";
    std::cout << "Port: " << this->Port << "\n";
    std::cout << "isIp: " << (this->isIp ? "true" : "false") << "\n";
    std::cout << "is_valid: " << (this->is_valid ? "true" : "false") << "\n";
    std::cout << "error_code: " << this->error_code << "\n";

    std::cout << "Headers(" << this->headers.size() << "):\n";
    for (ConstHeaderIterator it = this->headers.begin(); it != this->headers.end(); ++it) {
        std::cout << "  " << it->first << ": " << it->second << "\n";
    }

    std::cout << "Body(size): " << this->body.size() << "\n";
    if (!this->body.empty()) {
        std::cout << " Body(truncated, 200 chars): ";
        std::cout << this->body.substr(0, std::min((size_t)200, this->body.size())) << "\n";
    }

    std::cout << "Query params(" << this->query_params.size() << "):\n";
    for (std::map<std::string, std::string>::const_iterator it = this->query_params.begin(); it != this->query_params.end(); ++it) {
        std::cout << "  " << it->first << " = " << it->second << "\n";
    }

    std::cout << "Uploads(" << this->uploads.size() << "):\n";
    for (std::map<std::string, FilePart>::const_iterator it = this->uploads.begin(); it != this->uploads.end(); ++it) {
        std::cout << "  field: " << it->first << ", filename: " << it->second.filename << ", content-type: " << it->second.content_type << ", size: " << it->second.content.size() << "\n";
    }

    std::cout << "CGI extension: " << this->cgi_extension << "\n";
    std::cout << "CGI env(" << this->cgi_env.size() << "):\n";
    for (std::map<std::string, std::string>::const_iterator it = this->cgi_env.begin(); it != this->cgi_env.end(); ++it) {
        std::cout << "  " << it->first << " = " << it->second << "\n";
    }

    char client_ip[INET_ADDRSTRLEN] = "";
    if (this->client_addr.sin_family == AF_INET) {
        inet_ntop(AF_INET, &(this->client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    }
    std::cout << "client_addr: " << client_ip << ":" << ntohs(this->client_addr.sin_port) << "\n";

    std::cout << "cookies(" << this->cookies.size() << "):\n";
    for (std::map<std::string, std::string>::const_iterator it = this->cookies.begin(); it != this->cookies.end(); ++it) {
        std::cout << "  " << it->first << " = " << it->second << "\n";
    }

    std::cout << "is_chunked: " << (this->is_chunked ? "true" : "false") << "\n";
    std::cout << "chunks count: " << this->chunks.size() << "\n";

    // Print a short summary of server config if present
    if (!this->fullServerConfig.servers.empty()) {
        std::cout << "fullServerConfig.servers count: " << this->fullServerConfig.servers.size() << "\n";
    } else {
        std::cout << "fullServerConfig: empty\n";
    }

    std::cout << "serverConfig summary:\n";
    std::cout << "  host: " << this->serverConfig.host << "\n";
    std::cout << "  ports(" << this->serverConfig.ports.size() << "):\n";
    for (size_t i = 0; i < this->serverConfig.ports.size(); ++i) std::cout << "    " << this->serverConfig.ports[i] << "\n";
    std::cout << "  server_names(" << this->serverConfig.server_names.size() << "):\n";
    for (size_t i = 0; i < this->serverConfig.server_names.size(); ++i) std::cout << "    " << this->serverConfig.server_names[i] << "\n";

    std::cout << "-----------------------------\n";
}

bool Request::parseBodySection(const std::string& body_section) {
    // Delegate all body assembly/validation to parseBodyByType which will
    // enforce Content-Length, handle chunked decoding, and then dispatch
    // to the specific parsers based on content-type.
    if (!parseBodyByType(body_section)) {
        this->is_valid = false;
        return false;
    }

    parseCookies();
    extractCgiInfo();
    validateRequest();
    return this->is_valid;
}

bool Request::parseBodyByType(const std::string& raw_body_section) {
    // If Transfer-Encoding: chunked -> attempt to decode the chunked payload
    ConstHeaderIterator te = this->headers.find("transfer-encoding");
    if (te != this->headers.end() && te->second.find("chunked") != std::string::npos) {
        // parseChunkedTransfer will populate this->body on success
        if (!parseChunkedTransfer(raw_body_section)) {
            // incomplete or protocol error (parseChunkedTransfer sets error_code on fatal)
            return false;
        }
    } else {
        // Enforce Content-Length if present
        ConstHeaderIterator cl_it = this->headers.find("content-length");
        if (cl_it != this->headers.end()) {
            unsigned long expected_length = strtoul(cl_it->second.c_str(), NULL, 10);
            if (raw_body_section.length() != expected_length) {
                if (raw_body_section.length() < expected_length) {
                    // incomplete body
                    return false;
                }
                this->error_code = BAD_REQ;
                return false;
            }
        }

        // No chunked encoding: accept the provided raw body as the assembled body
        this->body = raw_body_section;
    }

    // At this point this->body contains the assembled payload. Now dispatch
    // to the appropriate parser based on Content-Type.
    ConstHeaderIterator it = this->headers.find("content-type");
    std::string content_type = (it != this->headers.end()) ? it->second : std::string();

    if (content_type.find("multipart/form-data") != std::string::npos) {
        size_t boundary_pos = content_type.find("boundary=");
        if (boundary_pos != std::string::npos) {
            std::string boundary = content_type.substr(boundary_pos + 9);
            if (!boundary.empty() && boundary[0] == '"' && boundary[boundary.length() - 1] == '"') {
                if (boundary.length() >= 2)
                    boundary = boundary.substr(1, boundary.length() - 2);
            }
            return parseBodyMultipart(this->body, boundary);
        }
        // no boundary -> treat as generic
        return parseBodyGeneric(this->body, content_type);
    }

    if (content_type.find("application/x-www-form-urlencoded") != std::string::npos) {
        return parseBodyUrlEncoded(this->body);
    }

    // Heuristic: if header present and starts with text/ or application/, treat as raw text
    if (!content_type.empty()) {
        if (content_type.find("text/") == 0 || content_type.find("application/json") != std::string::npos || content_type.find("application/xml") != std::string::npos) {
            return parseBodyRaw(this->body);
        }
        // For octet-stream treat as binary
        if (content_type.find("application/octet-stream") != std::string::npos) {
            return parseBodyBinary(this->body);
        }
    }

    // Fallback: generic parser
    return parseBodyGeneric(this->body, content_type);
}

bool Request::parseBodyMultipart(const std::string& body_data, const std::string& boundary) {
    // Reuse existing multipart parser
    return parseMultipartBody(body_data, boundary);
}

bool Request::parseBodyUrlEncoded(const std::string& body_data) {
    return parseFormData(body_data);
}

bool Request::parseBodyRaw(const std::string& body_data) {
    // store as-is
    this->body = body_data;
    return true;
}

bool Request::parseBodyBinary(const std::string& body_data) {
    // store raw binary payload in body (as bytes in string)
    this->body = body_data;
    return true;
}

bool Request::parseBodyGeneric(const std::string& body_data, const std::string& content_type) {
    // Default generic parsing: keep raw body and let higher-level handlers decide
    (void)content_type;
    this->body = body_data;
    return true;
}

void Request::setChunked(bool ischunked)
{
    this->is_chunked = ischunked;
}

bool Request::hasChunkedEncoding() const {
    ConstHeaderIterator it = this->headers.find("transfer-encoding");
    return (it != this->headers.end() && it->second.find("chunked") != std::string::npos);
}

size_t Request::expectedContentLength() const {
    ConstHeaderIterator it = this->headers.find("content-length");
    if (it == this->headers.end()) return 0;
    char *end = NULL;
    unsigned long v = strtoul(it->second.c_str(), &end, 10);
    if (end == it->second.c_str()) {
        return 0;
    }
    return static_cast<size_t>(v);
}

void Request::setMethod(const std::string& method) {
    if (this->method.empty() && !method.empty()) {
        this->method = method;
    }
    else if (error_code.empty()) {
        this->error_code = BAD_REQ;
    }
}

void Request::setPath(const std::string& path) {
    if (path.size() > MAX_PATH_SIZE && error_code.empty()) {
        this->error_code = URI_T_LONG;
    }
    else if (this->path.empty() && !path.empty()) {
        this->path = urlDecode(path);  // Decode URL-encoded characters like %20
    }
    else if (error_code.empty()) {
        this->error_code = BAD_REQ;
    }
}

void Request::setVersion(const std::string& version) {
    if (this->version.empty() && !version.empty() && version.compare("HTTP/1.1") == 0) {
        this->version = version;
    }
    else if (this->error_code.empty()) {
        this->error_code = VERSION_ERR;
    }
}

void Request::parseStartLine(const std::string& line) {
    size_t pos = std::string::npos;
    // Allow parsing of common HTTP methods so they can get proper 501 responses
    const char* methods[] = {"GET", "POST", "DELETE", "HEAD", "PUT", "PATCH", "OPTIONS", "TRACE", "CONNECT", NULL};
    
    
    for (size_t i = 0; methods[i]; i++) {
        if ((pos = line.find(methods[i], 0)) != std::string::npos) {
            break;
        }
    }
    
    if (pos == std::string::npos) {
        throw -1;
    }
    
    try {
        std::string str;
        std::stringstream ss(line);
        
        ss >> str;
        setMethod(str);
        
        ss >> str;
        setPath(str);
        
        ss >> str;
        setVersion(str);
        
        if (!this->error_code.empty()) {
            throw 1;
        }
    }
    catch (const std::exception& e) {
        throw 1;
    }
}

const std::string& Request::getMethod() const {
    return this->method;
}

const std::string& Request::getPath() const {
    return this->path;
}

const std::string& Request::getVersion() const {
    return this->version;
}

const std::string& Request::getHeader(const std::string& key) const {
    ConstHeaderIterator it = this->headers.find(key);
    if (it != headers.end()) {
        return it->second;
    }
    return key;
}

const std::string& Request::getBody() const {
    return this->body;
}

int Request::getClientFD() const {
    return this->clientFD;
}

sockaddr_in Request::getClientAddr() const {
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(this->client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    return this->client_addr;
}

void Request::setHeader(const std::string& key, const std::string& value) {
    if (!key.empty() && !value.empty())
        this->headers[key] = value;
}

void Request::setBody(const std::string& body) {
    this->body = body;
}

void Request::setClientFD(int fd) {
    this->clientFD = fd;
}

void Request::setClientAddr(const sockaddr_in& addr) {
    this->client_addr = addr;
}

void Request::setErrorCode(const std::string& error_code) {
    this->error_code = error_code;
}

const std::string& Request::getErrorCode() const {
    if (!this->error_code.empty())
        return this->error_code;
    static std::string default_ok = "200 OK";
    return default_ok;
}

void Request::addQueryParam(const std::string& key, const std::string& value) {
    if (!key.empty())
        this->query_params.insert(std::pair<std::string, std::string>(key, value));
}

void Request::parseQueryPair(const std::string &query, size_t start, size_t end) {
    size_t pos = query.find("=", start);
    if (pos != std::string::npos && pos <= end) {
        std::string key = query.substr(start, pos - start);
        std::string value = query.substr(pos + 1, end - pos - 1);
        
        key = urlDecode(key);
        value = urlDecode(value);
        
        addQueryParam(key, value);
    }
}

bool Request::parseQueryString() {
    size_t pos = path.find("?", 0);

    if (pos == std::string::npos)
        return false;
        
    std::string queryParams;
    queryParams = path.substr(pos + 1, std::string::npos);
    path.erase(pos);
    
    size_t start = 0;
    size_t end = 0;
    while (start < queryParams.size()) {
        end = queryParams.find("&", start);
        if (end != std::string::npos) {
            parseQueryPair(queryParams, start, end - 1);
            start = end + 1;
        }
        else {
            parseQueryPair(queryParams, start, queryParams.size() - 1);
            break;
        }
    }
    return true;
}

const std::map<std::string, std::string>& Request::getQueryParams() const {
    return this->query_params;
}

void Request::setHost(const std::string& host) {
    this->Host = host;
}

void Request::setPort(int port) {
    if (isValidPort(port)) {
        this->Port = port;
    } else {
        this->Port = 80;
    }
}

bool Request::isValidIpAddress(std::string& hostValue) {
    int octet1, octet2, octet3, octet4, portValue, consumed;
    portValue = -1;
    
    int result = sscanf(hostValue.c_str(), "%d.%d.%d.%d:%d%n", 
                        &octet1, &octet2, &octet3, &octet4, &portValue, &consumed);
    
    if ((octet1 >= 0 && octet1 <= 255) &&
        (octet2 >= 0 && octet2 <= 255) &&
        (octet3 >= 0 && octet3 <= 255) &&
        (octet4 >= 0 && octet4 <= 255) &&
        (static_cast<size_t>(consumed) == hostValue.length())) {
        

        if (result == 5) {
            if (isValidPort(portValue)) {
                this->Port = portValue;
                hostValue = hostValue.substr(0, hostValue.find(':'));
                setHost(hostValue);
                this->isIp = true;
                return true;
            }
            return false;
        } 
        else if (result == 4) {
            setHost(hostValue);
            this->Port = 80; 
            this->isIp = true;
            return true;
        }
    }
    return false;
}

bool Request::isValidDomainName(std::string& hostValue) {
    for (size_t i = 0; i < hostValue.length(); i++) {
        if (!isalnum(hostValue[i]) && hostValue[i] != '.' && hostValue[i] != '-' && hostValue[i] != ':')
            return false;
        if (i == 0 && (hostValue[i] == '.' || hostValue[i] == '-' || hostValue[i] == ':'))
            return false;
    }
    
    size_t colonPos = hostValue.find(':');
    if (colonPos != std::string::npos) {
        std::string portPart = hostValue.substr(colonPos + 1);
            for (size_t i = 0; i < portPart.length(); i++) {
                if (!isdigit(portPart[i]))
                    return false;
            }
            int port = atoi(portPart.c_str());
            if (isValidPort(port)) {
                this->Port = port;
                hostValue = hostValue.substr(0, colonPos);
                setHost(hostValue);
            } else {
                return false;
            }
    } else {
        setHost(hostValue);
        this->Port = -1;
    }
    
    this->isIp = false;
    return true;
}

bool Request::isValidHost(std::string& Host) {
    size_t pos = Host.find_first_not_of("0123456789.:");
    
    if (pos == std::string::npos) {
        return isValidIpAddress(Host);
    }
    
    return isValidDomainName(Host);
}

bool Request::parseHeaders(const std::string& headers_text) {
    std::string key, value;
    size_t pos = headers_text.find(":", 0);
    
    if (pos == std::string::npos) {
        this->error_code = BAD_REQ;
        throw 1;
    }
    
    key = headers_text.substr(0, pos);
    // Trim whitespace around key first
    size_t kstart = key.find_first_not_of(" \t");
    size_t kend = key.find_last_not_of(" \t");
    if (kstart == std::string::npos) {
        key.clear();
    } else {
        key = key.substr(kstart, kend - kstart + 1);
    }
    // Normalize key to lowercase for canonical lookup
    stringToLower(key);

    size_t valueStartIndex = headers_text.find_first_not_of(": \r\t\v\f", pos);
    if (valueStartIndex == std::string::npos) {
        valueStartIndex = pos + 1;
    }

    // Header line passed in does not contain CRLF terminator here (we split by CRLF earlier)
    value = headers_text.substr(valueStartIndex);
    // Trim whitespace around value and remove any trailing CR/LF
    size_t vstart = value.find_first_not_of(" \t\r\n\v\f");
    size_t vend = value.find_last_not_of(" \t\r\n\v\f");
    if (vstart == std::string::npos) {
        value.clear();
    } else {
        value = value.substr(vstart, vend - vstart + 1);
    }
    if (!isValidKey(key) || !isValidValue(value)){
        if (this->error_code.empty())
            this->error_code = BAD_REQ;
        throw 1;
    }

    if (key == "host") {
        if (value.empty()) {
            if (this->error_code.empty())
                this->error_code = BAD_REQ;
            throw 1;
        }
        
        std::string hostCopy = value;
        if (!isValidHost(hostCopy)) {
            if (this->error_code.empty())
                this->error_code = BAD_REQ;
            throw 1;
        }
        
        // Store the parsed host and port information for server matching
        this->Host = hostCopy;
        // Port is set by isValidHost methods (isValidIpAddress/isValidDomainName)
        if (this->Port == -1) {
            this->Port = 80; // Default port if none specified
        }
    }
    
    // Use assignment so repeated headers update the value (rather than silently failing insert)
    this->headers[key] = value;
    return true;
}

const std::map<std::string, std::string>& Request::getAllHeaders() const {
    return this->headers;
}

bool Request::isComplete() const {
    return this->is_Complete;
}

bool Request::setComplete(bool complete) {
    this->is_Complete = complete;
    return this->is_Complete;
}

double Request::getContentLength() {
    ConstHeaderIterator it = this->headers.find("content-length");
    if (it != this->headers.end()) {
        char *endptr = NULL;
        double val = strtod(it->second.c_str(), &endptr);
        if (endptr == it->second.c_str())
            return 0.0;
        return val;
    }
    return 0.0;
}

bool Request::isValid() {
    return this->is_valid;
}

const std::map<std::string, FilePart>& Request::getUploads() const {
    return this->uploads;
}

const std::map<std::string, std::string>& Request::getCGIEnv() const {
    return this->cgi_env;
}

const std::string& Request::getCGIExtension() const {
    return this->cgi_extension;
}

const std::map<std::string, std::string>& Request::getCookies() const {
    return this->cookies;
}

const std::vector<std::string>& Request::getChunks() const {
    return this->chunks;
}

bool Request::isChunked() const {
    return this->is_chunked;
}

void Request::addUpload(const std::string& key, const FilePart& file_part) {
    if (!key.empty())
        this->uploads[key] = file_part;
}

bool Request::parseBody(const std::string& body_data) {
    ConstHeaderIterator cl_it = this->headers.find("content-length");
    if (cl_it != this->headers.end()) {
        unsigned long expected_length = strtoul(cl_it->second.c_str(), NULL, 10);
        if (body_data.length() != expected_length) {
            if (body_data.length() < expected_length) {
                return false;
            }
            this->error_code = BAD_REQ;
            return false;
        }
    }
    
    if (body_data.empty()) {
        return true;
    }
    
    ConstHeaderIterator it = this->headers.find("content-type");
    if (it != this->headers.end()) {
        std::string content_type = it->second;
        if (content_type.find("multipart/form-data") != std::string::npos) {
            size_t boundary_pos = content_type.find("boundary=");
            if (boundary_pos != std::string::npos) {
                std::string boundary = content_type.substr(boundary_pos + 9);
                if (!boundary.empty() && boundary[0] == '"' && boundary[boundary.length() - 1] == '"') {
                    if (boundary.length() >= 2)
                        boundary = boundary.substr(1, boundary.length() - 2);
                }
                return parseMultipartBody(body_data, boundary);
            }
        } else if (content_type.find("application/x-www-form-urlencoded") != std::string::npos) {
            parseFormData(body_data);
        }
    }
    
    this->body = body_data;
    return true;
}

bool Request::parseMultipartBody(const std::string& body_data, const std::string& boundary) {
    if (boundary.empty() || body_data.empty()) {
        return false;
    }
    
    std::string delimiter = "--" + boundary;
    std::string end_delimiter = delimiter + "--";
    
    size_t pos = 0;
    size_t start_pos = body_data.find(delimiter, pos);
    
    while (start_pos != std::string::npos) {
        start_pos += delimiter.length();
        
        if (start_pos + 1 < body_data.length() && 
            body_data[start_pos] == '\r' && body_data[start_pos + 1] == '\n') {
            start_pos += 2;
        }
        
        size_t end_pos = body_data.find(delimiter, start_pos);
        if (end_pos == std::string::npos) {
            break;
        }
        
        std::string part_data = body_data.substr(start_pos, end_pos - start_pos);
        
        if (part_data.length() >= 2 && 
            part_data.substr(part_data.length() - 2) == "\r\n") {
            part_data = part_data.substr(0, part_data.length() - 2);
        }
        
        parseMultipartPart(part_data);
        
        pos = end_pos;
        start_pos = body_data.find(delimiter, pos);
        
        if (start_pos != std::string::npos) {
            size_t check_end = start_pos + delimiter.length();
            if (check_end + 1 < body_data.length() && 
                body_data.substr(check_end, 2) == "--") {
                break;
            }
        }
    }
    
    return true;
}

bool Request::parseMultipartPart(const std::string& part_data) {
    size_t headers_end = part_data.find("\r\n\r\n");
    if (headers_end == std::string::npos) {
        return false;
    }
    
    std::string headers_section = part_data.substr(0, headers_end);
    std::string body_section = part_data.substr(headers_end + 4);
    
    std::map<std::string, std::string> part_headers;
    std::stringstream ss(headers_section);
    std::string line;
    
    while (std::getline(ss, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line = line.substr(0, line.size() - 1);
        }
        
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            stringToLower(key);
            part_headers[key] = value;
        }
    }
    
    std::map<std::string, std::string>::iterator cd_it = part_headers.find("content-disposition");
    if (cd_it != part_headers.end()) {
        std::string disposition = cd_it->second;
        std::string name, filename;
        
        size_t name_pos = disposition.find("name=\"");
        if (name_pos != std::string::npos) {
            name_pos += 6;
            size_t name_end = disposition.find("\"", name_pos);
            if (name_end != std::string::npos) {
                name = disposition.substr(name_pos, name_end - name_pos);
            }
        }
        
        size_t filename_pos = disposition.find("filename=\"");
        if (filename_pos != std::string::npos) {
            filename_pos += 10;
            size_t filename_end = disposition.find("\"", filename_pos);
            if (filename_end != std::string::npos) {
                filename = disposition.substr(filename_pos, filename_end - filename_pos);
            }
        }
        
        if (!name.empty()) {
            if (!filename.empty()) {
                FilePart file_part;
                file_part.filename = filename;
                file_part.content = body_section;
                
                std::map<std::string, std::string>::iterator ct_it = part_headers.find("content-type");
                if (ct_it != part_headers.end()) {
                    file_part.content_type = ct_it->second;
                }
                
                addUpload(name, file_part);
            } else {
                addQueryParam(name, body_section);
            }
        }
    }
    
    return true;
}

bool Request::parseFormData(const std::string& form_data) {
    size_t start = 0;
    size_t pos = 0;
    
    while (start < form_data.length()) {
        pos = form_data.find('&', start);
        if (pos == std::string::npos) {
            pos = form_data.length();
        }
        
        std::string pair = form_data.substr(start, pos - start);
        size_t equals_pos = pair.find('=');
        
        if (equals_pos != std::string::npos) {
            std::string key = pair.substr(0, equals_pos);
            std::string value = pair.substr(equals_pos + 1);
            
            key = urlDecode(key);
            value = urlDecode(value);
            
            addQueryParam(key, value);
        }
        
        start = pos + 1;
    }
    
    return true;
}

std::string Request::urlDecode(const std::string& str) {
    std::string decoded;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            char hex1 = str[i + 1];
            char hex2 = str[i + 2];
            
            if (std::isxdigit(hex1) && std::isxdigit(hex2)) {
                int value = 0;
                if (hex1 >= '0' && hex1 <= '9') value += (hex1 - '0') * 16;
                else if (hex1 >= 'A' && hex1 <= 'F') value += (hex1 - 'A' + 10) * 16;
                else if (hex1 >= 'a' && hex1 <= 'f') value += (hex1 - 'a' + 10) * 16;
                
                if (hex2 >= '0' && hex2 <= '9') value += (hex2 - '0');
                else if (hex2 >= 'A' && hex2 <= 'F') value += (hex2 - 'A' + 10);
                else if (hex2 >= 'a' && hex2 <= 'f') value += (hex2 - 'a' + 10);
                
                decoded += static_cast<char>(value);
                i += 2;
            } else {
                decoded += str[i];
            }
        } else if (str[i] == '+') {
            decoded += ' ';
        } else {
            decoded += str[i];
        }
    }
    return decoded;
}

std::string Request::urlEncode(const std::string& str) {
    std::string encoded;
    for (size_t i = 0; i < str.length(); ++i) {
        char c = str[i];
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            encoded += '%';
            char hex1 = (c >> 4) & 0xF;
            char hex2 = c & 0xF;
            encoded += (hex1 < 10) ? ('0' + hex1) : ('A' + hex1 - 10);
            encoded += (hex2 < 10) ? ('0' + hex2) : ('A' + hex2 - 10);
        }
    }
    return encoded;
}

bool Request::parseChunkedTransfer(const std::string& chunked_data) {
    // If nothing provided yet, treat as incomplete
    if (chunked_data.empty()) return false;

    this->is_chunked = true;

    std::vector<std::string> decoded_chunks;
    std::string decoded_body;

    size_t pos = 0;
    const size_t len = chunked_data.length();

    while (true) {
        // find end of size line
        size_t crlf_pos = chunked_data.find("\r\n", pos);
        if (crlf_pos == std::string::npos) {
            // incomplete size line
            return false;
        }

        std::string size_line = chunked_data.substr(pos, crlf_pos - pos);
        // trim whitespace
        size_t sstart = size_line.find_first_not_of(" \t");
        size_t send = size_line.find_last_not_of(" \t");
        if (sstart == std::string::npos) {
            this->error_code = BAD_REQ;
            return false;
        }
        size_line = size_line.substr(sstart, send - sstart + 1);

        // strip chunk extensions
        size_t semi = size_line.find(';');
        std::string size_token = (semi == std::string::npos) ? size_line : size_line.substr(0, semi);

        // parse hex size
        char *endptr = NULL;
        unsigned long chunk_size = strtoul(size_token.c_str(), &endptr, 16);
        if (endptr == size_token.c_str()) {
            this->error_code = BAD_REQ;
            return false;
        }

        // move pos to payload start
        pos = crlf_pos + 2;

        // if terminating chunk
        if (chunk_size == 0) {
            // minimal terminator is an immediate CRLF after the 0-chunk (no trailers)
            if (pos + 2 > len) return false; // need more data
            if (chunked_data[pos] == '\r' && chunked_data[pos + 1] == '\n') {
                // success
                this->chunks.swap(decoded_chunks);
                this->body.swap(decoded_body);
                return true;
            }
            // otherwise trailers present; they must end with CRLFCRLF
            size_t trailers_end = chunked_data.find("\r\n\r\n", pos);
            if (trailers_end == std::string::npos) return false; // need more data
            this->chunks.swap(decoded_chunks);
            this->body.swap(decoded_body);
            return true;
        }

        // ensure we have payload + CRLF available
        if (pos + chunk_size + 2 > len) return false; // incomplete payload

        // extract payload
        decoded_chunks.push_back(chunked_data.substr(pos, chunk_size));
        decoded_body.append(chunked_data, pos, chunk_size);

        pos += chunk_size;

        // expect CRLF after payload
        if (pos + 1 >= len || chunked_data[pos] != '\r' || chunked_data[pos + 1] != '\n') {
            this->error_code = BAD_REQ;
            return false;
        }
        pos += 2; // move to next size line

        // if we've consumed everything, wait for more (incomplete)
        if (pos >= len) return false;
    }

    // unreachable
    return false;
}

bool Request::extractCgiInfo() {
    size_t dot_pos = this->path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        std::string extension = this->path.substr(dot_pos);
        if (extension == ".php" || extension == ".cgi" || extension == ".py") {
            this->cgi_extension = extension;
            
            this->cgi_env["QUERY_STRING"] = "";
            this->cgi_env["REQUEST_METHOD"] = this->method;
            this->cgi_env["CONTENT_TYPE"] = this->getHeader("content-type");
            this->cgi_env["CONTENT_LENGTH"] = this->getHeader("content-length");
            this->cgi_env["SCRIPT_NAME"] = this->path;
            
            if (!this->query_params.empty()) {
                std::stringstream ss;
                for (std::map<std::string, std::string>::const_iterator it = this->query_params.begin();
                     it != this->query_params.end(); ++it) {
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

bool Request::parseCookies() {
    ConstHeaderIterator it = this->headers.find("cookie");
    if (it != this->headers.end()) {
        std::string cookie_str = it->second;
        std::string::size_type pos = 0;
        std::string::size_type prev = 0;
        
        while ((pos = cookie_str.find(';', prev)) != std::string::npos) {
            std::string cookie = cookie_str.substr(prev, pos - prev);
            std::string::size_type equals_pos = cookie.find('=');
            
            if (equals_pos != std::string::npos) {
                std::string key = cookie.substr(0, equals_pos);
                std::string value = cookie.substr(equals_pos + 1);
                
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                this->cookies[key] = value;
            }
            
            prev = pos + 1;
        }
        
        std::string cookie = cookie_str.substr(prev);
        std::string::size_type equals_pos = cookie.find('=');
        
        if (equals_pos != std::string::npos) {
            std::string key = cookie.substr(0, equals_pos);
            std::string value = cookie.substr(equals_pos + 1);
            
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

bool Request::validateMethod() const {
    // Allow common HTTP methods to pass parsing - 501 handling is done in response generation
    return (this->method == "GET" || this->method == "POST" || this->method == "DELETE" ||
            this->method == "HEAD" || this->method == "PUT" || this->method == "PATCH" ||
            this->method == "OPTIONS" || this->method == "TRACE" || this->method == "CONNECT");
}

bool Request::validatePath() const {
    if (this->path.empty())
        return false;
    
    if (this->path.length() > MAX_PATH_SIZE)
        return false;
    
    if (this->path[0] != '/')
        return false;
    
    return true;
}

void Request::validateRequest() {
    if (this->method.empty() || this->path.empty() || this->version.empty()) {
        this->error_code = BAD_REQ;
        this->is_valid = false;
        return;
    }
    
    if (!validateMethod()) {
        this->error_code = BAD_REQ;
        this->is_valid = false;
        return;
    }
    
    if (!validatePath()) {
        this->error_code = URI_T_LONG;
        this->is_valid = false;
        return;
    }
    
    if (this->version != "HTTP/1.1") {
        this->error_code = VERSION_ERR;
        this->is_valid = false;
        return;
    }
    
    ConstHeaderIterator host_it = this->headers.find("host");
    if (host_it == this->headers.end()) {
        this->error_code = BAD_REQ;
        this->is_valid = false;
        return;
    }
    
    ConstHeaderIterator cl_it = this->headers.find("content-length");
    if (cl_it != this->headers.end()) {
        char *endptr = NULL;
        double content_length = strtod(cl_it->second.c_str(), &endptr);
        if (endptr == cl_it->second.c_str() || content_length < 0) {
            this->error_code = BAD_REQ;
            this->is_valid = false;
            return;
        }
    }
    
    this->is_valid = true;
}

// Server configuration methods implementation
void Request::setServerConfig(const Config& config) {
    this->fullServerConfig = config;
    this->configSet = true;
}

const Config& Request::getServerConfig() const {
    if (!configSet) {
        throw std::runtime_error("Server configuration not set for this request");
    }
    return fullServerConfig;
}

bool Request::hasServerConfig() const {
    return configSet;
}

Config Request::getserverConfig(std::string host , int port, bool isIp) const 
{
    // First, try to find server by exact host and port match
    for (size_t i = 0; i < fullServerConfig.servers.size(); i++) {
        const Config::ServerConfig& server = fullServerConfig.servers[i];
        
        // Check if the port matches any of the server's ports
        bool portMatch = false;
        for (size_t j = 0; j < server.ports.size(); j++) {
            if (server.ports[j] == port) {
                portMatch = true;
                break;
            }
        }
        
        if (!portMatch) {
            continue;
        }
        
        // If it's an IP address, match against server host
        if (isIp) {
            if (server.host == host) {
                Config matchedConfig;
                matchedConfig.servers.push_back(server);
                return matchedConfig;
            }
        } else {
            // If it's a domain name, check server_names
            for (size_t j = 0; j < server.server_names.size(); j++) {
                if (server.server_names[j] == host) {
                    Config matchedConfig;
                    matchedConfig.servers.push_back(server);
                    return matchedConfig;
                }
            }
        }
    }
    
    // If no exact match found, try to find default server for the port
    for (size_t i = 0; i < fullServerConfig.servers.size(); i++) {
        const Config::ServerConfig& server = fullServerConfig.servers[i];
        
        // Check if the port matches
        bool portMatch = false;
        for (size_t j = 0; j < server.ports.size(); j++) {
            if (server.ports[j] == port) {
                portMatch = true;
                break;
            }
        }
        
        if (portMatch && server.default_server) {
            Config matchedConfig;
            matchedConfig.servers.push_back(server);
            return matchedConfig;
        }
    }
    
    // If still no match, return the first server that matches the port
    for (size_t i = 0; i < fullServerConfig.servers.size(); i++) {
        const Config::ServerConfig& server = fullServerConfig.servers[i];
        
        for (size_t j = 0; j < server.ports.size(); j++) {
            if (server.ports[j] == port) {
                Config matchedConfig;
                matchedConfig.servers.push_back(server);
                return matchedConfig;
            }
        }
    }
    
    // If no server matches the port, return the first available server
    if (!fullServerConfig.servers.empty()) {
        Config matchedConfig;
        matchedConfig.servers.push_back(fullServerConfig.servers[0]);
        return matchedConfig;
    }
    
    // Return empty config if nothing matches
    Config emptyConfig;
    return emptyConfig;
}
void Request::matchServerConfiguration() {
    // if (!this->configSet || this->Host.empty()) {
    //     return; // Can't match without config or host
    // }
    
    try {
        // Get the matched server configuration
        Config matchedConfig = this->getserverConfig(this->Host, this->Port, this->isIp);
        
        // Store the matched server configuration
        if (!matchedConfig.servers.empty()) {
            this->serverConfig = matchedConfig.servers[0];
        }
                  
    } catch (const std::exception& e) {
        std::cerr << "Error during server matching: " << e.what() << std::endl;
        // Use default server config as fallback
        if (!fullServerConfig.servers.empty()) {
            this->serverConfig = fullServerConfig.servers[0];
        }
    }
}

const Config::ServerConfig* Request::getCurrentServer() const {
    if (!configSet) {
    return NULL;
    }
    return &serverConfig;
}
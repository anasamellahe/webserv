#include "Request.hpp"
#include "../Config/ConfigParser.hpp"

Request::Request() {
    this->clientFD = -1;
    this->is_valid = false;
    this->is_chunked = false;
    this->Port = 0;
    this->isIp = false;
    this->is_Complete = false;
    this->configSet = false;  // Initialize server config flag
    this->error_code.clear();
}

Request::Request(int clientFD) {
    this->clientFD = clientFD;
    this->is_valid = false;
    this->is_chunked = false;
    this->Port = 0;
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
    this->configSet = false;  // Reset server config flag
}

bool Request::parseFromSocket(int clientFD, const std::string& buffer, size_t size) {
    this->clientFD = clientFD;
    this->requestContent = buffer.substr(0, size);
    
    try {
        size_t header_end = this->requestContent.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            return false;
        }
        
        std::string headers_section = this->requestContent.substr(0, header_end);
        
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

        try {
            parseStartLine(header_lines[0]);
        } catch (int e) {
            this->is_valid = false;
            return false;
        }
        
        for (size_t i = 1; i < header_lines.size(); i++) {
            try {
                parseHeaders(header_lines[i]);
            } catch (int e) {
                this->is_valid = false;
                return false;
            }
        }
        
        size_t query_pos = this->path.find('?');
        if (query_pos != std::string::npos) {
            std::string query_string = this->path.substr(query_pos + 1);
            this->path = this->path.substr(0, query_pos);
            parseQueryString();
        }
        
        if (header_end + 4 < this->requestContent.length()) {
            std::string body_data = this->requestContent.substr(header_end + 4);
            
            ConstHeaderIterator transfer_encoding = this->headers.find("transfer-encoding");
            if (transfer_encoding != this->headers.cend() && 
                transfer_encoding->second.find("chunked") != std::string::npos) {
                parseChunkedTransfer(body_data);
            } else {
                parseBody(body_data);
            }
        }
        
        parseCookies();
        extractCgiInfo();
        validateRequest();
        
        // If request is valid and we have a host header, perform server matching
        if (this->is_valid && !this->Host.empty() && this->configSet) {
            matchServerConfiguration();
        }
        
        return this->is_valid;
    }
    catch (int error_code) {
        this->is_valid = false;
        return false;
    }
    catch (const std::exception& e) {
        this->is_valid = false;
        return false;
    }
    catch (...) {
        this->is_valid = false;
        return false;
    }
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
        this->path = path;
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
    const char* methods[] = {"GET", "POST", "DELETE", NULL};
    
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
    if (it != headers.cend()) {
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
        try {
            for (size_t i = 0; i < portPart.length(); i++) {
                if (!isdigit(portPart[i]))
                    return false;
            }
            int port = std::stoi(portPart);
            if (isValidPort(port)) {
                this->Port = port;
                hostValue = hostValue.substr(0, colonPos);
                setHost(hostValue);
            } else {
                return false;
            }
        } catch (...) {
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
    stringToLower(key);
    
    size_t valueStartIndex = headers_text.find_first_not_of(": \r\t\v\f", pos);
    if (valueStartIndex == std::string::npos) {
        valueStartIndex = pos;
    }
    
    value = headers_text.substr(valueStartIndex, headers_text.rfind("\r\n") - valueStartIndex);
    stringToLower(value);
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
    
    this->headers.insert(std::pair<std::string, std::string>(key, value));
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
    if (it != this->headers.cend()) {
        try {
            return std::stod(it->second);
        } catch (const std::invalid_argument&) {
            return 0.0;
        } catch (const std::out_of_range&) {
            return 0.0;
        }                   
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
    if (cl_it != this->headers.cend()) {
        try {
            size_t expected_length = std::stoul(cl_it->second);
            if (body_data.length() != expected_length) {
                if (body_data.length() < expected_length) {
                    return false;
                }
                this->error_code = BAD_REQ;
                return false;
            }
        } catch (const std::exception&) {
            this->error_code = BAD_REQ;
            return false;
        }
    }
    
    if (body_data.empty()) {
        return true;
    }
    
    ConstHeaderIterator it = this->headers.find("content-type");
    if (it != this->headers.cend()) {
        std::string content_type = it->second;
        if (content_type.find("multipart/form-data") != std::string::npos) {
            size_t boundary_pos = content_type.find("boundary=");
            if (boundary_pos != std::string::npos) {
                std::string boundary = content_type.substr(boundary_pos + 9);
                if (!boundary.empty() && boundary[0] == '"' && boundary.back() == '"') {
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
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
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
    
    auto cd_it = part_headers.find("content-disposition");
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
                
                auto ct_it = part_headers.find("content-type");
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
    if (chunked_data.empty()) {
        return true;
    }
    
    this->is_chunked = true;
    size_t pos = 0;
    
    while (pos < chunked_data.length()) {
        size_t crlf_pos = chunked_data.find("\r\n", pos);
        if (crlf_pos == std::string::npos) {
            break;
        }
        
        std::string size_line = chunked_data.substr(pos, crlf_pos - pos);
        
        size_t chunk_size = 0;
        try {
            chunk_size = std::stoul(size_line, nullptr, 16);
        } catch (const std::exception&) {
            this->error_code = BAD_REQ;
            return false;
        }
        
        if (chunk_size == 0) {
            pos = crlf_pos + 2;
            size_t final_crlf = chunked_data.find("\r\n", pos);
            if (final_crlf != std::string::npos) {
            }
            break;
        }
        
        pos = crlf_pos + 2;
        
        if (pos + chunk_size + 2 > chunked_data.length()) {
            return false;
        }
        
        std::string chunk = chunked_data.substr(pos, chunk_size);
        this->chunks.push_back(chunk);
        this->body += chunk;
        
        pos += chunk_size;
        if (pos + 1 < chunked_data.length() && 
            chunked_data[pos] == '\r' && chunked_data[pos + 1] == '\n') {
            pos += 2;
        } else {
            this->error_code = BAD_REQ;
            return false;
        }
    }
    
    return true;
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
    if (it != this->headers.cend()) {
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
    return (this->method == "GET" || this->method == "POST" || this->method == "DELETE");
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
    if (host_it == this->headers.cend()) {
        this->error_code = BAD_REQ;
        this->is_valid = false;
        return;
    }
    
    ConstHeaderIterator cl_it = this->headers.find("content-length");
    if (cl_it != this->headers.cend()) {
        try {
            double content_length = std::stod(cl_it->second);
            if (content_length < 0) {
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
    
    this->is_valid = true;
}

// Server configuration methods implementation
void Request::setServerConfig(const Config& config) {
    this->serverConfig = config;
    this->configSet = true;
}

const Config& Request::getServerConfig() const {
    if (!configSet) {
        throw std::runtime_error("Server configuration not set for this request");
    }
    return serverConfig;
}

bool Request::hasServerConfig() const {
    return configSet;
}

Config Request::getserverConfig(std::string host , int port, bool isIp) const 
{
    // First, try to find server by exact host and port match
    for (size_t i = 0; i < serverConfig.servers.size(); i++) {
        const Config::ServerConfig& server = serverConfig.servers[i];
        
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
    for (size_t i = 0; i < serverConfig.servers.size(); i++) {
        const Config::ServerConfig& server = serverConfig.servers[i];
        
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
    for (size_t i = 0; i < serverConfig.servers.size(); i++) {
        const Config::ServerConfig& server = serverConfig.servers[i];
        
        for (size_t j = 0; j < server.ports.size(); j++) {
            if (server.ports[j] == port) {
                Config matchedConfig;
                matchedConfig.servers.push_back(server);
                return matchedConfig;
            }
        }
    }
    
    // If no server matches the port, return the first available server
    if (!serverConfig.servers.empty()) {
        Config matchedConfig;
        matchedConfig.servers.push_back(serverConfig.servers[0]);
        return matchedConfig;
    }
    
    // Return empty config if nothing matches
    Config emptyConfig;
    return emptyConfig;
}

void Request::matchServerConfiguration() {
    if (!this->configSet || this->Host.empty()) {
        return; // Can't match without config or host
    }
    
    try {
        // Get the matched server configuration
        Config matchedConfig = this->getserverConfig(this->Host, this->Port, this->isIp);
        
        // Update the server configuration with the matched one
        this->serverConfig = matchedConfig;
        
        std::cout << "Successfully matched server for " << this->Host << ":" << this->Port 
                  << (this->isIp ? " (IP)" : " (domain)") << std::endl;
                  
    } catch (const std::exception& e) {
        std::cerr << "Error during server matching: " << e.what() << std::endl;
        // Keep existing server config as fallback
    }
}
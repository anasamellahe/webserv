#include "Request.hpp"

void request::setMethod(const std::string& method)
{
    if (this->method.empty() && !method.empty())
        this->method = method;
    else if (error_code.empty())
        this->error_code = BAD_REQ;
}
void request::setPath(const std::string& path)
{
    if (path.size() > MAX_PATH_SIZE && error_code.empty())
        this->error_code = URI_T_LONG;
    else if (this->path.empty() && !path.empty())
        this->path = path;
    else if (error_code.empty())
        this->error_code = BAD_REQ;
}
void request::setVersion(const std::string& version)
{
    if (this->version.empty() && !version.empty() && version.compare("HTTP/1.1") == 0)
        this->version = version;
    else if (this->error_code.empty())
        this->error_code = VERSION_ERR;
}
void request::parseStartLine(const std::string& line)
{

    size_t pos = std::string::npos;

    char * methods[] = {"GET", "POST", "DELETE", NULL};
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


void request::parseHeaders(const std::string& headers_text)
{
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
}


bool request::parseSingleLineHeader(const std::string line)
{
    bool isNotStartLine = false;

    try
    {
        parseStartLine(line);
    }
    catch(int number)
    {
        if (number == 1)
            return 1;
        if (number == -1)
            isNotStartLine = true;
    }

    if (this->method.empty() || this->path.empty() || this->version.empty())
    {
        if (this->error_code.empty())
            this->error_code = BAD_REQ;
        return 1;
    }

    try
    {
        if (isNotStartLine == true)
            parseHeaders(line);
    }
    catch(int number)
    {
        if (number == 1)
            return 1;
    }
    return 0;   
}

// 123456\r\n12345
bool request::parseRequestLine(const std::string& line)
{
    size_t start = 0;
    while (start < line.size())
    {
        size_t end = line.find("\r\n", start);
        if (end != std::string::npos)
        {
            if (parseSingleLineHeader(line.substr(start, end - start)) == 1)
                return 1;
            start = end + 2;
        }
        else
        {
            if (parseSingleLineHeader(line.substr(start)) == 1)
                return 1;
            break;
        }
    }
    return 0;
}



const std::string& request::getMethod() const
{
    return this->method;
}
const std::string& request::getPath() const
{
    return this->path;
}
const std::string& request::getVersion() const
{
    return this->version;
}
const std::string& request::getHeader(const std::string& key) const
{
    ConstHeaderIterator it =  this->headers.find(key);
    if (it != headers.cend())
        return it->second;
    return key;
}
int request::getClientFD() const
{
    return this->clientFD;
}
void request::setErrorCode(const std::string& error_code)
{
    this->error_code = error_code;
}
const std::string &request::getErrorCode() const
{
    if (!this->error_code.empty())
        return this->error_code;
    return "200 OK";
}


void request::addQueryParam(const std::string& key, const std::string& value)
{
    if (!key.empty())
        this->query_params.insert(std::pair<std::string, std::string>(key, value));
}


void request::parseQueryPair(const std::string &query, size_t start, size_t end) 
{

    size_t pos = query.find("=", start);
    if (pos != std::string::npos || pos <= end)
    {
        std::string key = query.substr(start, pos - start);
        std::string value = query.substr(pos + 1, end - pos);
        addQueryParam(key, value);
    }
}

bool  request::parseQueryString()
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
            parseQueryPair(queryParams, start, queryParams.length() - 1);
            break;
        }    
    }
    return true;
}
const std::string &request::getBody() const
{
    return this->body;
}

const std::map<std::string, std::string>& request::getQueryParams() const
{
    return this->query_params;
}

bool request::isValidHost(const std::string& Host)
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
                (consumed == Host.length())
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


bool request::validateHost(const std::string& Host)
{
    if (isValidHost(Host) == false)
    {
        if (this->error_code.empty())
            this->error_code = BAD_REQ;
        return false;
    }
    size_t pos = Host.find(":", 0);
    if (pos == std::string::npos)
    {
        this->Port = 8080;
        this->Host = Host;
    }
    else
    {
        this->Host = Host.substr(0, pos);
        this->Port = atoi(&Host.c_str()[pos+1]);
    }
    return true;
}
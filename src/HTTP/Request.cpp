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
    if (this->path.empty() && !path.empty())
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
        this->error_code = BAD_REQ;
        throw 1;
    }
    if (key == "host" && value.empty())
    {
        this->error_code = BAD_REQ;
        throw 1;
    }
    this->headers.insert(std::pair<std::string, std::string>(key, value));
}
bool request::parseRequestLine(const std::string& line)
{
    try
    {
        parseStartLine(line);
    }
    catch(int number)
    {
        if (number == 1)
            return 1;
        if (this->method.empty() || this->path.empty() || this->version.empty())
        {
            if (this->error_code.empty())
                this->error_code = BAD_REQ;
            return 1;
        }
    }

    try
    {
        parseHeaders(line);
    }
    catch(int number)
    {
        if (number == 1)
            return 1;
    }
    return 0;   
}



std::string request::getMethod() const
{
    return this->method;
}
std::string request::getPath() const
{
    return this->path;
}
std::string request::getVersion() const
{
    return this->version;
}
std::string request::getHeader(const std::string& key) const
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
std::string request::getErrorCode() const
{
    if (!this->error_code.empty())
        return this->error_code;
    return "200 OK";
}
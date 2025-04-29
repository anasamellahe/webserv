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


bool request::parseRequestLine(const std::string& line)
{
  
}

bool request::parseFromSocket(int clientFD)
{
    char buffer[1024];
    ssize_t bytesRead = read(clientFD, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0)
        return false;

    buffer[bytesRead] = '\0'; 
    requestContent = std::string(buffer);
    return parse(requestContent);
}

bool request::parse(const std::string& raw_request)
{
    std::istringstream request_stream(raw_request);
    std::string line;

 if (!std::getline(request_stream, line) || !parseRequestLine(line)) {
    return false;
}


   
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

    
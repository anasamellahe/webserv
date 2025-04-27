#include "Request.hpp"



request::request(int clientFD)
{
    char buff[1024];
    this->clientFD = clientFD;

    //handdel 
    //read request

}
request::~request()
{

}

// bool parse(const std::string& raw_request)
// {

// }
// bool parseFromSocket(int socket_fd)
// {

// }


void request::setMethod(const std::string& method)
{

}
void request::setPath(const std::string& path)
{

}
void request::setVersion(const std::string& version)
{

}
void request::setHeader(const std::string& key, const std::string& value)
{

}
void request::setBody(const std::string& body)
{

}
void request::setClientFD(int fd)
{

}
void request::setClientAddr(const sockaddr_in& addr)
{

}

std::string request::getMethod() const
{

}
std::string request::getPath() const
{

}
std::string request::getVersion() const
{

}
std::string request::getHeader(const std::string& key) const
{

}
std::string request::getBody() const
{

}
int request::getClientFD() const
{

}
sockaddr_in request::getClientAddr() const
{

}

void request::validateRequest()
{

}
void request::setErrorCode(const std::string& error_code)
{

}
std::string request::getErrorCode() const
{

}
std::map<std::string, std::string> request::getQueryParams() const
{

}
std::map<std::string, FilePart> request::getUploads() const
{

}
std::map<std::string, std::string> request::getCGIEnv() const
{

}
std::string request::getCGIExtension() const
{

}
std::map<std::string, std::string> request::getCookies() const
{

}
bool request::isChunked() const
{

}
std::vector<std::string> request::getChunks() const
{

}
void request::addQueryParam(const std::string& key, const std::string& value)
{

}
void request::addUpload(const std::string& key, const FilePart& file_part)
{

}

bool request::isValid()
{

}

    
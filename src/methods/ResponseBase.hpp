#pragma once

#include <string>
#include <map>
#include <vector>
#include "../HTTP/Request.hpp"

class ResponseBase
{
protected:
    Request &request;
    int statusCode;
    std::string statusText;
    std::map<std::string, std::string>  headers;
    std::string body;
    std::string response;
    bool finalized;

    virtual void handle() = 0;
    std::string buildDefaultBodyError(int code);
    std::string ReadFromFile(const std::string &path);
    std::string GenerateDefaultError(int code);

    bool isMethodAllowed();
    void setStatus(int statusCode, const std::string& statusText);
    void setBody(const std::string & body);
    void addHeader(const std::string& key, const std::string& value);
    std::string detectContentType();
    void finalize();




public:
    ResponseBase(Request& request);
    const std::string & generate();
    void buildError(int code, std::string text); // it most get the Error page if the server config provide  ones or use the defaults one tha comes with the server  
    virtual ~ResponseBase();
};

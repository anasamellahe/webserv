#pragma once
#include "webserv.hpp"

struct testSocketData;
class sock
{
private:
    std::vector<int> sockFDs;
public:

    sock(testSocketData data);
    void bindINET(testSocketData data);
    void closeFDs(const char *msg);
    class sockException : public std::exception
    {
        private:
            std::string ErrorMsg;
        public:
            sockException(std::string msg);
            const char *what() const throw();
            ~sockException() throw();
    };
    std::vector<int> getFDs() const;
    ~sock();
};


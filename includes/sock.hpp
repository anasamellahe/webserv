#pragma once
#include "webserv.hpp"

class sock
{
private:
    int sockFD;
public:
    class sockException : public std::exception
    {
        private:
            std::string ErrorMsg;
        public:
            sockException(std::string msg);
            const char *what() const throw();
            ~sockException() throw();
    };
    sock(int domain , int type, int protocol);
    int getFD() const;
    void bindINET(int AF_addres, int port, int addres);
    ~sock();
};


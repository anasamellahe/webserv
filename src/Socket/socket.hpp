#pragma once
#include "../main.hpp"

struct testSocketData;
class sock
{
private:
    std::vector<int> sockFDs;
    std::vector<std::pair<std::string, int> > hosts;
public:

    sock(std::vector<std::pair<std::string, int> > hosts);
    void bindINET();
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


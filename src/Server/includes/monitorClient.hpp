#pragma once
#include "webserv.hpp"


class sock;



class monitorClient
{
    public:
        struct SocketTracker
        {
            std::string request;
            std::string response;
            int WError;
            int RError;
        };
        typedef std::vector<pollfd>::iterator Viterator;
        typedef std::map<int, SocketTracker>::iterator Miterator;
        typedef std::vector<pollfd>::const_iterator CViterator;
        typedef std::map<int, SocketTracker>::const_iterator CMiterator;
    private:

        std::vector<pollfd> fds;
        size_t numberOfServers;
        std::map<int, SocketTracker> fdsTracker;
        void acceptNewClient(int serverFD);
        void removeClient(int index);
        int readClientRequest(int clientFd);
        void writeClientResponse(int clientFd);
        
    public:
        class monitorexception : public std::exception
        {
            private:
                std::string ErrorMsg;
            public:
                monitorexception(std::string msg);
                const char *what () const throw();
                ~monitorexception() throw();
        };
        monitorClient(std::vector<int> serverFDs);
        void startEventLoop();
        ~monitorClient();
    
};

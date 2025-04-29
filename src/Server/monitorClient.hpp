#pragma once
#include "../main.hpp"
#include "../HTTP/Request.hpp"


class sock;
struct FilePart;
class monitorClient
{
    public:
        //struct to use in the map that hold socket status  
            struct SocketTracker
        {
            std::string request;  // Stores the client's request body
            std::string response; // Stores the response to be sent to the client
            int WError;           // Write error status
            int RError;           // Read error status

            std::string method;   // HTTP method (e.g., GET, POST)
            std::string path;     // Request path
            std::map<std::string, std::string> headers;       // Stores request headers
            std::map<std::string, std::string> queryParams;   // Stores query parameters
            std::map<std::string, std::string> cookies;       // Stores cookies
            std::vector<std::string> chunks;                  // Stores chunks if the request is chunked
            std::map<std::string, FilePart> uploads;        // Stores uploaded files
            std::string error;   // Error message if any
        };
        //Create a shortcut for the vector and map iterators to make them easier to use
        typedef std::vector<pollfd>::iterator Viterator;
        typedef std::map<int, SocketTracker>::iterator Miterator;
        typedef std::vector<pollfd>::const_iterator CViterator;
        typedef std::map<int, SocketTracker>::const_iterator CMiterator;
    private:

        //socket that i ned to monitor with poll
        std::vector<pollfd> fds;
        //number of servers in the fds vector
        size_t numberOfServers;
        //map to track each socket status 
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

#pragma once
#include "../main.hpp"
#include "../HTTP/Common.hpp"
#include "../HTTP/Request.hpp"
#include <time.h>
class Request;
class sock;
class monitorClient

{
    public:
        //struct to use in the map that hold socket status  
        struct SocketTracker
        {
            Request *request_obj;      // Store the actual Request object
            std::string response;     // Response to send back
            std::string raw_buffer;   // Raw incoming data buffer
            int WError;
            int RError;
            time_t lastActive;
            std::string error;
            
            // Remove duplicated fields that are now in request_obj:
            // std::string method, path, host (these are in Request now)
            // std::map<std::string, std::string> headers, queryParams, cookies
            // etc.
            
            SocketTracker() ;
            void updateActivity()  ;
          
            
            bool hasTimedOut(time_t currentTime, time_t timeoutSeconds) const ;
            
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
        
        // Debug method to print request information after parsing

        // Timeout constants
        static const time_t CLIENT_TIMEOUT = 60;         // Client timeout in seconds (1 minute)
        static const time_t TIMEOUT_CHECK_INTERVAL = 10; // Check for timeouts every 10 seconds
        time_t lastTimeoutCheck;                         // Last time we checked for timeouts
        
        void acceptNewClient(int serverFD);
        void removeClient(int index);
        static const size_t CHUNK_SIZE = 8192; // 8KB chunk size
        int readChunkFromClient(int clientFd, std::string& buffer);
        int readClientRequest(int clientFd); // Updated to handle 8KB chunks and parsing
        bool isRequestComplete(SocketTracker& tracker);
        bool checkRequestCompletion(SocketTracker& tracker, size_t headerEnd);
        void writeClientResponse(int clientFd);
        
        // Timeout related methods
        void checkTimeouts();                  // Check all clients for timeout
        void removeTimedOutClients();          // Remove clients that have timed out
        bool shouldCheckTimeouts(time_t now);  // Determine if it's time to check for timeouts
        void initializeTimeouts();             // Initialize timeout settings
        void updateClientActivity(int clientFd); // Update activity timestamp for a client

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
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
    
            SocketTracker() ;
            void updateActivity()  ; // checked  by Anas and it work perfectly
            bool hasTimedOut(time_t currentTime, time_t timeoutSeconds) const ; // checked  by Anas and it work perfectly
            
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
        

        // Timeout constants
        static const time_t     CLIENT_TIMEOUT = 60;                        // Client timeout in seconds (1 minute)
        static const time_t     TIMEOUT_CHECK_INTERVAL = 10;                // Check for timeouts every 10 seconds
        static const size_t     CHUNK_SIZE = 8192;                          // 8KB chunk size
        time_t                  lastTimeoutCheck;                           // Last time we checked for timeouts

        void acceptNewClient(int serverFD);                                 // checked  by Anas and it work perfectly

        void removeClient(int index);                                       // checked  by Anas and it work perfectly

        int     readChunkFromClient(int clientFd, std::string& buffer);
        int     readClientRequest(int clientFd);                            // Updated to handle 8KB chunks and parsing
        bool    isRequestComplete(SocketTracker& tracker);
        bool    checkRequestCompletion(SocketTracker& tracker, size_t headerEnd);
        void    writeClientResponse(int clientFd);
        
        // Timeout related methods
        void    checkTimeouts();                                            // Check all clients for timeout   // checked  by Anas and it work perfectly
        void    removeTimedOutClients();                                    // Remove clients that have timed out
        bool    shouldCheckTimeouts(time_t now);                            // Determine if it's time to check for timeouts // checked  by Anas and it work perfectly
        void    initializeTimeouts();                                       // Initialize timeout settings // checked  by Anas and it work perfectly
        void    updateClientActivity(int clientFd);                         // Update activity timestamp for a client // checked  by Anas and it work perfectly

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
        //set up the pollFds of each server and stor them in a vector of fds
        monitorClient(std::vector<int> serverFDs);      // checked  by Anas and it work perfectly
        void    startEventLoop();                       //checked  by Anas and it needs some extra works
        ~monitorClient();                               // checked  by Anas and it work perfectly
};
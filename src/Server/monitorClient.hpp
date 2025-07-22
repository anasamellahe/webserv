#pragma once
#include "../main.hpp"
#include "../HTTP/Request.hpp"

#pragma once
#include "../main.hpp"
#include "../HTTP/Request.hpp"
#include <time.h>

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
            time_t lastActive;    // Timestamp of the last activity
            std::string host;      // Hostname of the client

            std::string method;   // HTTP method (e.g., GET, POST)
            std::string path;     // Request path
            std::map<std::string, std::string> headers;       // Stores request headers
            std::map<std::string, std::string> queryParams;   // Stores query parameters
            std::map<std::string, std::string> cookies;       // Stores cookies
            std::vector<std::string> chunks;                  // Stores chunks if the request is chunked
            std::map<std::string, FilePart> uploads;          // Stores uploaded files
            std::string error;    // Error message if any
            
            // Constructor to initialize the tracker with current time
            SocketTracker() ;
            
            // Update the last activity timestamp
            void updateActivity() ;
            
            // Check if this client has timed out
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
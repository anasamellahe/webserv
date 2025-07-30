#pragma once
#include "../main.hpp"
#include "../HTTP/Common.hpp"
#include "../HTTP/Request.hpp"
#include <time.h>

class sock;
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
            
            // Additional fields for request parsing
            bool headers_parsed;                              // Whether headers have been parsed
            bool is_chunked;                                  // Whether the request uses chunked encoding
            size_t expected_length;                           // Expected content length
            
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
        
        // Debug method to print request information after parsing
        void printRequestInfo(int clientFd) {
            Miterator it = fdsTracker.find(clientFd);
            if (it != fdsTracker.end()) {
                SocketTracker& tracker = it->second;
                std::cout << "\n===== REQUEST INFO FOR CLIENT " << clientFd << " =====\n";
                std::cout << "Method: " << tracker.method << "\n";
                std::cout << "Path: " << tracker.path << "\n";
                std::cout << "Headers parsed: " << (tracker.headers_parsed ? "Yes" : "No") << "\n";
                std::cout << "Is chunked: " << (tracker.is_chunked ? "Yes" : "No") << "\n";
                std::cout << "Expected length: " << tracker.expected_length << "\n";
                std::cout << "Error: " << (tracker.error.empty() ? "None" : tracker.error) << "\n";
                
                std::cout << "Headers (" << tracker.headers.size() << "):\n";
                for (std::map<std::string, std::string>::const_iterator h = tracker.headers.begin(); 
                     h != tracker.headers.end(); ++h) {
                    std::cout << "  " << h->first << ": " << h->second << "\n";
                }
                
                std::cout << "Query Params (" << tracker.queryParams.size() << "):\n";
                for (std::map<std::string, std::string>::const_iterator q = tracker.queryParams.begin(); 
                     q != tracker.queryParams.end(); ++q) {
                    std::cout << "  " << q->first << "=" << q->second << "\n";
                }
                
                std::cout << "Cookies (" << tracker.cookies.size() << "):\n";
                for (std::map<std::string, std::string>::const_iterator c = tracker.cookies.begin(); 
                     c != tracker.cookies.end(); ++c) {
                    std::cout << "  " << c->first << "=" << c->second << "\n";
                }
                
                std::cout << "Request Buffer Length: " << tracker.request.size() << " bytes\n";
                std::cout << "Response Buffer Length: " << tracker.response.size() << " bytes\n";
                std::cout << "============================================\n\n";
            }
        }
        
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
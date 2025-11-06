#pragma once

#include <vector>
#include <map>
#include <poll.h>
#include <time.h>
#include "../HTTP/Common.hpp"
#include "../HTTP/Request.hpp"
#include "../Config/ConfigParser.hpp"

// Forward declaration
class CGIHandler;
class sock;

/**
 * @brief Client connection monitor and event handler class
 * 
 * This class manages multiple client connections using poll() system call,
 * handles client timeouts, request reading, and response writing.
 * It supports non-blocking I/O operations and efficient connection management.
 */
class monitorClient {
public:
    /**
     * @brief Tracks individual socket connection state and data
     * 
     * Contains all information needed to handle a client connection including
     * request parsing, response generation, error handling, and timeout tracking.
     */
    struct SocketTracker {
        Request request_obj;      // Parsed HTTP request object
        std::string response;     // Generated HTTP response
        std::string raw_buffer;   // Raw incoming data buffer
        bool headersParsed;       // Whether current request headers are parsed
        size_t consumedBytes;     // Bytes consumed for the current parsed request
        int WError;              // Write error status
        int RError;              // Read error status  
        time_t lastActive;       // Last activity timestamp
        std::string error;       // Error message if any

        // CGI-specific fields
        bool isCgiRequest;       // True if this is a CGI request
        int cgiOutputFd;         // CGI output pipe file descriptor
        CGIHandler* cgiHandler;  // Pointer to CGI handler (owned by tracker)
        
        /**
         * @brief Default constructor - initializes tracker with current time
         */
        SocketTracker();

        /**
         * @brief Destructor - cleans up CGI handler if present
         */
        ~SocketTracker();

        /**
         * @brief Updates the last activity timestamp to current time
         * Used to prevent timeout for active connections
         */
        void updateActivity();

        /**
         * @brief Checks if connection has exceeded timeout threshold
         * @param currentTime Current system time
         * @param timeoutSeconds Timeout threshold in seconds
         * @return true if connection has timed out
         */
        bool hasTimedOut(time_t currentTime, time_t timeoutSeconds) const;
    };

    // Iterator type definitions for container access
    typedef std::vector<pollfd>::iterator PollFdsIt;
    typedef std::map<int, SocketTracker>::iterator TrackerIt;
    typedef std::vector<pollfd>::const_iterator CPollFdsIt;
    typedef std::map<int, SocketTracker>::const_iterator CTrackerIt;

private:
    ConfigParser ServerConfig;                  // add a object copy  of the parsed config file 
    std::vector<pollfd> fds;                    // Poll file descriptors array
    size_t numberOfServers;                     // Number of server sockets
    std::map<int, SocketTracker> fdsTracker;    // Connection tracking map


    // Timeout and chunk size constants
    static const time_t CLIENT_TIMEOUT = 15;           // Client timeout (15 seconds, reduced from 60)
    static const time_t TIMEOUT_CHECK_INTERVAL = 10;   // Timeout check interval
    static const size_t CHUNK_SIZE = 8192;             // Read chunk size (8KB)
    time_t lastTimeoutCheck;                           // Last timeout check time

    /**
     * @brief Accepts new client connection from server socket
     * @param serverFD Server socket file descriptor
     * Sets up non-blocking client socket and adds to monitoring list
     */
    void acceptNewClient(int serverFD);

    /**
     * @brief Removes client connection and cleans up resources
     * @param index Index of client in fds vector
     * Closes socket, removes from vectors and maps
     */
    void removeClient(int index);

    /**
     * @brief Reads data chunk from client socket
     * @param clientFd Client socket file descriptor
     * @param buffer Reference to buffer to store read data
     * @return Number of bytes read, 0 for EOF, negative for error/no-data
     */
    int readChunkFromClient(int clientFd, std::string& buffer);

    /**
     * @brief Reads and processes HTTP request from client
     * @param clientFd Client socket file descriptor
     * @return 1 for continue, 0 for close connection, -1 for error
     * Handles chunked reading and request parsing
     */
    int readClientRequest(int clientFd);

    /**
     * @brief Checks if HTTP request is complete
     * @param tracker Reference to socket tracker
     * @return true if request parsing is complete
     */
    bool isRequestComplete(SocketTracker& tracker);

    /**
     * @brief Checks request completion after finding headers end
     * @param tracker Reference to socket tracker
     * @param headerEnd Position where headers end
     * @return true if full request is available
     */
    bool checkRequestCompletion(SocketTracker& tracker, size_t headerEnd);

    /**
     * @brief Checks if the request body is completely received
     * @param tracker Reference to socket tracker containing request data
     * @param headerEnd Position where headers end in the raw buffer
     * @return true if body is complete, false if more data needed
     */
    bool isRequestBodyComplete(SocketTracker& tracker, size_t headerEnd);

    /**
     * @brief Writes HTTP response to client socket
     * @param clientFd Client socket file descriptor
     * Handles partial writes and connection management
     */
    int writeClientResponse(int clientFd);

    /**
     * @brief Checks all connections for timeout and handles timeouts
     * Generates timeout responses for clients that exceed time limit
     */
    void checkTimeouts();

    /**
     * @brief Removes connections that have timed out
     * Called after timeout detection to clean up dead connections
     */
    void removeTimedOutClients();

    /**
     * @brief Determines if enough time has passed to check timeouts
     * @param now Current system time
     * @return true if timeout check should be performed
     */
    bool shouldCheckTimeouts(time_t now);

    /**
     * @brief Initializes timeout tracking system
     * Sets up initial timeout check timestamp
     */
    void initializeTimeouts();

    /**
     * @brief Updates activity timestamp for specific client
     * @param clientFd Client socket file descriptor
     * Prevents timeout for active connections
     */
    void updateClientActivity(int clientFd);

    /**
     * @brief Generates an error response for the client
     * @param tracker Reference to socket tracker containing error information
     * Creates appropriate HTTP error response based on error code
     */
    void generateErrorResponse(SocketTracker& tracker);

    /**
     * @brief Generates a success response for the client
     * @param tracker Reference to socket tracker containing request information
     * Creates a 200 OK HTTP response with request details
     */
    void generateSuccessResponse(SocketTracker& tracker);

    /**
     * @brief Generates a simple fallback response
     * @param tracker Reference to socket tracker containing request information
     * Creates a basic HTTP response when server matching fails
     */
    void generateSimpleResponse(SocketTracker& tracker);

    /**
     * @brief Generates an error response using server configuration
     * @param tracker Reference to socket tracker
     * @param errorCode HTTP error code
     * @param errorMessage Error message string
     * @param server Server configuration to use for error pages
     */
    void generateErrorResponseWithConfig(SocketTracker& tracker, int errorCode, const std::string& errorMessage, const Config::ServerConfig& server);

    /**
     * @brief Generates an HTTP redirect response
     * @param tracker Reference to socket tracker
     * @param redirectCode HTTP redirect code (301, 302, etc.)
     * @param location Redirect target URL
     */
    void generateRedirectResponse(SocketTracker& tracker, int redirectCode, const std::string& location);

    /**
     * @brief Checks if a request should be handled as CGI
     * @param tracker Reference to socket tracker
     * @return true if request should use CGI, false otherwise
     */
    bool shouldHandleAsCGI(SocketTracker& tracker, std::string& scriptPath, std::string& interpreterPath);

    /**
     * @brief Starts asynchronous CGI execution for a request
     * @param tracker Reference to socket tracker
     * @param clientFd Client file descriptor
     * @param scriptPath Path to CGI script
     * @param interpreterPath Path to interpreter
     */
    void startAsyncCGI(SocketTracker& tracker, int clientFd, const std::string& scriptPath, const std::string& interpreterPath);

public:
    /**
     * @brief Exception class for monitor client errors
     * 
     * Custom exception type for handling monitor client specific errors
     * like socket operations, poll failures, etc.
     */
    class monitorexception : public std::exception {
    private:
        std::string ErrorMsg;   // Error message string

    public:
        /**
         * @brief Constructor with error message
         * @param msg Error message describing the exception
         */
        monitorexception(std::string msg);

        /**
         * @brief Returns error message as C-string
         * @return Pointer to error message C-string
         */
        const char *what() const throw();

        /**
         * @brief Destructor - cleans up exception resources
         */
        ~monitorexception() throw();
    };

    /**
     * @brief Constructor - initializes monitor with server sockets
     * @param serverSockets Copy of the socket class that holds the file descriptors and parsed configuration data
     * Sets up poll structures for all server sockets
     */
    monitorClient(sock serverSockets); //make the constructor accept a sock  class 

    /**
     * @brief Main event loop - monitors all connections and handles events
     * Runs indefinitely, handling new connections, client requests,
     * responses, and timeouts using poll() system call
     */
    void startEventLoop();

    /**
     * @brief Destructor - closes all file descriptors and cleans up
     * Ensures all sockets are properly closed before destruction
     */
    ~monitorClient();
};
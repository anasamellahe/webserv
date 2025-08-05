#pragma once

#include "../Config/ConfigParser.hpp" 
#include <vector>
#include <string>
#include <utility>
#include <exception>

struct testSocketData;

/**
 * @brief TCP socket creation and management class
 * 
 * Creates and manages multiple TCP server sockets for different host:port
 * combinations. Handles socket creation, binding, listening, and cleanup
 * with proper error handling and resource management.
 */
class sock {
private:
    ConfigParser config_parser; // object of the parsed config file
    std::vector<int> sockFDs;                           // Server socket file descriptors
    std::vector<std::pair<std::string, int> > hosts;    // Host:port combinations

public:
       /**
     * @brief Constructor - creates sockets for specified host:port pairs
     * @param config_parser Copy of the ConfigParser class that holds all the data needed to create the server sockets
     * Creates non-blocking TCP sockets with SO_REUSEADDR option
     */
    sock(ConfigParser config_parser); //change this constructor to accept a object insted of vector of hosts 

    /**
     * @brief Binds sockets to their respective addresses and starts listening
     * Binds each socket to its host:port and puts it in listening state
     * with a backlog of 100 connections
     */
    void bindINET();
    ConfigParser getConfig();

    /**
     * @brief Closes all socket file descriptors and throws exception
     * @param msg Error message to include in exception
     * Emergency cleanup function that closes all sockets before throwing
     */
    void closeFDs(const char *msg);

    /**
     * @brief Gets vector of all server socket file descriptors
     * @return Vector containing all server socket FDs
     */
    std::vector<int> getFDs() const;

    /**
     * @brief Destructor - performs cleanup of socket resources
     */
    ~sock();

    /**
     * @brief Custom exception class for socket-related errors
     * 
     * Specialized exception type for socket operation failures
     * including creation, binding, listening, and option setting errors.
     */
    class sockException : public std::exception {
    private:
        std::string ErrorMsg;   // Error message description

    public:
        /**
         * @brief Constructor with error message
         * @param msg Descriptive error message
         */
        sockException(std::string msg);

        /**
         * @brief Returns error message as C-string
         * @return Pointer to error message C-string
         */
        const char *what() const throw();

        /**
         * @brief Destructor - cleans up exception resources
         */
        ~sockException() throw();
    };
};


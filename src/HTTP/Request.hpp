#pragma once

#include <cctype>
#include <typeinfo>  
#include <vector>
#include <map>
#include <string>
#include <netinet/in.h>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <arpa/inet.h>
#include "Utils.hpp"
#include "Common.hpp"
#include "../Config/ConfigParser.hpp"  // Include full Config definition


#define MAX_PATH_SIZE 4000
#define BAD_REQ "400 Bad Request"
#define URI_T_LONG "414 URI Too Long"
#define VERSION_ERR "505 HTTP Version Not Supported"

// Remove forward declaration since we're including the full definition
// struct Config;

/**
 * @brief HTTP Request parser and handler class
 * 
 * This class handles parsing, validation, and management of HTTP requests.
 * It supports HTTP/1.1 protocol with features like chunked transfer encoding,
 * multipart form data, CGI integration, and cookie handling.
 */
class Request 
{

    public:
        typedef std::map<std::string, std::string>::iterator        HeaderIterator;
        typedef std::map<std::string, std::string>::const_iterator  ConstHeaderIterator;
    public:

        int clientFD;
        Config                  fullServerConfig;  // Full configuration (contains all servers)
        Config::ServerConfig    serverConfig;      // Matched server configuration for this request
    
        bool        configSet;     // Flag to track if server config has been set
        std::string requestContent;
        std::string method;             
        std::string path;                
        std::string version;
        std::string Host;
        bool       is_Complete; // Indicates if the request is fully parsed
        int         Port;
        bool        isIp;   
        std::map<std::string, std::string> headers;
        std::string body;             
    
        std::map<std::string, std::string> query_params; 
        std::map<std::string, FilePart> uploads;       
        std::string cgi_extension;                     
        std::map<std::string, std::string> cgi_env;                                       
        sockaddr_in client_addr;                         
        bool is_valid;                                   
        std::string error_code;                         

        std::map<std::string, std::string> cookies;    
        bool is_chunked;                                 
        std::vector<std::string> chunks;                 

       
        /**
         * @brief Parses the HTTP request start line (method, path, version)
         * @param line The first line of the HTTP request
         * @throws int Exception code if parsing fails
         */
        void parseStartLine(const std::string& line);

        /**
         * @brief Parses individual HTTP header lines
         * @param headers_text A single header line in "key: value" format
         * @return true if parsing successful, false otherwise
         * @throws int Exception code for malformed headers
         */
        bool parseHeaders(const std::string& headers_text);

        /**
         * @brief Validates if a string is a valid IPv4 address
         * @param hostValue Reference to potential IP address string (may include port)
         * @return true if valid IP address format, false otherwise
         * 
         * Checks if the string is a properly formatted IPv4 address with optional port.
         * Valid formats include "192.168.1.1" or "192.168.1.1:8080".
         * If valid, sets isIp=true, updates Port (default 80 if not specified),
         * and may modify hostValue to remove port component.
         */
        bool isValidIpAddress(std::string& hostValue);

        /**
         * @brief Validates if a string is a valid domain name
         * @param hostValue Reference to potential domain name string (may include port)
         * @return true if valid domain name format, false otherwise
         * 
         * Checks if the string follows proper domain name formatting rules.
         * Validates character set (alphanumeric, dots, hyphens) and structure.
         * Handles domains with optional port (e.g., "example.com:8080").
         * If valid, sets isIp=false, updates Port (default 80 if not specified),
         * and may modify hostValue to remove port component.
         */
        bool isValidDomainName(std::string& hostValue);


        /**
         * @brief Validates if the provided host is in correct format
         * @param Host The host string to validate (IP:port or domain:port)
         * @return true if host format is valid, false otherwise
         */
        bool isValidHost(std::string& Host);

        /**
         * @brief Parses query string parameters from URL
         * @return true if query string was found and parsed, false if no query string
         */
        bool parseQueryString();

        /**
         * @brief Parses a single key-value pair from query string
         * @param query The full query string
         * @param start Starting position of the key-value pair
         * @param end Ending position of the key-value pair
         */
        void parseQueryPair(const std::string& query, size_t start, size_t end);

        /**
         * @brief Adds a query parameter to the internal map
         * @param key Parameter name
         * @param value Parameter value
         */
        void addQueryParam(const std::string& key, const std::string& value);


        /**
         * @brief Parses HTTP request body data
         * @param body_data Raw body content as string
         * @return true if parsing successful, false otherwise
         */
        bool parseBody(const std::string& body_data);

    /**
     * @brief Parse body based on content type (state-machine dispatch)
     * @param body_data Raw body data (already assembled if chunked)
     * @return true on success
     */
    bool parseBodyByType(const std::string& body_data);

    // Type-specific body parsers
    bool parseBodyMultipart(const std::string& body_data, const std::string& boundary);
    bool parseBodyUrlEncoded(const std::string& body_data);
    bool parseBodyRaw(const std::string& body_data);
    bool parseBodyBinary(const std::string& body_data);
    bool parseBodyGeneric(const std::string& body_data, const std::string& content_type);

        /**
         * @brief Parses multipart form data with specified boundary
         * @param body_data Raw multipart body data
         * @param boundary Multipart boundary string
         * @return true if parsing successful, false otherwise
         */
        bool parseMultipartBody(const std::string& body_data, const std::string& boundary);

        /**
         * @brief Parses individual multipart section
         * @param part_data Raw part data including headers and body
         * @return true if parsing successful, false otherwise
         */
        bool parseMultipartPart(const std::string& part_data);

        /**
         * @brief Parses URL-encoded form data
         * @param form_data Raw form data string
         * @return true if parsing successful, false otherwise
         */
        bool parseFormData(const std::string& form_data);

        /**
         * @brief URL decodes a string (converts %XX to actual characters)
         * @param str URL-encoded string
         * @return Decoded string
         */
        std::string urlDecode(const std::string& str);

        /**
         * @brief URL encodes a string (converts special chars to %XX)
         * @param str Raw string to encode
         * @return URL-encoded string
         */
        std::string urlEncode(const std::string& str);

        /**
         * @brief Parses chunked transfer encoding data
         * @param chunked_data Raw chunked data
         * @return true if parsing successful, false otherwise
         */
        bool parseChunkedTransfer(const std::string& chunked_data);

        /**
         * @brief Extracts CGI-related information from request
         * @return true if request contains CGI elements, false otherwise
         */
        bool extractCgiInfo();

        /**
         * @brief Parses HTTP cookies from Cookie header
         * @return true if cookies were found and parsed, false otherwise
         */
        bool parseCookies();

        /**
         * @brief Validates if the HTTP method is supported
         * @return true if method is GET, POST, or DELETE
         */
        bool validateMethod() const;

        /**
         * @brief Validates the request path format and length
         * @return true if path is valid, false otherwise
         */
        bool validatePath() const;

        /**
         * @brief Validates host header format
         * @param host Host string to validate
         * @return true if host is valid, false otherwise
         */
        bool validateHost(const std::string& host);

    // Split parsing API for multiplexed reader
    // Parse start-line and header fields only (no body)
    bool parseHeadersSection(const std::string& headers_section);

    // Parse body based on headers (Content-Length or chunked), then cookies/CGI/validate
    bool parseBodySection(const std::string& body_section);

    // Helpers for body handling
    bool hasChunkedEncoding() const;
    size_t expectedContentLength() const;

    public:
        /**
         * @brief Default constructor - initializes empty request
         */
        Request();

        /**
         * @brief Constructor with client file descriptor
         * @param clientFD File descriptor of the client connection
         */
        Request(int clientFD);

        /**
         * @brief Destructor - cleans up request resources
         */
        ~Request();

        /**
         * @brief Main parsing function that processes raw HTTP data from socket
         * @param clientFd Client file descriptor
         * @param buffer Raw HTTP request data
         * @param bufferSize Size of the buffer
         * @return true if request was successfully parsed and is valid
         */
        bool parseFromSocket(int clientFd, const std::string& buffer, size_t bufferSize);


        /**
         * @brief Sets the host value from the Host header
         * @param host Host string (domain name or IP address)
         * 
         * Updates the internal Host member variable used for routing 
         * and virtual host selection. The host parameter should be 
         * pre-validated before calling this method.
         */
        void setHost(const std::string& host);

        /**
         * @brief Sets the port number from the Host header
         * @param port Port number (1-65535)
         * 
         * Updates the internal Port member variable used for server routing.
         * If the provided port is invalid (outside 1-65535 range), defaults to port 80.
         * Uses the isValidPort template function for validation.
         */
        void setPort(int port);

        /**
         * @brief Sets the HTTP method after validation
         * @param method HTTP method string (GET, POST, DELETE)
         */
        void setMethod(const std::string& method); // OK

        /**
         * @brief Sets the request path after validation
         * @param path URL path string
         */
        void setPath(const std::string& path); // OK

        /**
         * @brief Sets the HTTP version after validation
         * @param version HTTP version string (must be "HTTP/1.1")
         */
        void setVersion(const std::string& version); //OK

        /**
         * @brief Adds or updates an HTTP header
         * @param key Header name
         * @param value Header value
         */
        void setHeader(const std::string& key, const std::string& value);

        /**
         * @brief Sets the request body content
         * @param body Body content as string
         */
        void setBody(const std::string& body);

        /**
         * @brief Sets the client file descriptor
         * @param fd Client file descriptor
         */
        void setClientFD(int fd);

        /**
         * @brief Sets the client socket address information
         * @param addr Socket address structure
         */
        void setClientAddr(const sockaddr_in& addr);

        /**
         * @brief Gets the HTTP method
         * @return Reference to method string
         */
        const std::string&    getMethod() const; // OK

        /**
         * @brief Gets the request path
         * @return Reference to path string
         */
        const std::string&    getPath() const; // OK

        /**
         * @brief Gets the HTTP version
         * @return Reference to version string
         */
        const std::string&    getVersion() const; // OK

        /**
         * @brief Gets value of specific HTTP header
         * @param key Header name to lookup
         * @return Reference to header value, or key if not found
         */
        const std::string&    getHeader(const std::string& key) const; // OK

        /**
         * @brief Gets the request body content
         * @return Reference to body string
         */
        const std::string&    getBody() const; // OK

        /**
         * @brief Gets the client file descriptor
         * @return Client file descriptor integer
         */
        int getClientFD() const; // OK

        /**
         * @brief Gets the client socket address
         * @return Socket address structure
         */
        sockaddr_in getClientAddr() const;

        /**
         * @brief Validates the entire request for correctness
         * Sets error codes and validity flags based on validation results
         */
        void validateRequest();

        /**
         * @brief Sets an error code for the request
         * @param error_code HTTP error code string (e.g., "400 Bad Request")
         */
        void setErrorCode(const std::string& error_code); // OK

        /**
         * @brief Gets the current error code
         * @return Reference to error code string, "200 OK" if no error
         */
        const std::string&                          getErrorCode() const; // OK

        /**
         * @brief Gets all parsed query parameters
         * @return Const reference to query parameters map
         */
        const std::map<std::string, std::string>&   getQueryParams() const; // OK

        /**
         * @brief Gets all uploaded files (multipart form data)
         * @return Const reference to uploads map
         */
        const std::map<std::string, FilePart>&      getUploads() const;

        /**
         * @brief Gets CGI environment variables
         * @return Const reference to CGI environment map
         */
        const std::map<std::string, std::string>&   getCGIEnv() const;

        /**
         * @brief Gets the CGI file extension if detected
         * @return Const reference to CGI extension string
         */
        const std::string&                          getCGIExtension() const;

        /**
         * @brief Gets all parsed HTTP cookies
         * @return Const reference to cookies map
         */
        const std::map<std::string, std::string>&   getCookies() const;

        /**
         * @brief Gets chunks from chunked transfer encoding
         * @return Const reference to chunks vector
         */
        const std::vector<std::string>&             getChunks() const;

        /**
         * @brief Gets all HTTP headers
         * @return Const reference to headers map
         */
        const std::map<std::string, std::string>& getAllHeaders() const;

        /**
         * @brief Checks if request parsing is complete
         * @return true if request is complete and valid
         */
        bool isComplete() const;

        /**
         * @brief Sets the completion status of the request
         * @param complete Boolean completion status
         * @return The set completion status
         */
        bool setComplete(bool complete);    

        /**
         * @brief Gets the Content-Length header value as double
         * @return Content length as double, 0.0 if not present or invalid
         */
        double getContentLength();

        /**
         * @brief Adds an uploaded file to the uploads collection
         * @param key Form field name
         * @param file_part File part structure containing file data
         */
        void addUpload(const std::string& key, const FilePart& file_part);

        /**
         * @brief Checks if request uses chunked transfer encoding
         * @return true if request is chunked
         */
        bool isChunked() const;

        /**
         * @brief Checks if the request is valid
         * @return true if request passed all validations
         */
        bool isValid();

    /**
     * @brief Print the current request parsing state to stdout for debugging
     */
    void debugPrint() const;

        /**
         * @brief Resets all request data to initial state
         * Clears all parsed data but preserves connection-specific info
         */
        void reset();

        /**
         * @brief Gets server configuration based on host or server name
         * @param serverToFind Host or server name to match
         * @param serversConfigs Configuration containing all servers
         * @return Matching server configuration
         */
        Config getserverConfig(std::string host , int port, bool isIp) const ;

        /**
         * @brief Matches and sets the appropriate server configuration for this request
         * Uses the parsed Host header and port to find the matching server configuration
         * and updates the request's server config automatically
         */
        void matchServerConfiguration();

        /**
         * @brief Sets the server configuration for this request
         * @param config string containing  host and port and a boolean indicating if it is an IP
         */
        void setServerConfig(const Config& config);

        void setChunked(bool ischunked);

        /**
         * @brief Gets the current server configuration
         * @return Reference to the server configuration
         * @throws std::runtime_error if no server config has been set
         */
        const Config& getServerConfig() const;

        /**
         * @brief Checks if server configuration has been set
         * @return true if server config is available, false otherwise
         */
        bool hasServerConfig() const;

        /**
         * @brief Gets the currently matched server configuration for this request
         * @return Pointer to the matched server config, or nullptr if no server matched
         */
        const Config::ServerConfig* getCurrentServer() const;

    
};

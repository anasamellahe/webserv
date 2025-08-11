#ifndef WEBSERVER_CONFIGPARSER_HPP
#define WEBSERVER_CONFIGPARSER_HPP

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cctype>
#include <climits>


/**
 * @brief Configuration structure containing all server and route settings
 * 
 * Hierarchical configuration structure that holds parsed configuration data
 * from config files. Supports multiple servers with multiple routes each.
 */
struct Config {
    /**
     * @brief Route-specific configuration settings
     * 
     * Contains all settings for a specific URL path including methods,
     * redirects, CGI, uploads, and directory listing options.
     */
    struct RouteConfig {
        std::string path;                           // URL path pattern
        std::string root;                           // Document root directory
        std::vector<std::string> accepted_methods;  // Allowed HTTP methods
        bool has_redirect;                          // Has redirect rule
        int redirect_code;                          // HTTP redirect code
        std::string redirect_url;                   // Redirect target URL
        bool directory_listing;                     // Enable directory listing
        std::string index;                          // Index file names
        bool cgi_enabled;                          // CGI processing enabled
        std::string cgi_pass;                      // CGI interpreter path
        std::vector<std::string> cgi_extensions;   // CGI file extensions
        bool upload_enabled;                       // File upload enabled
        std::string upload_path;                   // Upload directory path

        // Iterator typedefs for vector access
        typedef std::vector<std::string>::iterator MethodIterator;
        typedef std::vector<std::string>::const_iterator ConstMethodIterator;
        typedef std::vector<std::string>::iterator CgiExtensionIterator;
        typedef std::vector<std::string>::const_iterator ConstCgiExtensionIterator;
    };

    /**
     * @brief Server-specific configuration settings
     * 
     * Contains all settings for a virtual server including network binding,
     * error pages, body size limits, and associated routes.
     */
    struct ServerConfig {
        std::string host;                              // Bind host/IP address
        std::vector<int> ports;                        // Listen port numbers
        std::vector<std::string> server_names;         // Server name aliases
        std::string root;                              // Default document root
        std::map<int, std::string> error_pages;       // Custom error pages
        size_t client_max_body_size;                  // Max request body size
        bool default_server;                          // Default server flag
        bool chunked_transfer;                        // Chunked encoding support
        std::vector<RouteConfig> routes;              // Route configurations

        // Iterator typedefs for vector access
        typedef std::vector<int>::iterator PortIterator;
        typedef std::vector<int>::const_iterator ConstPortIterator;
        typedef std::vector<std::string>::iterator ServerNameIterator;
        typedef std::vector<std::string>::const_iterator ConstServerNameIterator;
        typedef std::vector<RouteConfig>::iterator RouteIterator;
        typedef std::vector<RouteConfig>::const_iterator ConstRouteIterator;
        typedef std::map<int, std::string>::iterator ErrorPagesIterator;
    typedef std::map<int, std::string>::const_iterator ConstErrorPagesIterator;
    };
    
    std::vector<ServerConfig> servers;  // All server configurations

    // Iterator typedefs for server vector access
    typedef std::vector<ServerConfig>::iterator ServerIterator;
    typedef std::vector<ServerConfig>::const_iterator ConstServerIterator;
};

/**
 * @brief Configuration file parser and manager class
 * 
 * Parses webserver configuration files and provides access to structured
 * configuration data. Supports validation, duplicate detection, and
 * server/route mapping functionality.
 */
class ConfigParser {
private:
    Config config;                                              // Parsed configuration data
    std::vector<std::pair<std::string, int> > server_listen_addresses;  // Unique listen addresses
    std::map<std::string, Config::ServerConfig> server_map;     // Server name to config map
    std::map<std::string, Config::RouteConfig> route_map;       // Route path to config map

public:
    /**
     * @brief Default constructor - initializes empty parser
     */
    ConfigParser() {}

    /**
     * @brief Destructor - cleans up parser resources
     */
    ~ConfigParser() {}

    /**
     * @brief Parses server-level configuration key-value pairs
     * @param key Configuration directive name
     * @param value Configuration directive value
     * @param server Reference to server config to populate
     * @return 0 on success, -1 on error
     * Handles host, port, server_name, root, error_page, etc.
     */
    int parseServerKeyValue(const std::string& key, const std::string& value, Config::ServerConfig& server);

    /**
     * @brief Parses route-level configuration key-value pairs
     * @param key Configuration directive name
     * @param value Configuration directive value
     * @param route Reference to route config to populate
     * @return 0 on success, -1 on error
     * Handles path, methods, redirect, CGI, upload settings, etc.
     */
    int parseRouteKeyValue(const std::string& key, const std::string& value, Config::RouteConfig& route);

    /**
     * @brief Main configuration file parsing function
     * @param filename Path to configuration file
     * @return 0 on successful parsing, -1 on error
     * Parses entire config file, validates settings, detects duplicates
     */
    int parseConfigFile(const std::string& filename);

    /**
     * @brief Extracts unique server listen addresses from configuration
     * Builds list of unique host:port combinations for socket binding
     */
    void initializeServerListenAddresses();

    /**
     * @brief Creates server name to configuration mapping
     * @param server_map Reference to map to populate
     * @param config Configuration data to process
     * Maps server names to their respective configurations
     */
    void initializeServerMap(std::map<std::string, Config::ServerConfig>& server_map, const Config& config) const;

    /**
     * @brief Creates route path to configuration mapping
     * @param route_map Reference to map to populate
     * @param config Configuration data to process
     * Maps route paths to their respective configurations
     */
    void initializeRouteMap(std::map<std::string, Config::RouteConfig>& route_map, const Config& config) const;

    /**
     * @brief Gets list of unique server listen addresses
     * @return Vector of host:port pairs for socket binding
     */
    std::vector<std::pair<std::string, int> > getServerListenAddresses() const;

    /**
     * @brief Gets server configuration by server name
     * @param server_name Server name to lookup
     * @return Server configuration for the specified name
     */
    Config::ServerConfig getServerMap(const std::string& server_name) const;

    /**
     * @brief Gets route configuration mapping
     * @return Map of route paths to their configurations
     */
    std::map<std::string, Config::RouteConfig> getRouteMap() const;

    /**
     * @brief Prints server listen addresses to console
     * @param server_listen_addresses Vector of addresses to print
     * Debug utility function for displaying listen addresses
     */
    void printServerListenAddresses(std::vector<std::pair<std::string, int> > server_listen_addresses);

    /**
     * @brief Gets the complete parsed configuration
     * @return Const reference to parsed configuration structure
     */
    const Config getConfigs();

    /**
     * @brief Gets the server configuration using the host
     * @param Host The host for the server we are looking for
     * @param isIp Boolean that indicates whether the host is an IP or a server name 
     * @return Returns a struct containing the server config
     */
    Config::ServerConfig getServerConfig(std::string Host, int port , bool isIp);

};

#endif
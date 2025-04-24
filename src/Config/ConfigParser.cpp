#include "ConfigParser.hpp"
#include <fstream>
#include <sstream>

int ConfigParser::parseServerKeyValue(const std::string& key, const std::string& value, Config::ServerConfig& server)
{
    // Check for quotes and reject them
    if (value.find('"') != std::string::npos) {
        std::cerr << "Error: Quotes are not allowed in values: " << key << " = " << value << std::endl;
        return -1;
    }

    if (key == "port") {
        // Check if port contains only digits
        for (size_t i = 0; i < value.length(); i++) {
            if (!isdigit(value[i])) {
                std::cerr << "Error: Port must be a valid integer: " << value << std::endl;
                return -1;
            }
        }
        int port_c = std::atoi(value.c_str());
        if (port_c <= 0 || port_c > __INT_MAX__) {
            std::cerr << "Error: Port must be greater than 0 and less than " << __INT_MAX__ << ": " << value << std::endl;
            return -1;
        }
        server.ports.push_back(port_c);
    }
    else if (key == "host") {
        if (!server.host.empty() && server.host != "0.0.0.0") {
            std::cerr << "Error: Duplicate key 'host' detected" << std::endl;
            return -1;
        }
        server.host = value;
    }
    else if (key == "server_name") {
        server.server_names.push_back(value);
    }
    else if (key == "root") {
        if (!server.root.empty() && server.root != "/var/www/html") {
            std::cerr << "Error: Duplicate key 'root' detected" << std::endl;
            return -1;
        }
        server.root = value;
    }
    else if (key.find("error_page") != std::string::npos) {
        std::istringstream iss(key);
        std::string error_page_str;
        int errorCode;
        iss >> error_page_str >> errorCode;
        iss >> error_page_str >> errorCode;
        if (server.error_pages.find(errorCode) != server.error_pages.end()) {
            std::cerr << "Error: Duplicate error_page code " << errorCode << " detected" << std::endl;
            return -1;
        }
        server.error_pages[errorCode] = value;
    }
    else if (key == "client_max_body_size") {
        if (server.client_max_body_size != 1048576) {
            std::cerr << "Error: Duplicate key 'client_max_body_size' detected" << std::endl;
            return -1;
        }
        // Check if body size contains only digits
        for (size_t i = 0; i < value.length(); i++) {
            if (!isdigit(value[i])) {
                std::cerr << "Error: Client max body size must be a valid integer: " << value << std::endl;
                return -1;
            }
        }
        server.client_max_body_size = std::atoi(value.c_str());
        if (server.client_max_body_size <= 0) {
            std::cerr << "Error: Client max body size must be greater than 0: " << value << std::endl;
            return -1;
        }
        if(server.client_max_body_size > __INT_MAX__) {
            std::cerr << "Error: Client max body size exceeds maximum limit: " << value << std::endl;
            return -1;
        }
    }
    else if (key == "default_server") {
        if (server.default_server != false) {
            std::cerr << "Error: Duplicate key 'default_server' detected" << std::endl;
            return -1;
        }
        server.default_server = (value == "true" || value == "1" || value == "on");
    }
    return 0;
}

int ConfigParser::parseRouteKeyValue(const std::string& key, const std::string& value, Config::RouteConfig& route)
{
    // Check for quotes and reject them
    if (value.find('"') != std::string::npos) {
        std::cerr << "Error: Quotes are not allowed in values: " << key << " = " << value << std::endl;
        return -1;
    }
    
    if (key == "path") {
        if (!route.path.empty() && route.path != "/") {
            std::cerr << "Error: Duplicate key 'path' detected" << std::endl;
            return -1;
        }
        route.path = value;
    }

    else if (key == "accepted_methods") {
        if (!route.accepted_methods.empty()) {
            std::cerr << "Error: Duplicate key 'accepted_methods' detected" << std::endl;
            return -1;
        }
        std::istringstream iss(value);
        std::string method;
        while (iss >> method) {

            if (method != "GET" && method != "POST" && method != "DELETE") {
                std::cerr << "Error: Invalid HTTP method: " << method << std::endl;
                return -1;
            }
            for (size_t i = 0; i < route.accepted_methods.size(); i++) {
                if (route.accepted_methods[i] == method) {
                    std::cerr << "Error: Duplicate HTTP method: " << method << std::endl;
                    return -1;
                }
            }
            route.accepted_methods.push_back(method);
        }
    }
    else if (key == "redirect") {
        if (route.has_redirect) {
            std::cerr << "Error: Duplicate key 'redirect' detected" << std::endl;
            return -1;
        }
        route.has_redirect = true;
        size_t spacePos = value.find(' ');
        if (spacePos != std::string::npos) {
            std::string codeStr = value.substr(0, spacePos);
            // Check if redirect code contains only digits
            for (size_t i = 0; i < codeStr.length(); i++) {
                if (!isdigit(codeStr[i])) {
                    std::cerr << "Error: Redirect code must be a valid integer: " << codeStr << std::endl;
                    return -1;
                }
            }
            route.redirect_code = std::atoi(codeStr.c_str());
            if(route.redirect_code <= 0 || route.redirect_code > __INT_MAX__) {
                std::cerr << "Error: Redirect code must be greater than 0 and less than " << __INT_MAX__ << ": " << codeStr << std::endl;
                return -1;
            }
            route.redirect_url = value.substr(spacePos + 1);
        } else {
            // If no space found, assume the entire value is the URL with default code
            route.redirect_code = 301; // Default redirect code
            route.redirect_url = value; // Use the entire value as URL
        }
    }
    else if (key == "directory_listing") {
        if (route.directory_listing != false) {
            std::cerr << "Error: Duplicate key 'directory_listing' detected" << std::endl;
            return -1;
        }
        route.directory_listing = (value == "true" || value == "1" || value == "on");
    }
    else if (key == "index") {
        if (!route.index.empty()) {
            std::cerr << "Error: Duplicate key 'index' detected" << std::endl;
            return -1;
        }
        route.index = value;
    }
    else if (key == "cgi_pass") {
        if (route.cgi_enabled) {
            std::cerr << "Error: Duplicate key 'cgi_pass' detected" << std::endl;
            return -1;
        }
        route.cgi_pass = value;
        route.cgi_enabled = true;
    }
    else if (key == "cgi_extensions") {
        if (!route.cgi_extensions.empty()) {
            std::cerr << "Error: Duplicate key 'cgi_extensions' detected" << std::endl;
            return -1;
        }
        std::istringstream iss(value);
        std::string ext;
        while (iss >> ext) {
            route.cgi_extensions.push_back(ext);
        }
    }
    else if (key == "upload_path") {
        if (route.upload_enabled) {
            std::cerr << "Error: Duplicate key 'upload_path' detected" << std::endl;
            return -1;
        }
        route.upload_path = value;
        route.upload_enabled = true;
    }
    return 0;  // Return 0 to indicate success
}

int ConfigParser::parseConfigFile(const std::string& filename, Config& config)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file: " << filename << std::endl;
        return -1;
    }

    std::string line;
    Config::ServerConfig* currentServer = NULL;
    Config::RouteConfig* currentRoute = NULL;
    bool isServerSection = false;
    bool isRouteSection = false;

    while (std::getline(file, line)) {
        // Skip empty lines and trim whitespace
        if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
            continue;
        }
        
        // Trim leading whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        if (line.empty()) {
            continue;
        }

        // Skip comments (lines starting with #)
        if (line[0] == '#') {
            if (line.find("#server") != std::string::npos) {
                // Finish previous server if exists
                if (currentServer != NULL) {
                    if (currentRoute != NULL) {
                        currentServer->routes.push_back(*currentRoute);
                        delete currentRoute;
                        currentRoute = NULL;
                    }
                    config.servers.push_back(*currentServer);
                    delete currentServer;
                }
                
                // Create new server
                currentServer = new Config::ServerConfig();
                // Initialize server defaults
                currentServer->host = "0.0.0.0";  // Default to all interfaces
                currentServer->default_server = false;
                currentServer->client_max_body_size = 1048576; // 1MB default
                currentServer->root = "/var/www/html"; // Default root
                isServerSection = true;
                isRouteSection = false;
            }
            else if (line.find("#route") != std::string::npos) {
                if (currentServer == NULL) {
                    std::cerr << "Error: Route defined outside of server context" << std::endl;
                    // Clean up resources before returning
                    if (currentRoute != NULL) {
                        delete currentRoute;
                    }
                    return -1;
                }
                
                // Save previous route if exists
                if (currentRoute != NULL) {
                    currentServer->routes.push_back(*currentRoute);
                    delete currentRoute;
                }
                
                // Create new route
                currentRoute = new Config::RouteConfig();
                // Initialize route defaults
                currentRoute->path = "/"; // Default path to root
                currentRoute->directory_listing = false;
                currentRoute->has_redirect = false;
                currentRoute->redirect_code = 301; // Default redirect code
                currentRoute->cgi_enabled = false;
                currentRoute->upload_enabled = false;
                if (currentServer && !currentServer->root.empty()) {
                    currentRoute->root = currentServer->root; // Inherit from server
                }
                
                // Extract path from the route declaration if available
                size_t pathStart = line.find("path=");
                if (pathStart != std::string::npos) {
                    pathStart += 5; // Length of "path="
                    size_t pathEnd = line.find(" ", pathStart);
                    if (pathEnd != std::string::npos) {
                        currentRoute->path = line.substr(pathStart, pathEnd - pathStart);
                    } else {
                        currentRoute->path = line.substr(pathStart);
                    }
                }
                
                isServerSection = false;
                isRouteSection = true;
            }
            continue;
        }

        // Skip line if it starts with a comment or #
        if (line[0] == '#' || line.substr(0, 2) == "//") {
            continue;
        }
          int number_of_equals = 0; 
        // Parse key-value pairs
        size_t equalsPos = line.find('=');
        if (equalsPos != std::string::npos) {
            // Count the number of equals signs
            for (size_t i = 0; i < line.length(); i++) {
                if (line[i] == '=') {
                    number_of_equals++;
                }
            }
            if (number_of_equals > 1) {
                std::cerr << "Error: Multiple '=' signs detected in line: " << line << std::endl;
                return -1;
            }
        }
        if (equalsPos == std::string::npos) {
            // Check for error_page format: error_page code = path
            std::istringstream iss(line);
            std::string directive, code, equals, path;
            
            if (iss >> directive >> code >> equals >> path && directive == "error_page" && equals == "=") {
                if (currentServer != NULL && isServerSection) {
                    int errorCode = std::atoi(code.c_str());
                    
                    // Check for quotes and reject them
                    if (path.find('"') != std::string::npos) {
                        std::cerr << "Error: Quotes are not allowed in values: error_page " << code << " = " << path << std::endl;
                        continue;
                    }
                    
                    // Check for duplicates
                    if (currentServer->error_pages.find(errorCode) != currentServer->error_pages.end()) {
                        std::cerr << "Error: Duplicate error_page code " << errorCode << " detected" << std::endl;
                        continue;
                    }
                    
                    currentServer->error_pages[errorCode] = path;
                }
            }
            continue; // Skip other lines without '='
        }

        std::string key = line.substr(0, equalsPos);
        std::string value = line.substr(equalsPos + 1);

        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        // Remove comments from value
        size_t commentPos = value.find('#');
        if (commentPos != std::string::npos) {
            value = value.substr(0, commentPos);
        }
        
        if (isServerSection && currentServer != NULL) {
            if(parseServerKeyValue(key, value, *currentServer) == -1) {
                std::cerr << "Error parsing server key-value: " << key << " = " << value << std::endl;
                // Clean up resources before returning
                if (currentRoute != NULL) {
                    delete currentRoute;
                }
                delete currentServer;
                return -1;
            }
        }
        else if (isRouteSection && currentRoute != NULL) {
            if(parseRouteKeyValue(key, value, *currentRoute) == -1) {
                std::cerr << "Error parsing route key-value: " << key << " = " << value << std::endl;
                // Clean up resources before returning
                delete currentRoute;
                return -1;
            }
        }
        else {
            std::cerr << "Error: Key-value pair outside of server or route context: " << key << " = " << value << std::endl;
            // Clean up resources before returning
            if (currentRoute != NULL) {
                delete currentRoute;
            }
            if (currentServer != NULL) {
                delete currentServer;
            }
            return -1;
        }
    }

    // Add the last server and route
    if (currentRoute != NULL && currentServer != NULL) {
        currentServer->routes.push_back(*currentRoute);
        delete currentRoute;
    }

    if (currentServer != NULL) {
        config.servers.push_back(*currentServer);
        delete currentServer;
    }
    
    // Check for duplicate redirects in routes within each server
    for (size_t i = 0; i < config.servers.size(); i++) {
        std::map<std::string, bool> pathMap; // map to track paths
        for (size_t j = 0; j < config.servers[i].routes.size(); j++) {
            Config::RouteConfig& route = config.servers[i].routes[j];
            std::string path = route.path;
            
            // Check if this path was already defined in another route
            if (pathMap.find(path) != pathMap.end()) {
                std::cerr << "Error: Duplicate path '" << path 
                          << "' detected in server " << i + 1 << std::endl;
                return -1;
            }
            pathMap[path] = true;
        }
    }
    
    file.close();
    return 0;
}

void print_vector(const std::vector<std::string>& vec) {
    std::cout << "Vector contents: ";
    for (const auto& str : vec) {
        std::cout << str << " ";
    }
    std::cout << std::endl;
}

void ConfigParser::printConfig(const Config& config) const
{
    std::cout << "Configuration:" << std::endl;
    if (config.servers.empty()) {
        std::cout << "No servers configured." << std::endl;
        return;
    }
    
    for (const auto& server : config.servers) {
        std::cout << "Server Configuration:" << std::endl;
        std::cout << "  Host: " << server.host << std::endl;
        std::cout << "  Ports: ";
        for (const auto& port : server.ports) {
            std::cout << port << " ";
        }
        std::cout << std::endl;
        std::cout << "  Server Names: ";
        for (const auto& name : server.server_names) {
            std::cout << name << " ";
        }
        std::cout << std::endl;
        std::cout << "  Root: " << server.root << std::endl;
        std::cout << "  Error Pages:" << std::endl;
        for (const auto& errorPage : server.error_pages) {
            std::cout << "    " << errorPage.first << ": " << errorPage.second << std::endl;
        }
        std::cout << "  Client Max Body Size: " << server.client_max_body_size << std::endl;
        std::cout << "  Default Server: " << (server.default_server ? "true" : "false") << std::endl;

        for (const auto& route : server.routes) {
            std::cout << "  Route:" << std::endl;
            std::cout << "    Path: " << route.path << std::endl;
            std::cout << "    Root: " << route.root << std::endl;
            std::cout << "    Accepted Methods: ";
            for (const auto& method : route.accepted_methods) {
                std::cout << method << " ";
            }
            std::cout << std::endl;
            std::cout << "    Has Redirect: " << (route.has_redirect ? "true" : "false") << std::endl;
            if (route.has_redirect) {
                std::cout << "    Redirect Code: " << route.redirect_code << std::endl;
                std::cout << "    Redirect URL: " << route.redirect_url << std::endl;
            }
            std::cout << "    Directory Listing: " << (route.directory_listing ? "true" : "false") << std::endl;
            if (!route.index.empty()) {
                std::cout << "    Index: " << route.index << std::endl;
            }
            if (route.cgi_enabled) {
                std::cout << "    CGI Path: " << route.cgi_pass << std::endl;
                std::cout << "    CGI Extensions: ";
                for (const auto& ext : route.cgi_extensions) {
                    std::cout << ext << " ";
                }
                std::cout << std::endl;
            }
            if (route.upload_enabled) {
                std::cout << "    Upload Path: " << route.upload_path << std::endl;
            }
            if (server.client_max_body_size != 0) {
                std::cout << "    Route Max Body Size: " << server.client_max_body_size << std::endl;
            }
        }
        
        std::cout << std::endl;
    }
}

#include "ConfigParser.hpp"

int ConfigParser::parseServerKeyValue(const std::string& key, const std::string& value, Config::ServerConfig& server) {
    if (value.find('"') != std::string::npos) {
        std::cerr << "Error: Quotes are not allowed in values: " << key << " = " << value << std::endl;
        return -1;
    }

    if (key == "port") {
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

int ConfigParser::parseRouteKeyValue(const std::string& key, const std::string& value, Config::RouteConfig& route) {
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

    if (key == "root") {
        if (!value.empty())
            route.root = value;
    }

    else if (key == "accepted_methods") {
        if (!route.accepted_methods.empty()) {
            std::cerr << "Error: Duplicate key 'accepted_methods' detected" << std::endl;
            return -1;
        }
        std::istringstream iss(value);
        std::string method;
        while (iss >> method) {
  
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
        // Only enable redirect if value is not empty
        if (!value.empty()) {
            route.has_redirect = true;
            size_t spacePos = value.find(' ');
            if (spacePos != std::string::npos) {
                std::string codeStr = value.substr(0, spacePos);
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
                route.redirect_code = 301;
                route.redirect_url = value;
            }
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
    else if (key == "cgi_pass" || key == "cgi_path") { // support alias cgi_path
        // if (route.cgi_enabled) {
        //     std::cerr << "Error: Duplicate key 'cgi_pass' detected" << std::endl;
        //     return -1;
        // }
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
    else if (key == "cgi_extension") { // support single extension alias
        // allow multiple occurrences by pushing one token; do not treat as duplicate
        if (!value.empty()) {
            route.cgi_extensions.push_back(value);
        }
    }
    else if (key == "upload_enabled") {
        if (route.upload_enabled) {
            std::cerr << "Error: Duplicate key 'upload_path' detected" << std::endl;
            return -1;
        }
        route.upload_enabled =  (value == "true" ? true : false);
    }
    return 0;
}

int ConfigParser::parseConfigFile(const std::string& filename) {
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
    bool hasRootDirective = false;

    while (std::getline(file, line)) {
        if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
            continue;
        }
        
        line.erase(0, line.find_first_not_of(" \t"));
        if (line.empty()) {
            continue;
        }

        if (line[0] == '#') {
            if (line.find("#server") != std::string::npos) {
                if (currentServer != NULL) {
                    if (currentRoute != NULL) {
                        currentServer->routes.push_back(*currentRoute);
                        delete currentRoute;
                        currentRoute = NULL;
                    }
                    this->config.servers.push_back(*currentServer);
                    delete currentServer;
                }
                
                currentServer = new Config::ServerConfig();
                currentServer->host = "0.0.0.0";
                currentServer->default_server = false;
                currentServer->client_max_body_size = 1048576;
                currentServer->root = "/var/www/html";
                isServerSection = true;
                isRouteSection = false;
            }
            else if (line.find("#route") != std::string::npos) { 
                hasRootDirective = true;
                if (currentServer == NULL) {
                    std::cerr << "Error: Route defined outside of server context" << std::endl;
                    if (currentRoute != NULL) {
                        delete currentRoute;
                    }
                    return -1;
                }
                
                if (currentRoute != NULL) {
                    currentServer->routes.push_back(*currentRoute);
                    delete currentRoute;
                }
                
                currentRoute = new Config::RouteConfig();
                currentRoute->path = "/";
                currentRoute->directory_listing = false;
                currentRoute->has_redirect = false;
                currentRoute->redirect_code = 301;
                currentRoute->cgi_enabled = true;
                currentRoute->upload_enabled = false;
                if (currentServer && !currentServer->root.empty()) {
                    currentRoute->root = currentServer->root;
                }
                
                size_t pathStart = line.find("path=");
                if (pathStart != std::string::npos) {
                    pathStart += 5;
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

        if (line[0] == '#' || line.substr(0, 2) == "//") {
            continue;
        }

        int number_of_equals = 0; 
        size_t equalsPos = line.find('=');
        if (equalsPos != std::string::npos) {
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
            std::istringstream iss(line);
            std::string directive, code, equals, path;
            
            if (iss >> directive >> code >> equals >> path && directive == "error_page" && equals == "=") {
                if (currentServer != NULL && isServerSection) {
                    int errorCode = std::atoi(code.c_str());
                    
                    if (path.find('"') != std::string::npos) {
                        std::cerr << "Error: Quotes are not allowed in values: error_page " << code << " = " << path << std::endl;
                        continue;
                    }
                    
                    if (currentServer->error_pages.find(errorCode) != currentServer->error_pages.end()) {
                        std::cerr << "Error: Duplicate error_page code " << errorCode << " detected" << std::endl;
                        continue;
                    }
                    
                    currentServer->error_pages[errorCode] = path;
                }
            }
            continue;
        }

        std::string key = line.substr(0, equalsPos);
        std::string value = line.substr(equalsPos + 1);

        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        size_t commentPos = value.find('#');
        if (commentPos != std::string::npos) {
            value = value.substr(0, commentPos);
        }
        
        if (isServerSection && currentServer != NULL) {
            if(parseServerKeyValue(key, value, *currentServer) == -1) {
                std::cerr << "Error parsing server key-value: " << key << " = " << value << std::endl;
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
                delete currentRoute;
                return -1;
            }
        }
        else {
            std::cerr << "Error: Key-value pair outside of server or route context: " << key << " = " << value << std::endl;
            if (currentRoute != NULL) {
                delete currentRoute;
            }
            if (currentServer != NULL) {
                delete currentServer;
            }
            return -1;
        }
    }

    if (currentRoute != NULL && currentServer != NULL) {
        currentServer->routes.push_back(*currentRoute);
        delete currentRoute;
    }

    if (currentServer != NULL) {
        this->config.servers.push_back(*currentServer);
        delete currentServer;
    }
    
    for (size_t i = 0; i < this->config.servers.size(); i++) {
        if (this->config.servers[i].ports.empty()) {
            this->config.servers[i].ports.push_back(8080);
        }
    }
    
    std::map<std::string, bool> serverNameMap;
    for (size_t i = 0; i < this->config.servers.size(); i++) {
        for (Config::ServerConfig::ConstServerNameIterator name = this->config.servers[i].server_names.begin(); 
             name != this->config.servers[i].server_names.end(); ++name) {
            if (serverNameMap.find(*name) != serverNameMap.end()) {
                std::cerr << "Error: Duplicate server_name '" << *name 
                          << "' detected across multiple servers" << std::endl;
                return -1;
            }
            serverNameMap[*name] = true;
        }
    }
    
    for (size_t i = 0; i < this->config.servers.size(); i++) {
        std::map<std::string, bool> pathMap;
        for (size_t j = 0; j < this->config.servers[i].routes.size(); j++) {
            Config::RouteConfig& route = this->config.servers[i].routes[j];
            std::string path = route.path;
            
            if (pathMap.find(path) != pathMap.end()) {
                std::cerr << "Error: Duplicate path '" << path 
                          << "' detected in server " << i + 1 << std::endl;
                return -1;
            }
            pathMap[path] = true;
        }
    }
    
    if (!hasRootDirective && this->config.servers.size() > 0) {
        std::cerr << "Warning: no \"root\" directive in server" << std::endl;
        return -1;
    }
    
    file.close();
    return 0;
}

const Config ConfigParser::getConfigs() {
    return this->config;
}

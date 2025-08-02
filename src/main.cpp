#include "main.hpp"
#include "HTTP/Request.hpp"
#include "Socket/socket.hpp"
#include "Server/monitorClient.hpp"
#include <iostream>
#include <string>
#include <unistd.h>
#include <cstring>


void printConfig(const Config& config)
{
    std::cout << "\n========== CONFIGURATION SUMMARY ==========\n";
    std::cout << "Total servers: " << config.servers.size() << "\n\n";
    
    for (size_t i = 0; i < config.servers.size(); i++)
    {
        const Config::ServerConfig& server = config.servers[i];
        
        std::cout << "┌─────────────────────────────────────────┐\n";
        std::cout << "│         SERVER #" << (i + 1) << " - START           │\n";
        std::cout << "└─────────────────────────────────────────┘\n";
        
        std::cout << "  Host: " << server.host << "\n";
        
        std::cout << "  Ports: ";
        for (size_t j = 0; j < server.ports.size(); j++) {
            std::cout << server.ports[j];
            if (j < server.ports.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
        
        std::cout << "  Server Names: ";
        for (size_t j = 0; j < server.server_names.size(); j++) {
            std::cout << server.server_names[j];
            if (j < server.server_names.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
        
        std::cout << "  Root Directory: " << server.root << "\n";
        std::cout << "  Client Max Body Size: " << server.client_max_body_size << " bytes\n";
        std::cout << "  Default Server: " << (server.default_server ? "YES" : "NO") << "\n";
        std::cout << "  Chunked Transfer: " << (server.chunked_transfer ? "ENABLED" : "DISABLED") << "\n";
        
        std::cout << "  Error Pages:\n";
        for (std::map<int, std::string>::const_iterator it = server.error_pages.begin(); 
             it != server.error_pages.end(); ++it) {
            std::cout << "    " << it->first << " -> " << it->second << "\n";
        }
        
        std::cout << "  Routes (" << server.routes.size() << "):\n";
        for (size_t j = 0; j < server.routes.size(); j++) {
            const Config::RouteConfig& route = server.routes[j];
            std::cout << "    ► Route: " << route.path << "\n";
            std::cout << "      Root: " << route.root << "\n";
            std::cout << "      Accepted Methods: ";
            for (size_t k = 0; k < route.accepted_methods.size(); k++) {
                std::cout << route.accepted_methods[k];
                if (k < route.accepted_methods.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
            std::cout << "      Directory Listing: " << (route.directory_listing ? "ON" : "OFF") << "\n";
            if (!route.index.empty()) {
                std::cout << "      Index: " << route.index << "\n";
            }
            if (route.has_redirect) {
                std::cout << "      Redirect: " << route.redirect_code << " -> " << route.redirect_url << "\n";
            }
            if (route.cgi_enabled) {
                std::cout << "      CGI: ENABLED\n";
                std::cout << "      CGI Pass: " << route.cgi_pass << "\n";
                std::cout << "      CGI Extensions: ";
                for (size_t k = 0; k < route.cgi_extensions.size(); k++) {
                    std::cout << route.cgi_extensions[k];
                    if (k < route.cgi_extensions.size() - 1) std::cout << ", ";
                }
                std::cout << "\n";
            }
            if (route.upload_enabled) {
                std::cout << "      Upload: ENABLED\n";
                std::cout << "      Upload Path: " << route.upload_path << "\n";
            }
        }
        
        std::cout << "┌─────────────────────────────────────────┐\n";
        std::cout << "│         SERVER #" << (i + 1) << " - END             │\n";
        std::cout << "└─────────────────────────────────────────┘\n\n";
    }
    
    std::cout << "========== END OF CONFIGURATION ==========\n\n";
}

int main(int argc, char** argv) {
   
    try {
        ConfigParser config_parser;

        // Use default config file
        const char* config_file = "src/config.conf";
        if (argc > 2) {
            config_file = argv[2];
        }
        
        int parse_config = config_parser.parseConfigFile(config_file);
        if (parse_config == 0) {
            config_parser.initializeServerListenAddresses();
            printConfig(config_parser.getConfig());
    
            sock socketCreate(config_parser.getServerListenAddresses());
            std::cout << "Sockets created successfully" << std::endl;
            
            monitorClient mc(socketCreate.getFDs());
            std::cout << "Starting event loop..." << std::endl;
            mc.startEventLoop();
            
        } else {
            std::cerr << "Error parsing configuration file." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
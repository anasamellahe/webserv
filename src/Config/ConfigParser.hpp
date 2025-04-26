#ifndef WEBSERVER_CONFIGPARSER_HPP
#define WEBSERVER_CONFIGPARSER_HPP

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include "../main.hpp"



// struct root
// {
//     path = /media
//     is_redirection = true;
//     redirectionCode = 301;
//     location = /newMedia;

//     //if the redirection flag true no path need  
//     root = /var/www/newMedia/;
//     indexs = index.html 
//     accepted_method = post get;
//     directory_listing = false;

//     //-----------------------------------

//     bool cgi_enabled;
//     std::string cgi_pass;
//     std::vector<std::string> cgi_extensions;
//     bool upload_enabled;
//     std::string upload_path;
// }
// struct server
// {
//     hosts = sami;
//     ports = 8080
//     servar_names = sami.com www.sami.com
//     max_client_body_size = 100MB
//     error_page = /var/www/404.html;
//     std::vector<root> srverConfig
// };

// std::map <servername, server>

struct Config {
    struct RouteConfig {
        std::string path;
        std::string root;
        std::vector<std::string> accepted_methods;
        bool has_redirect;
        int redirect_code;
        std::string redirect_url;
        bool directory_listing;
        std::string index;
        bool cgi_enabled;
        std::string cgi_pass;
        std::vector<std::string> cgi_extensions;
        bool upload_enabled;
        std::string upload_path;
    };

    struct ServerConfig {
        std::string host;
        std::vector<int> ports;
        std::vector<std::string> server_names;
        std::string root;
        std::map<int, std::string> error_pages;
        size_t client_max_body_size;
        bool default_server;
        std::vector<RouteConfig> routes;
    };

    std::vector<ServerConfig> servers;
    
};

class ConfigParser 
{
    private:
        std::vector<std::pair<std::string, int>> server_listen_addresses;  
    public : 
        ConfigParser() {}
        ~ConfigParser() {}

        int parseServerKeyValue(const std::string& key, const std::string& value, Config::ServerConfig& server);
        int parseRouteKeyValue(const std::string& key, const std::string& value, Config::RouteConfig& route);
        int parseConfigFile(const std::string& filename, Config& config);
        void printConfig(const Config& config) const;

        void initializeServerListenAddresses(const Config& config);
        std::vector<std::pair<std::string, int>> getServerListenAddresses() const;
             void  printServerListenAddresses(std::vector<std::pair<std::string, int>> server_listen_addresses);

    
};

#endif
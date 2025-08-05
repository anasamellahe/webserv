#include "ConfigParser.hpp"


void ConfigParser::initializeServerListenAddresses() {
    for (size_t i = 0; i < this->config.servers.size(); ++i) {
        const Config::ServerConfig& server = this->config.servers[i];
        for (size_t j = 0; j < server.ports.size(); ++j) {
            std::pair<std::string, int> address(server.host, server.ports[j]);
            bool isDuplicate = false;
            for (size_t k = 0; k < server_listen_addresses.size(); ++k) {
                if (server_listen_addresses[k].first == address.first && 
                    server_listen_addresses[k].second == address.second) {
                    isDuplicate = true;
                    break;
                }
            }
            if (!isDuplicate) {
                this->server_listen_addresses.push_back(address);
            }
        }
    }
}

std::vector<std::pair<std::string, int> > ConfigParser::getServerListenAddresses() const {
    return this->server_listen_addresses;
}

void ConfigParser::printServerListenAddresses(std::vector<std::pair<std::string, int> > server_listen_addresses) {
    std::cout << "Server Listen Addresses:" << std::endl;
    for (size_t i = 0; i < server_listen_addresses.size(); ++i) {
        std::cout << "  Host: " << server_listen_addresses[i].first 
                  << ", Port: " << server_listen_addresses[i].second << std::endl;
    }
}

void ConfigParser::initializeServerMap(std::map<std::string, Config::ServerConfig>& server_map, const Config& config) const {
    for (size_t i = 0; i < config.servers.size(); ++i) {
        const Config::ServerConfig& server = config.servers[i];
        for (size_t j = 0; j < server.server_names.size(); ++j) {
            server_map[server.server_names[j]] = server;
        }
    }
}

void ConfigParser::initializeRouteMap(std::map<std::string, Config::RouteConfig>& route_map, const Config& config) const {
    for (size_t i = 0; i < config.servers.size(); ++i) {
        const Config::ServerConfig& server = config.servers[i];
        for (size_t j = 0; j < server.routes.size(); ++j) {
            const Config::RouteConfig& route = server.routes[j];
            route_map[route.path] = route;
        }
    }
}

std::map<std::string, Config::RouteConfig> ConfigParser::getRouteMap() const {
    std::map<std::string, Config::RouteConfig> route_map;
    initializeRouteMap(route_map, this->config);
    return route_map;
}


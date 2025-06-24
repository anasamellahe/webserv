#include "ConfigParser.hpp"
#include <fstream>
#include <sstream>


void ConfigParser::initializeServerListenAddresses(const Config& config)
{
    for (size_t i = 0; i < config.servers.size(); ++i) {
        const Config::ServerConfig& server = config.servers[i];
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
            // if we dont find the address in the vector we add it

           
        }
    }
}


std::vector<std::pair<std::string, int> > ConfigParser::getServerListenAddresses() const
{
    return this->server_listen_addresses;
}

void ConfigParser::printServerListenAddresses(std::vector<std::pair<std::string, int> > server_listen_addresses)
{
    std::cout << "Server Listen Addresses:" << std::endl;
    for (size_t i = 0; i < server_listen_addresses.size(); ++i) {
        std::cout << "  Host: " << server_listen_addresses[i].first 
                  << ", Port: " << server_listen_addresses[i].second << std::endl;
    }
}
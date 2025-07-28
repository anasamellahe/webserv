#include "main.hpp"
#include "HTTP/Request.hpp"
#include "Socket/socket.hpp"
#include "Server/monitorClient.hpp"
#include <iostream>
#include <string>
#include <unistd.h>
#include <cstring>



int main(int argc, char** argv) {
   
    try {
        ConfigParser config_parser;
        Config config;

        // Use default config file
        const char* config_file = "src/config.conf";
        if (argc > 2) {
            config_file = argv[2];
        }
        
        int parse_config = config_parser.parseConfigFile(config_file, config);
        if (parse_config == 0) {
            config_parser.initializeServerListenAddresses(config);
            config_parser.printConfig(config);
            
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
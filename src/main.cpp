#include "main.hpp"

int main (int ac , char **av)
{


    int parse_config = 0;
    if (ac != 2)
    {
        std::cerr << "Usage: config  file" << std::endl;
        return 1;
    }
    try
    {
        ConfigParser config_parser;
        Config config;
        parse_config = config_parser.parseConfigFile(av[1], config);
        if (parse_config == 0)
        {  
            
            // sock socketCreate(config_parser.getServerListenAddresses());
            config_parser.initializeServerListenAddresses(config);
            sock socketCreate(config_parser.getServerListenAddresses());
            monitorClient mc(socketCreate.getFDs());
            mc.startEventLoop();
        }
        else
            std::cerr << "Error parsing configuration file." << std::endl;
    }
    catch(const std::exception& e)
    {
         std::cerr << "Exception: " << e.what() << std::endl;
         return 1;
    }
    
    
}

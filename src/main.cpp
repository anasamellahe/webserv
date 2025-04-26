
#include "main.hpp"
int main (int ac , char **av)
{

    int parse_congfig = 0;
    if (ac != 2)
    {
        std::cerr << "Usage: config  file" << std::endl;
        return 1;
    }
    std::string config_file = av[1];

       try
       {
            if(parse_congfig == 0)
            {
                ConfigParser config_parser;
                Config config;
                parse_congfig = config_parser.parseConfigFile(config_file, config);
                if (parse_congfig == 0)
                {
                
                    config_parser.printConfig(config);
                }
            }
            else
            {
                std::cerr << "Error parsing configuration file." << std::endl;
            }
       }
       catch(const std::exception& e)
       {
            std::cerr << "Exception: " << e.what() << std::endl;
            return 1;
       }
    
}
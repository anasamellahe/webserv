#include "main.hpp"

int main(int ac, char *av[])
{
    if (ac != 2)
    {
        std::cerr << "Usage: " << av[0] << " <config_file>" << std::endl;
        return 1;
    }

    std::string config_file = av[1];
    ConfigParser config_parser;
    Config config;

    try
    {
        // Parse config
        int parse_config = config_parser.parseConfigFile(config_file, config);
        if (parse_config != 0)
        {
            std::cerr << "Error parsing configuration file." << std::endl;
            return 1;
        }

        config_parser.printConfig(config); 

        // Initialize listen addresses from config
        config_parser.initializeServerListenAddresses(config);
        std::vector<std::pair<std::string, int>> listen_addresses = config_parser.getServerListenAddresses();

        config_parser.printServerListenAddresses(listen_addresses); 

        // Set up server socket data
        testSocketData data;
        for (size_t i = 0; i < listen_addresses.size(); ++i)
        {
            data.ports.push_back(listen_addresses[i].second);
        }

        sock serverSocket(data);
        std::cout << "Server initialized successfully.\n";

        monitorClient myMonitor(serverSocket.getFDs());
        myMonitor.startEventLoop();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

#include "../includes/webserv.hpp"

#define PORTNUMBER 8080

int main (int ac, char *av[])
{
    (void)ac;
    (void)av;
    
    try
    {
        // create a socket using the sock class

        sock serverSOcket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

        // bind the socket  using the sock class
        serverSOcket.bindINET(AF_INET, PORTNUMBER, INADDR_ANY);


        //stet up monitoring session

        monitorClient myMonitor(serverSOcket.getFD());
        //start the event loop
        myMonitor.startEventLoop();
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << "\n";
    }

}
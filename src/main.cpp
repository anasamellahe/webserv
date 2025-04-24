#include "../includes/webserv.hpp"
#include "../includes/sock.hpp"
#define PORTNUMBER 8080

int main (int ac, char *av[])
{
    (void)ac;
    (void)av;
    
    try
    {
        // create a socket using the sock class
        testSocketData data;
        data.ports.push_back(8080);
        data.ports.push_back(5050);

        sock serverSOcket(data);
        std::cout << "good\n";
        

        //stet up monitoring session

        monitorClient myMonitor(serverSOcket.getFDs());
        // start the event loop
        myMonitor.startEventLoop();
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << "\n";
    }

}
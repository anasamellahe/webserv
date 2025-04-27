#ifndef WEBSERVER_SERVER_HPP
#define WEBSERVER_SERVER_HPP

#include <vector>
#include <map>
#include <string>
#include <poll.h>
#include "request.hpp"

class Server {
private:
    std::vector<std::pair<std::string, int>> server_listen_addresses;
    std::vector<int> server_sockets;
    std::map<int, Client> clients;  
    std::vector<pollfd> poll_fds;    
    Config* config;
public:

    void setServerListenAddresses(const std::vector<std::pair<std::string, int>>& addresses);
    bool initializeSockets();
    bool startListening();




    void handlePollEvents();
    void acceptNewConnection(int server_socket);
    void handleClientData(int client_socket);
    void sendResponse(int client_socket);
    void closeConnection(int client_socket);

    const std::vector<std::pair<std::string, int>>& getServerListenAddresses() const;
    bool isServerSocket(int socket) const;
};

#endif 

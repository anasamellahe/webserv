#include "monitorClient.hpp"
#include "../HTTP/Request.hpp"
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <sstream>

    // int socketAddres;
    // std::vector<pollfd> fds;

    monitorClient::monitorClient(std::vector<int> serverFDs)
    {
        //fill the fds vector withe the servers socket
        pollfd serverPollFd;
        for (size_t i = 0; i < serverFDs.size(); i++)
        {
            memset(&serverPollFd, 0, sizeof(serverPollFd));
            std::cout << "socket add == [ " << serverFDs[i] << " ]\n";
            serverPollFd.fd = serverFDs[i];
            serverPollFd.events = POLLIN;
            fds.push_back(serverPollFd);
        }
        this->numberOfServers = serverFDs.size();
    }
    void monitorClient::acceptNewClient(int serverFD)
    {
       
        int clientFd = accept(serverFD, NULL, NULL);
        if (clientFd == -1)
            throw monitorexception("[ERROR] accept fail \n");
        
        // make the clientFD file descriptor non-block  
        if (fcntl(clientFd, F_SETFL,  O_NONBLOCK) < 0)
            throw monitorexception("[ERROR]: fcntl fail");

        //create a pollfd for the new client
        pollfd serverPollFd;
        memset(&serverPollFd, 0, sizeof(serverPollFd));
        serverPollFd.fd = clientFd; 
        serverPollFd.events = POLLIN;
        fds.push_back(serverPollFd);

        // create a client tracker to track the Request and response delivery 
        SocketTracker st;
        this->fdsTracker.insert(std::pair<int, SocketTracker>(clientFd, st));
        std::cout << serverFD << " accept new connection " << clientFd << "\n";
    }
    void monitorClient::removeClient(int index)
    {
        Viterator it  = fds.begin() + index;
        close(it->fd);
        this->fds.erase(it);
        this->fdsTracker.erase(it->fd);
    }


    void monitorClient::startEventLoop() {
        int ready = 0;
        size_t lastProcessedClient = numberOfServers; // Track last processed client index
        initializeTimeouts();
        
        while (1) {   
            time_t now = time(NULL);
            if (shouldCheckTimeouts(now)) {
                checkTimeouts();
                lastTimeoutCheck = now;
            }
            
            // Use poll with a short timeout to prevent blocking indefinitely
            ready = poll(fds.data(), fds.size(), 100); // 100ms timeout
            if (ready == -1) {
                if (errno == EINTR) continue;
                throw monitorexception("[ERROR] poll fail: " + std::string(strerror(errno)));
            }
            
            // Process servers for new connections
            for (size_t i = 0; i < numberOfServers; i++) {
                if (fds[i].revents & POLLIN) {
                    acceptNewClient(fds[i].fd);
                }
            }
            
            // Process clients in round-robin order
            size_t clientCount = fds.size() - numberOfServers;
            if (clientCount == 0) continue;
            
            // Start processing from the next client
            size_t startIdx = (lastProcessedClient >= numberOfServers) 
                              ? lastProcessedClient + 1 
                              : numberOfServers;
            
            for (size_t i = 0; i < clientCount; i++) {
                size_t currentIdx = (startIdx + i) % clientCount + numberOfServers;
                if (currentIdx >= fds.size()) continue;
                
                pollfd& currentFd = fds[currentIdx];
                if (currentFd.revents & POLLIN) {
                    // Process only one chunk per iteration
                    int keepAlive = readClientRequest(currentFd.fd);
                    updateClientActivity(currentFd.fd);
                    
                    std::map<int, SocketTracker>::iterator it = fdsTracker.find(currentFd.fd);
                    if (it != fdsTracker.end() && !it->second.response.empty()) {
                        currentFd.events |= POLLOUT;
                    }
                    
                    if (keepAlive == 0) {
                        removeClient(currentIdx);
                        clientCount--; // Adjust count after removal
                        i--; // Adjust loop index
                    }
                }
                
                // Update last processed index
                lastProcessedClient = currentIdx;
            }
        }
    }

    monitorClient::~monitorClient()
    {
        std::cout << "close all fds \n";
        for (Viterator it = fds.begin(); it != fds.end(); it++)
        {
            if (it->fd > 0)
                close(it->fd);
        }
    }

    monitorClient::monitorexception::monitorexception(std::string msg)
    {
        this->ErrorMsg = msg;
    }
    const char *monitorClient::monitorexception::what () const throw()
    {
        return ErrorMsg.c_str();
    }


    monitorClient::monitorexception::~monitorexception() throw()
    {}

    // SocketTracker* monitorClient::getSocketTracker(int socketFd)
    // {
    //     CMiterator it = this->fdsTracker.find(socketFd);
    //     if (it != this->fdsTracker.end())
    //         return &it->second;
    //     return NULL;
    // }
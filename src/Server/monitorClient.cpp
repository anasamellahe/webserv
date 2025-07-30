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
        if (clientFd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No pending connections, not an error
                return;
            }
            throw monitorexception("[ERROR] accept fail: " + std::string(strerror(errno)));
        }
        
        // Validate the client file descriptor
        if (clientFd < 0) {
            std::cerr << "Invalid client file descriptor: " << clientFd << std::endl;
            return;
        }
        
        // make the clientFD file descriptor non-block  
        if (fcntl(clientFd, F_SETFL,  O_NONBLOCK) < 0) {
            close(clientFd);
            throw monitorexception("[ERROR]: fcntl fail: " + std::string(strerror(errno)));
        }

        //create a pollfd for the new client
        pollfd serverPollFd;
        memset(&serverPollFd, 0, sizeof(serverPollFd));
        serverPollFd.fd = clientFd; 
        serverPollFd.events = POLLIN;
        serverPollFd.revents = 0;
        
        try {
            fds.push_back(serverPollFd);

            // create a client tracker to track the Request and response delivery 
            SocketTracker st;
            std::pair<Miterator, bool> result = this->fdsTracker.insert(
                std::pair<int, SocketTracker>(clientFd, st)
            );
            
            if (!result.second) {
                // Insertion failed - client already exists
                std::cerr << "Warning: Client " << clientFd << " already exists in tracker" << std::endl;
                fds.pop_back(); // Remove the pollfd we just added
                close(clientFd);
                return;
            }
            
            std::cout << "Server " << serverFD << " accepted new connection " << clientFd << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Exception while adding client " << clientFd << ": " << e.what() << std::endl;
            close(clientFd);
            throw;
        }
    }
    
    void monitorClient::removeClient(int index)
    {
        if (index < 0 || index >= static_cast<int>(fds.size()) || index < static_cast<int>(numberOfServers)) {
            std::cerr << "Invalid index " << index << " for removeClient (size: " << fds.size() << ", servers: " << numberOfServers << ")" << std::endl;
            return;
        }
        
        // Store the file descriptor before erasing
        int clientFd = fds[index].fd;
        
        // Close the file descriptor
        close(clientFd);
        
        // Remove from fds vector
        fds.erase(fds.begin() + index);
        
        // Remove from tracker map
        fdsTracker.erase(clientFd);
        
        std::cout << "Client " << clientFd << " removed from index " << index << std::endl;
    }


    void monitorClient::startEventLoop() {
        int ready = 0;
        size_t lastProcessedClient = numberOfServers; // Track last processed client for round-robin
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
            
            // Process servers for new connections first
            for (size_t i = 0; i < numberOfServers; i++) {
                if (fds[i].revents & POLLIN) {
                    acceptNewClient(fds[i].fd);
                }
            }
            
            // ROUND-ROBIN CLIENT PROCESSING
            size_t clientCount = fds.size() - numberOfServers;
            if (clientCount > 0) {
                // Calculate next client to process in round-robin fashion
                size_t nextClientIndex = numberOfServers;
                if (lastProcessedClient >= numberOfServers && lastProcessedClient < fds.size()) {
                    nextClientIndex = lastProcessedClient + 1;
                    if (nextClientIndex >= fds.size()) {
                        nextClientIndex = numberOfServers; // Wrap around to first client
                    }
                } else {
                    nextClientIndex = numberOfServers; // Start from first client
                }
                
                // Process clients in round-robin order
                for (size_t processed = 0; processed < clientCount; processed++) {
                    size_t currentIndex = nextClientIndex + processed;
                    if (currentIndex >= fds.size()) {
                        currentIndex = numberOfServers + (currentIndex - fds.size()); // Wrap around
                    }
                    
                    // Safety check for valid index
                    if (currentIndex >= fds.size() || currentIndex < numberOfServers) {
                        continue;
                    }
                    
                    pollfd& currentFd = fds[currentIndex];
                    
                    // Process client if it has data ready OR for fairness in round-robin
                    if (currentFd.revents & POLLIN) {
                        int keepAlive = readClientRequest(currentFd.fd);
                        updateClientActivity(currentFd.fd);
                        
                        std::map<int, SocketTracker>::iterator it = fdsTracker.find(currentFd.fd);
                        if (it != fdsTracker.end() && !it->second.response.empty()) {
                            currentFd.events |= POLLOUT;
                        }
                        
                        if (keepAlive == 0) {
                            removeClient(currentIndex);
                            // Adjust indices after removal
                            if (currentIndex <= lastProcessedClient) {
                                lastProcessedClient--;
                            }
                            clientCount--; // Update client count
                            break; // Exit loop after removal to avoid index issues
                        }
                        
                        // Update last processed client for next round
                        lastProcessedClient = currentIndex;
                        break; // Process only one client per poll cycle for true round-robin
                    }
                    
                    // Handle POLLOUT events for sending responses
                    if (currentFd.revents & POLLOUT) {
                        //writeClientResponse(currentFd.fd);
                        currentFd.events = POLLIN; // Reset to listen for new requests
                        
                        // Update last processed client
                        lastProcessedClient = currentIndex;
                        break; // Process only one client per poll cycle
                    }
                }
                
                // If no clients were processed, advance the round-robin pointer
                if (ready == 0 && clientCount > 0) {
                    lastProcessedClient++;
                    if (lastProcessedClient >= fds.size()) {
                        lastProcessedClient = numberOfServers;
                    }
                }
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
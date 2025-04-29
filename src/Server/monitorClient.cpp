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

        // create a client tracker to track the request and response delivery 
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

int monitorClient::readChunkFromClient(int clientFd, std::string& buffer) {
    char chunk[CHUNK_SIZE];
    ssize_t bytesRead = read(clientFd, chunk, CHUNK_SIZE);

    if (bytesRead > 0) {
        buffer.append(chunk, bytesRead);
        return bytesRead;
    } else if (bytesRead == 0) {
        // Client closed the connection
        return 0;
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No data available, try again later
        return -1;
    } else {
        // Error occurred
        return -2;
    }
}

int monitorClient::readClientRequest(int clientFd) {
    auto it = fdsTracker.find(clientFd);
    if (it == fdsTracker.end()) {
        return -1; // Client not found
    }

    SocketTracker& tracker = it->second;

    // Call parseFromSocket to handle reading and parsing
    request req(clientFd);
    bool parseSuccess = req.parseFromSocket(clientFd);

    if (!parseSuccess) {
        // Parsing failed, log the error and close the connection
        tracker.error = req.getErrorCode();
        std::cerr << "[ERROR] Failed to parse request: " << tracker.error << "\n";
        return -1;
    }

    // If parsing is successful, check if the request is complete
    if (req.isComplete()) {
        return 1; // Request is complete
    }

    // Request is not complete, move to the next client
    return 0;
}

    void monitorClient::startEventLoop()
    {
        int ready = 0;
        while (1)
        {   
            std::cout << "monitor event loop started \n";
            ready = poll(fds.data(), fds.size(), -1);
            if (ready == -1)
                throw monitorexception("[ERROR] poll fail \n");
            // std::cout << "number of client who are ready is [ "  << ready << " ]\n";
            for (size_t i = 0; i < numberOfServers; i++)
            {
                if (fds[i].revents & POLLIN)
                    acceptNewClient(fds[i].fd);
            }

            for (size_t i = numberOfServers; i < fds.size(); i++)
            {
                int keepAlive = 1;
                if (fds[i].revents & POLLIN)
                {
                    std::cout << fds[i].fd <<" ready to start reading from\n";
                    keepAlive = readClientRequest(fds[i].fd);
                    fds[i].events |= POLLOUT;
                }

                if (fds[i].revents & POLLOUT )
                {
                    std::cout << fds[i].fd <<" ready to start writing to\n";
                    //writeClientResponse(fds[i].fd);
                    fds[i].events &= ~POLLOUT;
                }
                // if (keepAlive == 0 )
                // removeClient(i);
                // ready--;
                // if (ready == 0)
                //     break;
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
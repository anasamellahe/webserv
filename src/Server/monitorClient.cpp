#include "monitorClient.hpp"
#include "../HTTP/Request.hpp"

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



int monitorClient::readClientRequest(int clientFd)
{
    request req(clientFd);
    
    // Parse the request from the socket
    bool parseSuccess = req.parseFromSocket(clientFd);
    
    // Check if parsing failed or the request is invalid
    if (!parseSuccess )
    {
        std::cerr << "[ERROR] Can't read the request or invalid request\n";
       // fdsTracker[clientFd].error = req.getErrorCode(); // Store error code for logging/debugging
        return 0; // Signal to close the connection
    }
//        std::string key  ;
//     fdsTracker[clientFd].request = req.getBody();
    
//     // Store additional request details in the tracker
//     fdsTracker[clientFd].method = req.getMethod();
//     fdsTracker[clientFd].path = req.getPath();
//     fdsTracker[clientFd].headers = req.getAllHeaders();
//     fdsTracker[clientFd].queryParams = req.getQueryParams();
//     fdsTracker[clientFd].cookies = req.getCookies();
    
//     // If the request is chunked, store the chunks
//     if (req.isChunked())
//     {
//         fdsTracker[clientFd].chunks = req.getChunks();
//     }
    
//     // If there are file uploads, store them
//     if (!req.getUploads().empty())
//     {
//         fdsTracker[clientFd].uploads = req.getUploads();
//     }
    
//     // If the request has a "Connection: close" header, signal to close the connection
//     if (req.getAllHeaders().count("Connection") && 
//         req.getAllHeaders().at("Connection") == "close")
//     {
//         std::cout << "Client requested to close the connection\n";
//         fdsTracker[clientFd].WError = 1; // Set write error status
//         fdsTracker[clientFd].RError = 1; // Set read error status
//         removeClient(clientFd);
//         return 0;
//     }
    
//     // Keep the connection alive by default
//     return 1;
// }
//     void monitorClient::writeClientResponse(int clientFd)
//     {
//         std::string response = 
//                                 "HTTP/1.1 200 OK\r\n"
//                                 "Content-Type: text/html; charset=utf-8\r\n"
//                                 "Connection:  keep-alive\r\n"
//                                 "Content-Length: 94\r\n"
//                                 "\r\n"
//                                 "<!DOCTYPE html>\n"
//                                 "<html>\n"
//                                 "<head><title>Webserv</title></head>\n"
//                                 "<body><h1>Hello, Browser!</h1></body>\n"
//                                 "</html>";
//         std::cout << "sending response to clinet \n";
//         fdsTracker[clientFd].response = response;
//         int wByte = 0;
//         while ((wByte = write(clientFd,  fdsTracker[clientFd].response.c_str(), fdsTracker[clientFd].response.size())) > 0)
//             fdsTracker[clientFd].response.erase(0, wByte);
//         if (wByte == -1)
//             std::cerr << "[ERROR] can't send the request write error \n";
                  return 1;
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
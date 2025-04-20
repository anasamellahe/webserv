#include "../includes/monitorClient.hpp"

    // int socketAddres;
    // std::vector<pollfd> fds;

    monitorClient::monitorClient(int socketAddres) : socketAddres(socketAddres)
    {
        pollfd serverPollFd;
        memset(&serverPollFd, 0, sizeof(serverPollFd));
        std::cout << "socket add == [ " <<  this->socketAddres << " ]\n";
        serverPollFd.fd = this->socketAddres;
        serverPollFd.events = POLLIN;
        fds.push_back(serverPollFd);
    }
    void monitorClient::acceptNewClient()
    {
        int clFd = accept(this->socketAddres, NULL, NULL);
        if (clFd == -1)
            throw monitorexception("[ERROR] accept fail \n");
        if (fcntl(clFd, F_SETFL,  O_NONBLOCK) < 0)
            throw monitorexception("[ERROR]: fcntl fail");
        pollfd serverPollFd;
        memset(&serverPollFd, 0, sizeof(serverPollFd));
        serverPollFd.fd = clFd;
        serverPollFd.events = POLLIN;
        fds.push_back(serverPollFd);
        SocketTracker st;
        this->fdsTracker.insert(std::pair<int, SocketTracker>(clFd, st));
        std::cout << "new client accepted successfully \n";
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
        int readByte;
        std::string request;
        char buff[100];
        std::cout << "start reading \n";
        while ((readByte = read(clientFd, buff, 100)) > 0)
            request.append(buff, readByte);
        if (readByte == 0)
            return (std::cerr << strerror(errno) << readByte << "\n", 0);
        fdsTracker[clientFd].request = request;
        return 1;
    }
    void monitorClient::writeClientResponse(int clientFd)
    {
        std::string response = 
                                "HTTP/1.1 200 OK\r\n"
                                "Content-Type: text/html; charset=utf-8\r\n"
                                "Connection:  keep-alive\r\n"
                                "Content-Length: 94\r\n"
                                "\r\n"
                                "<!DOCTYPE html>\n"
                                "<html>\n"
                                "<head><title>Webserv</title></head>\n"
                                "<body><h1>Hello, Browser!</h1></body>\n"
                                "</html>";
        std::cout << "sending response to clinet \n";
        fdsTracker[clientFd].response = response;
        int wByte = 0;
        while ((wByte = write(clientFd,  fdsTracker[clientFd].response.c_str(), fdsTracker[clientFd].response.size())) > 0)
            fdsTracker[clientFd].response.erase(0, wByte);
        if (wByte == -1)
            std::cerr << "[ERROR] can't send the request write error \n";
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
            std::cout << "number of client who are ready is [ "  << ready << " ]\n";
            if (fds[0].revents & POLLIN)
                acceptNewClient();
            std::cout << "number of fds is [ " << fds.size() << " ]" << std::endl;
            for (size_t i = 1; i < fds.size(); i++)
            {
                int keepAlive = 1;
                if (fds[i].revents & POLLIN)
                    keepAlive = readClientRequest(fds[i].fd);
                if (fds[i].revents & POLLOUT )
                    writeClientResponse(fds[i].fd);
                if (keepAlive == 0 )
                    removeClient(i);
                ready--;
                if (ready == 0)
                    break;
            }
        }
    }
    

    monitorClient::~monitorClient()
    {
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


    const SocketTracker* monitorClient::getSocketTracker(int socketFd) const
    {
        CMiterator it = this->fdsTracker.find(socketFd);
        if (it != this->fdsTracker.end())
            return &it->second;
        return NULL;
    }
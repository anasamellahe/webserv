#include "socket.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

sock::sockException::sockException(std::string msg) {
    ErrorMsg = msg;
}

const char* sock::sockException::what() const throw() {
    return ErrorMsg.c_str();
}

sock::sockException::~sockException() throw() {}

sock::sock(ConfigParser config_parser) : config_parser(config_parser) {
    hosts = config_parser.getServerListenAddresses();
    int fd, op;
    for (size_t i = 0; i < hosts.size(); i++) {
        fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (fd < 0)
            closeFDs("[ERROR]: fail to create socket ");
        op = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)) < 0)
            closeFDs("[ERROR] setsockopt fail");
        sockFDs.push_back(fd);
    }
    std::cout << "Server: socket created successfully \n";
    bindINET();
}

void sock::bindINET() {
    sockaddr_in bindSocket;
    for (size_t i = 0; i < hosts.size(); i++) {
        bindSocket.sin_family = AF_INET;
        bindSocket.sin_port = htons(hosts[i].second);
        if (!inet_pton(AF_INET, hosts[i].first.c_str(), &bindSocket.sin_addr.s_addr))
            closeFDs("[ERROR] inet_pton fail");
        if (bind(sockFDs[i], (sockaddr *)&bindSocket, sizeof(bindSocket)) < 0)
            closeFDs("[ERROR] fail to bound the socket");
        if (listen(sockFDs[i], 100) == -1)
            closeFDs("[ERROR] fail to listen in  the server");
        std::cout << "Server: Socket bound successfully and start listening on port " << hosts[i].second << std::endl;
    }
}

std::vector<int> sock::getFDs() const {
    return this->sockFDs;
}
ConfigParser sock::getConfig()
{
    return this->config_parser;
}
void sock::closeFDs(const char *msg) {
    for (size_t i = 0; i < this->sockFDs.size(); i++)
        close(sockFDs[i]);
    throw sockException(msg);
}

sock::~sock() {}


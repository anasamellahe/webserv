#include "includes/sock.hpp"

sock::sockException::sockException(std::string msg)
{
    ErrorMsg = msg;
}
const char * sock::sockException::what() const throw()
{
    return ErrorMsg.c_str();
}
sock::sockException::~sockException()  throw()
{}


sock::sock(testSocketData data)
{
    int fd, op;
    for (size_t i = 0; i < data.ports.size(); i++)
    {
        fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (fd < 0)
            closeFDs("[ERROR]: fail to create socket ");
        op = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)) < 0)
            closeFDs("[ERROR] setsockopt fail");
        sockFDs.push_back(fd);
    }
    std::cout  << "Server: socket created successfully \n";
    bindINET(data);
}

void sock::bindINET(testSocketData data)
{
    sockaddr_in bindSocket;
    for (size_t i = 0; i < data.ports.size(); i++)
    {
        bindSocket.sin_family = AF_INET;
        bindSocket.sin_port = htons(data.ports[i]);
        bindSocket.sin_addr.s_addr = INADDR_ANY;

        if (bind(sockFDs[i], (sockaddr *)&bindSocket, sizeof(bindSocket)) < 0)
            closeFDs("[ERROR] fail to bound the socket");
        if (listen(sockFDs[i], 100) == -1)
            closeFDs("[ERROR] fail to listen in  the server");
        std::cout << "Server: Socket bound successfully and start listening on port " << data.ports[i] << std::endl;
    }
}

std::vector<int> sock::getFDs() const
{
    return this->sockFDs;
}

void sock::closeFDs(const char *msg)
{
    for (size_t i = 0; i < this->sockFDs.size(); i++)
        close(sockFDs[i]);
    throw sockException(msg);
}

sock::~sock()
{}


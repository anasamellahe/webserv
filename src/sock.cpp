#include "../includes/sock.hpp"

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
sock::sock(int domain , int type, int protocol)
{
    this->sockFD = socket(domain, type, protocol);
    int op = 1;
    if (setsockopt(sockFD, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)) < 0)
        throw sockException("[ERROR] setsockopt fail");
    if (this->sockFD < 0)
        throw sockException("[ERROR] Socket creation failed: ");
    std::cout  << "Server: socket created successfully \n";
}
int sock::getFD()const
{
    return this->sockFD;
}

void sock::bindINET(int sin_family , int port, int address)
{
    sockaddr_in bindSocket;
    bindSocket.sin_family = sin_family;
    bindSocket.sin_port = htons(port);
    bindSocket.sin_addr.s_addr = address;

    if (bind(this->sockFD, (sockaddr *)&bindSocket, sizeof(bindSocket)) < 0)
        throw sockException("[ERROR] fail to bound the socket");
    if (listen(this->sockFD, 100) == -1)
        throw sockException("[ERROR] fail to listen in  the server");
    std::cout << "Server: Socket bound successfully and start listening on port " << port << std::endl;
}
sock::~sock()
{
    std::cout << "Server: socket closed successfully \n";
    close(this->sockFD);
}

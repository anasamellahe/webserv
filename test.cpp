#include <iostream>
#include <string>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <exception>
#include <poll.h>
#include <vector>

int main ()
{
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sockfd == -1)
        return 120;
    sockaddr_in sd;
    sd.sin_addr.s_addr = INADDR_ANY;
    sd.sin_family = AF_INET;
    sd.sin_port = htons(8080);


    std::string response = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=utf-8\r\n"
    "Connection:  keep-alive\r\n"
    "Content-Length: 310\r\n"
    "\r\n"
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head><title>Webserv</title></head>\n"
    "<body><h1>Hello, Browser!</h1></body>\n"
    "</html>";
    std::cout << response.size() << std::endl;
    if (bind(sockfd, (sockaddr*)&sd, sizeof(sd)) < 0)
        return 122;
    if (listen(sockfd, 10)  < 0)
        return 123;
    int fd = 0;


        if (fd == 0)
            fd = accept(sockfd, NULL, NULL);
     
        int a;
        a = write(fd, response.c_str(), response.size());
        std::cout << "hello anas amellahe\n";
        sleep(20);
        std::cout << "hello anas amellahe2\n";
        a = write(fd, response.c_str(), response.size());
        std::cout << "a is  " << a << std::endl;
}
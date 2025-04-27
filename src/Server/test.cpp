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
#include <map>
#include <utility>

void binds(int f, int f2)
{
    sockaddr_in s;
    s.sin_family  = AF_INET;
    s.sin_port = htons(8080);
    s.sin_addr.s_addr = INADDR_ANY;

    if (bind(f, (sockaddr *)&s, sizeof(s)) < 0)
        std::cout << "error bind\n";

    s.sin_family  = AF_INET;
    s.sin_port = htons(5050);
    s.sin_addr.s_addr = INADDR_ANY;

    if (bind(f2, (sockaddr *)&s, sizeof(s)) < 0)
        std::cout << "error bind\n";
    if (listen(f, 5) < 0)
        std::cout << "error listen fail\n";
    if (listen(f2, 5) < 0)
        std::cout << "error listen fail\n";
}

// int main ()
// {
//    int f, f1;

//    if ((  f = socket(AF_INET, SOCK_STREAM, 0)) < 0)
//     std::cout << "error create socket fail\n";
//    if (( f1 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
//     std::cout << "error create socket fail\n";
//     int op = 1;
//     setsockopt(f, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op));
//     setsockopt(f1, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op));
//     binds( f,  f1);
//     while (1)
//     {
//         int a , b;
//         a = 0;
//         b = 0;
//         a = accept(f, NULL, NULL);
//         if (a)
//             std::cout << "f accept connection\n";
//         b = accept(f1, NULL, NULL);
//         if (b)
//             std::cout << "f1 accept connection\n";

//     }
// }



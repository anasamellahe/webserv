#include "monitorClient.hpp"
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <iostream>
#include <fcntl.h>

#define MAX_HEADER_SIZE 8192

int monitorClient::readChunkFromClient(int clientFd, std::string& buffer) {
    char chunk[CHUNK_SIZE];
    ssize_t bytesRead = read(clientFd, chunk, CHUNK_SIZE);

    if (bytesRead > 0) {
        buffer.append(chunk, bytesRead);
        return bytesRead;
    } else if (bytesRead == 0) {
        return 0;
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return -1;
    } else {
        return -2;
    }
}

int monitorClient::readClientRequest(int clientFd) {
    TrackerIt it = fdsTracker.find(clientFd);
    if (it == fdsTracker.end()) {
        std::cerr << "Client " << clientFd << " not found in tracker" << std::endl;
        return -1;
    }

    SocketTracker& tracker = it->second;
    if (tracker.request_obj.getClientFD() != clientFd) {
        tracker.request_obj.setClientFD(clientFd);
    }
 
    std::string buffer;
    int readResult = readChunkFromClient(clientFd, buffer);
    
    if (readResult <= 0) {
        if (readResult == 0) {
            std::cout << "Client " << clientFd << " closed connection" << std::endl;
            return 0;
        }
        if (readResult == -2) {
            std::cerr << "Error reading from client " << clientFd << std::endl;
            return -1;
        }
        return 1;
    }

    tracker.raw_buffer += buffer;
    
    if (tracker.raw_buffer.size() > MAX_HEADER_SIZE && 
        tracker.raw_buffer.find("\r\n\r\n") == std::string::npos) {
        tracker.error = "431 Request Header Fields Too Large";
        return 0;
    }

    bool validity = tracker.request_obj.parseFromSocket(clientFd, tracker.raw_buffer, tracker.raw_buffer.size());

    if (validity && tracker.request_obj.isValid()) {
        tracker.updateActivity();
        tracker.raw_buffer.clear();
        return 1;
    } else if (!tracker.request_obj.getErrorCode().empty() && 
               tracker.request_obj.getErrorCode() != "200 OK") {
        tracker.error = tracker.request_obj.getErrorCode();
        return 0;
    }
    
    return 1;
}

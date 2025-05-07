#include "CGIHandler.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <fcntl.h>

CGIHandler::CGIHandler(const CGIConfig& config) : config(config) {}

CGIHandler::~CGIHandler() {}

bool CGIHandler::isCGIRequest(const std::string& path) const {
    // Check if the file extension is in the list of allowed CGI extensions
}

bool CGIHandler::handleRequest(const std::string& path, 
                              const std::string& method,
                              const std::map<std::string, std::string>& headers,
                              const std::string& body,
                              std::string& response) {
    // Set up the environment
    // Extract query string if present
    // Prepare CGI environment
    // Execute the script
}

bool CGIHandler::executeScript(const std::string& scriptPath, const CGIEnvironment& env) {
    // Create pipes for communication with the CGI process

}

void CGIHandler::setRequestBody(const std::string& body) {
    requestBody = body;
}

void CGIHandler::setRequestMethod(const std::string& method) {
    requestMethod = method;
}

void CGIHandler::setQueryString(const std::string& query) {
    queryString = query;
}

void CGIHandler::setRequestHeaders(const std::map<std::string, std::string>& headers) {
    requestHeaders = headers;
}
#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <string>
#include <vector>
#include <map>

class CGIHandler {
private:
    CGIConfig config;
    std::string scriptPath;
    std::string requestBody;
    std::map<std::string, std::string> requestHeaders;
    std::string requestMethod;
    std::string queryString;
    
    // Internal methods
    bool isCGIRequest(const std::string& path) const;
    bool executeScript(const std::string& scriptPath, const CGIEnvironment& env);
    std::string formatCGIResponse(const std::string& rawOutput) const;

public:
    CGIHandler(const CGIConfig& config);
    ~CGIHandler();
    
    // Main interface methods
    bool handleRequest(const std::string& path, 
                       const std::string& method,
                       const std::map<std::string, std::string>& headers,
                       const std::string& body,
                       std::string& response);
                       
    // Setters
    void setRequestBody(const std::string& body);
    void setRequestMethod(const std::string& method);
    void setQueryString(const std::string& query);
    void setRequestHeaders(const std::map<std::string, std::string>& headers);
};

#endif // CGI_HANDLER_HPP
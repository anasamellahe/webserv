#pragma once

#include <string>
#include <map>
#include <vector>
#include "../HTTP/Request.hpp"
#include "../Config/ConfigParser.hpp"

/**
 * Minimal CGI executor bound to a Request and a matched Route.
 * Responsibilities:
 * - Build CGI environment
 * - Fork and exec interpreter or direct script
 * - Write request body to child stdin (POST)
 * - Read stdout from child and parse CGI headers/body
 * - Enforce a simple timeout
 */
class CGIHandler {
public:
    struct Result {
        int status_code;
        std::string status_text;
        std::map<std::string, std::string> headers;
        std::string body;
        bool ok;
        Result(): status_code(500), status_text("Internal Server Error"), ok(false) {}
    };

    CGIHandler(const Request &req,
               const Config::ServerConfig &srv);
    ~CGIHandler();

    // Synchronous execution (legacy - still available for backwards compatibility)
    Result run(const std::string &resolvedScriptPath,
               const std::string &interpreterPath);

    // Asynchronous execution - new API
    bool startCGI(const std::string &resolvedScriptPath,
                  const std::string &interpreterPath);
    
    // Process available output from CGI (non-blocking)
    // Returns: 1 = still running, 0 = completed, -1 = error/timeout
    int processCGIOutput();
    
    // Get the result after CGI completes
    Result getResult() const;
    
    // Get CGI output file descriptor for poll()
    int getCGIOutputFd() const { return cgiOutputFd; }
    
    // Get CGI process ID
    pid_t getCGIPid() const { return cgiPid; }
    
    // Check if CGI has timed out
    bool hasTimedOut() const;
    
    // Kill CGI process and clean up
    void killCGI();

private:
    const Request &request;
    const Config::ServerConfig &server;
    
    // Async CGI state
    pid_t cgiPid;
    int cgiOutputFd;
    int cgiInputFd;
    std::string cgiBuffer;
    time_t startTime;
    std::string resolvedScript;
    std::string interpreter;
    bool cgiStarted;
    static const int CGI_TIMEOUT = 5; // 5 seconds
    
    Result asyncResult;

    std::vector<std::string> buildEnv(const std::string &scriptPath) const;
    std::vector<char*> makeEnvp(const std::vector<std::string> &env) const;
    std::vector<char*> makeArgv(const std::string &interpreter,
                                const std::string &script) const;
    void freeCStringArray(std::vector<char*> &arr) const;
    void parseCgiOutput(const std::string &raw,
                        Result &out) const;
};

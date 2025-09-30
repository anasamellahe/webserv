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

    // Execute CGI script located at resolvedScriptPath; interpreterPath optional (cgi_pass)
    Result run(const std::string &resolvedScriptPath,
               const std::string &interpreterPath);

private:
    const Request &request;
    const Config::ServerConfig &server;
    // const Config::RouteConfig &route;

    std::vector<std::string> buildEnv(const std::string &scriptPath) const;
    std::vector<char*> makeEnvp(const std::vector<std::string> &env) const;
    std::vector<char*> makeArgv(const std::string &interpreter,
                                const std::string &script) const;
    void freeCStringArray(std::vector<char*> &arr) const;
    void parseCgiOutput(const std::string &raw,
                        Result &out) const;
};

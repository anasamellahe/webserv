#include "CGIHandler.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sstream>
#include <cstring>

static std::string itos_long(long v) {
    std::ostringstream ss; ss << v; return ss.str();
}

CGIHandler::CGIHandler(const Request &req,
                       const Config::ServerConfig &srv)
    : request(req), server(srv) {}

CGIHandler::~CGIHandler() {}

std::vector<std::string> CGIHandler::buildEnv(const std::string &scriptPath) const {
    std::vector<std::string> env;
    // Core CGI variables
    env.push_back(std::string("GATEWAY_INTERFACE=CGI/1.1"));
    env.push_back(std::string("SERVER_PROTOCOL=HTTP/1.1"));
    env.push_back(std::string("REQUEST_METHOD=") + request.getMethod());
    env.push_back(std::string("SCRIPT_FILENAME=") + scriptPath);
    env.push_back(std::string("SCRIPT_NAME=") + request.getPath());
    
    // Safely extract QUERY_STRING from CGI environment
    const std::map<std::string, std::string> &cgiEnv = request.getCGIEnv();
    std::map<std::string, std::string>::const_iterator qsIt = cgiEnv.find("QUERY_STRING");
    std::string queryString = (qsIt != cgiEnv.end()) ? qsIt->second : std::string();
    env.push_back(std::string("QUERY_STRING=") + queryString);

    // Content headers
    const std::string &ct = request.getHeader("content-type");
    if (ct != "content-type" && !ct.empty()) env.push_back(std::string("CONTENT_TYPE=") + ct);
    const std::string &cl = request.getHeader("content-length");
    if (cl != "content-length" && !cl.empty()) env.push_back(std::string("CONTENT_LENGTH=") + cl);

    // Host/port
    env.push_back(std::string("SERVER_NAME=") + (server.server_names.empty() ? server.host : server.server_names[0]));
    int port = server.ports.empty() ? 80 : server.ports[0];
    env.push_back(std::string("SERVER_PORT=") + itos_long(port));

    // HTTP_ headers (uppercase, hyphens to underscores)
    const std::map<std::string, std::string> &hdrs = request.getAllHeaders();
    for (std::map<std::string, std::string>::const_iterator it = hdrs.begin(); it != hdrs.end(); ++it) {
        std::string key = it->first;
        std::string val = it->second;
        if (key.empty() || val.empty()) continue;
        // Skip content-type/length, already added
        if (key == "content-type" || key == "content-length") {
            continue;
        }
        for (size_t i = 0; i < key.size(); ++i) {
            char &c = key[i];
            if (c == '-') c = '_';
            else c = (char)std::toupper(c);
        }
        env.push_back(std::string("HTTP_") + key + "=" + val);
    }

    return env;
}

std::vector<char*> CGIHandler::makeEnvp(const std::vector<std::string> &env) const {
    std::vector<char*> out;
    for (size_t i = 0; i < env.size(); ++i) out.push_back(const_cast<char*>(env[i].c_str()));
    out.push_back(NULL);
    return out;
}

std::vector<char*> CGIHandler::makeArgv(const std::string &interpreter,
                                         const std::string &script) const {
    std::vector<char*> argv;
    if (!interpreter.empty()) argv.push_back(const_cast<char*>(interpreter.c_str()));
    argv.push_back(const_cast<char*>(script.c_str()));
    argv.push_back(NULL);
    return argv;
}

void CGIHandler::freeCStringArray(std::vector<char*> &arr) const {
    (void)arr; // no-op since we point to existing strings
}

void CGIHandler::parseCgiOutput(const std::string &raw, Result &out) const {
    // CGI output starts with headers terminated by CRLFCRLF or \n\n
    std::string::size_type pos = raw.find("\r\n\r\n");
    std::string head, body;
    
    if (pos == std::string::npos) {
        // Try \n\n separator
        std::string::size_type pos2 = raw.find("\n\n");
        if (pos2 == std::string::npos) {
            // No proper CGI header separator found
            // Check if output starts with HTML or other non-header content
            // If so, treat entire output as body with default Content-Type
            if (raw.find("<html>") == 0 || raw.find("<!DOCTYPE") == 0 || raw.find("<?xml") == 0) {
                // Looks like HTML/XML without headers - use entire output as body
                out.status_code = 200;
                out.status_text = "OK";
                out.headers["Content-Type"] = "text/html; charset=utf-8";
                out.body = raw;
                out.ok = true;
                return;
            }
            // Otherwise, parsing failed
            out.ok = false;
            return;
        }
        head = raw.substr(0, pos2);
        body = raw.substr(pos2 + 2);
    } else {
        head = raw.substr(0, pos);
        body = raw.substr(pos + 4);
    }

    // Parse headers
    std::istringstream hs(head);
    std::string line;
    std::string status;
    while (std::getline(hs, line)) {
        if (!line.empty() && line[line.size()-1] == '\r') line.erase(line.size()-1);
        if (line.empty()) continue;
        std::string::size_type c = line.find(':');
        if (c == std::string::npos) continue;
        std::string k = line.substr(0, c);
        std::string v = line.substr(c+1);
        // trim
        while (!v.empty() && (v[0] == ' ' || v[0] == '\t')) v.erase(0,1);
        out.headers[k] = v;
        if (k == "Status") status = v; // e.g., "200 OK"
    }
    if (!status.empty()) {
        // parse first token as code, rest as text
        std::istringstream ss(status);
        int code = 200; std::string text;
        ss >> code;
        std::getline(ss, text);
        if (!text.empty() && text[0] == ' ') text.erase(0,1);
        out.status_code = code;
        out.status_text = text.empty() ? "OK" : text;
    } else {
        out.status_code = 200; out.status_text = "OK";
    }
    
    // Ensure Content-Type is set
    if (out.headers.find("Content-Type") == out.headers.end()) {
        out.headers["Content-Type"] = "text/html; charset=utf-8";
    }
    
    out.body = body;
    out.ok = true;
}

CGIHandler::Result CGIHandler::run(const std::string &resolvedScriptPath,
                                   const std::string &interpreterPath) {
    Result result;
    
    // Determine effective interpreter path with fallback defaults
    std::string effectiveInterpreter = interpreterPath;
    if (effectiveInterpreter.empty()) {
        // Extract extension from script path
        std::string ext;
        size_t dot = resolvedScriptPath.find_last_of('.');
        if (dot != std::string::npos) {
            ext = resolvedScriptPath.substr(dot);
        }
        
        // Provide default interpreter paths for common CGI extensions
        if (ext == ".php") {
            effectiveInterpreter = "/usr/bin/php";
        } else if (ext == ".py") {
            effectiveInterpreter = "/usr/bin/python3";
        } else if (ext == ".pl") {
            effectiveInterpreter = "/usr/bin/perl";
        }
    }
    
    int inpipe[2]; // parent writes body to child stdin
    int outpipe[2]; // child stdout to parent
    if (pipe(inpipe) == -1) return result;
    if (pipe(outpipe) == -1) { close(inpipe[0]); close(inpipe[1]); return result; }

    pid_t pid = fork();
    if (pid == -1) {
        close(inpipe[0]); close(inpipe[1]); close(outpipe[0]); close(outpipe[1]);
        return result;
    }

    if (pid == 0) {
        // Child
        dup2(inpipe[0], STDIN_FILENO);
        dup2(outpipe[1], STDOUT_FILENO);
        dup2(outpipe[1], STDERR_FILENO); // Redirect stderr to stdout for debugging
        // close unused
        close(inpipe[1]); close(outpipe[0]);

        // Build env and argv
        std::vector<std::string> envv = buildEnv(resolvedScriptPath);
        std::vector<char*> envp = makeEnvp(envv);
        std::vector<char*> argv = makeArgv(effectiveInterpreter, resolvedScriptPath);

        // Exec
        const char *execPath = effectiveInterpreter.empty() ? resolvedScriptPath.c_str() : effectiveInterpreter.c_str();
        execve(execPath, &argv[0], &envp[0]);
        // If execve fails
        _exit(127);
    }

    // Parent
    close(inpipe[0]);
    close(outpipe[1]);

    // Write request body if present
    const std::string &body = request.getBody();
    if (!body.empty()) {
        ssize_t off = 0;
        while (off < (ssize_t)body.size()) {
            ssize_t w = write(inpipe[1], body.c_str() + off, body.size() - off);
            if (w == -1) {
                if (errno == EINTR) continue;
                break;
            }
            off += w;
        }
    }
    close(inpipe[1]);

    // Read with a simple timeout using poll
    std::string raw;
    struct pollfd pfd;
    pfd.fd = outpipe[0];
    pfd.events = POLLIN;
    const int timeout_ms = 5000; // 5s
    char buf[4096];
    while (true) {
        int pr = poll(&pfd, 1, timeout_ms);
        if (pr == -1) {
            if (errno == EINTR) continue;
            break;
        }
        if (pr == 0) {
            // timeout -> kill child
            kill(pid, SIGKILL);
            break;
        }
        if (pfd.revents & POLLIN) {
            ssize_t r = read(outpipe[0], buf, sizeof(buf));
            if (r > 0) raw.append(buf, r);
            else if (r == 0) break; // EOF
            else if (errno != EAGAIN && errno != EWOULDBLOCK) break;
        } else {
            break;
        }
    }
    close(outpipe[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    // Check if child exited abnormally
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        // Child exited with non-zero status, likely execution failed
        result.status_code = 500;
        result.status_text = "Internal Server Error";
        result.body = "<html><body><h1>500 Internal Server Error</h1><p>CGI execution failed.</p></body></html>";
        result.headers.clear();
        result.headers["Content-Type"] = "text/html; charset=utf-8";
        result.ok = false;
        return result;
    }

    if (!raw.empty()) {
        parseCgiOutput(raw, result);
    }
    if (!result.ok) {
        result.status_code = 500;
        result.status_text = "Internal Server Error";
        result.body = "<html><body><h1>500 Internal Server Error</h1><p>CGI parsing failed.</p></body></html>";
        result.headers.clear();
        result.headers["Content-Type"] = "text/html; charset=utf-8";
        result.ok = false;
    }
    return result;
}

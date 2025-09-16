#include "ResponsePost.hpp"
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

ResponsePost::ResponsePost(Request& request)
    : ResponseBase(request)
{
}

ResponsePost::~ResponsePost()
{
}

void ResponsePost::handle(){
    // Find best matching route (longest-prefix)
    const Config::RouteConfig *matched = NULL;
    const std::vector<Config::RouteConfig> &routes = request.serverConfig.routes;
    size_t best_len = 0;
    for (std::vector<Config::RouteConfig>::const_iterator it = routes.begin(); it != routes.end(); ++it){
        const std::string &rpath = it->path;
        if (rpath.empty()) continue;
        if (request.path.compare(0, rpath.size(), rpath) == 0){
            if (rpath.size() > best_len){
                best_len = rpath.size();
                matched = &(*it);
            }
        }
    }

    // Method check for POST
    if (matched){
        const std::vector<std::string> &allowed = matched->accepted_methods;
        if (!allowed.empty()){
            bool ok = false;
            for (std::vector<std::string>::const_iterator mit = allowed.begin(); mit != allowed.end(); ++mit){
                if (*mit == "POST") { ok = true; break; }
            }
            if (!ok){
                setStatus(405, "Method Not Allowed");
                addHeader("Allow", "");
                body = buildDefaultBodyError(405);
                return;
            }
        }
    }

    // Resolve root + suffix
    std::string root = request.serverConfig.root;
    if (matched && !matched->root.empty()) root = matched->root;
    std::string suffix;
    if (matched && !matched->path.empty() && request.path.compare(0, matched->path.size(), matched->path) == 0)
        suffix = request.path.substr(matched->path.size());
    else
        suffix = request.path;
    std::string fsPath = root;
    if (!fsPath.empty() && fsPath.back() == '/') fsPath.pop_back();
    if (!suffix.empty() && suffix.front() != '/') fsPath += "/" + suffix; else fsPath += suffix;

    // Canonicalize and containment check
    char resolved_root[PATH_MAX];
    char resolved_target[PATH_MAX];
    if (realpath(root.c_str(), resolved_root) == NULL){ setStatus(404, "Not Found"); body = buildDefaultBodyError(404); return; }
    if (realpath(fsPath.c_str(), resolved_target) == NULL){ setStatus(404, "Not Found"); body = buildDefaultBodyError(404); return; }
    std::string rr(resolved_root), rt(resolved_target);
    if (rt.compare(0, rr.size(), rr) != 0){ setStatus(404, "Not Found"); body = buildDefaultBodyError(404); return; }

    // For simplicity: write request.body into the target file (append mode)
    std::ofstream ofs(fsPath.c_str(), std::ios::out | std::ios::binary | std::ios::app);
    if (!ofs){ setStatus(500, "Internal Server Error"); body = buildDefaultBodyError(500); return; }
    ofs << request.body;
    ofs.close();

    setStatus(201, "Created");
    body = std::string("Resource created or appended.");
    addHeader("Content-Type", "text/plain; charset=utf-8");
}

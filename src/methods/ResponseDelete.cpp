#include "ResponseDelete.hpp"
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

ResponseDelete::ResponseDelete(Request& request)
    : ResponseBase(request)
{
}

ResponseDelete::~ResponseDelete()
{
}

void ResponseDelete::handle(){
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

    // Method check for DELETE (empty accepted_methods means disallow all)
    if (matched){
        const std::vector<std::string> &allowed = matched->accepted_methods;
        bool ok = false;
        if (!allowed.empty()){
            for (std::vector<std::string>::const_iterator mit = allowed.begin(); mit != allowed.end(); ++mit){
                if (*mit == "DELETE") { ok = true; break; }
            }
        } else {
            ok = false; // No methods defined -> none allowed
        }
        if (!ok){
            setStatus(405, "Method Not Allowed");
            addHeader("Allow", "");
            body = buildDefaultBodyError(405);
            return;
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
    // C++98-compatible trimming of trailing slash
    if (!fsPath.empty() && fsPath[fsPath.size() - 1] == '/')
        fsPath.erase(fsPath.size() - 1);
    // Ensure single slash between root and suffix
    if (!suffix.empty() && suffix[0] != '/') fsPath += "/" + suffix; else fsPath += suffix;

    // Canonicalize and containment check
    char resolved_root[PATH_MAX];
    char resolved_target[PATH_MAX];
    if (realpath(root.c_str(), resolved_root) == NULL){ setStatus(404, "Not Found"); body = buildDefaultBodyError(404); return; }
    if (realpath(fsPath.c_str(), resolved_target) == NULL){ setStatus(404, "Not Found"); body = buildDefaultBodyError(404); return; }
    std::string rr(resolved_root), rt(resolved_target);
    if (rt.compare(0, rr.size(), rr) != 0){ setStatus(404, "Not Found"); body = buildDefaultBodyError(404); return; }

    // Attempt to delete file
    if (unlink(resolved_target) != 0){
        setStatus(404, "Not Found");
        body = buildDefaultBodyError(404);
        return;
    }

    setStatus(200, "OK");
    body = std::string("Resource deleted.");
    addHeader("Content-Type", "text/plain; charset=utf-8");
}

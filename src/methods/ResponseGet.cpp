#include "ResponseGet.hpp"
#include "../CGI/CGIHandler.hpp"
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

ResponseGet::ResponseGet(Request& request)
    : ResponseBase(request)
{
}

ResponseGet::~ResponseGet()
{
}


void ResponseGet::handle(){
    // Find the best matching route using longest-prefix matching (nginx-style)
    
    const Config::RouteConfig *matched = NULL;
    const std::vector<Config::RouteConfig> &routes = request.serverConfig.routes;
    size_t best_len = 0;
    for (std::vector<Config::RouteConfig>::const_iterator it = routes.begin(); it != routes.end(); ++it){
        const std::string &rpath = it->path;
        if (rpath.empty()) continue;
        // ensure rpath is a prefix of request.path
        if (request.path.compare(0, rpath.size(), rpath) == 0){
            // prefer longer (more specific) prefix
            if (rpath.size() > best_len){
                best_len = rpath.size();
                matched = &(*it);
                break;
            }
        }
    }

    // {
    //     std::cout << "PATH is  -- >" << matched->path << "root is -- >" << matched->root << std::endl;
        
    //     const std::vector<std::string> &allowed = matched->accepted_methods;
    //      for (std::vector<std::string>::const_iterator mit = allowed.begin(); mit != allowed.end(); ++mit){
    //            std::cout << "method is  == " << *mit << std::endl;
    //         }
    // }

    // If route matched, ensure GET is allowed
    if (matched){
        const std::vector<std::string> &allowed = matched->accepted_methods;
        if (!allowed.empty()){
            bool okmethod = false;
            for (std::vector<std::string>::const_iterator mit = allowed.begin(); mit != allowed.end(); ++mit){
                if (*mit == "GET") { okmethod = true; break; }
            }
            if (!okmethod){
                std::string stxt = "Method Not Allowed";
                setStatus(405, stxt);
                body = buildDefaultBodyError(405);
                return;
            }
        }
    }

    // Resolve root and produce filesystem path compatible with prefix routes
    std::string root = request.serverConfig.root;
    if (matched && !matched->root.empty()) root = matched->root;

    // Compute the path suffix after the matched route prefix
    std::string suffix;
    if (matched && !matched->path.empty() && request.path.compare(0, matched->path.size(), matched->path) == 0)
        suffix = request.path.substr(matched->path.size());
    else
        suffix = request.path;

    // If the request refers to the route root (empty suffix) and the route defines
    // an index list like "index.html index.htm", use the first index as the suffix
    if (suffix.empty() && matched && !matched->index.empty()){
        // tokenize by whitespace and pick the first non-empty token
        std::istringstream iss(matched->index);
        std::string first;
        if (iss >> first){
            if (!first.empty()){
                if (first.front() != '/') suffix = std::string("/") + first;
                else suffix = first;
            }
        }
    }

    // Normalize root (remove trailing slash) and ensure suffix begins with '/'
    std::string fsPath = root;
    if (!fsPath.empty() && fsPath.back() == '/') fsPath.pop_back();
    if (!suffix.empty() && suffix.front() != '/') fsPath += "/" + suffix; else fsPath += suffix;

    // Canonicalize fsPath and ensure it stays inside the root to prevent traversal
    char resolved_root[PATH_MAX];
    char resolved_target[PATH_MAX];
    if (realpath(root.c_str(), resolved_root) == NULL){
        std::string stxt = "Not Found";
        setStatus(404, stxt);
        body = buildDefaultBodyError(404);
        return;
    }
    if (realpath(fsPath.c_str(), resolved_target) == NULL){
        // target does not exist
        std::string stxt = "Not Found";
        setStatus(404, stxt);
        body = buildDefaultBodyError(404);
        return;
    }

    // Ensure resolved_target is inside resolved_root
    std::string rr(resolved_root);
    std::string rt(resolved_target);
    if (rt.compare(0, rr.size(), rr) != 0){
        std::string stxt = "Not Found";
        setStatus(404, stxt);
        body = buildDefaultBodyError(404);
        return;
    }

    struct stat st;
    if (stat(resolved_target, &st) != 0){
        std::string stxt = "Not Found";
        setStatus(404, stxt);
        body = buildDefaultBodyError(404);
        return;
    }

    if ((st.st_mode & S_IFMT) == S_IFDIR){
        // If request path does not end with '/', redirect to path with trailing slash
        if (request.path.empty() || request.path.back() != '/'){
            std::string redirect_to = request.path;
            if (redirect_to.empty() || redirect_to[0] != '/') redirect_to = "/" + redirect_to;
            redirect_to += "/";
            std::string stxt = "Moved Permanently";
            setStatus(301, stxt);
            addHeader("Location", redirect_to);
            body = buildDefaultBodyError(301);
            return;
        }
        // Try index from matched route, then default index.html
        std::string indexFile;
        if (matched && !matched->index.empty()) indexFile = matched->index;
        else indexFile = "index.html";

        std::string indexPath = fsPath;
        if (indexPath.size() == 0 || indexPath[indexPath.size()-1] != '/') indexPath += "/";
        indexPath += indexFile;

        if (stat(indexPath.c_str(), &st) == 0 && ((st.st_mode & S_IFMT) != S_IFDIR)){
            std::string content = ReadFromFile(indexPath);
            if (!content.empty()){
                body = content;
                addHeader(std::string("Content-Type"), contentTypeFromPath(indexPath));
                std::string ok = "OK";
                setStatus(200, ok);
                return;
            }
        }

        // If directory listing allowed in route, generate simple listing
        if (matched && matched->directory_listing){
            DIR *dir = opendir(fsPath.c_str());
            if (!dir){
                std::string stxt = "Not Found";
                setStatus(404, stxt);
                body = buildDefaultBodyError(404);
                return;
            }
            std::ostringstream ss;
            ss << "<html><head><title>Index of " << request.path << "</title></head>\n";
            ss << "<body><h1>Index of " << request.path << "</h1><ul>\n";
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL){
                std::string name = entry->d_name;
                if (name == "." || name == "..") continue;
                std::string href = request.path;
                if (href.empty() || href[0] != '/') href = "/" + href;
                if (href[href.size()-1] != '/') href += "/";
                ss << "<li><a href=\"" << href << name << "\">" << name << "</a></li>\n";
            }
            closedir(dir);
            ss << "</ul></body></html>\n";
            body = ss.str();
            addHeader(std::string("Content-Type"), std::string("text/html; charset=utf-8"));
            std::string ok = "OK";
            setStatus(200, ok);
            return;
        }

        // Not allowed and no index -> 404
        std::string stxt = "Not Found";
        setStatus(404, stxt);
        body = buildDefaultBodyError(404);
        return;
    }

    // If CGI is enabled on the matched route and the file extension matches, execute CGI
    bool isCgi = false;
    std :: cout<< matched->cgi_enabled << std::endl; 
    if (matched && matched->cgi_enabled) {
        size_t dot = fsPath.find_last_of('.');
        if (dot != std::string::npos) {
            std::string ext = fsPath.substr(dot);
            std :: cout<< "ext: " << ext << std::endl;
            { isCgi = true;}
            for (std::vector<std::string>::const_iterator eit = matched->cgi_extensions.begin(); eit != matched->cgi_extensions.end(); ++eit) {
                std :: cout<< "eit: " << *eit << std::endl;
                if (ext == *eit) 
            }
        }
    }
    if (isCgi) {
        std :: cout<< "<-matched->cgi_enabled" << std::endl; 
        CGIHandler cgi(request, request.serverConfig);
        CGIHandler::Result r = cgi.run(fsPath, matched->cgi_pass);
        setStatus(r.status_code, r.status_text);
        for (std::map<std::string,std::string>::const_iterator it = r.headers.begin(); it != r.headers.end(); ++it){
            addHeader(it->first, it->second);
        }
        body = r.body;
        return;
    }

    std::string content = ReadFromFile(fsPath);
    if (content.empty()){
        std::string stxt = "Not Found";
        setStatus(404, stxt);
        body = buildDefaultBodyError(404);
        return;
    }

    body = content;
    std::string ct = contentTypeFromPath(fsPath);
    addHeader(std::string("Content-Type"), ct);
    std::string ok = "OK";
    setStatus(200, ok);
}

std::string ResponseGet::contentTypeFromPath(const std::string &path){
    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos) return std::string("application/octet-stream");
    std::string ext = path.substr(pos+1);
    // simple mapping
    if (ext == "html" || ext == "htm") return std::string("text/html; charset=utf-8");
    if (ext == "css") return std::string("text/css");
    if (ext == "js") return std::string("application/javascript");
    if (ext == "png") return std::string("image/png");
    if (ext == "jpg" || ext == "jpeg") return std::string("image/jpeg");
    if (ext == "gif") return std::string("image/gif");
    return std::string("application/octet-stream");
}

#include "ResponsePost.hpp"
#include "../CGI/CGIHandler.hpp"
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctime>

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

    // Method check for POST (empty accepted_methods means disallow all)
    if (matched){
        const std::vector<std::string> &allowed = matched->accepted_methods;
        bool ok = false;
        if (!allowed.empty()){
            for (std::vector<std::string>::const_iterator mit = allowed.begin(); mit != allowed.end(); ++mit){
                if (*mit == "POST") { ok = true; break; }
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

    // If resolved target is a directory and there were no parsed multipart uploads,
    // adjust fsPath to point to a new file inside that directory to avoid opening a directory as a file.
    struct stat st_target;
    if (stat(resolved_target, &st_target) == 0) {
        if ((st_target.st_mode & S_IFMT) == S_IFDIR) {
            const std::map<std::string, FilePart>& uploads = request.getUploads();
            if (uploads.empty()) {
                // create a filename using timestamp and pid
                std::ostringstream name;
                name << "upload_" << time(NULL) << "_" << getpid();
                std::string newPath = std::string(resolved_target) + "/" + name.str();
                fsPath = newPath;
            }
        }
    }

    //If route enables CGI and the target path points to a CGI script, execute CGI with request body
    bool tryCgi = false;
    if (matched && matched->cgi_enabled)
     {
        size_t dot = fsPath.find_last_of('.');
        if (dot != std::string::npos) {
            std::string ext = fsPath.substr(dot);
            for (std::vector<std::string>::const_iterator eit = matched->cgi_extensions.begin(); eit != matched->cgi_extensions.end(); ++eit) {
                if (ext == *eit) { tryCgi = true; break; }
            }
        }
    }
    if (tryCgi) {
        CGIHandler cgi(request, request.serverConfig);
        CGIHandler::Result r = cgi.run(fsPath, matched->cgi_pass);
        setStatus(r.status_code, r.status_text);
        for (std::map<std::string,std::string>::const_iterator it = r.headers.begin(); it != r.headers.end(); ++it){
            addHeader(it->first, it->second);
        }
        body = r.body;
        return;
    }

    // If multipart uploads were parsed, save each uploaded FilePart into upload directory
    const std::map<std::string, FilePart>& uploads = request.getUploads();
    if (!uploads.empty()){
        // Determine upload directory: prefer route.upload_path, then matched root, then server root
        std::string uploadDir;
        if (matched && !matched->upload_path.empty()) uploadDir = matched->upload_path;
        else if (matched && !matched->root.empty()) uploadDir = matched->root;
        else uploadDir = request.serverConfig.root;

        // Ensure uploadDir has no trailing slash
    if (!uploadDir.empty() && uploadDir[uploadDir.size() - 1] == '/') uploadDir.erase(uploadDir.size() - 1);

        // Ensure directory exists (attempt to create if missing)
        struct stat stbuf;
        if (stat(uploadDir.c_str(), &stbuf) != 0) {
            if (mkdir(uploadDir.c_str(), 0755) != 0) {
                setStatus(500, "Internal Server Error");
                body = buildDefaultBodyError(500);
                return;
            }
        }

        // Save each upload
        for (std::map<std::string, FilePart>::const_iterator it = uploads.begin(); it != uploads.end(); ++it){
            const FilePart &fp = it->second;
            std::string filename = fp.filename;
            if (filename.empty()) {
                std::ostringstream ts;
                ts << time(NULL);
                filename = std::string("upload_") + ts.str();
            }
            std::string outPath = uploadDir + "/" + filename;
            std::ofstream ofs(outPath.c_str(), std::ios::out | std::ios::binary);
            if (!ofs){
                setStatus(500, "Internal Server Error");
                body = buildDefaultBodyError(500);
                return;
            }
            ofs << fp.content;
            ofs.close();
        }

        setStatus(201, "Created");
        body = std::string("Files uploaded successfully.");
        addHeader("Content-Type", "text/plain; charset=utf-8");
        return;
    }

    // Fallback: write raw request body into the target file (append mode)
    std::ofstream ofs(fsPath.c_str(), std::ios::out | std::ios::binary | std::ios::app);
    if (!ofs){ setStatus(500, "Internal Server Error"); body = buildDefaultBodyError(500); return; }
    ofs << request.body;
    ofs.close();

    setStatus(201, "Created");
    body = std::string("Resource created or appended.");
    addHeader("Content-Type", "text/plain; charset=utf-8");
}

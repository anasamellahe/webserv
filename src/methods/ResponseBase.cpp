#include "ResponseBase.hpp"
#include <sstream>
#include <fstream>
#include <iostream>

ResponseBase::ResponseBase(Request& request)
    : request(request),
    statusCode(200),
    statusText("ok"),
    finalized(false)

{}

void ResponseBase::buildError(int code,  std::string text){
    if (!finalized){
        this->statusCode = code;
        this->statusText = text;
        this->body = buildDefaultBodyError(code);
        finalize();
    }
}


std::string ResponseBase::buildDefaultBodyError(int code){


    const Config::ServerConfig & srv = request.serverConfig;
    const Config::ServerConfig::ConstErrorPagesIterator it = srv.error_pages.find(code);
    if (it != srv.error_pages.end()){
        std::string path = it->second;
        std::string content = ReadFromFile(path);
        if (!content.empty())
            return content;
    }
    return GenerateDefaultError(code);
}

std::string ResponseBase::ReadFromFile(const std::string &path){
    std::ifstream ifs(path.c_str(), std::ios::in | std::ios::binary);
    if (!ifs)
        return std::string();
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

std::string ResponseBase::GenerateDefaultError(int code){
    std::ostringstream ss;
    ss << "<html><head><title>" << code << "</title></head>\n";
    ss << "<body><h1>" << code << "</h1><p>";
    switch(code){
        case 400: ss << "Bad Request"; break;
        case 404: ss << "Not Found"; break;
        case 405: ss << "Method Not Allowed"; break;
        case 413: ss << "Payload Too Large"; break;
        case 414: ss << "URI Too Long"; break;
        case 500: ss << "Internal Server Error"; break;
        case 501: ss << "Not Implemented"; break;
        case 505: ss << "HTTP Version Not Supported"; break;
        default: ss << statusText; break;
    }
    ss << "</p></body></html>\n";
    return ss.str();
}

bool ResponseBase::isMethodAllowed(){
    const std::string &m = request.method;
    const std::vector<Config::RouteConfig> &routes = request.serverConfig.routes;

    // Find best matching route using longest-prefix matching (nginx-style)
    const Config::RouteConfig *matched = NULL;
    size_t best_len = 0;
    for (std::vector<Config::RouteConfig>::const_iterator it = routes.begin(); it != routes.end(); ++it){
        const std::string &route_path = it->path;
        if (route_path.empty()) continue;
        if (request.path.compare(0, route_path.size(), route_path) == 0){
            if (route_path.size() > best_len){
                best_len = route_path.size();
                matched = &(*it);
            }
        }
    }

    if (matched){
        const std::vector<std::string> &allowed = matched->accepted_methods;
        if (allowed.empty())
            return true; // no explicit restriction means all methods allowed at this level
        for (std::vector<std::string>::const_iterator mit = allowed.begin(); mit != allowed.end(); ++mit){
            if (*mit == m) return true;
        }
        return false; // matched but method not allowed
    }

    // No matching route: fall back to allow (server-level rules may apply elsewhere)
    return true;
}

void ResponseBase::setStatus(int code, const std::string& text){
    statusCode = code;
    statusText = text;
}

void ResponseBase::setBody(const std::string & body){
    this->body = body;
}

void ResponseBase::addHeader(const std::string& key, const std::string& value){
    headers[key] = value;
}

std::string ResponseBase::detectContentType(){
    // Very small heuristic based on body/content
    // If Content-Type header already set, return it
    std::map<std::string,std::string>::iterator it = headers.find("Content-Type");
    if (it != headers.end())
        return it->second;
    // Default to text/html
    return std::string("text/html; charset=utf-8");
}

void ResponseBase::finalize(){
    if (finalized) return;
    // Build status line
    std::ostringstream ss;
    ss << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";

    // Ensure Content-Length and Content-Type
    std::ostringstream len;
    len << body.size();
    headers["Content-Length"] = len.str();
    if (headers.find("Content-Type") == headers.end())
        headers["Content-Type"] = detectContentType();

    for (std::map<std::string,std::string>::iterator it = headers.begin(); it != headers.end(); ++it){
        ss << it->first << ": " << it->second << "\r\n";
    }
    ss << "\r\n";
    ss << body;

    response = ss.str();
    finalized = true;
}


ResponseBase::~ResponseBase(){ }
const std::string & ResponseBase::generate(){
    if (!finalized){
        try{
            handle();
            finalize();
        }catch(...){
            buildError(500, "Internal Server Error");
        }
    }
    return response;

}
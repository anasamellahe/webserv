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

    statusCode = code;
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
        // 1xx
        case 100: ss << "Continue"; break;
        case 101: ss << "Switching Protocols"; break;
        case 102: ss << "Processing"; break;
        case 103: ss << "Early Hints"; break;

        // 2xx
        case 200: ss << "OK"; break;
        case 201: ss << "Created"; break;
        case 202: ss << "Accepted"; break;
        case 203: ss << "Non-Authoritative Information"; break;
        case 204: ss << "No Content"; break;
        case 205: ss << "Reset Content"; break;
        case 206: ss << "Partial Content"; break;
        case 207: ss << "Multi-Status"; break;
        case 208: ss << "Already Reported"; break;
        case 226: ss << "IM Used"; break;

        // 3xx
        case 300: ss << "Multiple Choices"; break;
        case 301: ss << "Moved Permanently"; break;
        case 302: ss << "Found"; break;
        case 303: ss << "See Other"; break;
        case 304: ss << "Not Modified"; break;
        case 305: ss << "Use Proxy"; break;
        case 306: ss << "Unused"; break;
        case 307: ss << "Temporary Redirect"; break;
        case 308: ss << "Permanent Redirect"; break;

        // 4xx
        case 400: ss << "Bad Request"; break;
        case 401: ss << "Unauthorized"; break;
        case 402: ss << "Payment Required"; break;
        case 403: ss << "Forbidden"; break;
        case 404: ss << "Not Found"; break;
        case 405: ss << "Method Not Allowed"; break;
        case 406: ss << "Not Acceptable"; break;
        case 407: ss << "Proxy Authentication Required"; break;
        case 408: ss << "Request Timeout"; break;
        case 409: ss << "Conflict"; break;
        case 410: ss << "Gone"; break;
        case 411: ss << "Length Required"; break;
        case 412: ss << "Precondition Failed"; break;
        case 413: ss << "Payload Too Large"; break;
        case 414: ss << "URI Too Long"; break;
        case 415: ss << "Unsupported Media Type"; break;
        case 416: ss << "Range Not Satisfiable"; break;
        case 417: ss << "Expectation Failed"; break;
        case 418: ss << "I'm a teapot"; break;
        case 421: ss << "Misdirected Request"; break;
        case 422: ss << "Unprocessable Entity"; break;
        case 423: ss << "Locked"; break;
        case 424: ss << "Failed Dependency"; break;
        case 425: ss << "Too Early"; break;
        case 426: ss << "Upgrade Required"; break;
        case 428: ss << "Precondition Required"; break;
        case 429: ss << "Too Many Requests"; break;
        case 431: ss << "Request Header Fields Too Large"; break;
        case 451: ss << "Unavailable For Legal Reasons"; break;

        // 5xx
        case 500: ss << "Internal Server Error"; break;
        case 501: ss << "Not Implemented"; break;
        case 502: ss << "Bad Gateway"; break;
        case 503: ss << "Service Unavailable"; break;
        case 504: ss << "Gateway Timeout"; break;
        case 505: ss << "HTTP Version Not Supported"; break;
        case 506: ss << "Variant Also Negotiates"; break;
        case 507: ss << "Insufficient Storage"; break;
        case 508: ss << "Loop Detected"; break;
        case 510: ss << "Not Extended"; break;
        case 511: ss << "Network Authentication Required"; break;

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
#ifndef WEBSERVER_REQUEST_HPP
#define WEBSERVER_REQUEST_HPP

#include <vector>
#include <map>
#include <string>
#include <poll.h>
#include <netinet/in.h>
#include "../Config/ConfigParser.hpp"


struct FilePart {
    std::string filename;
    std::string content_type;
    std::string content;
};

class request 
{
private:
    int clientFD;
    

    std::string requestContent;
    std::string method;             
    std::string path;                
    std::string version;        
    std::map<std::string, std::string> headers;
    std::string body;             
 
    std::map<std::string, std::string> query_params; 
    std::map<std::string, FilePart> uploads;       
    std::string cgi_extension;                     
    std::map<std::string, std::string> cgi_env;                                       
    sockaddr_in client_addr;                         
    bool is_valid;                                   
    std::string error_code;                         

    std::map<std::string, std::string> cookies;    
    bool is_chunked;                                 
    std::vector<std::string> chunks;                 
    
    std::string raw_request;
    std::string status_line;
    bool headers_parsed = false;
    bool complete = false;

    bool parseRequestLine(const std::string& line);


    bool parseBody(const std::string& body_data);
    bool parse(const std::string& raw_request);
    bool parseQueryString(const std::string& query_string);

    bool parseMultipartBody(const std::string& boundary);

    bool parseChunkedTransfer(const std::string& chunked_data);
    bool extractCgiInfo();
    bool parseCookies();
    bool validateMethod() const;
    bool validatePath() const;
    bool validateVersion() const;

public:
    request(int clientFD);
        bool parseHeaders(const std::string& headers_text); // Parse headers from a string

    bool parseFromSocket(int socket_fd, const std::string& existing_data = "" , int total_bytes_read = 0 );
    bool isComplete() const { return complete; }
    void setMethod(const std::string& method);
    void setComplete(bool complete);
    void setPath(const std::string& path);
    void setVersion(const std::string& version);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);
    void setClientFD(int fd);
    void setClientAddr(const sockaddr_in& addr);

    std::string getMethod() const;
    std::string getPath() const;
    int getContentLength() const;
    std::string getVersion() const;
    std::map<std::string, std::string> getAllHeaders() const;
    std::string getBody() const;
    int getClientFD() const;
    sockaddr_in getClientAddr() const;

    void validateRequest();
    void setErrorCode(const std::string& error_code);
    std::string getErrorCode() const;
    std::map<std::string, std::string> getQueryParams() const;
    std::map<std::string, FilePart> getUploads() const;
    std::map<std::string, std::string> getCGIEnv() const;
    std::string getCGIExtension() const;
    std::string getHeader(const std::string& headerName) const;
    std::map<std::string, std::string> getCookies() const;
    bool isChunked() const;
    std::vector<std::string> getChunks() const;
    void addQueryParam(const std::string& key, const std::string& value);
    void addUpload(const std::string& key, const FilePart& file_part);
    
    bool isValid();

};

#endif






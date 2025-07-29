#pragma once

#include <vector>
#include <map>
#include <string>
#include <poll.h>
#include <netinet/in.h>
#include <sstream>
#include "Utils.hpp"
#include "Common.hpp"
#include "Config/ConfigParser.hpp"


#define MAX_PATH_SIZE 4000 // 4KB 
#define BAD_REQ "400 Bad Request"
#define URI_T_LONG "414 URI Too Long"
#define VERSION_ERR "505 HTTP Version Not Supported"

// Forward declaration
struct Config;
class Request 
{

    public:
        typedef std::map<std::string, std::string>::iterator        HeaderIterator;
        typedef std::map<std::string, std::string>::const_iterator  ConstHeaderIterator;
    public:

        int clientFD;
        std::string requestContent;
        std::string method;             
        std::string path;                
        std::string version;
        std::string Host;
        bool       is_Complete; // Indicates if the request is fully parsed
        int         Port;
        bool        isIp;   
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

       
        void parseStartLine(const std::string& line); // OK
        bool parseHeaders(const std::string& headers_text);// OK
        bool isValidHost(const std::string& Host);
        

        bool parseQueryString(); // OK
        void parseQueryPair(const std::string &query, size_t start, size_t end); // OK
        void addQueryParam(const std::string& key, const std::string& value); // OK


        bool parseBody(const std::string& body_data);
        bool parseMultipartBody(const std::string& boundary);

        bool parseChunkedTransfer(const std::string& chunked_data);
        bool extractCgiInfo();
        bool parseCookies();
        bool validateMethod() const;
        bool validatePath() const;
        bool validateHost(const std::string& host);

    public:
        Request();
        Request(int clientFD);
        ~Request();

        bool parseFromSocket(int clientFd, const std::string& buffer, size_t bufferSize);


        void setMethod(const std::string& method); // OK
        void setPath(const std::string& path); // OK
        void setVersion(const std::string& version); //OK
        void setHeader(const std::string& key, const std::string& value);
        void setBody(const std::string& body);
        void setClientFD(int fd);
        void setClientAddr(const sockaddr_in& addr);

        const std::string&    getMethod() const; // OK
        const std::string&    getPath() const; // OK
        const std::string&    getVersion() const; // OK
        const std::string&    getHeader(const std::string& key) const; // OK
        const std::string&    getBody() const; // OK
        int getClientFD() const; // OK
        sockaddr_in getClientAddr() const;

        void validateRequest();
        void setErrorCode(const std::string& error_code); // OK
        const std::string&                          getErrorCode() const; // OK
        const std::map<std::string, std::string>&   getQueryParams() const; // OK
        const std::map<std::string, FilePart>&      getUploads() const;
        const std::map<std::string, std::string>&   getCGIEnv() const;
        const std::string&                          getCGIExtension() const;
        const std::map<std::string, std::string>&   getCookies() const;
        const std::vector<std::string>&             getChunks() const;
        const std::map<std::string, std::string>& getAllHeaders() const;
        bool isComplete() const;
        bool setComplete(bool complete);    
        double getContentLength();
        void addUpload(const std::string& key, const FilePart& file_part);
        bool isChunked() const;
        bool isValid();
        void reset();

        Config getserverConfig(std::string serverToFind, const Config& serversConfigs); //   host or serverName

};

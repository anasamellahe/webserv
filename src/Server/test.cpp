#include <iostream>
#include <string>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <exception>
#include <poll.h>
#include <vector>
#include <map>
#include <utility>

std::map<std::string, std::string>query_params;

void addQueryParam(const std::string& key, const std::string& value)
{
    if (!key.empty())
        query_params.insert(std::pair<std::string, std::string>(key, value));
}

void querySpliter(const std::string &query, size_t start, size_t end) 
{

    size_t pos = query.find("=", start);
    if (pos != std::string::npos || pos <= end)
    {
        std::string key = query.substr(start, pos - start);
        std::string value = query.substr(pos + 1, end - pos);
        addQueryParam(key, value);
    }
}

bool  parseQueryString(std::string &path)
{

    size_t pos = path.find("?", 0);

    if (pos == std::string::npos)
        return false;
    std::string queryParams;
    queryParams = path.substr(pos + 1, std::string::npos);
    path.erase(pos);
    size_t start    = 0;
    size_t end      = 0;
    while (start < queryParams.size())
    {
        end = queryParams.find("&", start);
        if (end != std::string::npos)
        {
            querySpliter(queryParams, start, end - 1);
            start = end + 1;
        }
        else
        {
            querySpliter(queryParams, start, queryParams.length() - 1);
            break;
        }    
    }
    return true;
}

int main ()
{
    std::cout << "start parsing the query\n";
    std::string path =  "/home/med?anas=amellahe&test=anasameksgdhg&a=test%20";
    parseQueryString(path);
    std::cout <<  path<<std::endl;
    for (std::map<std::string, std::string>::iterator it = query_params.begin(); it != query_params.end(); it++)
    {
        std::cout <<"key == " <<it->first  <<  std::endl;
        std::cout <<"val == " <<it->second <<  std::endl;
        std::cout << "*****NEXT*****\n";
    }
}
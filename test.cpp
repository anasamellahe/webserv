#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream
#include <map>
#include <utility>
using namespace std;


std::string setMethod(const std::string& method)
{
  return method;
}
std::string setPath(const std::string& path)
{
  return path;
}
std::string  setVersion(const std::string& version)
{
  return version;
}
bool parseStartLine(const std::string& line, std::string &a, std::string &b, std::string &c)
{

    size_t pos = std::string::npos;

    char *methods[] = {"GET", "POST", "DELETE", NULL};
    for (int i = 0; methods[i]; i++)
    {
        if ((pos = line.find(methods[i], 0)) != std::string::npos)
            break;
    }

    if (pos != std::string::npos)
    {
        std::string str;
        std::stringstream ss(line);
        ss >> str;
        a = setMethod(str);
        ss >> str;
        b = setPath(str);
        ss >> str;
        c = setVersion(str);
    }
    return 0;
}
std::string stringToLower(std::string& str)
{
    for (size_t i = 0; i < str.size(); i++)
        str[i] = static_cast<char>(tolower(str[i]));
    return str;
}

bool keyValidationNot(const std::string &key)
{
    for (size_t i = 0; i < key.size(); i++)
    {
        if (!isprint(key[i]) || key[i] == ' ')
            return true;
    }
    return false;
}
bool valueValidationNot(const std::string &value)
{
    for (size_t i = 0; i < value.size(); i++)
    {
        if (!isprint(value[i]))
            return true;
    }
    return false;
}
bool parseHeaders(const std::string& headers_text, std::map<std::string, std::string>& headers)
{
    std::string key, value;
    size_t pos = headers_text.find(":", 0);
    if (pos == std::string::npos)
        return 1;
    key = headers_text.substr(0, pos);
    // std::cout << key << endl;
    stringToLower(key);
    size_t valuestart = headers_text.find_first_not_of(": \r\t\v\f", pos);
    value = headers_text.substr(valuestart, headers_text.rfind("\r\n") - valuestart);
    // cout << value << endl;
    stringToLower(value);
    std::cout << "["<< key << "] [" << value << "]\n";
    if (keyValidationNot(key) || valueValidationNot(value))
    {
      std::cout << "key or value invalid\n";
      return 1;
      
    }
    if (key == "host" && value.empty())
        return 1;
    headers.insert(std::pair<std::string, std::string>(key, value));
    return 0;
}
bool parseRequestLine(const std::string& line)
{
  std::string a, b , c;
  std::map<std::string, std::string> headers;

    if (parseStartLine(line, a, b, c))
        return 1;
    if(parseHeaders(line, headers))
        return 1;
    std::cout << a << " " << b << " " << c<< "\n";
    std::map<std::string, std::string>::iterator it = headers.begin();
    while (it != headers.end())
    {
      std::cout << "key == " << it->first << " value == " << it->second << std::endl;
      it++;
    }
    return 0;   
}

int main ()
{
  std::string line = "GET /media        \nHTTP/1.1\r\n";
  std::string line1 = "HOST: www.anas.com\r\n"; // \r == 18 : == 4 18 -4 = 14

  parseRequestLine(line1);
}

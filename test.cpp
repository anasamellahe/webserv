// #include <vector>
// #include <map>
// #include <string>
// #include <poll.h>
// #include <netinet/in.h>
// #include <sstream>
// #include <iostream>



// std::string stringToLower(std::string& str)
// {
//     for (size_t i = 0; i < str.size(); i++)
//         str[i] = static_cast<char>(tolower(str[i]));
//     return str;
// }

// bool keyValidationNot(const std::string &key)
// {
//     for (size_t i = 0; i < key.size(); i++)
//     {
//         if (!isprint(key[i]) || key[i] == ' ')
//             return true;
//     }
//     return false;
// }
// bool valueValidationNot(const std::string &value)
// {
//     for (size_t i = 0; i < value.size(); i++)
//     {
//         if (!isprint(value[i]))
//             return true;
//     }
//     return false;
// }
// void setMethod(const std::string& method)
// {
//     std::cout << method << std::endl;
// }
// void setPath(const std::string& path)
// {
//     std::cout << path << std::endl;
// }
// void setVersion(const std::string& version)
// {
//    std::cout << version << std::endl;
// }
// void parseStartLine(const std::string& line)
// {

//     size_t pos = std::string::npos;

//     char * methods[] = {"GET", "POST", "DELETE", NULL};
//     for (size_t i = 0; methods[i]; i++)
//     {
//         if ((pos = line.find(methods[i], 0)) != std::string::npos)
//             break;
//     }
//     if (pos == std::string::npos)
//         throw -1;
//     std::string str;
//     std::stringstream ss(line);
//     ss >> str;
//     setMethod(str);
//     ss >> str;
//     setPath(str);
//     ss >> str;
//     setVersion(str);
//     // if (!this->error_code.empty())
//     //     throw 1;
// }


// void parseHeaders(const std::string& headers_text,  std::map<std::string, std::string>& a)
// {
//     std::string key, value;
//     size_t pos = headers_text.find(":", 0);
//     if (pos == std::string::npos)
//     {
//         // this->error_code = BAD_REQ;
//         throw 1;
//     }
//     key = headers_text.substr(0, pos);
//     stringToLower(key);
//     size_t valueStartIndex = headers_text.find_first_not_of(": \r\t\v\f", pos);
//     if (valueStartIndex == std::string::npos)
//         valueStartIndex = pos;
//     value = headers_text.substr(valueStartIndex, headers_text.rfind("\r\n") - valueStartIndex);
//     stringToLower(value);
//     if (keyValidationNot(key) || valueValidationNot(value))
//     {
//         // this->error_code = BAD_REQ;
//         throw 1;
//     }
//     if (key == "host" && value.empty())
//     {
//         // this->error_code = BAD_REQ;
//         throw 1;
//     }
//     // std::cout << "key  ==  [ " << key << " ]" << "value  ==  [ "<<value <<" ]\n";
//     a.insert(std::pair<std::string, std::string>(key, value));
// }


// bool parseSingleLineHeader(const std::string line, std::map<std::string, std::string> &a)
// {


//     bool test = false;
//     try
//     {

//         parseStartLine(line);
//     }
//     catch(int number)
//     {
//         if (number == 1)
//             return 1;
//         if (number == -1)
//             test = true;
//         // if (this->method.empty() || this->path.empty() || this->version.empty())
//         // {
//         //     if (this->error_code.empty())
//         //         this->error_code = BAD_REQ;
//         //     return 1;
//         // }
//     }

//     try
//     {
//         if (test == true)
//         {
//             // std::cout << line << std::endl;
//             // std::cout << "**********call 2***************\n";
//             parseHeaders(line, a);
//         }
//     }
//     catch(int number)
//     {
//         if (number == 1)
//             return 1;
//     }
//     return 0;   
// }

// // 123456\r\n12345
// bool parseRequestLine(const std::string& line, std::map<std::string, std::string> &a)
// {
//     // std::cout << line << std::endl;
//     // std::cout << "---------------------------\n";
//     size_t start = 0;
//     while (start < line.size())
//     {
//         size_t end = line.find("\r\n", start);
//         if (end != std::string::npos)
//         {
//             if (parseSingleLineHeader(line.substr(start, end - start), a) == 1)
//                 return 1;
//             start = end + 2;
//         }
//         else
//         {
//             if (parseSingleLineHeader(line.substr(start), a) == 1)
//                 return 1;
//             break;
//         }
//     }

//     return 0;
// }

// int main ()
// {

//     std::map<std::string, std::string> b;
//     std::string a =  "GET /test HTTP/1.1\r\ntest: anas\r\ntest1:hhhhh\r\nanas:holla";
//     if (parseRequestLine(a, b) == 1)
//     {
//         std::cout << "[ error get ]\n";
//     }
//     for (std::map<std::string, std::string>::iterator it = b.begin(); it != b.end(); it++)
//     {
//         std::cout << it->first << " " << it->second << std::endl;
//     }

// }

#include <stdio.h>
#include <iostream>

int main ()
{
    const char *str = "120.20.20.1";
    int a, b, c, d, e;
    a = b = c = d = e = -1;
    int number = sscanf(str, "%d.%d.%d.%d.%d", &a, &b, &c, &d, &e);

    std::cout << a << " - "<< b << " - " << c << " - " << d << " - " << e << std::endl;
    std::cout << "number = " << number << std::endl;

}








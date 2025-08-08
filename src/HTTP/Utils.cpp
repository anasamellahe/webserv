#include "Utils.hpp"


std::string stringToLower(std::string& str)
{
    for (size_t i = 0; i < str.size(); i++)
        str[i] = static_cast<char>(tolower(str[i]));
    return str;
}


bool isValidKey(const std::string &key)
{
    if (key.empty())
        return false;
    for (size_t i = 0; i < key.size(); i++)
    {
        if (!isprint(key[i]) || key[i] == ' ')
            return false;
    }
    return true;
}

bool isValidValue(const std::string &value)
{
    for (size_t i = 0; i < value.size(); i++)
    {
        if (!isprint(value[i]))
            return false;
    }
    return true;
}

bool isValidPort(int port){
    return port >= 1 && port <= 65535;
}
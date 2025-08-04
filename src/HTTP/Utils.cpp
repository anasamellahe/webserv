#include "Utils.hpp"
#include <cctype>

std::string stringToLower(std::string& str)
{
    for (size_t i = 0; i < str.size(); i++)
        str[i] = static_cast<char>(tolower(str[i]));
    return str;
}

bool keyValidationNot(const std::string &key)
{
    if (key.empty())
        return true;
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
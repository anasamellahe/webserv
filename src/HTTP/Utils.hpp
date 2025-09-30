#pragma once
#include <string>
#include <cctype>
#include <typeinfo>  

std::string stringToLower(std::string& str);
bool isValidKey(const std::string &key);
bool isValidValue(const std::string &value);
bool isValidPort(int port);
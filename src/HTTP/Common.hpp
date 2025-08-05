#pragma once

#include <string>

// Full definition of FilePart
struct FilePart {
    std::string filename;
    std::string content_type;
    std::string content;
};

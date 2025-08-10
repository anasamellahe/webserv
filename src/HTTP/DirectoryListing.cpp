#include "DirectoryListing.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <limits.h>

bool DirectoryListing::isDirectory(const std::string& path) {
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) != 0) {
        return false;
    }
    return S_ISDIR(statbuf.st_mode);
}

bool DirectoryListing::hasIndexFile(const std::string& dirPath, const std::string& indexFiles) {
    if (indexFiles.empty()) {
        return false;
    }
    
    std::istringstream iss(indexFiles);
    std::string indexFile;
    
    while (iss >> indexFile) {
        std::string fullPath = dirPath;
        if (fullPath.back() != '/') {
            fullPath += '/';
        }
        fullPath += indexFile;
        
        struct stat statbuf;
        if (stat(fullPath.c_str(), &statbuf) == 0 && S_ISREG(statbuf.st_mode)) {
            return true;
        }
    }
    
    return false;
}

std::string DirectoryListing::generateDirectoryHTML(const std::string& dirPath, const std::string& requestPath) {
    if (!isDirectory(dirPath)) {
        return "";
    }
    
    std::string html = "<!DOCTYPE html>\n<html>\n<head>\n";
    html += "<title>Index of " + escapeHTML(requestPath) + "</title>\n";
    html += "<style>\n";
    html += "body { font-family: Arial, sans-serif; margin: 20px; }\n";
    html += "h1 { color: #333; border-bottom: 1px solid #ccc; padding-bottom: 10px; }\n";
    html += "table { border-collapse: collapse; width: 100%; margin-top: 20px; }\n";
    html += "th, td { text-align: left; padding: 8px 12px; border-bottom: 1px solid #ddd; }\n";
    html += "th { background-color: #f5f5f5; font-weight: bold; }\n";
    html += "tr:hover { background-color: #f9f9f9; }\n";
    html += "a { text-decoration: none; color: #0066cc; }\n";
    html += "a:hover { text-decoration: underline; }\n";
    html += ".dir { font-weight: bold; }\n";
    html += ".size { text-align: right; }\n";
    html += "</style>\n";
    html += "</head>\n<body>\n";
    
    html += "<h1>Index of " + escapeHTML(requestPath) + "</h1>\n";
    
    html += "<table>\n";
    html += "<thead>\n";
    html += "<tr><th>Name</th><th>Last Modified</th><th class='size'>Size</th></tr>\n";
    html += "</thead>\n";
    html += "<tbody>\n";
    
    // Add parent directory link if not at root
    if (requestPath != "/" && !requestPath.empty()) {
        html += "<tr>\n";
        html += "<td><a href=\"../\" class=\"dir\">../</a></td>\n";
        html += "<td>-</td>\n";
        html += "<td class='size'>-</td>\n";
        html += "</tr>\n";
    }
    
    // Get directory entries
    std::vector<std::string> entries = getDirectoryEntries(dirPath);
    std::sort(entries.begin(), entries.end());
    
    // Separate directories and files
    std::vector<std::string> directories;
    std::vector<std::string> files;
    
    for (size_t i = 0; i < entries.size(); i++) {
        const std::string& entry = entries[i];
        if (entry == "." || entry == "..") {
            continue;
        }
        
        std::string fullPath = dirPath;
        if (fullPath.back() != '/') {
            fullPath += '/';
        }
        fullPath += entry;
        
        if (isDirectory(fullPath)) {
            directories.push_back(entry);
        } else {
            files.push_back(entry);
        }
    }
    
    // Add directories first
    for (size_t i = 0; i < directories.size(); i++) {
        html += getFileEntryHTML(dirPath + "/" + directories[i], directories[i] + "/", requestPath);
    }
    
    // Then add files
    for (size_t i = 0; i < files.size(); i++) {
        html += getFileEntryHTML(dirPath + "/" + files[i], files[i], requestPath);
    }
    
    html += "</tbody>\n";
    html += "</table>\n";
    html += "</body>\n</html>\n";
    
    return html;
}

std::vector<std::string> DirectoryListing::getDirectoryEntries(const std::string& path) {
    std::vector<std::string> entries;
    
    DIR* dir = opendir(path.c_str());
    if (dir == NULL) {
        return entries;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        entries.push_back(std::string(entry->d_name));
    }
    
    closedir(dir);
    return entries;
}

bool DirectoryListing::isPathSafe(const std::string& path, const std::string& rootPath) {
    // Resolve absolute paths to prevent directory traversal
    char resolvedPath[PATH_MAX];
    char resolvedRoot[PATH_MAX];
    
    if (realpath(path.c_str(), resolvedPath) == NULL) {
        return false;
    }
    
    if (realpath(rootPath.c_str(), resolvedRoot) == NULL) {
        return false;
    }
    
    std::string absPath(resolvedPath);
    std::string absRoot(resolvedRoot);
    
    // Ensure the path starts with the root path
    if (absPath.find(absRoot) != 0) {
        return false;
    }
    
    // Prevent access to hidden files/directories (starting with .)
    // except for current directory
    std::string filename = path.substr(path.find_last_of("/") + 1);
    if (filename[0] == '.' && filename != "." && filename != "..") {
        return false;
    }
    
    return true;
}

std::string DirectoryListing::escapeHTML(const std::string& str) {
    std::string escaped;
    escaped.reserve(str.length() * 2);
    
    for (size_t i = 0; i < str.length(); i++) {
        switch (str[i]) {
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            case '&':
                escaped += "&amp;";
                break;
            case '"':
                escaped += "&quot;";
                break;
            case '\'':
                escaped += "&#39;";
                break;
            default:
                escaped += str[i];
                break;
        }
    }
    
    return escaped;
}

std::string DirectoryListing::formatFileSize(off_t size) {
    if (size < 1024) {
        return std::to_string(size) + " B";
    } else if (size < 1024 * 1024) {
        return std::to_string(size / 1024) + "." + std::to_string((size % 1024) / 102) + " K";
    } else if (size < 1024 * 1024 * 1024) {
        return std::to_string(size / (1024 * 1024)) + "." + std::to_string((size % (1024 * 1024)) / (1024 * 102)) + " M";
    } else {
        return std::to_string(size / (1024 * 1024 * 1024)) + "." + std::to_string((size % (1024 * 1024 * 1024)) / (1024 * 1024 * 102)) + " G";
    }
}

std::string DirectoryListing::formatLastModified(time_t mtime) {
    char buffer[100];
    struct tm* timeinfo = localtime(&mtime);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

std::string DirectoryListing::getFileEntryHTML(const std::string& filePath, const std::string& fileName, const std::string& requestPath) {
    struct stat statbuf;
    std::string html = "<tr>\n";
    
    // Create proper URL path
    std::string urlPath = requestPath;
    if (urlPath.back() != '/') {
        urlPath += '/';
    }
    urlPath += fileName;
    
    if (stat(filePath.c_str(), &statbuf) == 0) {
        // File exists, get info
        std::string escapedName = escapeHTML(fileName);
        bool isDir = S_ISDIR(statbuf.st_mode);
        
        html += "<td><a href=\"" + urlPath + "\"";
        if (isDir) {
            html += " class=\"dir\"";
        }
        html += ">" + escapedName + "</a></td>\n";
        
        html += "<td>" + formatLastModified(statbuf.st_mtime) + "</td>\n";
        
        if (isDir) {
            html += "<td class='size'>-</td>\n";
        } else {
            html += "<td class='size'>" + formatFileSize(statbuf.st_size) + "</td>\n";
        }
    } else {
        // File doesn't exist or no permission
        std::string escapedName = escapeHTML(fileName);
        html += "<td>" + escapedName + "</td>\n";
        html += "<td>-</td>\n";
        html += "<td class='size'>-</td>\n";
    }
    
    html += "</tr>\n";
    return html;
}
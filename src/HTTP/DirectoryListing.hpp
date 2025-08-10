#pragma once

#include <string>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>
#include <ctime>

/**
 * @brief Directory listing functionality for HTTP server
 * 
 * Provides methods to generate HTML directory indexes when directory listing
 * is enabled in route configuration. Handles security, HTML escaping, and
 * proper file system interaction.
 */
class DirectoryListing {
public:
    /**
     * @brief Check if a path points to a directory
     * @param path File system path to check
     * @return true if path is a directory, false otherwise
     */
    static bool isDirectory(const std::string& path);

    /**
     * @brief Check if directory contains any index files
     * @param dirPath Directory path to check
     * @param indexFiles Space-separated list of index file names (e.g., "index.html index.htm")
     * @return true if any index file exists in the directory
     */
    static bool hasIndexFile(const std::string& dirPath, const std::string& indexFiles);

    /**
     * @brief Generate HTML directory listing page
     * @param dirPath File system path to the directory
     * @param requestPath HTTP request path (for display and links)
     * @return Complete HTML page showing directory contents
     */
    static std::string generateDirectoryHTML(const std::string& dirPath, const std::string& requestPath);

    /**
     * @brief Get list of files and directories in a path
     * @param path Directory path to read
     * @return Vector of directory entry names (files and subdirectories)
     */
    static std::vector<std::string> getDirectoryEntries(const std::string& path);

    /**
     * @brief Check if path is safe and within allowed boundaries
     * @param path Path to validate
     * @param rootPath Root directory boundary
     * @return true if path is safe to access
     */
    static bool isPathSafe(const std::string& path, const std::string& rootPath);

private:
    /**
     * @brief Escape HTML special characters in string
     * @param str String to escape
     * @return HTML-escaped string
     */
    static std::string escapeHTML(const std::string& str);

    /**
     * @brief Format file size in human-readable format
     * @param size File size in bytes
     * @return Formatted size string (e.g., "1.5K", "2.3M")
     */
    static std::string formatFileSize(off_t size);

    /**
     * @brief Format file modification time as string
     * @param mtime Modification time
     * @return Formatted date/time string
     */
    static std::string formatLastModified(time_t mtime);

    /**
     * @brief Get file information for directory entry
     * @param filePath Full path to file
     * @param fileName File name for display
     * @param requestPath Base request path for links
     * @return HTML table row for this file entry
     */
    static std::string getFileEntryHTML(const std::string& filePath, const std::string& fileName, const std::string& requestPath);
};
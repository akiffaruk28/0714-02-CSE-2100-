#pragma once
#include <string>
#include <vector>

// ISP: Minimal interface for file operations
class IFileScanner {
public:
    virtual ~IFileScanner() = default;
    virtual std::vector<std::string> scan(const std::string& path, bool recursive, bool includeHidden) = 0;
};

// SRP: FileOperations handles only filesystem queries and copies
class FileOperations {
public:
    // File info helpers
    static bool        fileExists(const std::string& path);
    static bool        isDirectory(const std::string& path);
    static long        fileSize(const std::string& path);
    static std::string lastModified(const std::string& path);
    static bool        isHiddenFile(const std::string& path);

    // File manipulation
    static bool        copyFile(const std::string& src, const std::string& dest);
    static bool        createDirectory(const std::string& path);

    // Size formatting helper
    static std::string formatSize(long bytes);
};

// OCP: New scanners can be added without changing existing code
class RecursiveFileScanner : public IFileScanner {
public:
    std::vector<std::string> scan(const std::string& path, bool recursive, bool includeHidden) override;
};

#include "fileops.h"
#include "utils.h"
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <fstream>

#ifdef _WIN32
  #include <direct.h>
  #define MKDIR(p) _mkdir(p)
  #define PATH_SEP "\\"
  #define stat _stat
  #define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#else
  #include <sys/stat.h>
  #define MKDIR(p) mkdir(p, 0755)
  #define PATH_SEP "/"
#endif

// ─── FileOperations ──────────────────────────────────────────────────────────
// SRP: pure filesystem helpers — no GTK, no business logic, no UI

bool FileOperations::fileExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

bool FileOperations::isDirectory(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

long FileOperations::fileSize(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return -1;
    return static_cast<long>(st.st_size);
}

std::string FileOperations::lastModified(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return "";
    return Utils::formatTimestamp(st.st_mtime);
}

bool FileOperations::isHiddenFile(const std::string& path) {
    const std::string base = Utils::baseName(path);
    return !base.empty() && base[0] == '.';
}

bool FileOperations::copyFile(const std::string& src, const std::string& dest) {
    std::ifstream in(src,  std::ios::binary);
    std::ofstream out(dest, std::ios::binary);
    if (!in.is_open() || !out.is_open()) return false;
    out << in.rdbuf();
    return out.good();
}

bool FileOperations::createDirectory(const std::string& path) {
    if (path.empty()) return false;
    if (isDirectory(path)) return true;
    return MKDIR(path.c_str()) == 0;
}

std::string FileOperations::formatSize(long bytes) {
    return Utils::formatSize(bytes);
}

// ─── RecursiveFileScanner ────────────────────────────────────────────────────
// OCP: implements IFileScanner; new scan strategies can be added separately
// SRP: only responsibility is listing files under a directory

std::vector<std::string> RecursiveFileScanner::scan(const std::string& path,
                                                     bool               recursive,
                                                     bool               includeHidden) {
    std::vector<std::string> result;

    DIR* dir = opendir(path.c_str());
    if (!dir) return result;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (std::strcmp(entry->d_name, ".") == 0 ||
            std::strcmp(entry->d_name, "..") == 0)
            continue;

        std::string fullPath = Utils::joinPath(path, entry->d_name);

        // Hidden file filter
        if (!includeHidden && entry->d_name[0] == '.') continue;

        if (FileOperations::isDirectory(fullPath)) {
            if (recursive) {
                auto sub = scan(fullPath, true, includeHidden);
                result.insert(result.end(), sub.begin(), sub.end());
            }
        } else {
            result.push_back(fullPath);
        }
    }
    closedir(dir);
    return result;
}

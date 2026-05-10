#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <sys/stat.h>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #define MKDIR(p) _mkdir(p)
#else
    #include <dirent.h>
    #define MKDIR(p) mkdir(p, 0755)
#endif

class IBackupStrategy {
public:
    virtual ~IBackupStrategy() = default;
    virtual bool copyFile(const std::string& src, const std::string& dest) = 0;
    virtual bool copyFolder(const std::string& src, const std::string& dest,
                            bool includeHidden = false, bool includeSubfolders = true) = 0;
    virtual std::string getStrategyName() const = 0;
};

class SimpleCopyStrategy : public IBackupStrategy {
public:
    bool copyFile(const std::string& src, const std::string& dest) override {
        std::ifstream in(src, std::ios::binary);
        std::ofstream out(dest, std::ios::binary);
        if (!in.is_open() || !out.is_open()) return false;
        out << in.rdbuf();
        return out.good();
    }

    bool copyFolder(const std::string& src, const std::string& dest,
                    bool includeHidden = false, bool includeSubfolders = true) override {
        createDir(dest);

#ifdef _WIN32
        std::string searchPath = src + "\\*";
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
        // Empty folder is not an error — INVALID_HANDLE_VALUE with ERROR_FILE_NOT_FOUND
        // means the folder exists but has no contents. That's a valid backup (copy the dir).
        if (hFind == INVALID_HANDLE_VALUE) return true;

        bool success = true;
        do {
            std::string name = findData.cFileName;
            if (name == "." || name == "..") continue;

            // FIX: Include Hidden OFF হলে hidden files/folders skip করো
            bool isHidden = (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
            if (!includeHidden && isHidden) continue;

            std::string srcPath  = src  + "\\" + name;
            std::string destPath = dest + "\\" + name;

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (includeSubfolders) {
                    if (!copyFolder(srcPath, destPath, includeHidden, includeSubfolders)) success = false;
                }
            } else {
                if (!copyFile(srcPath, destPath)) success = false;
            }
        } while (FindNextFileA(hFind, &findData));

        FindClose(hFind);
        return success;
#else
        DIR* dir = opendir(src.c_str());
        if (!dir) return false;

        bool success = true;
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;

            // FIX: Linux-এ dot দিয়ে শুরু হলে hidden
            bool isHidden = (!name.empty() && name[0] == '.');
            if (!includeHidden && isHidden) continue;

            std::string srcPath  = src  + "/" + name;
            std::string destPath = dest + "/" + name;

            struct stat st;
            if (stat(srcPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                if (includeSubfolders) {
                    if (!copyFolder(srcPath, destPath, includeHidden, includeSubfolders)) success = false;
                }
            } else {
                if (!copyFile(srcPath, destPath)) success = false;
            }
        }
        closedir(dir);
        return success;
#endif
    }

    std::string getStrategyName() const override { return "SimpleCopy"; }

private:
    void createDir(const std::string& path) {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) {
            MKDIR(path.c_str());
        }
    }
};

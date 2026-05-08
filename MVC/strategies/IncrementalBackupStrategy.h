#pragma once
#include "SimpleCopyStrategy.h"
#include <sys/stat.h>
#include <string>
#include <fstream>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #define MKDIR(p) _mkdir(p)
#else
    #include <dirent.h>
    #define MKDIR(p) mkdir(p, 0755)
#endif

// ── IncrementalBackupStrategy ─────────────────────────────────────────────────
// Only copies a file if:
//   (a) it does not exist in the destination, OR
//   (b) the source mtime is newer than the destination mtime
//
// This makes repeated auto-backups fast — unchanged files are skipped entirely.
// A companion .incremental_manifest is written so the log can show skip counts.

class IncrementalBackupStrategy : public IBackupStrategy {
public:
    // Counters reset per backup run (BackupController reads them for the log)
    mutable int copied  = 0;
    mutable int skipped = 0;

    void resetCounters() const { copied = 0; skipped = 0; }

    bool copyFile(const std::string& src, const std::string& dest) override {
        if (isUpToDate(src, dest)) {
            ++skipped;
            return true;   // not an error — file is already current
        }
        std::ifstream in(src, std::ios::binary);
        std::ofstream out(dest, std::ios::binary | std::ios::trunc);
        if (!in.is_open() || !out.is_open()) return false;
        out << in.rdbuf();
        if (out.good()) { ++copied; return true; }
        return false;
    }

    bool copyFolder(const std::string& src, const std::string& dest,
                    bool includeHidden = false) override {
        createDir(dest);

#ifdef _WIN32
        std::string searchPath = src + "\\*";
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
        if (hFind == INVALID_HANDLE_VALUE) return true;

        bool success = true;
        do {
            std::string name = findData.cFileName;
            if (name == "." || name == "..") continue;
            bool isHidden = (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
            if (!includeHidden && isHidden) continue;

            std::string srcPath  = src  + "\\" + name;
            std::string destPath = dest + "\\" + name;

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (!copyFolder(srcPath, destPath, includeHidden)) success = false;
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
            bool isHidden = (!name.empty() && name[0] == '.');
            if (!includeHidden && isHidden) continue;

            std::string srcPath  = src  + "/" + name;
            std::string destPath = dest + "/" + name;

            struct stat st;
            if (stat(srcPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                if (!copyFolder(srcPath, destPath, includeHidden)) success = false;
            } else {
                if (!copyFile(srcPath, destPath)) success = false;
            }
        }
        closedir(dir);
        return success;
#endif
    }

    std::string getStrategyName() const override { return "Incremental"; }

private:
    // Returns true if dest exists AND its mtime >= src mtime → skip copy
    bool isUpToDate(const std::string& src, const std::string& dest) const {
#ifdef _WIN32
        struct _stat64 ss, ds;
        if (_stat64(src.c_str(),  &ss) != 0) return false;
        if (_stat64(dest.c_str(), &ds) != 0) return false;
        return ds.st_mtime >= ss.st_mtime && ds.st_size == ss.st_size;
#else
        struct stat ss, ds;
        if (::stat(src.c_str(),  &ss) != 0) return false;
        if (::stat(dest.c_str(), &ds) != 0) return false;
        return ds.st_mtime >= ss.st_mtime && ds.st_size == ss.st_size;
#endif
    }

    void createDir(const std::string& path) {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) MKDIR(path.c_str());
    }
};

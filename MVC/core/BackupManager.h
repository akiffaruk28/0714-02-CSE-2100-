#pragma once
#include "../strategies/SimpleCopyStrategy.h"
#include <vector>
#include <string>
#include <ctime>
#include <fstream>
#include <sys/stat.h>
#include <thread>
#include <functional>
#include <glib.h>   // FIX 3: g_idle_add — background thread থেকে GTK-কে safely call করতে

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(p) _mkdir(p)
#else
    #include <dirent.h>  // FIX 2: Linux countFiles এর জন্য
    #define MKDIR(p) mkdir(p, 0755)
#endif

class IBackupObserver {
public:
    virtual ~IBackupObserver() = default;
    virtual void onProgress(int current, int total, const std::string& filename) = 0;
    virtual void onComplete(int success, int total) = 0;
    virtual void onError(const std::string& message) = 0;
};

class BackupManager {
private:
    IBackupStrategy* m_strategy;
    IBackupObserver* m_observer = nullptr;
    bool m_running = false;
    std::string m_lastLogPath; // FIX: শেষ backup এর log path মনে রাখে

    std::string joinPath(const std::string& a, const std::string& b) {
        if (a.empty()) return b;
        if (a.back() == '/' || a.back() == '\\') return a + b;
#ifdef _WIN32
        return a + "\\" + b;
#else
        return a + "/" + b;
#endif
    }

    std::string baseName(const std::string& path) {
        size_t pos = path.find_last_of("/\\");
        return (pos == std::string::npos) ? path : path.substr(pos + 1);
    }

    std::string timestamp() {
        time_t now = time(nullptr);
        char buf[32];
        strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", localtime(&now));
        return buf;
    }

    // nested directory তৈরি করে (যেমন C:\Users\HP\Backups\Backup_xxx)
    void createDir(const std::string& path) {
        struct stat st;
        if (stat(path.c_str(), &st) == 0) return; // already exists

#ifdef _WIN32
        // Parent directories আগে তৈরি করো
        std::string tmp = path;
        for (size_t i = 1; i < tmp.size(); i++) {
            if (tmp[i] == '\\' || tmp[i] == '/') {
                tmp[i] = '\0';
                struct stat s;
                if (stat(tmp.c_str(), &s) != 0) _mkdir(tmp.c_str());
                tmp[i] = '\\';
            }
        }
        _mkdir(path.c_str());
#else
        std::string tmp = path;
        for (size_t i = 1; i < tmp.size(); i++) {
            if (tmp[i] == '/') {
                tmp[i] = '\0';
                struct stat s;
                if (stat(tmp.c_str(), &s) != 0) mkdir(tmp.c_str(), 0755);
                tmp[i] = '/';
            }
        }
        mkdir(path.c_str(), 0755);
#endif
    }

    // File নাকি Folder সেটা detect করে
    bool isDirectory(const std::string& path) {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) return false;
        return S_ISDIR(st.st_mode);
    }

    // FIX 2: Folder এর ভেতরে কতটা file আছে count করে — এখন Linux ও Windows দুটোতেই কাজ করবে
    int countFiles(const std::string& path) {
        if (!isDirectory(path)) return 1;
        int count = 0;

#ifdef _WIN32
        std::string searchPath = path + "\\*";
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
        if (hFind == INVALID_HANDLE_VALUE) return 0;
        do {
            std::string name = findData.cFileName;
            if (name == "." || name == "..") continue;
            std::string fullPath = path + "\\" + name;
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                count += countFiles(fullPath);
            else
                count++;
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
#else
        DIR* dir = opendir(path.c_str());
        if (!dir) return 0;
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;
            std::string fullPath = path + "/" + name;
            struct stat st;
            if (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
                count += countFiles(fullPath);
            else
                count++;
        }
        closedir(dir);
#endif
        return count;
    }

public:
    BackupManager(IBackupStrategy* strategy) : m_strategy(strategy) {}
    void setObserver(IBackupObserver* observer) { m_observer = observer; }
    void setStrategy(IBackupStrategy* strategy) { m_strategy = strategy; }  // runtime swap
    bool isRunning() const { return m_running; }
    std::string getLastLogPath() const { return m_lastLogPath; }

    // FIX 1: Background thread-এ চালাও, নইলে GTK main loop block হয়ে crash করে
    bool runBackup(const std::vector<std::string>& items, const std::string& destination,
                   bool includeHidden = false, bool includeSubfolders = true,
                   const std::string& customName = "") {
        if (m_running || items.empty()) return false;
        m_running = true;

        std::thread([this, items, destination, includeHidden, includeSubfolders, customName]() {
            runBackupInternal(items, destination, includeHidden, includeSubfolders, customName);
        }).detach();

        return true;
    }

private:
    bool runBackupInternal(const std::vector<std::string>& items, const std::string& destination,
                           bool includeHidden, bool includeSubfolders, const std::string& customName) {
        // Custom নাম দেওয়া থাকলে সেটা ব্যবহার করো, না থাকলে timestamp
        std::string folderName = customName.empty() ? ("Backup_" + timestamp()) : customName;
        std::string backupDir  = joinPath(destination, folderName);
        createDir(backupDir);

        std::string logPath = joinPath(backupDir, "backup_log.txt");
        m_lastLogPath = logPath; // FIX: log path সেভ করো
        std::ofstream log(logPath, std::ios::app);

        time_t now = time(nullptr);
        char timeBuf[64];
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        log << "Backup started: " << timeBuf << "\n";

        int total   = items.size();
        int success = 0;

        for (int i = 0; i < total; ++i) {
            std::string name = baseName(items[i]);
            std::string dest = joinPath(backupDir, name);

            // FIX 3: GTK is not thread-safe — observer calls must run on the GTK main thread
            if (m_observer) {
                int cur = i + 1;
                IBackupObserver* obs = m_observer;
                std::string* nameCopy = new std::string(name);
                g_idle_add([](gpointer data) -> gboolean {
                    auto* p = static_cast<std::tuple<IBackupObserver*, int, int, std::string*>*>(data);
                    std::get<0>(*p)->onProgress(std::get<1>(*p), std::get<2>(*p), *std::get<3>(*p));
                    delete std::get<3>(*p);
                    delete p;
                    return G_SOURCE_REMOVE;
                }, new std::tuple<IBackupObserver*, int, int, std::string*>(obs, cur, total, nameCopy));
            }

            if (isDirectory(items[i])) {
                log << "[FOLDER] " << items[i] << "\n";
                if (m_strategy->copyFolder(items[i], dest, includeHidden, includeSubfolders)) {
                    success++;
                    log << "[OK] Folder backed up: " << name << "\n";
                } else {
                    log << "[ERR] Folder failed: " << name << "\n";
                }
            } else {
                // ── Single file copy ──
                if (m_strategy->copyFile(items[i], dest)) {
                    success++;
                    log << "[OK] " << items[i] << "\n";
                } else {
                    log << "[ERR] " << items[i] << "\n";
                }
            }
        }

        log << "Backup complete: " << success << "/" << total << " items\n";
        log.close();

        // FIX 3: onComplete-ও GTK main thread-এ dispatch করো
        if (m_observer) {
            IBackupObserver* obs = m_observer;
            int s = success, t = total;
            g_idle_add([](gpointer data) -> gboolean {
                auto* p = static_cast<std::pair<IBackupObserver*, std::pair<int,int>>*>(data);
                p->first->onComplete(p->second.first, p->second.second);
                delete p;
                return G_SOURCE_REMOVE;
            }, new std::pair<IBackupObserver*, std::pair<int,int>>(obs, {s, t}));
        }
        m_running = false;
        return success == total;
    }
}; // class BackupManager

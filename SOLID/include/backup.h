#pragma once
#include <string>
#include <vector>

// ISP: Separate interfaces for different backup concerns
// DIP: High-level modules depend on abstractions

// Interface for backup strategy (OCP: open for extension)
class IBackupStrategy {
public:
    virtual ~IBackupStrategy() = default;
    virtual bool copyFile(const std::string& src, const std::string& dest) = 0;
    virtual std::string getStrategyName() const = 0;
};

// Interface for backup progress observer (OCP: add new observers without changing core)
class IBackupObserver {
public:
    virtual ~IBackupObserver() = default;
    virtual void onProgress(int current, int total, const std::string& filename) = 0;
    virtual void onComplete(int success, int total) = 0;
    virtual void onError(const std::string& message) = 0;
};

// SRP: BackupManager only manages backup orchestration
class BackupManager {
public:
    explicit BackupManager(IBackupStrategy* strategy);
    ~BackupManager();

    void setObserver(IBackupObserver* observer);
    bool runBackup(const std::vector<std::string>& items, const std::string& destination);
    bool isRunning() const;

private:
    IBackupStrategy*  m_strategy;
    IBackupObserver*  m_observer;
    bool              m_running;

    std::string createBackupDirectory(const std::string& destination);
    void        writeLog(const std::string& logPath, const std::string& entry);
};

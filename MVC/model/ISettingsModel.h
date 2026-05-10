#pragma once
#include <string>
#include <functional>

// ── Strategy enum ─────────────────────────────────────────────────────────────
enum class BackupStrategy {
    FullCopy    = 0,   // SimpleCopyStrategy  — always copy everything
    Incremental = 1,   // IncrementalBackupStrategy — skip unchanged files
};

// ── BackupSettings ─────────────────────────────────────────────────────────────
struct BackupSettings {
    std::string destination  = "";
    std::string backupName   = "";   // empty → auto timestamp
    bool autoBackup          = false;
    int  interval            = 300;  // seconds
    int  maxCopies           = 10;
    bool includeSubfolders   = true;
    bool includeHidden       = false;
    bool showNotifications   = true;

    // ── New beneficial fields ─────────────────────────────────────────────────
    BackupStrategy strategy      = BackupStrategy::FullCopy;
    bool retryOnFailure          = true;   // retry once automatically if backup fails
    bool backupOnAppStart        = false;  // run an immediate backup when app launches
};

// ── ISettingsModel ─────────────────────────────────────────────────────────────
class ISettingsModel {
public:
    virtual ~ISettingsModel() = default;

    virtual BackupSettings getSettings() const = 0;

    virtual void setDestination(const std::string& dest) = 0;
    virtual void setBackupName(const std::string& name)  = 0;
    virtual void setAutoBackup(bool enabled)             = 0;
    virtual void setInterval(int seconds)                = 0;
    virtual void setIncludeSubfolders(bool include)      = 0;
    virtual void setIncludeHidden(bool include)          = 0;
    virtual void setMaxCopies(int maxCopies)             = 0;
    virtual void setShowNotifications(bool show)         = 0;

    // New setters
    virtual void setStrategy(BackupStrategy s)           = 0;
    virtual void setRetryOnFailure(bool retry)           = 0;
    virtual void setBackupOnAppStart(bool onStart)       = 0;

    virtual bool saveToFile(const std::string& path) const = 0;
    virtual bool loadFromFile(const std::string& path)     = 0;

    using ObserverCallback = std::function<void()>;
    virtual void addObserver(ObserverCallback callback) = 0;
};

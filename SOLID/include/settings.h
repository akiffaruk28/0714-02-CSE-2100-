#pragma once
#include <string>

// ISP: Separate interface for settings persistence
class ISettingsRepository {
public:
    virtual ~ISettingsRepository() = default;
    virtual bool load() = 0;
    virtual bool save() = 0;
};

// SRP: Plain data structure — no logic, only holds configuration values
struct BackupConfig {
    std::string backupDestination = "C:\\Backups";
    bool        autoBackup        = true;
    int         backupInterval    = 300;   // seconds
    int         maxCopies         = 10;
    bool        backupSubfolders  = true;
    bool        includeHidden     = false;
    bool        showNotifications = true;
};

// SRP: SettingsManager only reads/writes config — does NOT apply it
// DIP: Depends on ISettingsRepository abstraction for persistence
class SettingsManager : public ISettingsRepository {
public:
    explicit SettingsManager(const std::string& configFilePath);

    bool load() override;
    bool save() override;

    BackupConfig&       getConfig();
    const BackupConfig& getConfig() const;
    void                setConfig(const BackupConfig& config);

private:
    std::string  m_configFilePath;
    BackupConfig m_config;
};

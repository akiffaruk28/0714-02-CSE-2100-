#pragma once
#include "ISettingsModel.h"
#include <vector>

class SettingsModel : public ISettingsModel {
private:
    BackupSettings m_settings;
    std::vector<ObserverCallback> m_observers;

    void notify() { for (auto& cb : m_observers) cb(); }

public:
    BackupSettings getSettings() const override { return m_settings; }

    void setDestination(const std::string& dest)  override { m_settings.destination      = dest;    notify(); }
    void setBackupName(const std::string& name)   override { m_settings.backupName        = name;    notify(); }
    void setAutoBackup(bool enabled)              override { m_settings.autoBackup        = enabled; notify(); }
    void setInterval(int seconds)                 override { m_settings.interval          = seconds; notify(); }
    void setIncludeSubfolders(bool include)       override { m_settings.includeSubfolders = include; notify(); }
    void setIncludeHidden(bool include)           override { m_settings.includeHidden     = include; notify(); }
    void setMaxCopies(int maxCopies)              override { m_settings.maxCopies         = maxCopies; notify(); }
    void setShowNotifications(bool show)          override { m_settings.showNotifications = show;    notify(); }
    void setStrategy(BackupStrategy s)            override { m_settings.strategy          = s;       notify(); }
    void setRetryOnFailure(bool retry)            override { m_settings.retryOnFailure    = retry;   notify(); }
    void setBackupOnAppStart(bool onStart)        override { m_settings.backupOnAppStart  = onStart; notify(); }

    void addObserver(ObserverCallback callback) override { m_observers.push_back(callback); }

    bool saveToFile(const std::string& path) const override;
    bool loadFromFile(const std::string& path) override;
};

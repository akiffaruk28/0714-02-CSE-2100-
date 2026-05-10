#pragma once
#include "../model/IBackupModel.h"
#include "../model/ISettingsModel.h"
#include "../view/IMainView.h"
#include "../core/BackupManager.h"
#include "../strategies/SimpleCopyStrategy.h"
#include "../strategies/IncrementalBackupStrategy.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <string>
#include <filesystem>

class BackupController : public IBackupObserver {
private:
    IBackupModel*    m_model;
    IMainView*       m_view;
    BackupManager*   m_backupManager;
    ISettingsModel*  m_settingsModel;
    std::string      m_settingsPath;

    SimpleCopyStrategy      m_fullCopyStrategy;
    IncrementalBackupStrategy m_incrementalStrategy;

    // ── Auto Backup Timer ─────────────────────────────────────────────────────
    std::thread       m_autoBackupThread;
    std::atomic<bool> m_autoBackupRunning{false};
    std::atomic<bool> m_stopTimer{false};
    std::atomic<int>  m_secondsUntilNext{0};

    // ── Retry state ───────────────────────────────────────────────────────────
    std::atomic<bool> m_lastBackupFailed{false};
    std::atomic<bool> m_retryPending{false};

    void startAutoBackupTimer();
    void stopAutoBackupTimer();
    void autoBackupLoop();
    void triggerBackup();           // dispatch a backup on GTK main thread
    void pruneOldBackups(const std::string& dest, int maxCopies);
    std::string makeBackupFolderName() const;
    IBackupStrategy* activeStrategy();  // returns strategy matching current settings

    void setupCallbacks();
    void setupModelObservers();
    void syncModelToView();

    void handleFileSelection(const std::vector<std::string>& paths, bool recursive);
    void handleRemoveSelected();
    void handleClearAll();
    void handleStartBackup();
    void handleViewLog();

public:
    BackupController(IBackupModel* model, IMainView* view,
                     BackupManager* backupMgr,
                     ISettingsModel* settingsModel,
                     const std::string& settingsPath = "");
    ~BackupController();

    // IBackupObserver
    void onProgress(int current, int total, const std::string& filename) override;
    void onComplete(int success, int total) override;
    void onError(const std::string& message) override;

    // Called from main() after construction if backupOnAppStart is set
    void runStartupBackupIfNeeded();
};

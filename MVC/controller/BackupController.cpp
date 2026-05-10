#include "BackupController.h"
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <ctime>

namespace fs = std::filesystem;

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string fmtCountdown(int secs) {
    if (secs <= 0) return "Auto backup: running now...";
    int h = secs / 3600, m = (secs % 3600) / 60, s = secs % 60;
    char buf[48];
    if (h > 0)
        snprintf(buf, sizeof(buf), "Next backup in %dh %02dm", h, m);
    else if (m > 0)
        snprintf(buf, sizeof(buf), "Next backup in %dm %02ds", m, s);
    else
        snprintf(buf, sizeof(buf), "Next backup in %ds", s);
    return buf;
}

// ── Constructor / Destructor ──────────────────────────────────────────────────

BackupController::BackupController(IBackupModel* model, IMainView* view,
                                   BackupManager* backupMgr,
                                   ISettingsModel* settingsModel,
                                   const std::string& settingsPath)
    : m_model(model), m_view(view), m_backupManager(backupMgr),
      m_settingsModel(settingsModel),
      m_settingsPath(settingsPath)
{
    setupCallbacks();
    setupModelObservers();
    m_backupManager->setObserver(this);

    // When settings change: save, swap strategy, restart timer
    m_settingsModel->addObserver([this]() {
        if (!m_settingsPath.empty())
            m_settingsModel->saveToFile(m_settingsPath);

        // Swap active strategy based on new setting
        m_backupManager->setStrategy(activeStrategy());

        stopAutoBackupTimer();
        if (m_settingsModel->getSettings().autoBackup) {
            startAutoBackupTimer();
        } else {
            g_idle_add([](gpointer data) -> gboolean {
                static_cast<BackupController*>(data)->m_view->updateCountdown("");
                return G_SOURCE_REMOVE;
            }, this);
        }
    });
}

BackupController::~BackupController() {
    stopAutoBackupTimer();
}

// ── Strategy selector ─────────────────────────────────────────────────────────

IBackupStrategy* BackupController::activeStrategy() {
    if (m_settingsModel->getSettings().strategy == BackupStrategy::Incremental)
        return &m_incrementalStrategy;
    return &m_fullCopyStrategy;
}

// ── Startup backup ────────────────────────────────────────────────────────────

void BackupController::runStartupBackupIfNeeded() {
    BackupSettings s = m_settingsModel->getSettings();
    if (!s.backupOnAppStart || s.destination.empty() || m_model->getItemCount() == 0)
        return;
    m_view->updateStatus("\xE2\x8F\xB1 Startup backup starting\xE2\x80\xA6", 0.0);
    triggerBackup();
}

// ── Auto Backup Timer ─────────────────────────────────────────────────────────

void BackupController::startAutoBackupTimer() {
    if (m_autoBackupRunning.load()) return;
    m_stopTimer.store(false);
    m_autoBackupRunning.store(true);
    m_autoBackupThread = std::thread([this]() { autoBackupLoop(); });
    m_autoBackupThread.detach();
}

void BackupController::stopAutoBackupTimer() {
    m_stopTimer.store(true);
    m_autoBackupRunning.store(false);
}

void BackupController::autoBackupLoop() {
    while (!m_stopTimer.load()) {

        // ── Retry wait: if previous backup failed, wait 60s before retrying ──
        if (m_retryPending.load()) {
            m_retryPending.store(false);
            for (int i = 60; i > 0 && !m_stopTimer.load(); --i) {
                int rem = i;
                struct Ctx { BackupController* c; int r; };
                auto* ctx = new Ctx{this, rem};
                g_idle_add([](gpointer d) -> gboolean {
                    auto* c = static_cast<Ctx*>(d);
                    c->c->m_view->updateCountdown("Retry in " + std::to_string(c->r) + "s\u2026");
                    delete c; return G_SOURCE_REMOVE;
                }, ctx);
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (m_stopTimer.load()) break;
            triggerBackup();
            // After retry, loop back to top (retryPending may be set again if it fails)
            continue;
        }

        // ── Normal interval countdown ─────────────────────────────────────────
        int intervalSec = m_settingsModel->getSettings().interval;
        if (intervalSec <= 0) intervalSec = 300;

        m_secondsUntilNext.store(intervalSec);

        while (m_secondsUntilNext.load() > 0 && !m_stopTimer.load()) {
            int remaining = m_secondsUntilNext.load();

            struct Ctx { BackupController* ctrl; int rem; };
            auto* ctx = new Ctx{this, remaining};
            g_idle_add([](gpointer data) -> gboolean {
                auto* c = static_cast<Ctx*>(data);
                c->ctrl->m_view->updateCountdown(fmtCountdown(c->rem));
                delete c;
                return G_SOURCE_REMOVE;
            }, ctx);

            std::this_thread::sleep_for(std::chrono::seconds(1));
            m_secondsUntilNext.fetch_sub(1);
        }

        if (m_stopTimer.load()) break;

        triggerBackup();

        // If backup failed and retryOnFailure is set, next loop iteration
        // will enter the retry block at the top.
        if (m_lastBackupFailed.load() &&
            m_settingsModel->getSettings().retryOnFailure) {
            m_retryPending.store(true);
            m_lastBackupFailed.store(false);
        }
    }
    m_autoBackupRunning.store(false);
}

// ── triggerBackup — GTK main-thread dispatch ──────────────────────────────────

void BackupController::triggerBackup() {
    g_idle_add([](gpointer data) -> gboolean {
        auto* ctrl = static_cast<BackupController*>(data);
        BackupSettings s = ctrl->m_settingsModel->getSettings();

        if (s.destination.empty() || ctrl->m_model->getItemCount() == 0)
            return G_SOURCE_REMOVE;
        if (ctrl->m_backupManager->isRunning())
            return G_SOURCE_REMOVE;

        // Reset incremental counters before each run
        ctrl->m_incrementalStrategy.resetCounters();

        ctrl->m_view->updateStatus("\xE2\x8F\xB1 Auto backup starting\xE2\x80\xA6", 0.0);
        ctrl->m_model->resetResults();
        ctrl->m_model->setBackupRunning(true);

        std::string folderName = ctrl->makeBackupFolderName();
        ctrl->m_backupManager->runBackup(
            ctrl->m_model->getItems(), s.destination,
            s.includeHidden, s.includeSubfolders, folderName);
        // NOTE: pruneOldBackups is now called in onComplete() to avoid race condition

        return G_SOURCE_REMOVE;
    }, this);
}

// ── Folder naming ─────────────────────────────────────────────────────────────

std::string BackupController::makeBackupFolderName() const {
    std::string customName = m_settingsModel->getSettings().backupName;
    if (!customName.empty()) return customName;
    time_t now = time(nullptr);
    char buf[32];
    strftime(buf, sizeof(buf), "Backup_%Y%m%d_%H%M%S", localtime(&now));
    return std::string(buf);
}

// ── Pruning ───────────────────────────────────────────────────────────────────

void BackupController::pruneOldBackups(const std::string& dest, int maxCopies) {
    if (maxCopies <= 0 || dest.empty()) return;
    try {
        std::vector<fs::directory_entry> dirs;
        for (const auto& e : fs::directory_iterator(dest)) {
            if (e.is_directory()) {
                std::string name = e.path().filename().string();
                if (name.rfind("Backup_", 0) == 0)
                    dirs.push_back(e);
            }
        }
        std::sort(dirs.begin(), dirs.end(),
            [](const fs::directory_entry& a, const fs::directory_entry& b) {
                return a.path().filename().string() < b.path().filename().string();
            });
        while ((int)dirs.size() > maxCopies) {
            fs::remove_all(dirs.front().path());
            dirs.erase(dirs.begin());
        }
    } catch (...) {}
}

// ── Callbacks / Observers ─────────────────────────────────────────────────────

void BackupController::setupCallbacks() {
    m_view->onFileSelected([this](const std::vector<std::string>& paths, bool recursive) {
        handleFileSelection(paths, recursive);
    });
    m_view->onRemoveSelected([this]() { handleRemoveSelected(); });
    m_view->onClearAll([this]()       { handleClearAll();       });
    m_view->onStartBackup([this]()    { handleStartBackup();    });
    m_view->onViewLog([this]()        { handleViewLog();        });
}

void BackupController::setupModelObservers() {
    m_model->addObserver([this]() { syncModelToView(); });
}

void BackupController::syncModelToView() {
    m_view->updateFileList(m_model->getItems());

    if (m_model->isBackupRunning()) {
        int current = m_model->getCurrentProgress();
        int total   = m_model->getTotalItems();
        if (total > 0)
            m_view->updateStatus("Backing up: " + m_model->getCurrentFilename(),
                                  static_cast<double>(current) / total);
    } else {
        int count = m_model->getItemCount();
        if (count > 0)
            m_view->updateStatus(std::to_string(count) + " items ready", 0.0);
        else
            m_view->updateStatus("\xE2\x9C\x93 Ready", 0.0);
    }
}

// ── Handlers ─────────────────────────────────────────────────────────────────

void BackupController::handleFileSelection(const std::vector<std::string>& paths, bool) {
    for (const auto& p : paths) m_model->addItem(p);
}

void BackupController::handleRemoveSelected() {
    std::string sel = m_view->getSelectedItem();
    if (!sel.empty()) {
        m_model->removeItem(sel);
        m_view->updateStatus("Removed: " + sel, 0.0);
    } else {
        m_view->showNotification("Notice", "Please select an item to remove.");
    }
}

void BackupController::handleClearAll() {
    m_model->clearItems();
    m_view->updateStatus("All items cleared", 0.0);
}

void BackupController::handleStartBackup() {
    if (m_model->isBackupRunning()) { m_view->showError("Backup already running"); return; }
    if (m_model->getItemCount() == 0) { m_view->showError("No items selected"); return; }
    std::string dest = m_settingsModel->getSettings().destination;
    if (dest.empty()) { m_view->showError("No backup destination configured"); return; }

    m_view->setBackupButtonEnabled(false);
    m_model->resetResults();
    m_model->setBackupRunning(true);
    m_incrementalStrategy.resetCounters();
    m_backupManager->setStrategy(activeStrategy());
    BackupSettings s = m_settingsModel->getSettings();
    m_backupManager->runBackup(m_model->getItems(), dest,
                               s.includeHidden, s.includeSubfolders,
                               makeBackupFolderName());
    // NOTE: pruneOldBackups is called in onComplete() to avoid race condition
}

void BackupController::handleViewLog() {
    std::string logPath = m_backupManager->getLastLogPath();
    if (logPath.empty()) { m_view->showNotification("Log", "No backup has been run yet."); return; }
    std::ifstream file(logPath);
    if (!file.is_open()) { m_view->showNotification("Log", "Log file not found:\n" + logPath); return; }
    std::string content((std::istreambuf_iterator<char>(file)), {});
    m_view->showLogDialog("\xF0\x9F\x93\x8B Backup Log", content);
}

// ── IBackupObserver ───────────────────────────────────────────────────────────

void BackupController::onProgress(int current, int total, const std::string& filename) {
    m_model->updateProgress(current, total, filename);
}

void BackupController::onComplete(int success, int total) {
    m_model->setBackupRunning(false);
    m_view->setBackupButtonEnabled(true);
    m_lastBackupFailed.store(false);

    // Prune old backups AFTER the new one is complete (fixes race condition)
    BackupSettings s = m_settingsModel->getSettings();
    pruneOldBackups(s.destination, s.maxCopies);

    // If incremental, show skip stats in the status bar
    std::string msg;
    if (s.strategy == BackupStrategy::Incremental) {
        msg = "Backup complete: " + std::to_string(m_incrementalStrategy.copied) +
              " copied, " + std::to_string(m_incrementalStrategy.skipped) + " skipped (up-to-date)";
    } else {
        msg = "Backup complete: " + std::to_string(success) + "/" +
              std::to_string(total) + " files backed up";
    }

    m_view->updateStatus(msg, 1.0);
    if (s.showNotifications)
        m_view->showNotification("Backup Complete", msg);
}

void BackupController::onError(const std::string& message) {
    m_model->setBackupRunning(false);
    m_view->setBackupButtonEnabled(true);
    m_lastBackupFailed.store(true);
    m_view->showError(message);
}

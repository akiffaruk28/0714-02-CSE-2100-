#include "backup.h"
#include "utils.h"
#include "fileops.h"
#include <fstream>
#include <ctime>

// ─── BackupManager ────────────────────────────────────────────────────────────
// SRP : orchestrates backup flow; file I/O delegated to IBackupStrategy
// DIP : depends on IBackupStrategy and IBackupObserver abstractions
// OCP : swap strategy (e.g. compressed backup) without touching this class

BackupManager::BackupManager(IBackupStrategy* strategy)
    : m_strategy(strategy)
    , m_observer(nullptr)
    , m_running(false)
{}

BackupManager::~BackupManager() = default;

void BackupManager::setObserver(IBackupObserver* observer) {
    m_observer = observer;
}

bool BackupManager::isRunning() const {
    return m_running;
}

bool BackupManager::runBackup(const std::vector<std::string>& items,
                              const std::string&              destination) {
    if (m_running || items.empty()) return false;

    m_running = true;

    const std::string backupDir = createBackupDirectory(destination);
    FileOperations::createDirectory(backupDir);

    const std::string logPath = Utils::joinPath(backupDir, "backup_log.txt");
    time_t now = time(nullptr);
    writeLog(logPath, "Backup started: " + Utils::formatTimestamp(now) + "\n");

    int total   = static_cast<int>(items.size());
    int success = 0;

    for (int i = 0; i < total; ++i) {
        const std::string& src      = items[i];
        const std::string  filename = Utils::baseName(src);
        const std::string  dest     = Utils::joinPath(backupDir, filename);

        if (m_observer)
            m_observer->onProgress(i + 1, total, filename);

        if (m_strategy->copyFile(src, dest)) {
            ++success;
            writeLog(logPath, "[OK]  " + src + " -> " + dest + "\n");
        } else {
            writeLog(logPath, "[ERR] " + src + "\n");
        }
    }

    writeLog(logPath, "\nBackup complete: " +
             Utils::intToStr(success) + "/" + Utils::intToStr(total) + " files\n");

    if (m_observer)
        m_observer->onComplete(success, total);

    m_running = false;
    return (success == total);
}

// ── private helpers ───────────────────────────────────────────────────────────

std::string BackupManager::createBackupDirectory(const std::string& destination) {
    return Utils::joinPath(destination, "Backup_" + Utils::currentTimestampForPath());
}

void BackupManager::writeLog(const std::string& logPath, const std::string& entry) {
    std::ofstream log(logPath, std::ios::app);
    if (log.is_open())
        log << entry;
}

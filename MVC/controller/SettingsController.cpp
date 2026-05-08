#include <string>
#include "SettingsController.h"
#include "../view/SettingsDialog.h"
#include "../view/MainView.h"

SettingsController::SettingsController(ISettingsModel* model, IMainView* view)
    : m_model(model), m_view(view)
{
    m_view->onOpenSettings([this]() { showSettingsDialog(); });
}

void SettingsController::showSettingsDialog() {
    BackupSettings current = m_model->getSettings();

    GtkWindow* parentWin = nullptr;
    if (auto* mv = dynamic_cast<MainView*>(m_view))
        parentWin = mv->getWindow();

    SettingsDialog::show(parentWin, current,
        [this](const std::string& dest, const std::string& backupName,
               bool autoBackup, int interval, int maxCopies,
               bool subfolders, bool hidden,
               BackupStrategy strategy, bool retryOnFailure, bool backupOnAppStart)
        {
            m_model->setDestination(dest);
            m_model->setBackupName(backupName);
            m_model->setAutoBackup(autoBackup);
            m_model->setInterval(interval);
            m_model->setMaxCopies(maxCopies);
            m_model->setIncludeSubfolders(subfolders);
            m_model->setIncludeHidden(hidden);
            m_model->setStrategy(strategy);
            m_model->setRetryOnFailure(retryOnFailure);
            m_model->setBackupOnAppStart(backupOnAppStart);

            if (autoBackup) {
                int mins = interval / 60;
                std::string ivStr = (mins < 60)
                    ? std::to_string(mins) + " min"
                    : std::to_string(mins / 60) + " hour" + (mins/60 != 1 ? "s" : "");

                std::string stratStr = (strategy == BackupStrategy::Incremental)
                    ? "Incremental" : "Full Copy";

                m_view->showNotification("Settings Saved",
                    "Auto Backup enabled every " + ivStr +
                    "\nStrategy: " + stratStr +
                    "\nMax copies: " + std::to_string(maxCopies) +
                    (retryOnFailure   ? "\nRetry on failure: ON"       : "") +
                    (backupOnAppStart ? "\nBackup on app start: ON"    : ""));
            } else {
                m_view->showNotification("Settings", "Settings saved.");
            }
        });
}

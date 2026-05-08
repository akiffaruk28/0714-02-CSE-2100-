#include <gtk/gtk.h>

// Model
#include "model/BackupModel.h"
#include "model/SettingsModel.h"

// View
#include "view/MainView.h"

// Controller
#include "controller/BackupController.h"
#include "controller/SettingsController.h"

// Core / Strategies
#include "core/BackupManager.h"
#include "strategies/SimpleCopyStrategy.h"
#include "strategies/IncrementalBackupStrategy.h"

#include <string>
#include <cstdlib>

static std::string getSettingsPath() {
#ifdef _WIN32
    const char* appdata = getenv("APPDATA");
    return appdata ? (std::string(appdata) + "\\smart_backup.conf") : "smart_backup.conf";
#else
    const char* home = getenv("HOME");
    return home ? (std::string(home) + "/.smart_backup.conf") : "smart_backup.conf";
#endif
}

static std::string getDefaultDest() {
#ifdef _WIN32
    const char* p = getenv("USERPROFILE");
    return p ? (std::string(p) + "\\Backups") : "C:\\Backups";
#else
    const char* p = getenv("HOME");
    return p ? (std::string(p) + "/Backups") : "/tmp/Backups";
#endif
}

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);

    // ── Model ─────────────────────────────────────────────────────────────────
    BackupModel   backupModel;
    SettingsModel settingsModel;

    std::string settingsPath = getSettingsPath();

    // Defaults (used on first run before config file exists)
    settingsModel.setDestination(getDefaultDest());
    settingsModel.setAutoBackup(false);
    settingsModel.setInterval(300);
    settingsModel.setMaxCopies(10);
    settingsModel.setIncludeSubfolders(true);
    settingsModel.setIncludeHidden(false);
    settingsModel.setShowNotifications(true);
    settingsModel.setStrategy(BackupStrategy::FullCopy);
    settingsModel.setRetryOnFailure(true);
    settingsModel.setBackupOnAppStart(false);

    // Load saved settings (observers don't fire yet — controller not created)
    settingsModel.loadFromFile(settingsPath);

    // ── View ──────────────────────────────────────────────────────────────────
    MainView mainView;

    // ── Strategy + BackupManager ──────────────────────────────────────────────
    // BackupController owns both strategy objects internally now.
    // BackupManager starts with a dummy pointer; controller sets correct one.
    SimpleCopyStrategy dummyStrategy;
    BackupManager      backupManager(&dummyStrategy);

    // ── Controllers ───────────────────────────────────────────────────────────
    BackupController backupController(
        &backupModel, &mainView,
        &backupManager, &dummyStrategy,
        &settingsModel,
        settingsPath
    );

    SettingsController settingsController(&settingsModel, &mainView);

    // Trigger startup backup if the user has that option enabled
    backupController.runStartupBackupIfNeeded();

    // ── Run ───────────────────────────────────────────────────────────────────
    mainView.show();
    gtk_main();

    return 0;
}

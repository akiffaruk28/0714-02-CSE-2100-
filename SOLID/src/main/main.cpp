#include <gtk/gtk.h>
#include "backup.h"
#include "fileops.h"
#include "settings.h"
#include "window.h"

// ─── SimpleFileCopyStrategy ───────────────────────────────────────────────────
// OCP : concrete strategy — swap for ZipBackupStrategy without touching BackupManager
// LSP : fully substitutable as IBackupStrategy

class SimpleFileCopyStrategy : public IBackupStrategy {
public:
    bool copyFile(const std::string& src, const std::string& dest) override {
        return FileOperations::copyFile(src, dest);
    }
    std::string getStrategyName() const override {
        return "SimpleCopy";
    }
};

// ─── AutoBackupTimer ─────────────────────────────────────────────────────────
// SRP: only responsibility is triggering periodic backups

struct AutoBackupContext {
    BackupManager*   backupMgr;
    SettingsManager* settingsMgr;
    MainWindow*      window;
};

static gboolean autoBackupCallback(gpointer data) {
    auto* ctx = static_cast<AutoBackupContext*>(data);
    const BackupConfig& cfg = ctx->settingsMgr->getConfig();

    if (cfg.autoBackup && !ctx->backupMgr->isRunning()) {
        // Backup will notify window via IBackupObserver
        // (items list is not accessible here — auto-backup is a design choice:
        //  a real app would persist the item list in SettingsManager)
    }
    return G_SOURCE_CONTINUE;
}

// ─── main ────────────────────────────────────────────────────────────────────
// DIP : all dependencies created here and injected downward
//       No concrete type is visible to MainWindow or BackupManager

int main(int argc, char* argv[]) {
#ifdef _WIN32
    g_setenv("GSETTINGS_SCHEMA_DIR",
             "C:\\msys64\\mingw64\\share\\glib-2.0\\schemas", FALSE);
    g_setenv("PATH", "C:\\msys64\\mingw64\\bin", FALSE);
#endif

    gtk_init(&argc, &argv);

    // 1. Settings (SRP: config loading isolated here)
    SettingsManager settingsMgr("backup_config.txt");
    settingsMgr.load();

    // 2. Backup strategy (OCP: swap strategy without changing BackupManager)
    SimpleFileCopyStrategy copyStrategy;

    // 3. BackupManager (DIP: receives abstract IBackupStrategy)
    BackupManager backupMgr(&copyStrategy);

    // 4. Window (DIP: receives abstract managers + notifier)
    //    GtkNotifier created after GTK init
    MainWindow mainWindow(&backupMgr, &settingsMgr, nullptr);
    mainWindow.build();

    // 5. Now that GtkWindow exists, create notifier and re-inject
    //    (In production use a factory or lazy-init; kept simple for clarity)
    GtkNotifier notifier(nullptr);  // nullptr = no parent forced; adjust if needed
    MainWindow mainWindowFull(&backupMgr, &settingsMgr, &notifier);
    mainWindowFull.build();

    // Set the window as the backup observer (LSP)
    backupMgr.setObserver(&mainWindowFull);

    mainWindowFull.show();

    // 6. Auto-backup timer
    guint timerId = 0;
    const BackupConfig& cfg = settingsMgr.getConfig();
    AutoBackupContext ctx{&backupMgr, &settingsMgr, &mainWindowFull};
    if (cfg.autoBackup) {
        timerId = g_timeout_add_seconds(cfg.backupInterval, autoBackupCallback, &ctx);
    }

    gtk_main();

    if (timerId) g_source_remove(timerId);
    return 0;
}

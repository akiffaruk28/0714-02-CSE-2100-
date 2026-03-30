#pragma once
#include <gtk/gtk.h>
#include <string>
#include "backup.h"
#include "settings.h"

// ISP: Separate notification interface
class INotifier {
public:
    virtual ~INotifier() = default;
    virtual void notify(const std::string& title, const std::string& message) = 0;
};

// LSP: GtkNotifier substitutable anywhere INotifier is expected
class GtkNotifier : public INotifier {
public:
    explicit GtkNotifier(GtkWindow* parent);
    void notify(const std::string& title, const std::string& message) override;
private:
    GtkWindow* m_parent;
};

// SRP: MainWindow only manages UI widgets and delegates to managers
// DIP: Receives BackupManager and SettingsManager via constructor (injected)
class MainWindow : public IBackupObserver {
public:
    MainWindow(BackupManager* backupMgr, SettingsManager* settingsMgr, INotifier* notifier);

    void build();
    void show();

    // IBackupObserver implementation (LSP: substitutable observer)
    void onProgress(int current, int total, const std::string& filename) override;
    void onComplete(int success, int total) override;
    void onError(const std::string& message) override;

    // GTK signal callbacks (static, forwarded to instance methods)
    static void cbAddFiles(GtkWidget* w, gpointer data);
    static void cbAddFolder(GtkWidget* w, gpointer data);
    static void cbRemoveSelected(GtkWidget* w, gpointer data);
    static void cbClearAll(GtkWidget* w, gpointer data);
    static void cbStartBackup(GtkWidget* w, gpointer data);
    static void cbOpenSettings(GtkWidget* w, gpointer data);
    static void cbViewLog(GtkWidget* w, gpointer data);

private:
    // Injected dependencies (DIP)
    BackupManager*  m_backupMgr;
    SettingsManager* m_settingsMgr;
    INotifier*       m_notifier;

    // GTK widgets
    GtkWidget*    m_window;
    GtkWidget*    m_progressBar;
    GtkWidget*    m_statusLabel;
    GtkListStore* m_itemsList;
    GtkWidget*    m_treeview;
    GtkWidget*    m_destEntry;   // kept for settings dialog

    // UI helpers
    void updateStatus(const std::string& msg, double fraction);
    void addFileRow(const std::string& path);
    void openSettingsDialog();
    void openLogViewer();
    std::vector<std::string> collectListItems();
};

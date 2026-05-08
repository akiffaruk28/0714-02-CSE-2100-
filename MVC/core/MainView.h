#pragma once
#include "IMainView.h"
#include <gtk/gtk.h>
#include <vector>
#include <string>

class MainView : public IMainView {
private:
    GtkWidget* m_window;
    GtkWidget* m_progressBar;
    GtkWidget* m_statusLabel;
    GtkListStore* m_listStore;
    GtkWidget* m_treeView;
    GtkWidget* m_startButton;
    GtkWidget* m_countdownLabel;
    GtkTreeSelection* m_selection;

    FileSelectedCallback   m_onFileSelected;
    RemoveSelectedCallback m_onRemoveSelected;
    ClearAllCallback       m_onClearAll;
    StartBackupCallback    m_onStartBackup;
    OpenSettingsCallback   m_onOpenSettings;
    ViewLogCallback        m_onViewLog;

    std::string formatSize(long long bytes);
    long long getFolderSize(const std::string& path);
    std::string lastModified(const std::string& path);
    std::vector<std::string> showFileChooserDialog();
    std::vector<std::string> showFolderDialog();
    void buildUI();

    // GTK static signal callbacks
    static void onAddFilesClicked(GtkWidget*, gpointer data);
    static void onAddFolderClicked(GtkWidget*, gpointer data);
    static void onRemoveSelectedClicked(GtkWidget*, gpointer data);
    static void onClearAllClicked(GtkWidget*, gpointer data);
    static void onStartBackupClicked(GtkWidget*, gpointer data);
    static void onSettingsClicked(GtkWidget*, gpointer data);
    static void onViewLogClicked(GtkWidget*, gpointer data);

public:
    MainView();

    void show() override;
    void updateStatus(const std::string& message, double progress) override;
    void updateFileList(const std::vector<std::string>& files) override;
    void showNotification(const std::string& title, const std::string& message) override;
    void showError(const std::string& error) override;
    void showLogDialog(const std::string& title, const std::string& content) override;
    void setBackupButtonEnabled(bool enabled) override;
    void updateCountdown(const std::string& countdownText) override;
    std::string getSelectedItem() override;

    // FIX 5: SettingsController-কে parent window দেওয়ার জন্য
    GtkWindow* getWindow() const { return GTK_WINDOW(m_window); }

    void onFileSelected(FileSelectedCallback callback) override;
    void onRemoveSelected(RemoveSelectedCallback callback) override;
    void onClearAll(ClearAllCallback callback) override;
    void onStartBackup(StartBackupCallback callback) override;
    void onOpenSettings(OpenSettingsCallback callback) override;
    void onViewLog(ViewLogCallback callback) override;
};

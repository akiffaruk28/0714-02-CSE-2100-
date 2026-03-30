#include "window.h"
#include "fileops.h"
#include "utils.h"
#include <gtk/gtk.h>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>

// ─── GtkNotifier ─────────────────────────────────────────────────────────────
// LSP: substitutable wherever INotifier is required

GtkNotifier::GtkNotifier(GtkWindow* parent) : m_parent(parent) {}

void GtkNotifier::notify(const std::string& title, const std::string& message) {
    GtkWidget* dlg = gtk_message_dialog_new(
        m_parent,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "%s", message.c_str());
    gtk_window_set_title(GTK_WINDOW(dlg), title.c_str());
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

// ─── MainWindow ──────────────────────────────────────────────────────────────
// SRP : only builds UI and forwards events to injected managers
// DIP : depends on BackupManager, SettingsManager, INotifier abstractions

MainWindow::MainWindow(BackupManager*  backupMgr,
                       SettingsManager* settingsMgr,
                       INotifier*       notifier)
    : m_backupMgr(backupMgr)
    , m_settingsMgr(settingsMgr)
    , m_notifier(notifier)
    , m_window(nullptr)
    , m_progressBar(nullptr)
    , m_statusLabel(nullptr)
    , m_itemsList(nullptr)
    , m_treeview(nullptr)
    , m_destEntry(nullptr)
{}

// ── IBackupObserver (LSP) ─────────────────────────────────────────────────────

void MainWindow::onProgress(int current, int total, const std::string& filename) {
    std::string msg = "Backing up (" + Utils::intToStr(current) + "/" +
                      Utils::intToStr(total) + "): " + filename;
    updateStatus(msg, static_cast<double>(current) / total);
}

void MainWindow::onComplete(int success, int total) {
    std::string msg = "Backup complete: " + Utils::intToStr(success) +
                      "/" + Utils::intToStr(total) + " files backed up";
    updateStatus(msg, 1.0);
    m_notifier->notify("Backup Complete", msg);
}

void MainWindow::onError(const std::string& message) {
    updateStatus("Error: " + message, 0.0);
    m_notifier->notify("Backup Error", message);
}

// ── Build UI ──────────────────────────────────────────────────────────────────

void MainWindow::build() {
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(m_window), "Smart Backup Utility");
    gtk_window_set_default_size(GTK_WINDOW(m_window), 800, 600);
    gtk_window_set_position(GTK_WINDOW(m_window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(m_window), 10);
    g_signal_connect(m_window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(m_window), mainBox);

    // Header
    GtkWidget* header = gtk_label_new("SMART BACKUP UTILITY");
    gtk_box_pack_start(GTK_BOX(mainBox), header, FALSE, FALSE, 0);

    // Progress bar
    m_progressBar = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(mainBox), m_progressBar, FALSE, FALSE, 0);

    // Status label
    m_statusLabel = gtk_label_new("Ready");
    gtk_widget_set_halign(m_statusLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(mainBox), m_statusLabel, FALSE, FALSE, 0);

    // Scrollable file list
    GtkWidget* scrolled = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(scrolled, TRUE);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_pack_start(GTK_BOX(mainBox), scrolled, TRUE, TRUE, 0);

    m_itemsList = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    m_treeview  = gtk_tree_view_new_with_model(GTK_TREE_MODEL(m_itemsList));

    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();

    GtkTreeViewColumn* col = gtk_tree_view_column_new_with_attributes(
        "File/Folder", renderer, "text", 0, nullptr);
    gtk_tree_view_column_set_expand(col, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_treeview), col);

    col = gtk_tree_view_column_new_with_attributes("Size",     renderer, "text", 1, nullptr);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_treeview), col);

    col = gtk_tree_view_column_new_with_attributes("Modified", renderer, "text", 2, nullptr);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_treeview), col);

    gtk_container_add(GTK_CONTAINER(scrolled), m_treeview);

    // File buttons row
    GtkWidget* btnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign(btnBox, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(mainBox), btnBox, FALSE, FALSE, 0);

    auto makeBtn = [&](const char* label, GCallback cb) {
        GtkWidget* b = gtk_button_new_with_label(label);
        g_signal_connect(b, "clicked", cb, this);
        gtk_box_pack_start(GTK_BOX(btnBox), b, TRUE, TRUE, 0);
    };

    makeBtn("Add Files/Folders", G_CALLBACK(cbAddFiles));
    makeBtn("Add Folder Only",   G_CALLBACK(cbAddFolder));
    makeBtn("Remove Selected",   G_CALLBACK(cbRemoveSelected));
    makeBtn("Clear All",         G_CALLBACK(cbClearAll));

    // Action buttons row
    GtkWidget* actBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(actBox, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(actBox, 10);
    gtk_box_pack_start(GTK_BOX(mainBox), actBox, FALSE, FALSE, 0);

    auto makeActBtn = [&](const char* label, GCallback cb) {
        GtkWidget* b = gtk_button_new_with_label(label);
        gtk_widget_set_size_request(b, 140, 40);
        g_signal_connect(b, "clicked", cb, this);
        gtk_box_pack_start(GTK_BOX(actBox), b, FALSE, FALSE, 0);
    };

    makeActBtn("Start Backup", G_CALLBACK(cbStartBackup));
    makeActBtn("Settings",     G_CALLBACK(cbOpenSettings));
    makeActBtn("View Log",     G_CALLBACK(cbViewLog));
}

void MainWindow::show() {
    gtk_widget_show_all(m_window);
}

// ── Static callbacks ──────────────────────────────────────────────────────────

void MainWindow::cbAddFiles(GtkWidget*, gpointer data) {
    auto* self = static_cast<MainWindow*>(data);
    const BackupConfig& cfg = self->m_settingsMgr->getConfig();

    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Select Files and Folders", GTK_WINDOW(self->m_window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Add",    GTK_RESPONSE_ACCEPT, nullptr);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

    GtkWidget* recurseChk = gtk_check_button_new_with_label("Include subfolders recursively");
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), recurseChk);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gboolean recursive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(recurseChk))
                             && cfg.backupSubfolders;
        GSList* files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));

        for (GSList* it = files; it; it = it->next) {
            std::string path = static_cast<char*>(it->data);
            if (FileOperations::isDirectory(path)) {
                RecursiveFileScanner scanner;
                auto items = scanner.scan(path, recursive, cfg.includeHidden);
                for (auto& f : items) self->addFileRow(f);
            } else {
                self->addFileRow(path);
            }
            g_free(it->data);
        }
        g_slist_free(files);

        int count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(self->m_itemsList), nullptr);
        self->updateStatus(Utils::intToStr(count) + " items ready for backup", 0.0);
    }
    gtk_widget_destroy(dialog);
}

void MainWindow::cbAddFolder(GtkWidget*, gpointer data) {
    auto* self = static_cast<MainWindow*>(data);
    const BackupConfig& cfg = self->m_settingsMgr->getConfig();

    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Select Folder to Backup", GTK_WINDOW(self->m_window),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Select", GTK_RESPONSE_ACCEPT, nullptr);

    GtkWidget* recurseChk = gtk_check_button_new_with_label("Include subfolders");
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), recurseChk);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gboolean recursive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(recurseChk))
                             && cfg.backupSubfolders;
        char* folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (folder) {
            RecursiveFileScanner scanner;
            auto items = scanner.scan(std::string(folder), recursive, cfg.includeHidden);
            for (auto& f : items) self->addFileRow(f);
            g_free(folder);
        }
        int count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(self->m_itemsList), nullptr);
        self->updateStatus(Utils::intToStr(count) + " items ready for backup", 0.0);
    }
    gtk_widget_destroy(dialog);
}

void MainWindow::cbRemoveSelected(GtkWidget*, gpointer data) {
    auto* self = static_cast<MainWindow*>(data);
    GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->m_treeview));
    GtkTreeModel* model;
    GtkTreeIter   iter;
    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
        int count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(self->m_itemsList), nullptr);
        self->updateStatus(Utils::intToStr(count) + " items remaining", 0.0);
    } else {
        self->m_notifier->notify("Notice", "Please select an item to remove");
    }
}

void MainWindow::cbClearAll(GtkWidget*, gpointer data) {
    auto* self = static_cast<MainWindow*>(data);
    gtk_list_store_clear(self->m_itemsList);
    self->updateStatus("List cleared", 0.0);
}

void MainWindow::cbStartBackup(GtkWidget*, gpointer data) {
    auto* self = static_cast<MainWindow*>(data);
    if (self->m_backupMgr->isRunning()) {
        self->m_notifier->notify("Busy", "Backup already in progress");
        return;
    }
    auto items = self->collectListItems();
    const std::string& dest = self->m_settingsMgr->getConfig().backupDestination;
    self->m_backupMgr->runBackup(items, dest);
}

void MainWindow::cbOpenSettings(GtkWidget*, gpointer data) {
    auto* self = static_cast<MainWindow*>(data);
    self->openSettingsDialog();
}

void MainWindow::cbViewLog(GtkWidget*, gpointer data) {
    auto* self = static_cast<MainWindow*>(data);
    self->openLogViewer();
}

// ── Private helpers ───────────────────────────────────────────────────────────

void MainWindow::updateStatus(const std::string& msg, double fraction) {
    gtk_label_set_text(GTK_LABEL(m_statusLabel), msg.c_str());
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(m_progressBar), fraction);
    while (gtk_events_pending()) gtk_main_iteration();
}

void MainWindow::addFileRow(const std::string& path) {
    // Prevent duplicates
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(m_itemsList), &iter)) {
        do {
            char* existing;
            gtk_tree_model_get(GTK_TREE_MODEL(m_itemsList), &iter, 0, &existing, -1);
            bool dup = (path == existing);
            g_free(existing);
            if (dup) return;
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(m_itemsList), &iter));
    }

    long sz = FileOperations::fileSize(path);
    std::string sizeStr = (sz >= 0) ? FileOperations::formatSize(sz) : "?";
    std::string modStr  = FileOperations::lastModified(path);

    gtk_list_store_append(m_itemsList, &iter);
    gtk_list_store_set(m_itemsList, &iter,
                       0, path.c_str(),
                       1, sizeStr.c_str(),
                       2, modStr.c_str(),
                       -1);
}

std::vector<std::string> MainWindow::collectListItems() {
    std::vector<std::string> items;
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(m_itemsList), &iter))
        return items;
    do {
        char* path;
        gtk_tree_model_get(GTK_TREE_MODEL(m_itemsList), &iter, 0, &path, -1);
        items.push_back(path);
        g_free(path);
    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(m_itemsList), &iter));
    return items;
}

void MainWindow::openSettingsDialog() {
    BackupConfig cfg = m_settingsMgr->getConfig();  // local copy

    GtkWidget* dlg = gtk_dialog_new_with_buttons(
        "Backup Settings", GTK_WINDOW(m_window), GTK_DIALOG_MODAL,
        "_Save", GTK_RESPONSE_ACCEPT,
        "_Cancel", GTK_RESPONSE_REJECT, nullptr);
    gtk_window_set_default_size(GTK_WINDOW(dlg), 500, 400);

    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 20);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dlg))), grid);

    int row = 0;
    auto addLabel = [&](const char* text, int r) {
        GtkWidget* lbl = gtk_label_new(text);
        gtk_widget_set_halign(lbl, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), lbl, 0, r, 1, 1);
    };

    // Destination
    addLabel("Backup Destination:", row);
    GtkWidget* destBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_grid_attach(GTK_GRID(grid), destBox, 1, row, 2, 1);
    m_destEntry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(m_destEntry), cfg.backupDestination.c_str());
    gtk_widget_set_hexpand(m_destEntry, TRUE);
    gtk_box_pack_start(GTK_BOX(destBox), m_destEntry, TRUE, TRUE, 0);
    GtkWidget* browseBtn = gtk_button_new_with_label("Browse...");
    g_signal_connect(browseBtn, "clicked", G_CALLBACK(+[](GtkWidget*, gpointer d) {
        auto* self = static_cast<MainWindow*>(d);
        GtkWidget* fd = gtk_file_chooser_dialog_new(
            "Select Backup Destination", GTK_WINDOW(self->m_window),
            GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
            "_Cancel", GTK_RESPONSE_CANCEL, "_Select", GTK_RESPONSE_ACCEPT, nullptr);
        gtk_file_chooser_set_create_folders(GTK_FILE_CHOOSER(fd), TRUE);
        if (gtk_dialog_run(GTK_DIALOG(fd)) == GTK_RESPONSE_ACCEPT) {
            char* f = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fd));
            if (f) { gtk_entry_set_text(GTK_ENTRY(self->m_destEntry), f); g_free(f); }
        }
        gtk_widget_destroy(fd);
    }), this);
    gtk_box_pack_start(GTK_BOX(destBox), browseBtn, FALSE, FALSE, 0);
    row++;

    // Auto backup
    addLabel("Auto Backup:", row);
    GtkWidget* autoSw = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(autoSw), cfg.autoBackup);
    gtk_grid_attach(GTK_GRID(grid), autoSw, 1, row++, 1, 1);

    // Interval
    addLabel("Interval (seconds):", row);
    GtkWidget* intervalSpin = gtk_spin_button_new_with_range(60, 86400, 60);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(intervalSpin), cfg.backupInterval);
    gtk_grid_attach(GTK_GRID(grid), intervalSpin, 1, row++, 1, 1);

    // Max copies
    addLabel("Max Copies:", row);
    GtkWidget* copiesSpin = gtk_spin_button_new_with_range(1, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(copiesSpin), cfg.maxCopies);
    gtk_grid_attach(GTK_GRID(grid), copiesSpin, 1, row++, 1, 1);

    // Subfolders
    addLabel("Include Subfolders:", row);
    GtkWidget* subSw = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(subSw), cfg.backupSubfolders);
    gtk_grid_attach(GTK_GRID(grid), subSw, 1, row++, 1, 1);

    // Hidden files
    addLabel("Include Hidden:", row);
    GtkWidget* hiddenSw = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(hiddenSw), cfg.includeHidden);
    gtk_grid_attach(GTK_GRID(grid), hiddenSw, 1, row++, 1, 1);

    gtk_widget_show_all(dlg);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        cfg.backupDestination = gtk_entry_get_text(GTK_ENTRY(m_destEntry));
        cfg.autoBackup        = gtk_switch_get_active(GTK_SWITCH(autoSw));
        cfg.backupInterval    = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(intervalSpin));
        cfg.maxCopies         = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(copiesSpin));
        cfg.backupSubfolders  = gtk_switch_get_active(GTK_SWITCH(subSw));
        cfg.includeHidden     = gtk_switch_get_active(GTK_SWITCH(hiddenSw));

        m_settingsMgr->setConfig(cfg);
        m_settingsMgr->save();
        FileOperations::createDirectory(cfg.backupDestination);
        updateStatus("Settings saved", 0.0);
    }
    gtk_widget_destroy(dlg);
}

void MainWindow::openLogViewer() {
    GtkWidget* dlg = gtk_dialog_new_with_buttons(
        "Backup History", GTK_WINDOW(m_window),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_CLOSE, nullptr);
    gtk_window_set_default_size(GTK_WINDOW(dlg), 600, 400);

    GtkWidget* scrolled = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_set_border_width(GTK_CONTAINER(scrolled), 10);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkWidget* tv = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tv), GTK_WRAP_WORD);
    gtk_container_add(GTK_CONTAINER(scrolled), tv);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dlg))),
                       scrolled, TRUE, TRUE, 0);

    // Find latest backup log
    const std::string& dest = m_settingsMgr->getConfig().backupDestination;
    DIR* dir = opendir(dest.c_str());
    std::string latestLog;
    time_t latestTime = 0;

    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (Utils::startsWith(entry->d_name, "Backup_")) {
                std::string logPath = Utils::joinPath(
                    Utils::joinPath(dest, entry->d_name), "backup_log.txt");
                struct stat st;
                if (stat(logPath.c_str(), &st) == 0 && st.st_mtime > latestTime) {
                    latestTime = st.st_mtime;
                    latestLog  = logPath;
                }
            }
        }
        closedir(dir);
    }

    GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    if (!latestLog.empty()) {
        std::ifstream log(latestLog);
        if (log.is_open()) {
            std::ostringstream oss;
            oss << log.rdbuf();
            gtk_text_buffer_set_text(buf, oss.str().c_str(), -1);
        }
    } else {
        gtk_text_buffer_set_text(buf, "No backup logs found", -1);
    }

    gtk_widget_show_all(dlg);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

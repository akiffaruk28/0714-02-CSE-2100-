#include "MainView.h"
#include <sstream>
#include <cstring>

#ifdef _WIN32
    #include <sys/stat.h>
    #include <windows.h>
    #include <commdlg.h>
    #include <shlobj.h>
    // Windows stat helpers — use _stat64 to avoid macro clashes
    static int platform_stat(const char* path, struct _stat64* st) {
        return _stat64(path, st);
    }
    static bool platform_isdir(const struct _stat64& st) {
        return (st.st_mode & _S_IFMT) == _S_IFDIR;
    }
    static long long platform_size(const struct _stat64& st) {
        return (long long)st.st_size;
    }
    static time_t platform_mtime(const struct _stat64& st) {
        return (time_t)st.st_mtime;
    }
#else
    #include <sys/stat.h>
    #include <dirent.h>
    static int platform_stat(const char* path, struct stat* st) {
        return ::stat(path, st);
    }
    static bool platform_isdir(const struct stat& st) {
        return S_ISDIR(st.st_mode);
    }
    static long long platform_size(const struct stat& st) {
        return (long long)st.st_size;
    }
    static time_t platform_mtime(const struct stat& st) {
        return st.st_mtime;
    }
#endif

// ── Helpers ────────────────────────────────────────────────────────────────

std::string MainView::formatSize(long long bytes) {
    std::ostringstream oss;
    oss.precision(1);
    oss << std::fixed;
    if      (bytes < 1024LL)              oss << bytes            << " B";
    else if (bytes < 1024LL*1024)         oss << bytes/1024.0     << " KB";
    else if (bytes < 1024LL*1024*1024)    oss << bytes/1048576.0  << " MB";
    else                                   oss << bytes/1073741824.0 << " GB";
    return oss.str();
}

long long MainView::getFolderSize(const std::string& path) {
    long long total = 0;
#ifdef _WIN32
    std::string searchPath = path + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return 0;
    do {
        std::string name = findData.cFileName;
        if (name == "." || name == "..") continue;
        std::string fullPath = path + "\\" + name;
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            total += getFolderSize(fullPath);
        } else {
            LARGE_INTEGER sz;
            sz.LowPart  = findData.nFileSizeLow;
            sz.HighPart = (LONG)findData.nFileSizeHigh;
            total += (long long)sz.QuadPart;
        }
    } while (FindNextFileA(hFind, &findData));
    FindClose(hFind);
#else
    DIR* dir = opendir(path.c_str());
    if (!dir) return 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        std::string fullPath = path + "/" + name;
        struct stat st;
        if (::stat(fullPath.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) total += getFolderSize(fullPath);
            else                     total += (long long)st.st_size;
        }
    }
    closedir(dir);
#endif
    return total;
}

std::string MainView::lastModified(const std::string& path) {
#ifdef _WIN32
    struct _stat64 st;
    if (_stat64(path.c_str(), &st) != 0) return "";
    time_t t = (time_t)st.st_mtime;
#else
    struct stat st;
    if (::stat(path.c_str(), &st) != 0) return "";
    time_t t = st.st_mtime;
#endif
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", localtime(&t));
    return buf;
}

std::vector<std::string> MainView::showFileChooserDialog() {
    std::vector<std::string> paths;
#ifdef _WIN32
    OPENFILENAMEW ofn;
    wchar_t szFile[32768] = {0};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = NULL;
    ofn.lpstrFile    = szFile;
    ofn.nMaxFile     = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter  = L"All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |
                       OFN_ALLOWMULTISELECT | OFN_EXPLORER;
    if (GetOpenFileNameW(&ofn)) {
        wchar_t* p = szFile;
        std::wstring dir(p);
        p += dir.size() + 1;
        if (*p == 0) {
            paths.push_back(std::string(dir.begin(), dir.end()));
        } else {
            while (*p) {
                std::wstring file(p);
                std::wstring full = dir + L"\\" + file;
                paths.push_back(std::string(full.begin(), full.end()));
                p += file.size() + 1;
            }
        }
    }
#else
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Select Files", GTK_WINDOW(m_window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Add",    GTK_RESPONSE_ACCEPT, nullptr);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        GSList* files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        for (GSList* it = files; it; it = it->next) {
            paths.push_back(static_cast<char*>(it->data));
            g_free(it->data);
        }
        g_slist_free(files);
    }
    gtk_widget_destroy(dialog);
#endif
    return paths;
}

std::vector<std::string> MainView::showFolderDialog() {
    std::vector<std::string> paths;
#ifdef _WIN32
    BROWSEINFOW bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.ulFlags   = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpszTitle = L"Select Backup Folder";
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, path)) {
            paths.push_back(std::string(path, path + wcslen(path)));
        }
        CoTaskMemFree(pidl);
    }
#else
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Select Folder", GTK_WINDOW(m_window),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Select", GTK_RESPONSE_ACCEPT, nullptr);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (folder) { paths.push_back(folder); g_free(folder); }
    }
    gtk_widget_destroy(dialog);
#endif
    return paths;
}

// ── Static GTK callbacks ───────────────────────────────────────────────────

void MainView::onAddFilesClicked(GtkWidget*, gpointer data) {
    auto* self = static_cast<MainView*>(data);
    if (self->m_onFileSelected) {
        auto paths = self->showFileChooserDialog();
        if (!paths.empty()) self->m_onFileSelected(paths, false);
    }
}

void MainView::onAddFolderClicked(GtkWidget*, gpointer data) {
    auto* self = static_cast<MainView*>(data);
    if (self->m_onFileSelected) {
        auto paths = self->showFolderDialog();
        if (!paths.empty()) self->m_onFileSelected(paths, true);
    }
}

void MainView::onRemoveSelectedClicked(GtkWidget*, gpointer data) {
    auto* self = static_cast<MainView*>(data);
    if (self->m_onRemoveSelected) self->m_onRemoveSelected();
}

void MainView::onClearAllClicked(GtkWidget*, gpointer data) {
    auto* self = static_cast<MainView*>(data);
    if (self->m_onClearAll) self->m_onClearAll();
}

void MainView::onStartBackupClicked(GtkWidget*, gpointer data) {
    auto* self = static_cast<MainView*>(data);
    if (self->m_onStartBackup) self->m_onStartBackup();
}

void MainView::onSettingsClicked(GtkWidget*, gpointer data) {
    auto* self = static_cast<MainView*>(data);
    if (self->m_onOpenSettings) self->m_onOpenSettings();
}

void MainView::onViewLogClicked(GtkWidget*, gpointer data) {
    auto* self = static_cast<MainView*>(data);
    if (self->m_onViewLog) self->m_onViewLog();
}

// ── UI builder ────────────────────────────────────────────────────────────

void MainView::buildUI() {
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(m_window), "Smart Backup - MVC Edition");
    gtk_window_set_default_size(GTK_WINDOW(m_window), 900, 600);
    gtk_window_set_position(GTK_WINDOW(m_window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(m_window), 10);
    g_signal_connect(m_window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(m_window), mainBox);

    // Header
    GtkWidget* header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header),
        "<span size='x-large' weight='bold'>\xF0\x9F\x93\xA6 SMART BACKUP UTILITY</span>\n"
        "<span size='small'>MVC + SOLID Architecture</span>");
    gtk_box_pack_start(GTK_BOX(mainBox), header, FALSE, FALSE, 0);

    // Progress bar + status label
    m_progressBar = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(mainBox), m_progressBar, FALSE, FALSE, 0);

    // Status row: status label left, countdown label right
    GtkWidget* statusRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(mainBox), statusRow, FALSE, FALSE, 0);

    m_statusLabel = gtk_label_new("\xe2\x9c\x93 Ready");
    gtk_widget_set_halign(m_statusLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(statusRow), m_statusLabel, TRUE, TRUE, 0);

    m_countdownLabel = gtk_label_new("");
    gtk_widget_set_halign(m_countdownLabel, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(statusRow), m_countdownLabel, FALSE, FALSE, 4);

    // File list (scrolled tree view)
    GtkWidget* scrolled = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(scrolled, TRUE);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_pack_start(GTK_BOX(mainBox), scrolled, TRUE, TRUE, 0);

    m_listStore = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    m_treeView  = gtk_tree_view_new_with_model(GTK_TREE_MODEL(m_listStore));
    m_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(m_treeView));

    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn* col;

    col = gtk_tree_view_column_new_with_attributes("File/Folder", renderer, "text", 0, nullptr);
    gtk_tree_view_column_set_expand(col, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_treeView), col);

    col = gtk_tree_view_column_new_with_attributes("Size", renderer, "text", 1, nullptr);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_treeView), col);

    col = gtk_tree_view_column_new_with_attributes("Modified", renderer, "text", 2, nullptr);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_treeView), col);

    gtk_container_add(GTK_CONTAINER(scrolled), m_treeView);

    // File action buttons
    GtkWidget* btnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign(btnBox, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(mainBox), btnBox, FALSE, FALSE, 0);

    struct { const char* label; GCallback cb; } btns[] = {
        { "\xF0\x9F\x93\x81 Add Files",  G_CALLBACK(onAddFilesClicked)      },
        { "\xF0\x9F\x93\x82 Add Folder", G_CALLBACK(onAddFolderClicked)     },
        { "\xe2\x9d\x8c Remove",         G_CALLBACK(onRemoveSelectedClicked) },
        { "\xF0\x9F\x97\x91 Clear All",  G_CALLBACK(onClearAllClicked)      },
    };
    for (auto& b : btns) {
        GtkWidget* btn = gtk_button_new_with_label(b.label);
        g_signal_connect(btn, "clicked", b.cb, this);
        gtk_box_pack_start(GTK_BOX(btnBox), btn, TRUE, TRUE, 0);
    }

    // Main action buttons
    GtkWidget* actBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(actBox, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(actBox, 10);
    gtk_box_pack_start(GTK_BOX(mainBox), actBox, FALSE, FALSE, 0);

    m_startButton = gtk_button_new_with_label("\xe2\x96\xb6 START BACKUP");
    gtk_widget_set_size_request(m_startButton, 160, 45);
    g_signal_connect(m_startButton, "clicked", G_CALLBACK(onStartBackupClicked), this);
    gtk_box_pack_start(GTK_BOX(actBox), m_startButton, FALSE, FALSE, 0);

    GtkWidget* settingsBtn = gtk_button_new_with_label("\xe2\x9a\x99 SETTINGS");
    gtk_widget_set_size_request(settingsBtn, 160, 45);
    g_signal_connect(settingsBtn, "clicked", G_CALLBACK(onSettingsClicked), this);
    gtk_box_pack_start(GTK_BOX(actBox), settingsBtn, FALSE, FALSE, 0);

    GtkWidget* logBtn = gtk_button_new_with_label("\xF0\x9F\x93\x8B VIEW LOG");
    gtk_widget_set_size_request(logBtn, 160, 45);
    g_signal_connect(logBtn, "clicked", G_CALLBACK(onViewLogClicked), this);
    gtk_box_pack_start(GTK_BOX(actBox), logBtn, FALSE, FALSE, 0);
}

// ── Constructor & public methods ───────────────────────────────────────────

MainView::MainView() { buildUI(); }

void MainView::show() { gtk_widget_show_all(m_window); }

void MainView::updateStatus(const std::string& message, double progress) {
    gtk_label_set_text(GTK_LABEL(m_statusLabel), message.c_str());
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(m_progressBar), progress);
}

void MainView::updateFileList(const std::vector<std::string>& files) {
    gtk_list_store_clear(m_listStore);
    for (const auto& file : files) {
        GtkTreeIter iter;
        gtk_list_store_append(m_listStore, &iter);

        long long size = -1;
        bool isDir = false;

#ifdef _WIN32
        struct _stat64 st;
        if (_stat64(file.c_str(), &st) == 0) {
            isDir = ((st.st_mode & _S_IFMT) == _S_IFDIR);
            size  = isDir ? getFolderSize(file) : (long long)st.st_size;
        }
#else
        struct stat st;
        if (::stat(file.c_str(), &st) == 0) {
            isDir = S_ISDIR(st.st_mode);
            size  = isDir ? getFolderSize(file) : (long long)st.st_size;
        }
#endif

        std::string sizeStr = (size >= 0) ? formatSize(size) : "?";
        std::string modStr  = lastModified(file);

        gtk_list_store_set(m_listStore, &iter,
                           0, file.c_str(),
                           1, sizeStr.c_str(),
                           2, modStr.c_str(), -1);
    }
}

void MainView::showNotification(const std::string& title, const std::string& message) {
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK, "%s", message.c_str());
    gtk_window_set_title(GTK_WINDOW(dialog), title.c_str());
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void MainView::showLogDialog(const std::string& title, const std::string& content) {
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        title.c_str(), GTK_WINDOW(m_window),
        GTK_DIALOG_MODAL,
        "_Close", GTK_RESPONSE_CLOSE, nullptr);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 700, 480);

    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(contentArea), 0);

    // Dark header bar
    GtkWidget* headerLabel = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(headerLabel),
        "<span font_family='monospace' size='small' foreground='#aaaaaa'>"
        "  Backup Log Viewer  </span>");
    gtk_widget_set_halign(headerLabel, GTK_ALIGN_START);
    GtkWidget* headerBox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(headerBox), headerLabel);
    GdkRGBA headerBg = {0.12, 0.12, 0.15, 1.0};
    gtk_widget_override_background_color(headerBox, GTK_STATE_FLAG_NORMAL, &headerBg);
    gtk_box_pack_start(GTK_BOX(contentArea), headerBox, FALSE, FALSE, 0);

    // Scrollable text view
    GtkWidget* scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget* textView = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textView), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(textView), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textView), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(textView), 14);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(textView), 14);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(textView), 10);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(textView), 10);

    // Dark background
    GdkRGBA bgColor = {0.10, 0.10, 0.13, 1.0};
    GdkRGBA fgColor = {0.88, 0.88, 0.88, 1.0};
    gtk_widget_override_background_color(textView, GTK_STATE_FLAG_NORMAL, &bgColor);
    gtk_widget_override_color(textView, GTK_STATE_FLAG_NORMAL, &fgColor);

    // Color tags
    GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
    gtk_text_buffer_create_tag(buf, "ok",       "foreground", "#55dd88", NULL);
    gtk_text_buffer_create_tag(buf, "err",      "foreground", "#ff6666", NULL);
    gtk_text_buffer_create_tag(buf, "folder",   "foreground", "#66ccff", NULL);
    gtk_text_buffer_create_tag(buf, "ts",       "foreground", "#ffcc44", NULL);
    gtk_text_buffer_create_tag(buf, "complete", "foreground", "#aaddff",
                                "weight", PANGO_WEIGHT_BOLD, NULL);

    // Insert lines with color tags
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        std::string lineNL = line + "\n";
        const char* tag = nullptr;
        if      (line.find("[OK]")            != std::string::npos) tag = "ok";
        else if (line.find("[ERR]")           != std::string::npos) tag = "err";
        else if (line.find("[FOLDER]")        != std::string::npos) tag = "folder";
        else if (line.find("Backup started:") != std::string::npos) tag = "ts";
        else if (line.find("Backup complete:")!= std::string::npos) tag = "complete";

        GtkTextIter endIter;
        gtk_text_buffer_get_end_iter(buf, &endIter);
        if (tag)
            gtk_text_buffer_insert_with_tags_by_name(buf, &endIter,
                lineNL.c_str(), -1, tag, NULL);
        else
            gtk_text_buffer_insert(buf, &endIter, lineNL.c_str(), -1);
    }

    gtk_container_add(GTK_CONTAINER(scroll), textView);
    gtk_box_pack_start(GTK_BOX(contentArea), scroll, TRUE, TRUE, 0);

    gtk_widget_show_all(dialog);

    // Scroll to end
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buf, &end);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(textView), &end, 0.0, FALSE, 0.0, 1.0);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void MainView::showError(const std::string& error) {
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK, "%s", error.c_str());
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void MainView::setBackupButtonEnabled(bool enabled) {
    gtk_widget_set_sensitive(m_startButton, enabled);
}

void MainView::updateCountdown(const std::string& countdownText) {
    if (countdownText.empty()) {
        gtk_label_set_text(GTK_LABEL(m_countdownLabel), "");
    } else {
        // Markup দিয়ে muted style — backup চলছে কিনা সেটা দেখায় না
        std::string markup = "<span size='small' foreground='#888888'>" + countdownText + "</span>";
        gtk_label_set_markup(GTK_LABEL(m_countdownLabel), markup.c_str());
    }
}

std::string MainView::getSelectedItem() {
    GtkTreeModel* model;
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(m_selection, &model, &iter)) {
        char* path;
        gtk_tree_model_get(model, &iter, 0, &path, -1);
        std::string result(path);
        g_free(path);
        return result;
    }
    return "";
}

void MainView::onFileSelected(FileSelectedCallback cb)    { m_onFileSelected   = cb; }
void MainView::onRemoveSelected(RemoveSelectedCallback cb) { m_onRemoveSelected = cb; }
void MainView::onClearAll(ClearAllCallback cb)            { m_onClearAll       = cb; }
void MainView::onStartBackup(StartBackupCallback cb)      { m_onStartBackup    = cb; }
void MainView::onOpenSettings(OpenSettingsCallback cb)    { m_onOpenSettings   = cb; }
void MainView::onViewLog(ViewLogCallback cb)              { m_onViewLog        = cb; }

#include "../headers/log_viewer.h"

void on_view_log(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Backup History",
        GTK_WINDOW(window),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL);

    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 400);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_set_border_width(GTK_CONTAINER(scrolled), 10);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD);

    gtk_container_add(GTK_CONTAINER(scrolled), textview);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                      scrolled, TRUE, TRUE, 0);

    DIR *dir = opendir(settings.backup_destination);
    if (dir) {
        struct dirent *entry;
        char latest_log[MAX_PATH] = "";
        time_t latest_time = 0;

        while ((entry = readdir(dir)) != NULL) {
            if (strncmp(entry->d_name, "Backup_", 7) == 0) {
                char log_path[MAX_PATH];
                sprintf(log_path, "%s\\%s\\backup_log.txt", settings.backup_destination, entry->d_name);

                struct stat st;
                if (stat(log_path, &st) == 0 && st.st_mtime > latest_time) {
                    latest_time = st.st_mtime;
                    strcpy(latest_log, log_path);
                }
            }
        }
        closedir(dir);

        if (strlen(latest_log) > 0) {
            FILE *log = fopen(latest_log, "r");
            if (log) {
                GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
                char line[256];
                GString *content = g_string_new("");

                while (fgets(line, sizeof(line), log)) {
                    g_string_append(content, line);
                }
                fclose(log);

                gtk_text_buffer_set_text(buffer, content->str, -1);
                g_string_free(content, TRUE);
            }
        } else {
            GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
            gtk_text_buffer_set_text(buffer, "No backup logs found", -1);
        }
    }

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

#include "../headers/ui_components.h"
#include "../headers/file_operations.h"
#include "../headers/backup_core.h"
#include "../headers/config_manager.h"
#include "../headers/log_viewer.h"

void create_main_window() {
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Smart Backup Utility");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), main_box);

    GtkWidget *header = gtk_label_new("SMART BACKUP UTILITY");
    gtk_box_pack_start(GTK_BOX(main_box), header, FALSE, FALSE, 0);

    progress_bar = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(main_box), progress_bar, FALSE, FALSE, 0);

    status_label = gtk_label_new("Ready");
    gtk_widget_set_halign(status_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(main_box), status_label, FALSE, FALSE, 0);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(scrolled, TRUE);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_pack_start(GTK_BOX(main_box), scrolled, TRUE, TRUE, 0);

    items_list = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(items_list));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
        "File/Folder", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    column = gtk_tree_view_column_new_with_attributes(
        "Size", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    column = gtk_tree_view_column_new_with_attributes(
        "Modified", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    gtk_container_add(GTK_CONTAINER(scrolled), treeview);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_box), button_box, FALSE, FALSE, 0);

    GtkWidget *button;

    button = gtk_button_new_with_label("Add Files/Folders");
    g_signal_connect(button, "clicked", G_CALLBACK(on_select_files_folders), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);

    button = gtk_button_new_with_label("Add Folder Only");
    g_signal_connect(button, "clicked", G_CALLBACK(on_select_folder_only), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);

    button = gtk_button_new_with_label("Remove Selected");
    g_signal_connect(button, "clicked", G_CALLBACK(on_remove_items), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);

    button = gtk_button_new_with_label("Clear All");
    g_signal_connect(button, "clicked", G_CALLBACK(on_clear_all), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);

    GtkWidget *action_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(action_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(action_box, 10);
    gtk_box_pack_start(GTK_BOX(main_box), action_box, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("Start Backup");
    gtk_widget_set_size_request(button, 140, 40);
    g_signal_connect(button, "clicked", G_CALLBACK(on_backup_now), NULL);
    gtk_box_pack_start(GTK_BOX(action_box), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("Settings");
    gtk_widget_set_size_request(button, 140, 40);
    g_signal_connect(button, "clicked", G_CALLBACK(on_settings), NULL);
    gtk_box_pack_start(GTK_BOX(action_box), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("View Log");
    gtk_widget_set_size_request(button, 140, 40);
    g_signal_connect(button, "clicked", G_CALLBACK(on_view_log), NULL);
    gtk_box_pack_start(GTK_BOX(action_box), button, FALSE, FALSE, 0);

    gtk_widget_show_all(window);
}

#include "../headers/progress_display.h"

void update_status(const char *message, double progress) {
    gtk_label_set_text(GTK_LABEL(status_label), message);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), progress);

    while (gtk_events_pending()) gtk_main_iteration();
}

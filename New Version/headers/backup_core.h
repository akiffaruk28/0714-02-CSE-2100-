#ifndef BACKUP_CORE_H
#define BACKUP_CORE_H

#include "backup_settings.h"

gboolean perform_backup(gpointer data);
gboolean auto_backup_timer(gpointer data);
void on_backup_now(GtkWidget *widget, gpointer data);

#endif

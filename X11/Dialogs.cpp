/**
 * This file is part of CernVM Web API Plugin.
 *
 * CVMWebAPI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CVMWebAPI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CVMWebAPI. If not, see <http://www.gnu.org/licenses/>.
 *
 * Developed by Ioannis Charalampidis 2013
 * Contact: <ioannis.charalampidis[at]cern.ch>
 */

#include <string>
#include <gtk/gtk.h>
#include "../Dialogs.h"

typedef struct {
    int result;
    const char * text;
    GtkWidget * window;
} DialogData;

void CVMInitializeDialogs()
{
    // Initialize GTK
    if (!gtk_init_check(NULL, NULL)) {
        std::cerr << "[Dialogs] gtk_init_check failed" << std::endl;
        return;
    }
    
    // Initialize GTK thread-safety
    g_thread_init(NULL);
    gdk_threads_init();
}

gboolean display_dialog( gpointer user_data )
{
    DialogData *dialog_data = (DialogData*) user_data;
    GtkWidget *dialog;
    
    /* Create dialog */
    dialog = gtk_message_dialog_new (NULL,
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_QUESTION,
                                     GTK_BUTTONS_YES_NO,
                                     "%s", dialog_data->text);

    /*  Prepare dialog */
    gtk_window_set_keep_above(GTK_WINDOW(dialog), true);
    gtk_window_set_title(GTK_WINDOW(dialog), "CernVM Web API");

    /* Run and destroy when completed */
    dialog_data->result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    /* Quit the main loop */
    gtk_main_quit(); 

    return FALSE;
}

bool CVMConfirmDialog(const FB::BrowserHostPtr& host, FB::PluginWindow* win, std::string message) {

    // Prepare dialog I/O
    DialogData dialog_data;
    dialog_data.text = (const char * ) message.c_str();

    // Thread-safe GTK init
    g_idle_add( display_dialog, &dialog_data );
    GDK_THREADS_ENTER();
    gtk_main();
    GDK_THREADS_LEAVE();

    return (dialog_data.result == GTK_RESPONSE_YES);
}

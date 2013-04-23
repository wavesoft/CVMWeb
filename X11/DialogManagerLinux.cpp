
#include <string>
#include <gtk/gtk.h>
#include "DialogManagerWin.h"

typedef struct {
    int type;
    int result;
    const char * text;
} DialogData;

DialogManager* DialogManager::get()
{
    static DialogManagerLinux inst;
    return &inst;
}

static gboolean display_dialog( gpointer user_data ) {
    DialogData *dialog_data = user_data;
    GtkWidget *dialog;
    
    /* Fetch window */
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);;

    /* Create dialog */
    dialog = gtk_message_dialog_new(
        GTK_WINDOW(window), 
        GTK_DIALOG_DESTROY_WITH_PARENT, 
        GTK_MESSAGE_QUESTION, 
        GTK_BUTTONS_YES_NO, ( const gchar * ) user_data->text);

    /* Set caption */
    gtk_window_set_title(GTK_WINDOW(dialog), "CernVM Web API");
    dialog_data->result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_main_quit();  // Quits the main loop run in MessageBox()

    return FALSE;
}

bool DialogManagerLinux::ConfirmDialog(const FB::BrowserHostPtr& host, FB::PluginWindow* win, std::string message) {
    DialogData dialog_data;
    dialog_data.type = type;
    dialog_data.text = (const char * ) message.c_str();
    gtk_idle_add(display_dialog, &dialog_data);
    gtk_main();
}

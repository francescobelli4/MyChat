#ifndef GOOGLE_LOGIN
#define GOOGLE_LOGIN

#include <pthread.h>
#include <gtk/gtk.h>

extern pthread_mutex_t sync_mutex;

typedef struct google_login_button_arg {
	GtkBuilder *builder;
	gboolean (*callback)(gpointer); 
} GoogleLoginButtonArg;

void google_button_clicked(GtkWidget *widget, gpointer user_data);

#endif

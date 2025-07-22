#include "gio/gio.h"
#include "glib-object.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui.h"
#include "../../../shared/shared.h"
#include "gtk/gtkshortcut.h"

GtkBuilder *builder;

void google_button_clicked() {

	const char *url_format = "https://accounts.google.com/o/oauth2/v2/auth?access_type=offline&scope=openid%%20email%%20profile&include_granted_scopes=true&response_type=code&redirect_uri=https://localhost:8080/&client_id=%s";

	char *url = malloc(strlen(url_format) + strlen(GOOGLE_CLIENT_ID) + 1);

	snprintf(url, strlen(url_format) + strlen(GOOGLE_CLIENT_ID), url_format, GOOGLE_CLIENT_ID);

	char *command = malloc(strlen(url) + strlen("xdg-open '%s'"));

	snprintf(command, strlen(url) + strlen("xdg-open '%s'") + 1, "xdg-open '%s'", url);

	printf("COMMAND: [%s]\n", command);

	system(command);
}

void setup_login_mode() {

	GtkButton *google_login = GTK_BUTTON(gtk_builder_get_object(builder, "login_mode_google"));
	g_signal_connect(google_login, "clicked", G_CALLBACK(google_button_clicked), NULL);
}

void setup_register_mode() {

}

void login_mode_btn_clicked(GtkButton *button, gpointer arg) {
	char *data = (char *) arg;
	printf("CLICK %s\n", data);

	GtkStack *stack = GTK_STACK(gtk_builder_get_object(builder, "login_mode_stack"));


	if (strcmp(data, "login") == 0) {
		gtk_stack_set_visible_child_name(stack, "login_page");
		setup_login_mode();
	} else {
		gtk_stack_set_visible_child_name(stack, "register_page");
	}
}

int display_login() {
	
	builder = gtk_builder_new_from_file("ui/Login/main.ui");

	GtkWidget *content = GTK_WIDGET(gtk_builder_get_object(builder, "background"));

	gtk_window_set_child(get_app_window(), content);

	setup_login_mode();

	GtkButton *login_mode_btn = GTK_BUTTON(gtk_builder_get_object(builder, "login_mode_btn"));
	GtkButton *register_mode_btn = GTK_BUTTON(gtk_builder_get_object(builder, "register_mode_btn"));

	g_signal_connect(login_mode_btn, "clicked", G_CALLBACK(login_mode_btn_clicked), (gpointer) "login");
	g_signal_connect(register_mode_btn, "clicked", G_CALLBACK(login_mode_btn_clicked), (gpointer) "register");

	GtkCssProvider *provider = gtk_css_provider_new();
	gtk_css_provider_load_from_path(provider, "ui/Login/main.css");

	gtk_style_context_add_provider_for_display(
    		gdk_display_get_default(),
    		GTK_STYLE_PROVIDER(provider),
    		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
	);
	g_object_unref(provider);
}


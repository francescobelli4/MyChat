#include <fcntl.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "login.h"
#include "../shared/shared.h"
#include "ui.h"

GtkBuilder *builder;
pthread_t http_server;

/*
 * 	a 	b 	c 	% 	2 	F 	d 	e 	f 	g 	% 	2 	F 	h
 *	0 	1 	2 	3 	4 	5 	6 	7 	8 	9 	10      11      12	13
 *
 *
 */

char *decode_code (char *encoded, int encoded_len) {

	int decoded_len = encoded_len;
	char *temp = encoded;
	char *decoded = malloc(decoded_len);
	memset(decoded, 0, decoded_len);
	int offset = 0;
	int decoded_offset = 0;

	while ((temp = strchr(temp, '%')) != NULL) {

		int copying_bytes = temp - (encoded + offset);

		memcpy(decoded + decoded_offset, encoded + offset, copying_bytes);

		decoded_offset += copying_bytes;
		offset += (copying_bytes + 3);

		char *encoded_char = malloc(3);
		memset(encoded_char, 0, 3);

		memcpy(encoded_char, temp + 1, 2);
		printf("Encoded_char: [%s]\n", encoded_char);
		char decoded_char = (char) strtol(encoded_char, NULL, 16);

		memcpy(decoded + decoded_offset, &decoded_char, 1);
		decoded_offset++;

		temp+= 3;
	}

	memcpy(decoded + decoded_offset, encoded + offset, encoded_len - offset);

	printf("RESSSSS: [%s]\n", decoded);

	return decoded;
}

char *get_decoded_code_from_response (char *response) {

	char *code;
	
	char *start = strstr(response, "code=");
	start += strlen("code=");

	char *end = strchr(start, '&');

	int code_len = end - start;

	code = malloc(code_len + 1);
	memset(code, 0, code_len + 1);
	memcpy(code, start, code_len);

	printf("Code: [%s]\n", code);

	decode_code(code, strlen(code));
}

void *setup_http_google_server(void *arg) {

	struct sockaddr_in server_address;

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(8080);
	server_address.sin_addr.s_addr = INADDR_ANY;

	int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (server_socket == -1)
		return NULL;

	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)))
		return NULL;

	if (listen(server_socket, 1))
		return NULL;

	char *buffer = malloc(4096);

	struct sockaddr client_address;
	int client_addrlen;


	int client = accept(server_socket, (struct sockaddr *) &client_address, (socklen_t *) &client_addrlen);

	read(client, buffer, 4096);

	printf("RECEIVED: [%s]\n", buffer);
	get_decoded_code_from_response(buffer);
	
	close(server_socket);
	close(client);

	return NULL;
}

void google_button_clicked() {

	const char *url_format = "https://accounts.google.com/o/oauth2/v2/auth?access_type=offline&scope=openid%%20email%%20profile&include_granted_scopes=true&response_type=code&redirect_uri=http://localhost:8080/&client_id=%s";

	char *url = malloc(strlen(url_format) + strlen(GOOGLE_CLIENT_ID) + 1);

	snprintf(url, strlen(url_format) + strlen(GOOGLE_CLIENT_ID), url_format, GOOGLE_CLIENT_ID);

	char *command = malloc(strlen(url) + strlen("xdg-open '%s'"));

	snprintf(command, strlen(url) + strlen("xdg-open '%s'") + 1, "xdg-open '%s'", url);

	system(command);

	GtkStack *stack = GTK_STACK(gtk_builder_get_object(builder, "login_mode_stack"));

	gtk_stack_set_visible_child_name(stack, "waiting_for_google_page");

	pthread_create(&http_server, NULL, setup_http_google_server, NULL);

	
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

void display_login() {
	
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


int get_login_file() {
	
	int login_fd = open("user_login.json", O_EXCL|O_CREAT|O_RDWR, 0666);

	if (login_fd == -1) {
		
		if (errno == EEXIST) {
			return LOGIN_FILE_DOES_NOT_EXIST;
		}

		return LOGIN_ERROR;
	}

	return login_fd;
}

int login_handler() {
	
	int login_fd = get_login_file();

	if (login_fd == LOGIN_ERROR)
		return LOGIN_ERROR;

	if (login_fd == LOGIN_FILE_DOES_NOT_EXIST) {

		// Registra
		display_login();
	} else {

		// Login automatico
	}
}

SERVER = server
CLIENT = client

CC = gcc

GTK_CFLAGS = $(shell pkg-config --cflags gtk4)
GTK_LIBS = $(shell pkg-config --libs gtk4)

all: client

client: 
	gcc $(GTK_CFLAGS) -o bin/ui.o -c src/client/ui/ui.c $(GTK_LIBS)
	gcc $(GTK_CFLAGS) -o bin/ui_login.o -c src/client/ui/login/ui_login.c $(GTK_LIBS)
	gcc $(GTK_CFLAGS) -o bin/$(CLIENT) src/client/main.c bin/ui_login.o bin/ui.o $(GTK_LIBS)

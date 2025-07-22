SERVER = server
CLIENT = client

CC = gcc

GTK_CFLAGS = $(shell pkg-config --cflags gtk4)
GTK_LIBS = $(shell pkg-config --libs gtk4)

all: client

client:
	mkdir -p bin
	gcc $(GTK_CFLAGS) -o bin/ui.o -c src/client/ui.c
	gcc $(GTK_CFLAGS) -o bin/login.o -c src/client/login.c
	gcc $(GTK_CFLAGS) -o bin/$(CLIENT) src/client/main.c bin/login.o bin/ui.o $(GTK_LIBS)

SERVER = server
CLIENT = client

CC = gcc

GTK_CFLAGS = $(shell pkg-config --cflags gtk4)
OPENSSL_FLAGS= $(shell pkg-config --cflags openssl)
GTK_LIBS = $(shell pkg-config --libs gtk4)
OPENSSL_LIBS = $(shell pkg-config --libs openssl)
JSONCFLAGS = $(shell pkg-config --cflags json-c)
JSONCLDFLAGS = $(shell pkg-config --libs json-c)

all: client

client:
	mkdir -p bin
	gcc $(GTK_CFLAGS) $(OPENSSL_FLAGS) $(JSONCFLAGS) -o bin/ui.o -c src/client/ui.c
	gcc $(GTK_CFLAGS) $(OPENSSL_FLAGS) $(JSONCFLAGS) -o bin/google_login.o -c src/client/google/utils.c
	gcc $(GTK_CFLAGS) $(OPENSSL_FLAGS) $(JSONCFLAGS) -o bin/login.o -c src/client/login.c
	gcc $(GTK_CFLAGS) $(OPENSSL_FLAGS) $(JSONCFLAGS) -o bin/$(CLIENT) src/client/main.c bin/google_login.o bin/login.o bin/ui.o $(GTK_LIBS) $(OPENSSL_LIBS) $(JSONCLDFLAGS) -lm

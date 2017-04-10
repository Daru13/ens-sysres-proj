# Makefile for Systèmes et Réseau (16-17)'s course projet : web server.
CC = clang
CCFLAGS = -g -W -Wall -pedantic -std=c99

##### THIS LIST MUST BE UPDATED #####
# List of all  object files which must be produced before any binary
OBJS = build/toolbox.o build/http.o build/server.o build/main.o

# Dependencies and compiling rules
all: server

server: $(OBJS)
	$(CC) $(CCFLAGS) $(OBJS) -o webserver


build/main.o: src/main.c src/main.h build/server.o build/toolbox.o
	$(CC) $(CCFLAGS) -c src/main.c -o build/main.o

build/server.o: src/server.c src/server.h build/toolbox.o
	$(CC) $(CCFLAGS) -c src/server.c -o build/server.o

build/http.o: src/http.c src/http.h
	$(CC) $(CCFLAGS) -c src/http.c -o build/http.o	

build/toolbox.o: src/toolbox.c src/toolbox.h
	$(CC) $(CCFLAGS) -c src/toolbox.c -o build/toolbox.o

# Cleaning rule
clean:
	- rm -f build/webserver
	- rm -f build/*.o

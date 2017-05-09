# Makefile for Systèmes et Réseau (16-17)'s course projet : web server.
CC = clang
CCFLAGS = -g -W -Wall -pedantic -std=c99

##### THIS LIST MUST BE UPDATED #####
# List of all  object files which must be produced before any binary
OBJS = build/toolbox.o build/file_cache.o build/parse_header.o build/http.o build/server.o build/main.o

# Dependencies and compiling rules
all: server

server: $(OBJS)
	$(CC) $(CCFLAGS) $(OBJS) -o webserver

build/main.o: src/main.c src/main.h src/server.h src/file_cache.h src/parse_header.h src/toolbox.h
	$(CC) $(CCFLAGS) -c src/main.c -o build/main.o

build/server.o: src/server.c src/server.h src/http.h src/file_cache.h src/toolbox.h
	$(CC) $(CCFLAGS) -c src/server.c -o build/server.o

src/server.h: src/http.h

build/parse_header.o: src/parse_header.c src/parse_header.h src/http.h src/toolbox.h
	$(CC) $(CCFLAGS) -c src/parse_header.c -o build/parse_header.o

src/parse_header.h: src/http.h

build/http.o: src/http.c src/http.h src/parse_header.h src/file_cache.h src/toolbox.h
	$(CC) $(CCFLAGS) -c src/http.c -o build/http.o	

src/http.h: src/file_cache.h

build/file_cache.o: src/file_cache.c src/file_cache.h src/toolbox.h
	$(CC) $(CCFLAGS) -c src/file_cache.c -o build/file_cache.o

build/toolbox.o: src/toolbox.c src/toolbox.h
	$(CC) $(CCFLAGS) -c src/toolbox.c -o build/toolbox.o

# Cleaning rule
clean:
	- rm -f build/webserver
	- rm -f build/*.o

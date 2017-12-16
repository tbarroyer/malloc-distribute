CC=g++
CFLAGS=-W -Wall -ansi -pedantic -g
LDFLAGS=

all: client server

client: obj/client/main.o
	$(CC) -o client obj/client/main.o $(LDFLAGS)

server: obj/server/main.o
	$(CC) -o server obj/server/main.o $(LDFLAGS)

obj/client/main.o: src/client/main.cc obj/client
	$(CC) -o obj/client/main.o -c src/client/main.cc $(CFLAGS)

obj/server/main.o: src/server/main.cc obj/server
	$(CC) -o obj/server/main.o -c src/server/main.cc $(CFLAGS)

obj/client: obj
	mkdir -p $@

obj/server: obj
	mkdir -p $@

obj:
	mkdir -p $@

clean:
	rm -rf client
	rm -rf server
	rm -rf obj

CC=mpic++
CFLAGS=-Wall -Wextra -Werror
LDFLAGS=

all: main

check: main
	mpirun main

main: obj/main.o obj/api/api.o
	$(CC) -o $@ $^ $(LDFLAGS)

obj/main.o: src/main.cc obj
	$(CC) -o $@ -c $< $(CFLAGS)

obj/api/api.o: src/api/api.cc obj/api
	$(CC) -o $@ -c $< $(CFLAGS)

obj/api: obj
	mkdir -p $@

obj:
	mkdir -p $@

clean:
	rm -rf main
	rm -rf obj

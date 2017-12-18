CC=mpic++
CFLAGS=-Wall -Wextra -Werror -g -std=c++11
LDFLAGS=
SRC2= src/main.cc src/api/api2.cc
OBJ2 = $(SRC2:.c=.o)
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

main2: $(OBJ2)
	$(CC) -o $@ $^ $(CFLAGS)


clean:
	rm -rf main
	rm -rf obj

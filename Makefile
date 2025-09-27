CC=gcc
CFLAGS=-O2 -Wall -Wextra -Iinclude -Ithird_party -DUSE_STB
LDFLAGS=-pthread -lm
SRC=src/main.c src/utils_conc.c src/conv.c src/sobel.c src/rotate.c src/resize.c

all: reto2
reto2: $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

clean:
	rm -f reto2

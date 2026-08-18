CC = gcc
CFLAGS = -Wall -g `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -lGL -lm -lpthread

SRC = src/main.c src/renderer.c src/gps.c
OBJ = $(SRC:.c=.o)
BIN = bin/airport_metaverse

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean

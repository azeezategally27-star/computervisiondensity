CC = gcc
CFLAGS = -Wall -g `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -lGL -lm -lpthread

SRC = src/main.c src/renderer.c src/xray_simulator.c src/ai_stub.c src/sensors.c src/conveyor.c src/ui.c src/logging.c
OBJ = $(SRC:.c=.o)
BIN = bin/xray_metaverse_demo

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean

CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lpthread

SRC = src/backend.c
BIN = bin/pixel_backend

all: $(BIN)

$(BIN): $(SRC)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(BIN)

.PHONY: all clean

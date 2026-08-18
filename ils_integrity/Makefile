CC = gcc
CFLAGS = -O2 -Wall `pkg-config --cflags gtk+-3.0` -I.
LDFLAGS = `pkg-config --libs gtk+-3.0` -lm
SRCS = main.c
BIN = ils_integrity

all: $(BIN)

$(BIN): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(BIN) *.o

.PHONY: all clean

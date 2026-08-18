CC = gcc
CFLAGS = -O2 -Wall `pkg-config --cflags gtk+-3.0` -I.
LDFLAGS = `pkg-config --libs gtk+-3.0` -lsqlite3 -lm
SRCS = main.c kb.c
BIN = security_assistant

all: $(BIN)

$(BIN): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(BIN) *.o security_kb.db

.PHONY: all clean

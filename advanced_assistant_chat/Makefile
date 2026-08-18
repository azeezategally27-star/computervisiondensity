CC = gcc
CFLAGS = -O2 -Wall `pkg-config --cflags gtk+-3.0` -I.
LDFLAGS = `pkg-config --libs gtk+-3.0` -lsqlite3 -lm
SRCS = main.c kb.c
OBJS = $(SRCS:.c=.o)
BIN = assistant_chat

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(BIN) $(OBJS) assistant_chat.db

.PHONY: all clean

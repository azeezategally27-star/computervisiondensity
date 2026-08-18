CC = gcc
CFLAGS = -O2 -Wall `pkg-config --cflags gtk+-3.0` -I.
LDFLAGS = `pkg-config --libs gtk+-3.0` -lsqlite3 -lpthread -lm
SRCS = main.c ui.c data.c simulator.c api_server.c assistant.c gate_map.c baggage.c scheduler.c
OBJS = $(SRCS:.c=.o)
BIN = flight_assistant

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(BIN) $(OBJS)

.PHONY: all clean

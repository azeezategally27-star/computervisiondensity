CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -pthread
SRCS = src/main.c src/msgbus.c src/agents.c
OBJS = $(SRCS:.c=.o)
TARGET = airport_sim

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

clean:
	rm -f $(OBJS) $(TARGET)

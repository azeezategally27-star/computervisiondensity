CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -pthread
LDFLAGS = -lncurses

# Targets
all: airport_sim biometric_app

airport_sim: src/main.c src/msgbus.c src/agents.c
	$(CC) $(CFLAGS) -o $@ src/main.c src/msgbus.c src/agents.c

biometric_app: src/biometric_app.c src/biometric_agents.c src/msgbus.c
	$(CC) $(CFLAGS) -o $@ src/biometric_app.c src/biometric_agents.c src/msgbus.c $(LDFLAGS)

clean:
	rm -f airport_sim biometric_app

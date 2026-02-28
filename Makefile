CC = gcc
DEBUG = -ggdb
WARN = -Wall -Wextra
CFLAGS = $(DEBUG) $(WARN)

SERVER = server
CLIENT = client

all: $(SERVER) $(CLIENT)

$(SERVER): server.c
	$(CC) $(CFLAGS) $< -o $@

$(CLIENT): client.c
	$(CC) $(CFLAGS) $< -o $@

run-server: $(SERVER)
	./$(SERVER)

run-client: $(CLIENT)
	./$(CLIENT)

clean:
	rm -f $(SERVER) $(CLIENT)

.PHONY: all clean

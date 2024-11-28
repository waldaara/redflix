# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pedantic
LDFLAGS = -pthread

# Default target
all: server client

# Server target
server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LDFLAGS)

# Client target
client: client.c
	$(CC) $(CFLAGS) -o client client.c $(LDFLAGS)

# Clean up generated files
clean:
	rm -f server client

# Help message
help:
	@echo "Makefile usage:"
	@echo "  make        - Build both server and client"
	@echo "  make server - Build the server program"
	@echo "  make client - Build the client program"
	@echo "  make clean  - Remove compiled binaries"

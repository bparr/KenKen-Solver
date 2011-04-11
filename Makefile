
all: kenken debug.kenken
debug: debug.kenken

CC = gcc
CFLAGS = -O
DEBUGFLAGS = -g -Wall -Werror

kenken: kenken.c
	$(CC) $(CFLAGS) $< -o $@

debug.kenken: kenken.c
	$(CC) $(DEBUGFLAGS) $< -o $@

clean:
	rm -f kenken debug.kenken


all: kenken debug.kenken parallelken debug.parallelken
debug: debug.kenken debug.parallelken

CC = icc
CFLAGS = -openmp -O
DEBUGFLAGS = -openmp -g -Wall -Werror

kenken: kenken.c
	$(CC) $(CFLAGS) $< -o $@

parallelken: parallelken.c
	$(CC) $(CFLAGS) $< -o $@

debug.kenken: kenken.c
	$(CC) $(DEBUGFLAGS) $< -o $@

debug.parallelken: parallelken.c
	$(CC) $(DEBUGFLAGS) $< -o $@

clean:
	rm -f kenken debug.kenken parallelken debug.parallelken

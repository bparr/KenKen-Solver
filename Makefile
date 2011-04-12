all: serial parallelken debug.parallelken
#debug: debug.kenken debug.parallelken

CC = icc
CFLAGS = -openmp -O
DEBUGFLAGS = -openmp -g -Wall -Werror

kenken: kenken.c kenken.h
	$(CC) $(CFLAGS) -c kenken.c

parallelken: parallelken.c kenken.o
	$(CC) $(CFLAGS) $< -o $@

#debug.kenken: kenken.c kenken.h
#	$(CC) $(DEBUGFLAGS) -c kenken.c

serial.o: serial.c kenken.h
	$(CC) $(CFLAGS) -c serial.c

serial: serial.o kenken.o

debug.parallelken: parallelken.c
	$(CC) $(DEBUGFLAGS) $< -o $@

clean:
	rm -f *.o kenken serial 

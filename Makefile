all: parallelken
#debug: debug.parallelken

CC = icc
CFLAGS = -openmp -O
DEBUGFLAGS = -openmp -g -Wall -Werror

kenken: kenken.c kenken.h
	$(CC) $(DEBUGFLAGS) -c kenken.c

parallelken.o: parallelken.c kenken.h
	$(CC) $(DEBUGFLAGS) -c parallelken.c

parallelken: parallelken.o kenken.o
	$(CC) $(DEBUGFLAGS) $^ -o $@

#debug.kenken: kenken.c kenken.h
#	$(CC) $(DEBUGFLAGS) -c kenken.c

#serial.o: serial.c kenken.h
#	$(CC) $(CFLAGS) -c serial.c

#serial: serial.o kenken.o

clean:
	rm -f *.o parallelken debug.parallelken

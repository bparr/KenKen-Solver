
all: serial
#debug: debug.kenken

CC = gcc
CFLAGS = -O
DEBUGFLAGS = -g -Wall -Werror

kenken: kenken.c kenken.h
	$(CC) $(CFLAGS) -c kenken.c

#debug.kenken: kenken.c kenken.h
#	$(CC) $(DEBUGFLAGS) -c kenken.c

serial.o: serial.c kenken.h
	$(CC) $(CFLAGS) -c serial.c

serial: serial.o kenken.o

clean:
	rm -f *.o kenken serial

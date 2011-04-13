all: serial parallel
#debug: debug.parallel

CC = icc
CFLAGS = -openmp -O
DEBUGFLAGS = -openmp -g -Wall -Werror

kenken.o: kenken.c kenken.h
	$(CC) $(CFLAGS) -c kenken.c

parallel.o: parallel.c kenken.h
	$(CC) $(CFLAGS) -c parallel.c

parallel: parallel.o kenken.o
	$(CC) $(CFLAGS) $^ -o $@

#debug.parallel: kenken.c kenken.h
#	$(CC) $(DEBUGFLAGS) -c kenken.c

serial.o: serial.c kenken.h
	$(CC) $(CFLAGS) -c serial.c

serial: serial.o kenken.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f *.o serial parallel

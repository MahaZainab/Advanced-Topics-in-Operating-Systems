CC = gcc
CFLAGS = -Wall -Wextra -O2

all: pwordcount

pwordcount: pwordcount.o wordcount.o
	$(CC) $(CFLAGS) -o pwordcount pwordcount.o wordcount.o

pwordcount.o: pwordcount.c wordcount.h
	$(CC) $(CFLAGS) -c pwordcount.c

wordcount.o: wordcount.c wordcount.h
	$(CC) $(CFLAGS) -c wordcount.c

clean:
	rm -f *.o pwordcount

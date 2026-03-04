CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

OBJS = pwordcount.o wordcount.o

all: pwordcount

pwordcount: $(OBJS)
	$(CC) $(CFLAGS) -o pwordcount $(OBJS)

pwordcount.o: pwordcount.c wordcount.h
	$(CC) $(CFLAGS) -c pwordcount.c

wordcount.o: wordcount.c wordcount.h
	$(CC) $(CFLAGS) -c wordcount.c

clean:
	rm -f *.o pwordcount

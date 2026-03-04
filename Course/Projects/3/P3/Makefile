# COMP7500 - Advanced Operating Systems
# Project 3: AUbatch
#
# Maha
# Auburn University

CC     = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -g
LIBS   = -lpthread

OBJS = aubatch.o scheduling.o cmd_parser.o job.o evaluation.o

all: aubatch batch_job

aubatch: $(OBJS)
	$(CC) $(CFLAGS) -o aubatch $(OBJS) $(LIBS)

batch_job: batch_job.c
	$(CC) $(CFLAGS) -o batch_job batch_job.c

aubatch.o: aubatch.c job.h scheduling.h cmd_parser.h
	$(CC) $(CFLAGS) -c aubatch.c

scheduling.o: scheduling.c scheduling.h job.h
	$(CC) $(CFLAGS) -c scheduling.c

cmd_parser.o: cmd_parser.c cmd_parser.h job.h scheduling.h evaluation.h
	$(CC) $(CFLAGS) -c cmd_parser.c

job.o: job.c job.h
	$(CC) $(CFLAGS) -c job.c

evaluation.o: evaluation.c evaluation.h job.h
	$(CC) $(CFLAGS) -c evaluation.c

clean:
	rm -f $(OBJS) aubatch batch_job

.PHONY: all clean

/*
 * COMP7500 - Advanced Operating Systems
 * Project 3: AUbatch
 *
 * Maha
 * Auburn University
 *
 * batch_job.c - micro benchmark
 *
 * This is the job that gets submitted to aubatch for testing.
 * It just sleeps for however many seconds you pass it.
 * No output - the spec says batch jobs should be silent.
 *
 * Usage: ./batch_job <seconds>
 * Example: ./batch_job 5
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <seconds>\n", argv[0]);
        return 1;
    }

    int secs = atoi(argv[1]);
    if (secs <= 0) {
        fprintf(stderr, "seconds must be > 0\n");
        return 1;
    }

    sleep(secs);
    return 0;
}

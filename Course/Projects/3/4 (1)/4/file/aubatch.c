#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "job.h"
#include "scheduling.h"
#include "cmd_parser.h"

int main(void) {
    pthread_t sched_tid, disp_tid;

    job_init();

    if (pthread_create(&sched_tid, NULL, scheduling_thread, NULL) != 0) {
        perror("failed to create scheduling thread");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&disp_tid, NULL, dispatching_thread, NULL) != 0) {
        perror("failed to create dispatching thread");
        exit(EXIT_FAILURE);
    }

    printf("Welcome to Maha's batch job scheduler Version 1.0\n");
    printf("Type 'help' to find more about AUbatch commands.\n");

    run_command_loop();

    aubatch_running = 0;
    pthread_cond_broadcast(&queue_not_empty);
    pthread_cond_broadcast(&queue_not_full);

    pthread_join(sched_tid, NULL);
    pthread_join(disp_tid,  NULL);

    pthread_mutex_destroy(&queue_lock);
    pthread_cond_destroy(&queue_not_empty);
    pthread_cond_destroy(&queue_not_full);

    return 0;
}

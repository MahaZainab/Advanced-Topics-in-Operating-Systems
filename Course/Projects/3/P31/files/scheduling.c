
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#include "job.h"
#include "scheduling.h"

volatile int aubatch_running = 1;

/*
 * sort_queue - reorders the waiting jobs by the given policy
 *
 * If a job is already running at index 0 we leave it alone.
 * Only the waiting jobs behind it get sorted.
 * Using insertion sort - simple enough for this project.
 */
void sort_queue(int policy) {
    /* figure out where to start sorting from */
    int start = 0;
    if (job_count > 0 && job_queue[0].status == STATUS_RUNNING)
        start = 1;

    if (job_count - start < 2)
        return; /* nothing to sort */

    for (int i = start + 1; i < job_count; i++) {
        job_t key = job_queue[i];
        int j = i - 1;

        while (j >= start) {
            int swap = 0;

            if (policy == POLICY_SJF)
                swap = (job_queue[j].cpu_time > key.cpu_time);
            else if (policy == POLICY_PRIORITY)
                swap = (job_queue[j].priority > key.priority);
            else
                break; /* FCFS - keep arrival order, don't touch anything */

            if (!swap) break;
            job_queue[j + 1] = job_queue[j];
            j--;
        }
        job_queue[j + 1] = key;
    }
}

/*
 * compute_expected_wait - adds up cpu_time for all jobs in queue
 * used to tell the user how long their new job might wait
 */
int compute_expected_wait(void) {
    int total = 0;
    for (int i = 0; i < job_count; i++)
        total += job_queue[i].cpu_time;
    return total;
}

/*
 * scheduling_thread - producer side
 *
 * Honestly this thread just stays alive in the background.
 * cmd_run() does the real work of adding jobs to the queue
 * since that's where the user types the run command.
 * This thread's job is just to not die until the program exits.
 */
void *scheduling_thread(void *arg) {
    (void)arg;
    while (aubatch_running)
        sleep(1);
    return NULL;
}

/*
 * dispatching_thread - consumer side
 *
 * Waits for a job to appear, grabs it, runs it with fork+execv,
 * waits for it to finish, then updates the performance counters.
 * Loops forever until aubatch_running goes to 0.
 */
void *dispatching_thread(void *arg) {
    (void)arg;

    while (aubatch_running) {

        pthread_mutex_lock(&queue_lock);

        /* wait until there's actually something in the queue */
        while (job_count == 0 && aubatch_running)
            pthread_cond_wait(&queue_not_empty, &queue_lock);

        if (!aubatch_running && job_count == 0) {
            pthread_mutex_unlock(&queue_lock);
            break;
        }

        /* mark the head job as running and grab a copy of what we need */
        job_queue[0].status     = STATUS_RUNNING;
        job_queue[0].start_time = time(NULL);

        char job_name[MAX_NAME_LEN];
        int  job_cpu = job_queue[0].cpu_time;
        strncpy(job_name, job_queue[0].name, MAX_NAME_LEN);

        pthread_mutex_unlock(&queue_lock);

        /* fork and run the job */
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");

        } else if (pid == 0) {
            /* child - run the job executable */
            char cpu_str[16];
            snprintf(cpu_str, sizeof(cpu_str), "%d", job_cpu);

            char *args[3];
            args[0] = job_name;
            args[1] = cpu_str;
            args[2] = NULL;

            execv(job_name, args);
            /* only gets here if execv fails */
            perror("execv failed");
            exit(EXIT_FAILURE);

        } else {
            /* parent - wait for child to finish */
            waitpid(pid, NULL, 0);
        }

        /* job is done, update the timing data */
        pthread_mutex_lock(&queue_lock);

        time_t finish            = time(NULL);
        job_queue[0].finish_time = finish;
        job_queue[0].status      = STATUS_DONE;

        double turnaround = difftime(finish, job_queue[0].arrival_time);
        double waiting    = difftime(job_queue[0].start_time, job_queue[0].arrival_time);

        total_turnaround += turnaround;
        total_cpu_time   += (double)job_queue[0].cpu_time;
        total_waiting    += waiting;

        job_remove_head();

        pthread_cond_signal(&queue_not_full);
        pthread_mutex_unlock(&queue_lock);
    }

    return NULL;
}

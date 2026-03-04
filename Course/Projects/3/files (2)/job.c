
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "job.h"

/* define the actual globals here, other files use extern */
job_t job_queue[MAX_JOBS];
int job_count = 0;
int current_policy = POLICY_FCFS;
int total_submitted = 0;

double total_turnaround = 0.0;
double total_cpu_time = 0.0;
double total_waiting = 0.0;
time_t first_job_time = 0;

pthread_mutex_t queue_lock;
pthread_cond_t queue_not_empty;
pthread_cond_t queue_not_full;

/*
 * job_init - call this before creating any threads
 * sets up the mutex and condition variables
 */
void job_init(void)
{
    job_count = 0;
    total_submitted = 0;
    total_turnaround = 0.0;
    total_cpu_time = 0.0;
    total_waiting = 0.0;
    first_job_time = 0;
    current_policy = POLICY_FCFS;

    pthread_mutex_init(&queue_lock, NULL);
    pthread_cond_init(&queue_not_empty, NULL);
    pthread_cond_init(&queue_not_full, NULL);
}

/*
 * job_add - add a new job to the end of the queue
 * caller must hold queue_lock before calling this
 */
void job_add(const char *name, int cpu_time, int priority)
{
    if (job_count >= MAX_JOBS)
    {
        printf("queue is full, can't add more jobs right now\n");
        return;
    }

    job_t *j = &job_queue[job_count];
    strncpy(j->name, name, MAX_NAME_LEN - 1);
    j->name[MAX_NAME_LEN - 1] = '\0';
    j->cpu_time = cpu_time;
    j->priority = priority;
    j->arrival_time = time(NULL);
    j->start_time = 0;
    j->finish_time = 0;
    j->status = STATUS_WAITING;

    job_count++;
    total_submitted++;

    /* remember when the very first job ever arrived for throughput calc */
    if (total_submitted == 1)
        first_job_time = j->arrival_time;
}

/*
 * job_remove_head - remove the job at index 0 after it finishes
 * shifts everything else forward
 * caller must hold queue_lock
 */
void job_remove_head(void)
{
    if (job_count <= 0)
        return;

    for (int i = 0; i < job_count - 1; i++)
        job_queue[i] = job_queue[i + 1];

    job_count--;
}

/*
 * job_count_waiting - returns how many jobs are still waiting
 * (not counting the one currently running)
 * caller must hold queue_lock
 */
int job_count_waiting(void)
{
    int n = 0;
    for (int i = 0; i < job_count; i++)
    {
        if (job_queue[i].status == STATUS_WAITING)
            n++;
    }
    return n;
}

/*
 * job_print_list - prints the job table like the spec shows
 * caller must hold queue_lock
 */
void job_print_list(void)
{
    const char *pname;
    if (current_policy == POLICY_SJF)
        pname = "SJF";
    else if (current_policy == POLICY_PRIORITY)
        pname = "Priority";
    else
        pname = "FCFS";

    printf("Total number of jobs in the queue are: %d\n", job_count);
    printf("Scheduling Policy: %s.\n", pname);

    if (job_count == 0)
    {
        printf("(no jobs in queue)\n");
        return;
    }

    printf("%-20s %-10s %-5s %-12s %s\n",
           "Name", "CPU_Time", "Pri", "Arrival_time", "Progress");

    for (int i = 0; i < job_count; i++)
    {
        job_t *j = &job_queue[i];

        struct tm *t = localtime(&j->arrival_time);
        char tstr[16];
        strftime(tstr, sizeof(tstr), "%H:%M:%S", t);

        const char *prog = (j->status == STATUS_RUNNING) ? "Run" : "";
        printf("%-20s %-10d %-5d %-12s %s\n",
               j->name, j->cpu_time, j->priority, tstr, prog);
    }
}

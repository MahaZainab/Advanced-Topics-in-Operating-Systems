

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <time.h>

#include "job.h"
#include "evaluation.h"

/*
 * print_performance - called on quit or after a test command finishes
 */
void print_performance(int num_jobs)
{
    if (num_jobs == 0)
    {
        printf("No jobs have been completed yet.\n");
        return;
    }

    double avg_turnaround = total_turnaround / (double)num_jobs;
    double avg_cpu = total_cpu_time / (double)num_jobs;
    double avg_waiting = total_waiting / (double)num_jobs;

    double elapsed = difftime(time(NULL), first_job_time);
    double throughput = (elapsed > 0.0) ? ((double)num_jobs / elapsed) : 0.0;

    printf("Total number of jobs that have been submitted: %d\n", num_jobs);
    printf("Average turnaround time would be:  %.2f seconds\n", avg_turnaround);
    printf("Average CPU time would be:         %.2f seconds\n", avg_cpu);
    printf("Average waiting time would be:     %.2f seconds\n", avg_waiting);
    printf("Throughput would be:               %.3f No./second\n", throughput);
}

/*
 * reset_metrics - wipe the counters so test runs don't bleed into each other
 */
void reset_metrics(void)
{
    total_turnaround = 0.0;
    total_cpu_time = 0.0;
    total_waiting = 0.0;
    first_job_time = 0;
    total_submitted = 0;
}

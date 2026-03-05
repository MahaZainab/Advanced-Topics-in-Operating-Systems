#ifndef JOB_H
#define JOB_H

#include <time.h>
#include <pthread.h>

#define POLICY_FCFS     0
#define POLICY_SJF      1
#define POLICY_PRIORITY 2

#define STATUS_WAITING  0
#define STATUS_RUNNING  1
#define STATUS_DONE     2

#define MAX_JOBS     100
#define MAX_NAME_LEN  64

typedef struct {
    char   name[MAX_NAME_LEN];
    int    cpu_time;
    int    priority;
    time_t arrival_time;
    time_t start_time;
    time_t finish_time;
    int    status;
} job_t;

extern job_t           job_queue[MAX_JOBS];
extern int             job_count;
extern int             current_policy;
extern int             total_submitted;

extern double          total_turnaround;
extern double          total_cpu_time;
extern double          total_waiting;
extern time_t          first_job_time;

extern pthread_mutex_t queue_lock;
extern pthread_cond_t  queue_not_empty;
extern pthread_cond_t  queue_not_full;

void job_init(void);
void job_add(const char *name, int cpu_time, int priority);
void job_remove_head(void);
int  job_count_waiting(void);
void job_print_list(void);

#endif

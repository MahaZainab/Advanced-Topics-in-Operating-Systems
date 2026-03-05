#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "job.h"
#include "scheduling.h"
#include "evaluation.h"
#include "cmd_parser.h"

static const char *helpmenu[] = {
    "run <job> <time> <pri>  : submit a job named <job>,",
    "                          execution time is <time>,",
    "                          priority is <pri>.",
    "list                    : display the job status.",
    "fcfs                    : change the scheduling policy to FCFS.",
    "sjf                     : change the scheduling policy to SJF.",
    "priority                : change the scheduling policy to priority.",
    "test <benchmark> <policy> <num_of_jobs> <arrival_rate>",
    "     <priority_levels> <min_CPU_time> <max_CPU_time>",
    "quit                    : exit AUbatch",
    NULL
};

int cmd_help(int nargs, char **args) {
    (void)nargs; (void)args;
    printf("\n");
    for (int i = 0; helpmenu[i] != NULL; i++)
        printf("%s\n", helpmenu[i]);
    printf("\n");
    return CMD_OK;
}

int cmd_run(int nargs, char **args) {
    if (nargs < 3 || nargs > 4) {
        printf("Usage: run <job> <time> <priority>\n");
        return CMD_EINVAL;
    }

    const char *name = args[1];
    int cpu_time = atoi(args[2]);
    int priority = (nargs == 4) ? atoi(args[3]) : 1;

    if (cpu_time <= 0) {
        printf("Error: execution time must be a positive number\n");
        return CMD_EINVAL;
    }
    if (priority <= 0) {
        printf("Error: priority must be a positive number\n");
        return CMD_EINVAL;
    }

    pthread_mutex_lock(&queue_lock);

    while (job_count >= MAX_JOBS)
        pthread_cond_wait(&queue_not_full, &queue_lock);

    job_add(name, cpu_time, priority);
    sort_queue(current_policy);

    int wait_time = compute_expected_wait();
    int qsize     = job_count;

    const char *pname;
    if (current_policy == POLICY_SJF)           pname = "SJF";
    else if (current_policy == POLICY_PRIORITY) pname = "Priority";
    else                                        pname = "FCFS";

    pthread_cond_signal(&queue_not_empty);
    pthread_mutex_unlock(&queue_lock);

    printf("Job %s was submitted.\n", name);
    printf("Total number of jobs in the queue: %d\n", qsize);
    printf("Expected waiting time: %d seconds\n", wait_time);
    printf("Scheduling Policy: %s.\n", pname);

    return CMD_OK;
}

int cmd_list(int nargs, char **args) {
    (void)nargs; (void)args;
    pthread_mutex_lock(&queue_lock);
    job_print_list();
    pthread_mutex_unlock(&queue_lock);
    return CMD_OK;
}

static void switch_policy(int new_policy, const char *pname) {
    pthread_mutex_lock(&queue_lock);
    current_policy = new_policy;
    sort_queue(new_policy);
    int waiting = job_count_waiting();
    pthread_mutex_unlock(&queue_lock);
    printf("Scheduling policy is switched to %s. All the %d waiting jobs have been rescheduled.\n",
           pname, waiting);
}

int cmd_fcfs(int nargs, char **args) {
    (void)nargs; (void)args;
    switch_policy(POLICY_FCFS, "FCFS");
    return CMD_OK;
}

int cmd_sjf(int nargs, char **args) {
    (void)nargs; (void)args;
    switch_policy(POLICY_SJF, "SJF");
    return CMD_OK;
}

int cmd_priority(int nargs, char **args) {
    (void)nargs; (void)args;
    switch_policy(POLICY_PRIORITY, "Priority");
    return CMD_OK;
}

typedef struct {
    char benchmark[MAX_NAME_LEN];
    int  num_jobs;
    int  min_cpu;
    int  max_cpu;
    int  pri_levels;
    unsigned int interval_us;
} test_args_t;

static void *test_submit_thread(void *arg) {
    test_args_t *t = (test_args_t *)arg;

    srand((unsigned int)time(NULL));

    for (int i = 0; i < t->num_jobs; i++) {
        int cpu = t->min_cpu + (rand() % (t->max_cpu - t->min_cpu + 1));
        int pri = 1 + (rand() % t->pri_levels);

        pthread_mutex_lock(&queue_lock);
        while (job_count >= MAX_JOBS)
            pthread_cond_wait(&queue_not_full, &queue_lock);
        job_add(t->benchmark, cpu, pri);
        sort_queue(current_policy);
        pthread_cond_signal(&queue_not_empty);
        pthread_mutex_unlock(&queue_lock);

        if (i < t->num_jobs - 1)
            usleep(t->interval_us);
    }

    printf("\nAll %d jobs submitted. Use 'list' to check status.\n> ", t->num_jobs);
    fflush(stdout);
    free(t);
    return NULL;
}

int cmd_test(int nargs, char **args) {
    if (nargs != 8) {
        printf("Usage: test <benchmark> <policy> <num_of_jobs> <arrival_rate> "
               "<priority_levels> <min_CPU_time> <max_CPU_time>\n");
        return CMD_EINVAL;
    }

    const char *benchmark  = args[1];
    const char *policy_str = args[2];
    int    num_jobs        = atoi(args[3]);
    double arrival_rate    = atof(args[4]);
    int    pri_levels      = atoi(args[5]);
    int    min_cpu         = atoi(args[6]);
    int    max_cpu         = atoi(args[7]);

    if (num_jobs <= 0) {
        printf("Error: num_of_jobs must be a positive integer\n");
        return CMD_EINVAL;
    }
    if (arrival_rate <= 0.0) {
        printf("Error: arrival_rate must be a positive number\n");
        return CMD_EINVAL;
    }
    if (pri_levels <= 0) {
        printf("Error: priority_levels must be a positive integer\n");
        return CMD_EINVAL;
    }
    if (min_cpu <= 0 || max_cpu <= 0) {
        printf("Error: CPU times must be positive integers\n");
        return CMD_EINVAL;
    }
    if (min_cpu > max_cpu) {
        printf("Error: min_CPU_time cannot be greater than max_CPU_time\n");
        return CMD_EINVAL;
    }

    int new_policy;
    const char *pname;
    if (strcmp(policy_str, "fcfs") == 0) {
        new_policy = POLICY_FCFS;     pname = "FCFS";
    } else if (strcmp(policy_str, "sjf") == 0) {
        new_policy = POLICY_SJF;      pname = "SJF";
    } else if (strcmp(policy_str, "priority") == 0) {
        new_policy = POLICY_PRIORITY; pname = "Priority";
    } else {
        printf("Error: unknown policy '%s' - use fcfs, sjf, or priority\n", policy_str);
        return CMD_EINVAL;
    }

    pthread_mutex_lock(&queue_lock);
    current_policy = new_policy;
    reset_metrics();
    pthread_mutex_unlock(&queue_lock);

    printf("Running test: policy=%s, jobs=%d, arrival_rate=%.2f, cpu=[%d,%d]\n",
           pname, num_jobs, arrival_rate, min_cpu, max_cpu);
    printf("Jobs are being submitted in the background. Use 'list' to check status.\n");

    test_args_t *targs = malloc(sizeof(test_args_t));
    if (!targs) { perror("malloc"); return CMD_EINVAL; }
    strncpy(targs->benchmark, benchmark, MAX_NAME_LEN - 1);
    targs->benchmark[MAX_NAME_LEN - 1] = '\0';
    targs->num_jobs    = num_jobs;
    targs->min_cpu     = min_cpu;
    targs->max_cpu     = max_cpu;
    targs->pri_levels  = pri_levels;
    targs->interval_us = (unsigned int)(1000000.0 / arrival_rate);

    pthread_t tid;
    pthread_create(&tid, NULL, test_submit_thread, targs);
    pthread_detach(tid);

    return CMD_OK;
}

int cmd_quit(int nargs, char **args) {
    (void)nargs; (void)args;

    while (1) {
        pthread_mutex_lock(&queue_lock);
        int left = job_count;
        pthread_mutex_unlock(&queue_lock);
        if (left == 0) break;
        usleep(200000);
    }

    print_performance(total_submitted);

    aubatch_running = 0;
    pthread_cond_broadcast(&queue_not_empty);
    pthread_cond_broadcast(&queue_not_full);
    exit(0);
}

static struct {
    const char *name;
    int (*func)(int nargs, char **args);
} cmdtable[] = {
    { "help",     cmd_help     },
    { "h",        cmd_help     },
    { "?",        cmd_help     },
    { "run",      cmd_run      },
    { "r",        cmd_run      },
    { "list",     cmd_list     },
    { "l",        cmd_list     },
    { "fcfs",     cmd_fcfs     },
    { "sjf",      cmd_sjf      },
    { "priority", cmd_priority },
    { "test",     cmd_test     },
    { "quit",     cmd_quit     },
    { "q",        cmd_quit     },
    { NULL, NULL }
};

static int cmd_dispatch(char *line) {
    char *args[MAXARGS];
    int   nargs = 0;
    char *word, *ctx;

    for (word = strtok_r(line, " \t\n", &ctx);
         word != NULL;
         word = strtok_r(NULL, " \t\n", &ctx)) {
        if (nargs >= MAXARGS) {
            printf("too many arguments\n");
            return CMD_E2BIG;
        }
        args[nargs++] = word;
    }

    if (nargs == 0) return CMD_OK;

    for (int i = 0; cmdtable[i].name != NULL; i++) {
        if (strcmp(args[0], cmdtable[i].name) == 0)
            return cmdtable[i].func(nargs, args);
    }

    printf("%s: command not found, type 'help' to see available commands\n", args[0]);
    return CMD_ENOENT;
}

void run_command_loop(void) {
    char  *line   = NULL;
    size_t buflen = 0;

    while (aubatch_running) {
        printf("> ");
        fflush(stdout);

        if (getline(&line, &buflen, stdin) < 0) {
            printf("\n");
            cmd_quit(0, NULL);
            break;
        }
        cmd_dispatch(line);
    }

    free(line);
}

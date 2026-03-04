
#ifndef EVALUATION_H
#define EVALUATION_H

/* print avg turnaround, cpu time, waiting time, and throughput */
void print_performance(int num_jobs);

/* zero out the counters before a new test run */
void reset_metrics(void);

#endif

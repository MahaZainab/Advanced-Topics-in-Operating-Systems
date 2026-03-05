#ifndef SCHEDULING_H
#define SCHEDULING_H

#include "job.h"

void *scheduling_thread(void *arg);
void *dispatching_thread(void *arg);

void sort_queue(int policy);
int  compute_expected_wait(void);

extern volatile int aubatch_running;

#endif

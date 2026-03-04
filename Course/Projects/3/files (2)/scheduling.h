/*
 * COMP7500 - Advanced Operating Systems
 * Project 3: AUbatch
 *
 * Maha
 * Auburn University
 *
 * scheduling.h
 */

#ifndef SCHEDULING_H
#define SCHEDULING_H

#include "job.h"

/* the two thread functions */
void *scheduling_thread(void *arg);
void *dispatching_thread(void *arg);

/* sort the waiting part of the queue by the given policy */
void sort_queue(int policy);

/* sum of cpu_times of all queued jobs - used for wait time estimate */
int compute_expected_wait(void);

/* set to 0 when user types quit */
extern volatile int aubatch_running;

#endif

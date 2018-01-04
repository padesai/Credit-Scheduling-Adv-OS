#ifndef __GT_INCLUDE_H
#define __GT_INCLUDE_H

#include "gt_signal.h"
#include "gt_spinlock.h"
#include "gt_tailq.h"
#include "gt_bitops.h"

#include "gt_uthread.h"
#include "gt_pq.h"
#include "gt_kthread.h"
#include <math.h>

int g_argc;
char *g_argv[2];

typedef struct group {

long int sum_exec_time; // sum of 8 threads' execution times subtracting the queueing and scheduling time 	
long int mean_exec_time;
long int std_dev_exec_time;

long int sum_tot_time; // sum of 8 threads' total execution time including queueing and scheduling time
long int mean_tot_time;
long int std_dev_tot_time;

long int uthreads_exec_time[8]; // each thread's execution time
long int uthreads_tot_time[8]; // each thread's total time

int num_threads_reached_done;

} group_t;

group_t groups[16];

#endif

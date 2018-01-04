#include <stdio.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sched.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <assert.h>

#include "gt_include.h"
/**********************************************************************/
/** DECLARATIONS **/
/**********************************************************************/


/**********************************************************************/
/* kthread runqueue and env */

/* XXX: should be the apic-id */
#define KTHREAD_CUR_ID	0

/**********************************************************************/
/* uthread scheduling */
static void uthread_context_func(int);
static int uthread_init(uthread_struct_t *u_new);

/**********************************************************************/
/* uthread creation */
#define UTHREAD_DEFAULT_SSIZE (16 * 1024 + 100)

extern int uthread_create(uthread_t *u_tid, int (*u_func)(void *), void *u_arg, uthread_group_t u_gid);

/**********************************************************************/
/** DEFNITIONS **/
/**********************************************************************/

/**********************************************************************/
/* uthread scheduling */

/* Assumes that the caller has disabled vtalrm and sigusr1 signals */
/* uthread_init will be using */
static int uthread_init(uthread_struct_t *u_new)
{
	stack_t oldstack;
	sigset_t set, oldset;
	struct sigaction act, oldact;

	gt_spin_lock(&(ksched_shared_info.uthread_init_lock));

	/* Register a signal(SIGUSR2) for alternate stack */
	act.sa_handler = uthread_context_func;
	act.sa_flags = (SA_ONSTACK | SA_RESTART);
	if(sigaction(SIGUSR2,&act,&oldact))
	{
		fprintf(stderr, "uthread sigusr2 install failed !!");
		return -1;
	}

	/* Install alternate signal stack (for SIGUSR2) */
	if(sigaltstack(&(u_new->uthread_stack), &oldstack))
	{
		fprintf(stderr, "uthread sigaltstack install failed.");
		return -1;
	}

	/* Unblock the signal(SIGUSR2) */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR2);
	sigprocmask(SIG_UNBLOCK, &set, &oldset);


	/* SIGUSR2 handler expects kthread_runq->cur_uthread
	 * to point to the newly created thread. We will temporarily
	 * change cur_uthread, before entering the synchronous call
	 * to SIGUSR2. */

	/* kthread_runq is made to point to this new thread
	 * in the caller. Raise the signal(SIGUSR2) synchronously */
#if 0
	raise(SIGUSR2);
#endif
	syscall(__NR_tkill, kthread_cpu_map[kthread_apic_id()]->tid, SIGUSR2);

	/* Block the signal(SIGUSR2) */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR2);
	sigprocmask(SIG_BLOCK, &set, &oldset);
	if(sigaction(SIGUSR2,&oldact,NULL))
	{
		fprintf(stderr, "uthread sigusr2 revert failed !!");
		return -1;
	}

	/* Disable the stack for signal(SIGUSR2) handling */
	u_new->uthread_stack.ss_flags = SS_DISABLE;

	/* Restore the old stack/signal handling */
	if(sigaltstack(&oldstack, NULL))
	{
		fprintf(stderr, "uthread sigaltstack revert failed.");
		return -1;
	}

	gt_spin_unlock(&(ksched_shared_info.uthread_init_lock));
	return 0;
}



extern void uthread_schedule(uthread_struct_t * (*kthread_best_sched_uthread)(kthread_runqueue_t *))
{
	kthread_context_t *k_ctx;
	kthread_runqueue_t *kthread_runq;
	uthread_struct_t *u_obj;
	struct timeval uthread_tv2;
	long int uthread_elapsed_time; // time spent executing before being preempted. 


	/* Signals used for cpu_thread scheduling */
	// kthread_block_signal(SIGVTALRM);
	// kthread_block_signal(SIGUSR1);

#if 0
	fprintf(stderr, "uthread_schedule invoked !!\n");
#endif	
	
//credits scaling
//if statement 
//whether to return if statement
// if statement whether to combine with others
//if statement placement
//printf placement locks

	k_ctx = kthread_cpu_map[kthread_apic_id()];
	kthread_runq = &(k_ctx->krunqueue);

	if((u_obj = kthread_runq->cur_uthread))
	{	
		

		if(*g_argv[1] == '1')
		{
			//fprintf(stderr,"hellooooooooooooooooooooooo");
			//gt_spin_lock(&(kthread_runq->kthread_runqlock));
			//kthread_runq->kthread_runqlock.holder = 0x04;						
			gettimeofday(&(uthread_tv2), NULL);		
			uthread_elapsed_time = ((long int)uthread_tv2.tv_sec - (long int)u_obj->uthread_tv1.tv_sec) * 1000000 + ((long int)uthread_tv2.tv_usec - (long int)u_obj->uthread_tv1.tv_usec);
			

			// printf("uthread from group %d with id %d the timings: %ld .. %ld .. %ld .. %ld ", u_obj->uthread_gid, u_obj->uthread_tid, (long int) uthread_tv2.tv_sec, (long int) u_obj->uthread_tv1.tv_sec, (long int) uthread_tv2.tv_usec, (long int) u_obj->uthread_tv1.tv_usec );
			
			if ( ((u_obj->uthread_time_slice)*1000 - uthread_elapsed_time) <= 0 ) {
				u_obj->uthread_credits = -1; 
			}
			else {
				u_obj->uthread_credits -= (uthread_elapsed_time / ((u_obj->uthread_time_slice)*1000)) * u_obj->uthread_credits;
				
			} 			

			//printf("uthread from group %d with id %d has credits: %ld\n", u_obj->uthread_gid, u_obj->uthread_tid, u_obj->uthread_credits);			
		
			
			u_obj->uthread_exec_time += uthread_elapsed_time;
					

			//gt_spin_unlock(&(kthread_runq->kthread_runqlock));			
						
			// / DEFAULT_UTHREAD_CREDITS;
/*
			if (u_obj->uthread_credits > 0){
	
				gt_spin_lock(&(kthread_runq->kthread_runqlock));
				kthread_runq->kthread_runqlock.holder = 0x01;
				printf("uthread from group %d with id %d has credits: %d\n", u_obj->uthread_gid, u_obj->uthread_tid, u_obj->uthread_credits);
				gt_spin_unlock(&(kthread_runq->kthread_runqlock));	
			
			}	
*/
		
		}

		/*Go through the runq and schedule the next thread to run */
		kthread_runq->cur_uthread = NULL;
		
		if(u_obj->uthread_state & (UTHREAD_DONE | UTHREAD_CANCELLED))
		{
			if(u_obj->uthread_state & UTHREAD_DONE) {

				//printf("uthread from group %d with id %d executed in: %ld with priority: %d\n", u_obj->uthread_gid, u_obj->uthread_tid, u_obj->uthread_exec_time, u_obj->uthread_priority);
			
						
				if (u_obj->uthread_tid % 4 == 0 && u_obj->uthread_tid < 32) {
					
					groups[0].sum_exec_time = groups[0].sum_exec_time + u_obj->uthread_exec_time;

					groups[0].uthreads_exec_time[groups[0].num_threads_reached_done] = u_obj->uthread_exec_time;
								
					groups[0].num_threads_reached_done++;					

					if (groups[0].num_threads_reached_done == 8){
						groups[0].mean_exec_time = groups[0].sum_exec_time / 8;

						long int sum_of_diff_squared_0 = 0;
					
						for(int i=0; i<8;i++){
							 sum_of_diff_squared_0 += pow((groups[0].uthreads_exec_time[i] - groups[0].mean_exec_time), 2); 
						}						
						
						groups[0].std_dev_exec_time = sqrt(sum_of_diff_squared_0/8);
	
						printf("Mean execution time of threads with credits 25 and matrix size 32 was: %ld us with std_dev: %ld\n", groups[0].mean_exec_time, groups[0].std_dev_exec_time);
					}												

				}
				else if (u_obj->uthread_tid % 4 == 1 && u_obj->uthread_tid < 32) {

					groups[1].sum_exec_time = groups[1].sum_exec_time + u_obj->uthread_exec_time;

					groups[1].uthreads_exec_time[groups[1].num_threads_reached_done] = u_obj->uthread_exec_time;

					groups[1].num_threads_reached_done++;

					if (groups[1].num_threads_reached_done == 8){
						groups[1].mean_exec_time = groups[1].sum_exec_time / 8;

						long int sum_of_diff_squared_1 = 0;
					
						for(int i=0; i<8;i++){
							 sum_of_diff_squared_1 += pow((groups[1].uthreads_exec_time[i] - groups[1].mean_exec_time), 2); 
						}						
						
						groups[1].std_dev_exec_time = sqrt(sum_of_diff_squared_1/8);
	
						printf("Mean execution time of threads with credits 50 and matrix size 32 was: %ld us with std_dev: %ld\n", groups[1].mean_exec_time, groups[1].std_dev_exec_time);					
					}
					
				}
				else if (u_obj->uthread_tid % 4 == 2 && u_obj->uthread_tid < 32) {
					groups[2].sum_exec_time = groups[2].sum_exec_time + u_obj->uthread_exec_time;

					groups[2].uthreads_exec_time[(u_obj->uthread_tid % 4) + groups[2].num_threads_reached_done - 2] = u_obj->uthread_exec_time;

					groups[2].num_threads_reached_done++;

					if (groups[2].num_threads_reached_done == 8){
						groups[2].mean_exec_time = groups[2].sum_exec_time / 8;

						long int sum_of_diff_squared_2 = 0;
					
						for(int i=0; i<8;i++){
							 sum_of_diff_squared_2 += pow((groups[2].uthreads_exec_time[i] - groups[2].mean_exec_time), 2); 
						}						
						
						groups[2].std_dev_exec_time = sqrt(sum_of_diff_squared_2/8);
	
						printf("Mean execution time of threads with credits 75 and matrix size 32 was: %ld us with std_dev: %ld\n", groups[2].mean_exec_time, groups[2].std_dev_exec_time);					
					}
															
				}
				else if (u_obj->uthread_tid % 4 == 3 && u_obj->uthread_tid < 32) {
					groups[3].sum_exec_time = groups[3].sum_exec_time + u_obj->uthread_exec_time;

					groups[3].uthreads_exec_time[(u_obj->uthread_tid % 4) + groups[3].num_threads_reached_done - 3] = u_obj->uthread_exec_time;

					groups[3].num_threads_reached_done++;

					if (groups[3].num_threads_reached_done == 8){
						groups[3].mean_exec_time = groups[3].sum_exec_time / 8;

						long int sum_of_diff_squared_3 = 0;
					
						for(int i=0; i<8;i++){
							 sum_of_diff_squared_3 += pow((groups[3].uthreads_exec_time[i] - groups[3].mean_exec_time), 2); 
						}						
						
						groups[3].std_dev_exec_time = sqrt(sum_of_diff_squared_3/8);
	
						printf("Mean execution time of threads with credits 100 and matrix size 32 was: %ld us with std_dev: %ld\n", groups[3].mean_exec_time, groups[3].std_dev_exec_time);					
					}
															
				}	
				else if (u_obj->uthread_tid % 4 == 0 && u_obj->uthread_tid < 64) {
					groups[4].sum_exec_time = groups[4].sum_exec_time + u_obj->uthread_exec_time;

					groups[4].uthreads_exec_time[(u_obj->uthread_tid % 4) + groups[4].num_threads_reached_done] = u_obj->uthread_exec_time;

					groups[4].num_threads_reached_done++;

					if (groups[4].num_threads_reached_done == 8){
						groups[4].mean_exec_time = groups[4].sum_exec_time / 8;
						
						long int sum_of_diff_squared_4 = 0;
					
						for(int i=0; i<8;i++){
							 sum_of_diff_squared_4 += pow((groups[4].uthreads_exec_time[i] - groups[4].mean_exec_time), 2); 
						}						
						
						groups[4].std_dev_exec_time = sqrt(sum_of_diff_squared_4/8);
	
						printf("Mean execution time of threads with credits 25 and matrix size 64 was: %ld us with std_dev: %ld\n", groups[4].mean_exec_time, groups[4].std_dev_exec_time);					
					}
															
				}	
				else if (u_obj->uthread_tid % 4 == 1 && u_obj->uthread_tid < 64) {
					groups[5].sum_exec_time = groups[5].sum_exec_time + u_obj->uthread_exec_time;
					
					groups[5].uthreads_exec_time[(u_obj->uthread_tid % 4) + groups[5].num_threads_reached_done - 1] = u_obj->uthread_exec_time;
					
					groups[5].num_threads_reached_done++;

					if (groups[5].num_threads_reached_done == 8){
						groups[5].mean_exec_time = groups[5].sum_exec_time / 8;

						long int sum_of_diff_squared_5 = 0;
					
						for(int i=0; i<8;i++){
							 sum_of_diff_squared_5 += pow((groups[5].uthreads_exec_time[i] - groups[5].mean_exec_time), 2); 
						}						
						
						groups[5].std_dev_exec_time = sqrt(sum_of_diff_squared_5/8);
	
						printf("Mean execution time of threads with credits 50 and matrix size 64 was: %ld us with std_dev: %ld\n", groups[5].mean_exec_time, groups[5].std_dev_exec_time);					
					}
															
				}				
				else if (u_obj->uthread_tid % 4 == 2 && u_obj->uthread_tid < 64) {
					groups[6].sum_exec_time = groups[6].sum_exec_time + u_obj->uthread_exec_time;
				
					groups[6].uthreads_exec_time[(u_obj->uthread_tid % 4) + groups[6].num_threads_reached_done - 2] = u_obj->uthread_exec_time;					
					
					groups[6].num_threads_reached_done++;

					if (groups[6].num_threads_reached_done == 8){
						groups[6].mean_exec_time = groups[6].sum_exec_time / 8;

						long int sum_of_diff_squared_6 = 0;
					
						for(int i=0; i<8;i++){
							 sum_of_diff_squared_6 += pow((groups[6].uthreads_exec_time[i] - groups[6].mean_exec_time), 2); 
						}						
						
						groups[6].std_dev_exec_time = sqrt(sum_of_diff_squared_6/8);
	
						printf("Mean execution time of threads with credits 75 and matrix size 64 was: %ld us with std_dev: %ld\n", groups[6].mean_exec_time, groups[6].std_dev_exec_time);					
					}
															
				}	
				else if (u_obj->uthread_tid % 4 == 3 && u_obj->uthread_tid < 64) {
					groups[7].sum_exec_time = groups[7].sum_exec_time + u_obj->uthread_exec_time;

					groups[7].uthreads_exec_time[(u_obj->uthread_tid % 4) + groups[7].num_threads_reached_done - 3] = u_obj->uthread_exec_time;
					
					groups[7].num_threads_reached_done++;	
					
					if (groups[7].num_threads_reached_done == 8){
						groups[7].mean_exec_time = groups[7].sum_exec_time / 8;

						long int sum_of_diff_squared_7 = 0;
					
						for(int i=0; i<8;i++){
							 sum_of_diff_squared_7 += pow((groups[7].uthreads_exec_time[i] - groups[7].mean_exec_time), 2); 
						}						
						
						groups[7].std_dev_exec_time = sqrt(sum_of_diff_squared_7/8);
	
						printf("Mean execution time of threads with credits 100 and matrix size 64 was: %ld us with std_dev: %ld\n", groups[7].mean_exec_time, groups[7].std_dev_exec_time);					
					}
														
				}	
				else if (u_obj->uthread_tid % 4 == 0 && u_obj->uthread_tid < 96) {
					groups[8].sum_exec_time = groups[8].sum_exec_time + u_obj->uthread_exec_time;
	
					groups[8].uthreads_exec_time[(u_obj->uthread_tid % 4) + groups[8].num_threads_reached_done] = u_obj->uthread_exec_time;

					groups[8].num_threads_reached_done++;

					if (groups[8].num_threads_reached_done == 8){
						groups[8].mean_exec_time = groups[8].sum_exec_time / 8;
			
						long int sum_of_diff_squared_8 = 0;
					
						for(int i=0; i<8;i++){
							 sum_of_diff_squared_8 += pow((groups[8].uthreads_exec_time[i] - groups[8].mean_exec_time), 2); 
						}						
						
						groups[8].std_dev_exec_time = sqrt(sum_of_diff_squared_8/8);
	
						printf("Mean execution time of threads with credits 25 and matrix size 128 was: %ld us with std_dev: %ld\n", groups[8].mean_exec_time, groups[8].std_dev_exec_time);					
					}
															
				}	
				else if (u_obj->uthread_tid % 4 == 1 && u_obj->uthread_tid < 96) {
					groups[9].sum_exec_time = groups[9].sum_exec_time + u_obj->uthread_exec_time;

					groups[9].uthreads_exec_time[(u_obj->uthread_tid % 4) + groups[9].num_threads_reached_done - 1] = u_obj->uthread_exec_time;

					groups[9].num_threads_reached_done++;					
					
					if (groups[9].num_threads_reached_done == 8){
						groups[9].mean_exec_time = groups[9].sum_exec_time / 8;

						long int sum_of_diff_squared_9 = 0;
					
						for(int i=0; i<8;i++){
							 sum_of_diff_squared_9 += pow((groups[9].uthreads_exec_time[i] - groups[9].mean_exec_time), 2); 
						}						
						
						groups[9].std_dev_exec_time = sqrt(sum_of_diff_squared_9/8);
	
						printf("Mean execution time of threads with credits 50 and matrix size 128 was: %ld us with std_dev: %ld\n", groups[9].mean_exec_time, groups[9].std_dev_exec_time);					
					}
															
				}	
				else if (u_obj->uthread_tid % 4 == 2 && u_obj->uthread_tid < 96) {
					groups[10].sum_exec_time = groups[10].sum_exec_time + u_obj->uthread_exec_time;
					
					groups[10].uthreads_exec_time[(u_obj->uthread_tid % 4) + groups[10].num_threads_reached_done - 2] = u_obj->uthread_exec_time;
				
					groups[10].num_threads_reached_done++;

					if (groups[10].num_threads_reached_done == 8){
						groups[10].mean_exec_time = groups[10].sum_exec_time / 8;

						long int sum_of_diff_squared_10 = 0;
					
						for(int i=0; i<8;i++){
							 sum_of_diff_squared_10 += pow((groups[10].uthreads_exec_time[i] - groups[10].mean_exec_time), 2); 
						}						
						
						groups[10].std_dev_exec_time = sqrt(sum_of_diff_squared_10/8);
	
						printf("Mean execution time of threads with credits 75 and matrix size 128 was: %ld us with std_dev: %ld\n", groups[10].mean_exec_time, groups[10].std_dev_exec_time);					
					}
															
				}	
				else if (u_obj->uthread_tid % 4 == 3 && u_obj->uthread_tid < 96) {
					groups[11].sum_exec_time = groups[11].sum_exec_time + u_obj->uthread_exec_time;
					
					groups[11].uthreads_exec_time[(u_obj->uthread_tid % 4) + groups[11].num_threads_reached_done - 3] = u_obj->uthread_exec_time;

					groups[11].num_threads_reached_done++;

					if (groups[11].num_threads_reached_done == 8){
						groups[11].mean_exec_time = groups[11].sum_exec_time / 8;
			
						long int sum_of_diff_squared_11 = 0;
					
						for(int i=0; i<8;i++){
							 sum_of_diff_squared_11 += pow((groups[11].uthreads_exec_time[i] - groups[11].mean_exec_time), 2); 
						}						
						
						groups[11].std_dev_exec_time = sqrt(sum_of_diff_squared_11/8);
	
						printf("Mean execution time of threads with credits 100 and matrix size 128 was: %ld us with std_dev: %ld\n", groups[11].mean_exec_time, groups[11].std_dev_exec_time);					
					}
										
				}	
				else if (u_obj->uthread_tid % 4 == 0 && u_obj->uthread_tid < 128) {
					groups[12].sum_exec_time = groups[12].sum_exec_time + u_obj->uthread_exec_time;

					groups[12].uthreads_exec_time[(u_obj->uthread_tid % 4) + groups[12].num_threads_reached_done] = u_obj->uthread_exec_time;

					groups[12].num_threads_reached_done++;

					if (groups[12].num_threads_reached_done == 8){
						groups[12].mean_exec_time = groups[12].sum_exec_time / 8;
			
						long int sum_of_diff_squared_12 = 0;
					
						for(int i=0; i<8;i++){
							 sum_of_diff_squared_12 += pow((groups[12].uthreads_exec_time[i] - groups[12].mean_exec_time), 2); 
						}						
						
						groups[12].std_dev_exec_time = sqrt(sum_of_diff_squared_12/8);
	
						printf("Mean execution time of threads with credits 25 and matrix size 256 was: %ld us with std_dev: %ld\n", groups[12].mean_exec_time, groups[12].std_dev_exec_time);					
					}
															
				}	
				else if (u_obj->uthread_tid % 4 == 1 && u_obj->uthread_tid < 128) {
					groups[13].sum_exec_time = groups[13].sum_exec_time + u_obj->uthread_exec_time;

					groups[13].uthreads_exec_time[(u_obj->uthread_tid % 4) + groups[13].num_threads_reached_done - 1] = u_obj->uthread_exec_time;
	
					groups[13].num_threads_reached_done++;

					if (groups[13].num_threads_reached_done == 8){
						groups[13].mean_exec_time = groups[13].sum_exec_time / 8;
			
						long int sum_of_diff_squared_13 = 0;
					
						for(int i=0; i<8;i++){
							 sum_of_diff_squared_13 += pow((groups[13].uthreads_exec_time[i] - groups[13].mean_exec_time), 2); 
						}						
						
						groups[13].std_dev_exec_time = sqrt(sum_of_diff_squared_13/8);
	
						printf("Mean execution time of threads with credits 50 and matrix size 256 was: %ld us with std_dev: %ld\n", groups[13].mean_exec_time, groups[13].std_dev_exec_time);					
					}
															
				}	
				else if (u_obj->uthread_tid % 4 == 2 && u_obj->uthread_tid < 128) {
					groups[14].sum_exec_time = groups[14].sum_exec_time + u_obj->uthread_exec_time;
					
					groups[14].uthreads_exec_time[(u_obj->uthread_tid % 4) + groups[14].num_threads_reached_done - 2] = u_obj->uthread_exec_time;

					groups[14].num_threads_reached_done++;	

					if (groups[14].num_threads_reached_done == 8){
						groups[14].mean_exec_time = groups[14].sum_exec_time / 8;
			
						long int sum_of_diff_squared_14 = 0;
					
						for(int i=0; i<8;i++){
							 sum_of_diff_squared_14 += pow((groups[14].uthreads_exec_time[i] - groups[14].mean_exec_time), 2); 
						}						
						
						groups[14].std_dev_exec_time = sqrt(sum_of_diff_squared_14/8);
	
						printf("Mean execution time of threads with credits 75 and matrix size 256 was: %ld us with std_dev: %ld\n", groups[14].mean_exec_time, groups[14].std_dev_exec_time);					
					}
														
				}	
				else {
					groups[15].sum_exec_time = groups[15].sum_exec_time + u_obj->uthread_exec_time;
					
					groups[15].uthreads_exec_time[(u_obj->uthread_tid % 4) + groups[15].num_threads_reached_done - 3] = u_obj->uthread_exec_time;

					groups[15].num_threads_reached_done++;	

					if (groups[15].num_threads_reached_done == 8){
						groups[15].mean_exec_time = groups[15].sum_exec_time / 8;
					
						long int sum_of_diff_squared_15 = 0;
					
						for(int i=0; i<8;i++){
							 sum_of_diff_squared_15 += pow((groups[15].uthreads_exec_time[i] - groups[15].mean_exec_time), 2); 
						}						
						
						groups[15].std_dev_exec_time = sqrt(sum_of_diff_squared_15/8);
	
						printf("Mean execution time of threads with credits 100 and matrix size 256 was: %ld us with std_dev: %ld\n", groups[15].mean_exec_time, groups[15].std_dev_exec_time);
					}
					
				}										
				
				
			}					
	
			
			/* XXX: Inserting uthread into zombie queue is causing improper
			 * cleanup/exit of uthread (core dump) */
			uthread_head_t * kthread_zhead = &(kthread_runq->zombie_uthreads);
			gt_spin_lock(&(kthread_runq->kthread_runqlock));
			kthread_runq->kthread_runqlock.holder = 0x01;
			TAILQ_INSERT_TAIL(kthread_zhead, u_obj, uthread_runq);	
			gt_spin_unlock(&(kthread_runq->kthread_runqlock));
		
			{
				ksched_shared_info_t *ksched_info = &ksched_shared_info;	
				gt_spin_lock(&ksched_info->ksched_lock);
				ksched_info->kthread_cur_uthreads--;
				gt_spin_unlock(&ksched_info->ksched_lock);
			}
		}
		else
		{
			/* XXX: Apply uthread_group_penalty before insertion */
			u_obj->uthread_state = UTHREAD_RUNNABLE;
			if(*g_argv[1] == '1') {
				if (u_obj->uthread_credits <= 0) {
/*									
					gt_spin_lock(&(kthread_runq->kthread_runqlock));
					kthread_runq->kthread_runqlock.holder = 0x01;
					printf("uthread from group %d with id %d has credits: %d\n\n", u_obj->uthread_gid, u_obj->uthread_tid, u_obj->uthread_credits);
					gt_spin_unlock(&(kthread_runq->kthread_runqlock));
					
*/					if (u_obj->uthread_tid % 16 == 0 || u_obj->uthread_tid % 16 == 4 || u_obj->uthread_tid % 16 == 8 || u_obj->uthread_tid % 16 == 12) {
						u_obj->uthread_credits = DEFAULT_UTHREAD_CREDITS;
					}
					else if (u_obj->uthread_tid % 16 == 1 || u_obj->uthread_tid % 16 == 5 || u_obj->uthread_tid % 16 == 9 || u_obj->uthread_tid % 16 == 13) {
						u_obj->uthread_credits = DEFAULT_UTHREAD_CREDITS*2;
					}
					else if (u_obj->uthread_tid % 16 == 2 || u_obj->uthread_tid % 16 == 6 || u_obj->uthread_tid % 16 == 10 || u_obj->uthread_tid % 16 == 14) {
						u_obj->uthread_credits = DEFAULT_UTHREAD_CREDITS*3;
					}
					else {
						u_obj->uthread_credits = DEFAULT_UTHREAD_CREDITS*4;
					}	
				}
	
				if ( u_obj->uthread_credits > 3	 && u_obj->uthread_credits < 10) {			
					
					gt_yield(u_obj);

				}

			}
						
			add_to_runqueue(kthread_runq->expires_runq, &(kthread_runq->kthread_runqlock), u_obj);
			/* XXX: Save the context (signal mask not saved) */
			if(sigsetjmp(u_obj->uthread_env, 0))
				return;
		}
	}

	
	/* kthread_best_sched_uthread acquires kthread_runqlock. Dont lock it up when calling the function. */
	if(!(u_obj = kthread_best_sched_uthread(kthread_runq)))
	{
		//fprintf(stderr, "entered here about to 2\n");
		/* Done executing all uthreads. Return to main */
		/* XXX: We can actually get rid of KTHREAD_DONE flag */
		if(ksched_shared_info.kthread_tot_uthreads && !ksched_shared_info.kthread_cur_uthreads)
		{

			fprintf(stderr, "Quitting kthread (%d)\n", k_ctx->cpuid);
			k_ctx->kthread_flags |= KTHREAD_DONE;
		}

		siglongjmp(k_ctx->kthread_env, 1);
		return;
	}
	
	
	kthread_runq->cur_uthread = u_obj;
	if((u_obj->uthread_state == UTHREAD_INIT) && (uthread_init(u_obj)))
	{
		fprintf(stderr, "uthread_init failed on kthread(%d)\n", k_ctx->cpuid);
		exit(0);
	}

	
	u_obj->uthread_state = UTHREAD_RUNNING;
	
	/* Re-install the scheduling signal handlers */
	kthread_install_sighandler(SIGVTALRM, k_ctx->kthread_sched_timer);
	kthread_install_sighandler(SIGUSR1, k_ctx->kthread_sched_relay);
	
	if (*g_argv[1] == '1'){

		gettimeofday(&(u_obj->uthread_tv1), NULL);
/*					
		gt_spin_lock(&(kthread_runq->kthread_runqlock));
		kthread_runq->kthread_runqlock.holder = 0x01;
		printf("uthread from group %d with id %d has credits: %d\n", u_obj->uthread_gid, u_obj->uthread_tid, u_obj->uthread_credits);
		gt_spin_unlock(&(kthread_runq->kthread_runqlock));	
*/
	}	

	/* Jump to the selected uthread context */
	siglongjmp(u_obj->uthread_env, 1);

	return;
}


/* For uthreads, we obtain a seperate stack by registering an alternate
 * stack for SIGUSR2 signal. Once the context is saved, we turn this 
 * into a regular stack for uthread (by using SS_DISABLE). */
static void uthread_context_func(int signo)
{
	uthread_struct_t *cur_uthread;
	kthread_runqueue_t *kthread_runq;

	kthread_runq = &(kthread_cpu_map[kthread_apic_id()]->krunqueue);

	//printf("..... uthread_context_func .....\n");
	/* kthread->cur_uthread points to newly created uthread */
	if(!sigsetjmp(kthread_runq->cur_uthread->uthread_env,0))
	{
		/* In UTHREAD_INIT : saves the context and returns.
		 * Otherwise, continues execution. */
		/* DONT USE any locks here !! */
		assert(kthread_runq->cur_uthread->uthread_state == UTHREAD_INIT);
		kthread_runq->cur_uthread->uthread_state = UTHREAD_RUNNABLE;
		return;
	}

	/* UTHREAD_RUNNING : siglongjmp was executed. */
	cur_uthread = kthread_runq->cur_uthread;
	assert(cur_uthread->uthread_state == UTHREAD_RUNNING);
	/* Execute the uthread task */
	cur_uthread->uthread_func(cur_uthread->uthread_arg);
	cur_uthread->uthread_state = UTHREAD_DONE;

	uthread_schedule(&sched_find_best_uthread);
	
	return;
}

/**********************************************************************/
/* uthread creation */

extern kthread_runqueue_t *ksched_find_target(uthread_struct_t *);

extern void gt_yield(uthread_struct_t *u_obj) {
	
	kthread_runqueue_t *kthread_runq;
	kthread_context_t *k_ctx;
	
	k_ctx = kthread_cpu_map[kthread_apic_id()];

	kthread_runq = &(k_ctx->krunqueue);	

	add_to_runqueue(kthread_runq->expires_runq, &(kthread_runq->kthread_runqlock), u_obj);
	if(sigsetjmp(u_obj->uthread_env, 0))
		uthread_schedule(&sched_find_best_uthread);
}

extern int uthread_create(uthread_t *u_tid, int (*u_func)(void *), void *u_arg, uthread_group_t u_gid)
{
	kthread_runqueue_t *kthread_runq;
	uthread_struct_t *u_new;

	/* Signals used for cpu_thread scheduling */
	// kthread_block_signal(SIGVTALRM);
	// kthread_block_signal(SIGUSR1);

	/* create a new uthread structure and fill it */
	if(!(u_new = (uthread_struct_t *)MALLOCZ_SAFE(sizeof(uthread_struct_t))))
	{
		fprintf(stderr, "uthread mem alloc failure !!");
		exit(0);
	}

	u_new->uthread_state = UTHREAD_INIT;
	u_new->uthread_priority = DEFAULT_UTHREAD_PRIORITY;
	u_new->uthread_weight = DEFAULT_UTHREAD_WEIGHT;
	u_new->uthread_cap = DEFAULT_UTHREAD_CAP;
	u_new->uthread_gid = u_gid;
	u_new->uthread_func = u_func;
	u_new->uthread_arg = u_arg;
	u_new->uthread_exec_time = DEFAULT_UTHREAD_EXEC_TIME;
	
	if (u_new->uthread_tid < 32 ) {
		u_new->uthread_weight = DEFAULT_UTHREAD_WEIGHT;
	}
	else if (u_new->uthread_tid < 64) {
		u_new->uthread_weight = DEFAULT_UTHREAD_WEIGHT*4;
	}
	else if (u_new->uthread_tid < 96) {
		u_new->uthread_weight = DEFAULT_UTHREAD_WEIGHT*4*4;
	}
	else {
		u_new->uthread_weight = DEFAULT_UTHREAD_WEIGHT*4*4*4;
	}

	u_new->uthread_time_slice = (u_new->uthread_weight / DEFAULT_TOTAL_WEIGHT) * DEFAULT_TIME_SLICE;

	if (u_new->uthread_tid % 16 == 0 || u_new->uthread_tid % 16 == 4 || u_new->uthread_tid % 16 == 8 || u_new->uthread_tid % 16 == 12) {
		u_new->uthread_credits = DEFAULT_UTHREAD_CREDITS;
	}
	else if (u_new->uthread_tid % 16 == 1 || u_new->uthread_tid % 16 == 5 || u_new->uthread_tid % 16 == 9 || u_new->uthread_tid % 16 == 13) {
		u_new->uthread_credits = DEFAULT_UTHREAD_CREDITS*2;
	}
	else if (u_new->uthread_tid % 16 == 2 || u_new->uthread_tid % 16 == 6 || u_new->uthread_tid % 16 == 10 || u_new->uthread_tid % 16 == 14) {
		u_new->uthread_credits = DEFAULT_UTHREAD_CREDITS*3;
	}
	else {
		u_new->uthread_credits = DEFAULT_UTHREAD_CREDITS*4;
	}

	/* Allocate new stack for uthread */
	u_new->uthread_stack.ss_flags = 0; /* Stack enabled for signal handling */
	if(!(u_new->uthread_stack.ss_sp = (void *)MALLOC_SAFE(UTHREAD_DEFAULT_SSIZE)))
	{
		fprintf(stderr, "uthread stack mem alloc failure !!");
		return -1;
	}
	u_new->uthread_stack.ss_size = UTHREAD_DEFAULT_SSIZE;


	{
		ksched_shared_info_t *ksched_info = &ksched_shared_info;

		gt_spin_lock(&ksched_info->ksched_lock);
		u_new->uthread_tid = ksched_info->kthread_tot_uthreads++;
		ksched_info->kthread_cur_uthreads++;
		gt_spin_unlock(&ksched_info->ksched_lock);
	}

	/* XXX: ksched_find_target should be a function pointer */
	kthread_runq = ksched_find_target(u_new);

	*u_tid = u_new->uthread_tid;


	/* Queue the uthread for target-cpu. Let target-cpu take care of initialization. */
	add_to_runqueue(kthread_runq->active_runq, &(kthread_runq->kthread_runqlock), u_new);


	/* WARNING : DONOT USE u_new WITHOUT A LOCK, ONCE IT IS ENQUEUED. */

	/* Resume with the old thread (with all signals enabled) */
	// kthread_unblock_signal(SIGVTALRM);
	// kthread_unblock_signal(SIGUSR1);

	return 0;
}

#if 0
/**********************************************************************/
kthread_runqueue_t kthread_runqueue;
kthread_runqueue_t *kthread_runq = &kthread_runqueue;
sigjmp_buf kthread_env;

/* Main Test */
typedef struct uthread_arg
{
	int num1;
	int num2;
	int num3;
	int num4;	
} uthread_arg_t;

#define NUM_THREADS 10
static int func(void *arg);

int main()
{
	uthread_struct_t *uthread;
	uthread_t u_tid;
	uthread_arg_t *uarg;

	int inx;

	/* XXX: Put this lock in kthread_shared_info_t */
	gt_spinlock_init(&uthread_group_penalty_lock);

	/* spin locks are initialized internally */
	kthread_init_runqueue(kthread_runq);

	for(inx=0; inx<NUM_THREADS; inx++)
	{
		uarg = (uthread_arg_t *)MALLOC_SAFE(sizeof(uthread_arg_t));
		uarg->num1 = inx;
		uarg->num2 = 0x33;
		uarg->num3 = 0x55;
		uarg->num4 = 0x77;
		uthread_create(&u_tid, func, uarg, (inx % MAX_UTHREAD_GROUPS));
	}

	kthread_init_vtalrm_timeslice();
	kthread_install_sighandler(SIGVTALRM, kthread_sched_vtalrm_handler);
	if(sigsetjmp(kthread_env, 0) > 0)
	{
		/* XXX: (TODO) : uthread cleanup */
		exit(0);
	}
	
	uthread_schedule(&ksched_priority);
	return(0);
}

static int func(void *arg)
{
	unsigned int count;
#define u_info ((uthread_arg_t *)arg)
	printf("Thread %d created\n", u_info->num1);
	count = 0;
	while(count <= 0xffffff)
	{
		if(!(count % 5000000))
			printf("uthread(%d) => count : %d\n", u_info->num1, count);
		count++;
	}
#undef u_info
	return 0;
}
#endif

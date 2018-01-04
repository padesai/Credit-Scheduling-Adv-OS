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
#include <math.h>

#include "gt_include.h"


#define ROWS 32
#define COLS ROWS
#define SIZE COLS

#define NUM_CPUS 2
#define NUM_GROUPS NUM_CPUS
#define PER_GROUP_COLS (SIZE/NUM_GROUPS)

#define NUM_THREADS 128
#define PER_THREAD_ROWS (SIZE/NUM_THREADS)


/* A[SIZE][SIZE] X B[SIZE][SIZE] = C[SIZE][SIZE]
 * Let T(g, t) be thread 't' in group 'g'. 
 * T(g, t) is responsible for multiplication : 
 * A(rows)[(t-1)*SIZE -> (t*SIZE - 1)] X B(cols)[(g-1)*SIZE -> (g*SIZE - 1)] */

typedef struct matrix
{
	int m32[SIZE][SIZE];
	int m64[SIZE*2][SIZE*2];
	int m128[SIZE*4][SIZE*4];
	int m256[SIZE*8][SIZE*8];

	int rows;
	int cols;
	unsigned int reserved[2];
} matrix_t;

typedef struct __uthread_arg
{
	matrix_t *_A, *_B, *_C;
	unsigned int reserved0;

	unsigned int tid;
	unsigned int gid;
	int start_row; /* start_row -> (start_row + PER_THREAD_ROWS) */
	int start_col; /* start_col -> (start_col + PER_GROUP_COLS) */
	
} uthread_arg_t;
	
struct timeval tv1;

static void generate_matrix(matrix_t *mat, int val, int mat_size)
{

	int i,j;
	mat->rows = mat_size;
	mat->cols = mat_size;
	for(i = 0; i < mat->rows;i++)
		for( j = 0; j < mat->cols; j++ )
		{ 
			if (mat_size == SIZE) {
				mat->m32[i][j] = val;
			}
			else if (mat_size == SIZE*2) {
				mat->m64[i][j] = val;			
			} 	
			else if (mat_size == SIZE*4) {
				mat->m128[i][j] = val;
			}
			else {
				mat->m256[i][j] = val;
			}
		}
	return;
}

static void print_matrix(matrix_t *mat, int mat_size)
{
	int i, j;

	for(i=0;i<mat_size;i++)
	{
		for(j=0;j<mat_size;j++)
		{
			if (mat_size == SIZE){
				printf(" %d ",mat->m32[i][j]);
			}
			if (mat_size == SIZE*2) {
				printf(" %d ",mat->m64[i][j]);
			}	
			if (mat_size == SIZE*4) {
				printf(" %d ",mat->m128[i][j]);
			}	
			if (mat_size == SIZE*8) {
				printf(" %d ",mat->m256[i][j]);
			}		
		}		
		printf("\n");
	}

	return;
}

static void * uthread_mulmat(void *p)
{
	int i, j, k;
	int start_row, end_row;
	int start_col, end_col;
	unsigned int cpuid;
	struct timeval tv2;

#define ptr ((uthread_arg_t *)p)

	i=0; j= 0; k=0;

	start_row = ptr->start_row;
	
	end_row = ptr->_A->rows;
#if 0

#ifdef GT_GROUP_SPLIT
	start_col = ptr->start_col;
	end_col = (ptr->start_col + PER_THREAD_ROWS);
#else
	start_col = 0;
	end_col = SIZE;
#endif

#endif
	start_col = 0;
	end_col = ptr->_A->cols;

#ifdef GT_THREADS
	cpuid = kthread_cpu_map[kthread_apic_id()]->cpuid;
	fprintf(stderr, "\nThread(id:%d, group:%d, cpu:%d) started\n",ptr->tid, ptr->gid, cpuid);
#else
	//fprintf(stderr, "\nThread(id:%d, group:%d) started\n",ptr->tid, ptr->gid);
#endif

	for(i = start_row; i < end_row; i++)
		for(j = start_col; j < end_col; j++)
			for(k = 0; k < end_row; k++) {
				if (end_row == 32) {
					ptr->_C->m32[i][j] += ptr->_A->m32[i][k] * ptr->_B->m32[k][j];
					
				}
				if (end_row == 64) {
					ptr->_C->m64[i][j] += ptr->_A->m64[i][k] * ptr->_B->m64[k][j];			
				}
				if (end_row == 128) {
					ptr->_C->m128[i][j] += ptr->_A->m128[i][k] * ptr->_B->m128[k][j];			
				}
				if (end_row == 256) {
					ptr->_C->m256[i][j] += ptr->_A->m256[i][k] * ptr->_B->m256[k][j];			
				}				
			}
#ifdef GT_THREADS
	fprintf(stderr, "\nThread(id:%d, group:%d, cpu:%d) finished (TIME : %ld us)\n",
			ptr->tid, ptr->gid, cpuid, ((long int)tv2.tv_sec - (long int)tv1.tv_sec)*1000000 + (tv2.tv_usec - tv1.tv_usec));
#else/*
	gettimeofday(&tv2,NULL);
	fprintf(stderr, "\nThread(id:%d, group:%d) finished (TIME : %ld us)\n",
			ptr->tid, ptr->gid, ((long int)tv2.tv_sec - (long int)tv1.tv_sec)*1000000 + (tv2.tv_usec - tv1.tv_usec)); 
*/
#endif

#undef ptr
	return 0;
}

matrix_t A32, B32, C32;
matrix_t A64, B64, C64;
matrix_t A128, B128, C128;
matrix_t A256, B256, C256;

static void init_matrices()
{
	generate_matrix(&A32, 1, SIZE);
	generate_matrix(&B32, 1, SIZE);
	generate_matrix(&C32, 0, SIZE);

	generate_matrix(&A64, 1, SIZE*2);
	generate_matrix(&B64, 1, SIZE*2);
	generate_matrix(&C64, 0, SIZE*2);

	generate_matrix(&A128, 1, SIZE*4);
	generate_matrix(&B128, 1, SIZE*4);
	generate_matrix(&C128, 0, SIZE*4);

	generate_matrix(&A256, 1, SIZE*8);
	generate_matrix(&B256, 1, SIZE*8);
	generate_matrix(&C256, 0, SIZE*8);

	return;
}


uthread_arg_t uargs[NUM_THREADS];
uthread_t utids[NUM_THREADS];


int main(int argc, char *argv[])
{

    if ( argc != 2 ) /* argc should be 2 for correct execution */
    {
        /* We print argv[0] assuming it is the program name */
        printf( "usage: %s scheduling_mechanism\n", argv[0] );
	
    }
  
    else 
    {
	g_argc = argc;
	g_argv[0] = argv[0];
	g_argv[1] = argv[1];
/*
	if(*argv[1] == '1') {
		fprintf(stderr, "hello!!!!!!!!!!!!!!!!!!!!!!!!!");	
	}
*/
	
	for (int i=0; i < 16; i++) {
		groups[i].sum_tot_time	= 0;
		groups[i].sum_tot_time = 0;
		groups[i].num_threads_reached_done = 0;
	}

	uthread_arg_t *uarg;
	int inx;

	gtthread_app_init();

	init_matrices();

	gettimeofday(&tv1,NULL);	
	
	for(inx=0; inx<NUM_THREADS; inx++)
	{
		uarg = &uargs[inx];
		
		if (inx < 32) {			
			uarg->_A = &A32;
			uarg->_B = &B32;
			uarg->_C = &C32;
		}
		
		else if (inx < 64) {
			uarg->_A = &A64;
			uarg->_B = &B64;
			uarg->_C = &C64;
		}		
		
		else if (inx < 96) {
			uarg->_A = &A128;
			uarg->_B = &B128;
			uarg->_C = &C128;
		}
	
		else {
			uarg->_A = &A256;
			uarg->_B = &B256;
			uarg->_C = &C256;
		}

		uarg->tid = inx;

		uarg->gid = (inx % NUM_GROUPS);

		uarg->start_row = 0; //(inx * PER_THREAD_ROWS);
#ifdef GT_GROUP_SPLIT
		/* Wanted to split the columns by groups !!! */
		uarg->start_col = (uarg->gid * PER_GROUP_COLS);
#endif

		uthread_create(&utids[inx], uthread_mulmat, uarg, uarg->gid);
	}



	gtthread_app_exit();
	
     }													

	return(0);
	// print_matrix(&C);
	// fprintf(stderr, "********************************");
	
     
}

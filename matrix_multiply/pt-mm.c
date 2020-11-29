/* Multi-threaded matrix multiply program
 *
 *   June 04, 2019, Michael Albert
 *   Modified June 06, 2019
 *
 */

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

/* idx macro calculates the correct 2-d based 1-d index
 * of a location (x,y) in an array that has col columns.
 * This is calculated in row major order.
 */

#define idx(x,y,col)  ((x)*(col) + (y))

/* Struct for storing thread info */
struct info {
  double *A, *B, *C;
  int x, y, z, threads, threadn, times;
};
pthread_barrier_t barrier;

  /* Calculation of subset of matrix elements */
  void matrix_calc (double *A, double *B, double *C, int x, int y, int z, int threads,
    int threadn) {
    /* Determine what elements to calculate */
    int elements = x*z;
    int perthread = elements/threads;
    int remainder = elements%threads;
    if (perthread == 0) {
      perthread = 1;
      remainder = 0;
    }
    int start, finish, ixstart, ixfinish;
    start = finish = ixstart = ixfinish = 0;
    int jxstart[z];
    int jxfinish[z];
    if (threadn < remainder) {
      start = (threadn * perthread) + threadn;
      finish = start + perthread;
    }
    else if (threadn < elements) {
      start = (threadn * perthread) + remainder;
      finish = start + perthread - 1;
    }
    else {
      return;
    }
    ixstart = start/z;
    ixfinish = finish/z + 1;
    jxstart[ixstart] = start%z;
    jxfinish[ixstart] = finish%z + 1;
    if (ixstart +1 < ixfinish) {
      jxfinish[ixstart] = z;
      for (int i = ixstart + 1; i < ixfinish; i++) {
        jxstart[i] = 0;
        jxfinish[i] = z;
      }
      jxfinish[ixfinish-1] = finish%z + 1;
    }

    /* Calculate the elements */
    int ix, jx, kx;
    for (ix = ixstart; ix < ixfinish; ix++) {
      // Rows of solution
      for (jx = jxstart[ix]; jx < jxfinish[ix]; jx++) {
        // Columns of solution
        float tval = 0;
        for (kx = 0; kx < y; kx++) {
          // Sum the A row time B column
          tval += A[idx(ix,kx,y)] * B[idx(kx,jx,z)];
        }
        C[idx(ix,jx,z)] = tval;
      }
    }
    return;
  }

/* Matrix Multiply:
 *  C (x by z)  =  A ( x by y ) times B (y by z)
 *  This is the slow n^3 algorithm
 *  A and B are not be modified
 */

 /* Thread body for mat_mul */
 void* mul_body (void *arg) {
   /* Extract info */
   struct info* argcp = (struct info*) arg;
   double *A = argcp->A;
   double *B = argcp->B;
   double *C = argcp->C;
   int x = argcp->x;
   int y = argcp->y;
   int z = argcp->z;
   int threads = argcp->threads;
   int threadn = argcp->threadn;
   matrix_calc(A, B, C, x, y, z, threads, threadn);
   pthread_exit((void *) NULL);
 }

void MatMul (double *A, double *B, double *C, int x, int y, int z, int threads) {
  pthread_t ids[threads];
  struct info threadinfo[threads];
  /* Create threads */
  for (int i = 0; i < threads; i++) {
    /* Declare and set info */
    threadinfo[i].A = A;
    threadinfo[i].B = B;
    threadinfo[i].C = C;
    threadinfo[i].x = x;
    threadinfo[i].y = y;
    threadinfo[i].z = z;
    threadinfo[i].threads = threads;
    threadinfo[i].threadn = i;
    int err = pthread_create (&ids[i], NULL, mul_body, (void *)&threadinfo[i]);
    if (err) {
      fprintf (stderr, "Can't create thread %d\n", i);
      exit (1);
    }
  }
  // wait for all threads by joining them
  for (int i = 0; i < threads; i++) {
    pthread_join(ids[i], NULL);
  }
  return;
}

/* Matrix Square:
 *  B = A ^ 2*times
 *
 *    A are not be modified.
 */

 /* Thread body for mat_mul */
 void* square_body (void *arg) {
   /* Extract info */
   struct info* argcp = (struct info*) arg;
   double *A = argcp->A;
   double *B = argcp->B;
   double *C = argcp->C;
   int x = argcp->x;
   int threads = argcp->threads;
   int threadn = argcp->threadn;
   int times = argcp->times;
   matrix_calc(A, A, B, x, x, x, threads, threadn);


   if (times > 1) {
     for (int i = 1; i < times; i+= 2) {
       pthread_barrier_wait(&barrier);
       matrix_calc(B, B, C, x, x, x, threads, threadn);
       pthread_barrier_wait(&barrier);
       if (i == times - 1) {
         memcpy(B, C, sizeof(double)*x*x);
       }
       else {
         matrix_calc(C, C, B, x, x, x, threads, threadn);
       }
     }
   }


   pthread_exit((void *) NULL);
 }

void MatSquare (double *A, double *B, int x, int times, int threads) {
  pthread_t ids[threads];
  struct info threadinfo[threads];
  double *C = (double *)malloc(sizeof(double) * x * x);
  /* Init barrier */
  pthread_barrier_init(&barrier, NULL, threads);
  /* Create threads */
  for (int i = 0; i < threads; i++) {
    /* Declare and set info */
    threadinfo[i].A = A;
    threadinfo[i].B = B;
    threadinfo[i].C = C;
    threadinfo[i].x = x;
    threadinfo[i].threads = threads;
    threadinfo[i].threadn = i;
    threadinfo[i].times = times;
    int err = pthread_create (&ids[i], NULL, square_body, (void *)&threadinfo[i]);
    if (err) {
      fprintf (stderr, "Can't create thread %d\n", i);
      exit (1);
    }
  }
  /* Wait for all threads by joining them */
  for (int i = 0; i < threads; i++) {
    pthread_join(ids[i], NULL);
  }
  free(C);
  pthread_barrier_destroy(&barrier); //Destroy barrier
  return;
}

/* Print a matrix: */
void MatPrint (double *A, int x, int y)
{
  int ix, iy;

  for (ix = 0; ix < x; ix++) {
    printf ("Row %d: ", ix);
    for (iy = 0; iy < y; iy++)
      printf (" %10.5G", A[idx(ix,iy,y)]);
    printf ("\n");
  }
}


/* Generate data for a matrix: */
void MatGen (double *A, int x, int y, int rand)
{
  int ix, iy;

  for (ix = 0; ix < x ; ix++) {
    for (iy = 0; iy < y ; iy++) {
      A[idx(ix,iy,y)] = ( rand ?
			  ((double)(random() % 200000000))/2000000000.0 :
			  (1.0 + (((double)ix)/100.0) + (((double)iy/1000.0))));
    }
  }
}

/* Print a help message on how to run the program */

void usage(char *prog)
{
  fprintf (stderr, "%s: [-Tdr] -n val -x val -y val -z val\n", prog);
  fprintf (stderr, "%s: [-Tdr] -s num -n val -x val\n", prog);
  exit(1);
}


/* Main function
 *
 *  args:  -T   -- record the program computation time
 *         -d   -- debug and print results
 *         -r   -- use random data between 0 and 1
 *         -N   -- number of threads to create
 *         -s t -- square the matrix t times
 *         -x   -- rows of the first matrix, r & c for squaring
 *         -y   -- cols of A, rows of B
 *         -z   -- cols of B
 *
 */

int main (int argc, char ** argv)
{
  extern char *optarg;   /* defined by getopt(3) */
  int ch;                /* for use with getopt(3) */

  /* option data */
  int x = 0, y = 0, z = 0;
  int threads = 8;
  int timer = 0;
  int debug = 0;
  int square = 0;
  int useRand = 0;
  int sTimes = 0;

  while ((ch = getopt(argc, argv, "Tdrs:n:x:y:z:")) != -1) {
    switch (ch) {
    case 'T':  /* timing */
      timer = 1;
      break;
    case 'd':  /* debug */
      debug = 1;
      break;
    case 'r':  /* debug */
      useRand = 1;
      srandom(time(NULL));
      break;
    case 's':  /* s times */
      sTimes = atoi(optarg);
      square = 1;
      break;
    case 'n':  /* s times */
      threads = atoi(optarg);
      break;
    case 'x':  /* x size */
      x = atoi(optarg);
      break;
    case 'y':  /* y size */
      y = atoi(optarg);
      break;
    case 'z':  /* z size */
      z = atoi(optarg);
      break;
    case '?': /* help */
    default:
      usage(argv[0]);
    }
  }

  /* verify options are correct. */
  if (square) {
    if (y != 0 || z != 0 || x <= 0 || sTimes < 1) {
      fprintf (stderr, "Inconsistent options\n");
      usage(argv[0]);
    }
  } else if (x <= 0 || y <= 0 || z <= 0 || threads <= 0) {
    fprintf (stderr, "-n, -x, -y, and -z all need to be specified or -s, -n, and -x.\n");
    usage(argv[0]);
  }

  /* timers */
  clock_t start_time, end_time, cpu_time;
  time_t wall_time;
  struct timeval start_tv, end_tv;

  /* Matrix storage */
  double *A;
  double *B;
  double *C;

  if (square) {
    A = (double *) malloc (sizeof(double) * x * x);
    B = (double *) malloc (sizeof(double) * x * x);
    MatGen(A,x,x,useRand);
    /* Calculate run time */
    if (timer) {
      gettimeofday(&start_tv, NULL);
      start_time = clock();
      MatSquare(A, B, x, sTimes, threads);
      end_time = clock();
      gettimeofday(&end_tv, NULL);
      cpu_time = end_time - start_time;
      wall_time = end_tv.tv_sec - start_tv.tv_sec;
      printf("Clock time is %ld, CPU time is %ld\n", wall_time, cpu_time);
    }
    /* Run normally */
    else {
      MatSquare(A, B, x, sTimes, threads);
    }
    if (debug) {
      printf ("-------------- orignal matrix ------------------\n");
      MatPrint(A,x,x);
      printf ("--------------  result matrix ------------------\n");
      MatPrint(B,x,x);
    }
  } else {
    A = (double *) malloc (sizeof(double) * x * y);
    B = (double *) malloc (sizeof(double) * y * z);
    C = (double *) malloc (sizeof(double) * x * z);
    MatGen(A,x,y,useRand);
    MatGen(B,y,z,useRand);
    /* Calculate run time */
    if (timer) {
      gettimeofday(&start_tv, NULL);
      start_time = clock();
      MatMul(A, B, C, x, y, z, threads);
      end_time = clock();
      gettimeofday(&end_tv, NULL);
      cpu_time = end_time - start_time;
      wall_time = end_tv.tv_sec - start_tv.tv_sec;
      printf("Clock time is %ld, CPU time is %ld\n", wall_time, cpu_time);
    }
    /* Run normally */
    else {
      MatMul(A, B, C, x, y, z, threads);
    }
    if (debug) {
      printf ("-------------- orignal A matrix ------------------\n");
      MatPrint(A,x,y);
      printf ("-------------- orignal B matrix ------------------\n");
      MatPrint(B,y,z);
      printf ("--------------  result C matrix ------------------\n");
      MatPrint(C,x,z);
    }
  }
  return 0;
}

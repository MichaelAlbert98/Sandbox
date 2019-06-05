/* Simple matrix multiply program
 *
 * Phil Nelson, March 5, 2019
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
  int x, y, z, threads, threadn;
};

/* Matrix Multiply:
 *  C (x by z)  =  A ( x by y ) times B (y by z)
 *  This is the slow n^3 algorithm
 *  A and B are not be modified
 */

 /* Thread body for mat_mul */
 void * mul_body (void *arg) {
   /* Extract info */
   int x = ((struct info*) arg)->x;
   int y = ((struct info*) arg)->y;
   int z = ((struct info*) arg)->z;
   int threads = ((struct info*) arg)->threads;
   int threadn = ((struct info*) arg)->threadn;

   /* Determine what elements to calculate */
   int elements = x*y;
   int perthread = elements/threads;
   int remainder = elements%threads;
   if (perthread == 0) {
     perthread = 1;
     remainder = 0;
   }
   int start, finish, ixstart, ixfinish;
   int jxstart[y];
   int jxfinish[y];
   if (threadn < remainder) {
     start = (threadn * perthread) + threadn;
     finish = start + perthread;
   }
   else if (threadn < elements) {
     start = (threadn * perthread) + remainder;
     finish = start + perthread - 1;
   }
   else {
     pthread_exit((void *) NULL);
   }
   ixstart = start/x;
   ixfinish = finish/x + 1;
   for (int i = ixstart; i < ixfinish; i++) {
     jxstart[i] = 0;
     jxfinish[i] = 0;
   }
   jxstart[ixstart] = start%y;
   jxfinish[ixfinish] = finish%y + 1;
   for (int i = ixstart; i < ixfinish; i++) {
     jxfinish[i] = y;
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
         tval += ((struct info*) arg)->A[idx(ix,kx,y)] * ((struct info*) arg)->B[idx(kx,jx,z)];
       }
       ((struct info*) arg)->C[idx(ix,jx,z)] = tval;
     }
   }
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

void MatSquare (double *A, double *B, int x, int times, int threads) {
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
  fprintf (stderr, "%s: [-Tdr] -n val -s num -x val\n", prog);
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
  int threads;
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
  clock_t start_t, end_t, cpu_t;
  time_t start_tvt, end_tvt, wall_t;
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
      start_t = clock();
      MatSquare(A, B, x, sTimes, threads);
      end_t = clock();
      gettimeofday(&end_tv, NULL);
      cpu_t = end_t - start_t;
      wall_t = end_tv.tv_sec - start_tv.tv_sec;
      printf("Clock time is %ld, CPU time is %ld\n", wall_t, cpu_t);
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
      start_t = clock();
      MatMul(A, B, C, x, y, z, threads);
      end_t = clock();
      gettimeofday(&end_tv, NULL);
      cpu_t = end_t - start_t;
      wall_t = end_tv.tv_sec - start_tv.tv_sec;
      printf("Clock time is %ld, CPU time is %ld\n", wall_t, cpu_t);
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

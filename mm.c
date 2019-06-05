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
#include <time.h>
#include <sys/time.h>

/* idx macro calculates the correct 2-d based 1-d index
 * of a location (x,y) in an array that has col columns.
 * This is calculated in row major order.
 */

#define idx(x,y,col)  ((x)*(col) + (y))

/* Matrix Multiply:
 *  C (x by z)  =  A ( x by y ) times B (y by z)
 *  This is the slow n^3 algorithm
 *  A and B are not be modified
 */

void MatMul (double *A, double *B, double *C, int x, int y, int z)
{
  int ix, jx, kx;

  for (ix = 0; ix < x; ix++) {
    // Rows of solution
    for (jx = 0; jx < z; jx++) {
      // Columns of solution
      float tval = 0;
      for (kx = 0; kx < y; kx++) {
	// Sum the A row time B column
	tval += A[idx(ix,kx,y)] * B[idx(kx,jx,z)];
      }
      C[idx(ix,jx,z)] = tval;
    }
  }
}

/* Matrix Square:
 *  B = A ^ 2*times
 *
 *    A are not be modified.
 */

void MatSquare (double *A, double *B, int x, int times)
{
  int i;

  MatMul (A, A, B, x, x, x);
  if (times > 1) {
    /* Need a Temporary for the computation */
    double *T = (double *)malloc(sizeof(double)*x*x);
    for (i = 1; i < times; i+= 2) {
      MatMul (B, B, T, x, x, x);
      if (i == times - 1)
	memcpy(B, T, sizeof(double)*x*x);
      else
	MatMul (T, T, B, x, x, x);
    }
    free(T);
  }
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
  fprintf (stderr, "%s: [-Tdr] -x val -y val -z val\n", prog);
  fprintf (stderr, "%s: [-Tdr] -s num -x val\n", prog);
  exit(1);
}


/* Main function
 *
 *  args:  -T   -- record the program computation time
 *         -d   -- debug and print results
 *         -r   -- use random data between 0 and 1
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
  int timer = 0;
  int debug = 0;
  int square = 0;
  int useRand = 0;
  int sTimes = 0;

  while ((ch = getopt(argc, argv, "Tdrs:x:y:z:")) != -1) {
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
  } else if (x <= 0 || y <= 0 || z <= 0) {
    fprintf (stderr, "-x, -y, and -z all need to be specified or -s and -x.\n");
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
      MatSquare(A, B, x, sTimes);
      end_t = clock();
      gettimeofday(&end_tv, NULL);
      cpu_t = end_t - start_t;
      wall_t = end_tv.tv_sec - start_tv.tv_sec;
      printf("Clock time is %ld, CPU time is %ld\n", wall_t, cpu_t);
    }
    /* Run normally */
    else {
      MatSquare(A, B, x, sTimes);
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
      MatMul(A, B, C, x, y, z);
      end_t = clock();
      gettimeofday(&end_tv, NULL);
      cpu_t = end_t - start_t;
      wall_t = end_tv.tv_sec - start_tv.tv_sec;
      printf("Clock time is %ld, CPU time is %ld\n", wall_t, cpu_t);
    }
    /* Run normally */
    else {
      MatMul(A, B, C, x, y, z);
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

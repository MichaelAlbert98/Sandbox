
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef N
#define N 5
#endif

long global_data = 0;
pthread_mutex_t mutex1;

void printids (char *name) {
  pid_t      pid = getpid();
  pthread_t  tid = pthread_self();

  printf ("%s: pid %u tid %lu\n", name, (unsigned) pid, (unsigned long)tid);
}

void fatal (long n) {
  printf ("Fatal error, lock or unlock error, thread %ld.\n", n);
  exit(n);
}

void * thread_body ( void *arg ) {
  long threadn = (long) arg;
  long local_data = random() % 100000;
  long ix;

  printf ("Starting thread %ld, local_data is %ld\n", threadn, local_data);
  for (ix = 0; ix < local_data; ix++) {
    if (pthread_mutex_lock(&mutex1)) { fatal(threadn); }
    global_data ++;
    if (pthread_mutex_unlock(&mutex1)) { fatal(threadn); }
  }
  pthread_exit((void *)local_data);
}


int main () {
  pthread_t ids[N];
  int err;
  long i;

  long final_data = 0;

  srandom(0);
  pthread_mutex_init(&mutex1, NULL);

  for (i = 0; i < N; i++) {
    err = pthread_create (&ids[i], NULL, thread_body, (void *)i);
    if (err) {
      fprintf (stderr, "Can't create thread %ld\n", i);
      exit (1);
    }
  }

  printids("main");

  void *retval;

  for (i=0; i < N; i++) {
    pthread_join(ids[i], &retval);
    final_data += (long)retval;
  }

  printf ("global_data is %ld,  final_data is %ld\n", global_data, final_data);

  pthread_mutex_destroy(&mutex1);  // Not needed, but here for completeness
  return 0;
}

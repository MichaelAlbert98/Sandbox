

#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef N
#define N 5
#endif



int global_jackets = 10;
struct queue deadthreads;
char* global_craft[] = {"kayak", "canoe", "sailboat"};
int craft_jackets[] = {1, 2, 4};

//monitor
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
struct queue groups;

/* Queue implementation by Phil Nelson */
struct node {
  int data;
  struct node *next;
};

struct queue {
  int size;
  struct node *head;
  struct node *tail;
};

void queue_init (struct queue *queue) {
  queue->size = 0;
  queue->head = NULL;
  queue->tail = NULL;
}

bool queue_isEmpty (struct queue *queue) {
  return queue->head == NULL;
}

void queue_insert (struct queue* queue, long value) {
  struct node *tmp = malloc(sizeof(struct node));
  if (tmp == NULL) {
    fputs ("malloc failed\n", stderr);
    exit(1);
  }

  queue->size += 1;
  /* create the node */
  tmp->data = value;
  tmp->next = NULL;

  if (queue->head == NULL) {
    queue->head = tmp;
  } else {
    queue->tail->next = tmp;
  }
  queue->tail = tmp;
}

int queue_remove ( struct queue *queue ) {
  int retval = 0;
  struct node *tmp;

  if (!queue_isEmpty(queue)) {
    queue->size -= 1;
    tmp = queue->head;
    retval = tmp->data;
    queue->head = tmp->next;
    free(tmp);
  }
  return retval;
}
/* End of queue implementation */


void printids (char *name) {
  pid_t      pid = getpid();
  pthread_t  tid = pthread_self();

  printf ("%s: pid %u tid %lu\n", name, (unsigned) pid, (unsigned long)tid);
}

void fatal (int n) {
  printf ("Fatal error, lock or unlock error, thread %d.\n", n);
  exit(n);
}

void putjackets (int request, long threadn) {
  /* Add jackets back and signal */
  if (pthread_mutex_lock(&mutex1)) { fatal(threadn); }
  global_jackets += request;
  pthread_cond_broadcast(&cond1);
  /* Unlock mutex */
  if (pthread_mutex_unlock(&mutex1)) { fatal(threadn); }
  return;
}

int getjackets (int request, long threadn) {
  /* Lock mutex */
  if (pthread_mutex_lock(&mutex1)) { fatal(threadn); }
  /* Add to waiting queue */
  if ((global_jackets < request && groups.size < 5) || groups.size > 0) {
    /* Return if queue too large */
    if (groups.size >= 5) {
      /* Unlock mutex */
      if (pthread_mutex_unlock(&mutex1)) { fatal(threadn); }
      return -1;
    }
    queue_insert(&groups, threadn);
    printf("Group %ld waiting for %d lifevests.\n", threadn, request);
    printf("Size of queue is currently %d.\n", groups.size);
    /* Wait in queue order until jackets available */
    while (global_jackets < request || groups.head->data < threadn) {
      pthread_cond_wait(&cond1, &mutex1);
    }
  }
  global_jackets -= request;
  queue_remove(&groups);
  /* Unlock mutex */
  if (pthread_mutex_unlock(&mutex1)) { fatal(threadn); }
  return 0;
}

void * thread_body ( void *arg ) {
  long threadn = (long) arg;
  int craftnum = random() % 3;
  char *craft = global_craft[craftnum];
  long jackets = craft_jackets[craftnum];

  /* Request jackets */
  if (getjackets(jackets, threadn)) {
    printf("Group %ld got tired of waiting and left.\n", threadn);
  }
  else {
    /* Print group info */
    printf("Group %ld requesting a %s with %ld lifevests.\n"
           , threadn, craft, jackets);
    /* Use craft */
    printf("Group %ld issued %ld lifevests, %d remaining.\n", threadn, jackets, global_jackets);
    sleep(random() % 7);
    /* Return jackets */
    putjackets(jackets, threadn);
    printf("Group %ld returning %ld lifevests, now have %d.\n", threadn, jackets, global_jackets);
  }
  /* Add thread to deadthreads */
  queue_insert(&deadthreads, threadn);
  pthread_exit((void *) jackets);
}

int main (int mainargc, char **mainargv) {
  pthread_t ids[atoi(mainargv[1])];
  int err;
  long i;
  void *retval;
  int grouprate = 10;
  queue_init(&deadthreads);
  queue_init(&groups);

  if (mainargc == 3) {
    grouprate = atoi(mainargv[2]);
    srandom(0);
  }
  else if (mainargc == 4) {
    grouprate = atoi(mainargv[2]);
    srandom(atoi(mainargv[3]));
  }

  for (i = 0; i < atoi(mainargv[1]); i++) {
    sleep(random() % grouprate);
    err = pthread_create (&ids[i], NULL, thread_body, (void *)i);
    if (err) {
      fprintf (stderr, "Can't create thread %ld\n", i);
      exit (1);
    }
    /* Join exited threads */
    while (!queue_isEmpty(&deadthreads)) {
      pthread_join(ids[queue_remove(&deadthreads)], &retval);
    }
  }

  /* Join any possible remaining threads */
  for (i=0; i < 5; i++) {
    pthread_join(ids[i], &retval);
  }

  pthread_mutex_destroy(&mutex1);  // Not needed, but here for completeness
  return 0;
}

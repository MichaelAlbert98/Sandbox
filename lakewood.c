

#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

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

int queue_peek(struct queue *queue, int i) {
  int retval = 0;
  struct node *tmp;

  if (i < queue->size) {
    tmp = queue->head;
    for (int j = 0; j < i; j++) {
      tmp = tmp->next;
    }
    retval = tmp->data;
  }

  return retval;
}

void print_queue(struct queue *queue) {
  int i = 0;
  printf("    Queue: [");
  while (i < queue->size - 1) {
    printf("%d, ", queue_peek(queue,i));
    i++;
  }
  printf("%d]\n", queue_peek(queue,i));
  return;
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
  /* Lock mutex */
  if (pthread_mutex_lock(&mutex1)) { fatal(threadn); }
  /* Add jackets back and signal */
  global_jackets += request;
  printf("Group %ld returning %d lifevests, now have %d.\n", threadn, request, global_jackets);
  pthread_cond_broadcast(&cond1);
  /* Unlock mutex */
  if (pthread_mutex_unlock(&mutex1)) { fatal(threadn); }
  return;
}

int getjackets (char *craft, int request, long threadn) {
  /* Lock mutex */
  if (pthread_mutex_lock(&mutex1)) { fatal(threadn); }
  /* Print group info */
  printf("Group %ld requesting a %s with %d lifevests.\n"
         , threadn, craft, request);
  /* Add to waiting queue */
  if ((global_jackets < request && groups.size < 5) || groups.size > 0) {
    /* Return if queue too large */
    if (groups.size >= 5) {
      /* Unlock mutex */
      if (pthread_mutex_unlock(&mutex1)) { fatal(threadn); }
      return -1;
    }
    queue_insert(&groups, threadn);
    printf("   Group %ld waiting in line for %d lifevests.\n", threadn, request);
    print_queue(&groups);
    /* Wait in queue order until jackets available */
    while (global_jackets < request || groups.head->data < threadn) {
      pthread_cond_wait(&cond1, &mutex1);
      if (groups.head->data == threadn && global_jackets >= request) {
        printf("\t Waiting group %ld may now proceed.\n", threadn);
      }
    }
  }
  global_jackets -= request;
  queue_remove(&groups);
  /* Use craft */
  printf("Group %ld issued %d lifevests, %d remaining.\n", threadn, request, global_jackets);
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
  if (getjackets(craft, jackets, threadn)) {
    printf("Group %ld leaves due to too long a line.\n", threadn);
  }
  else {
    sleep(random() % 7);
    /* Return jackets */
    putjackets(jackets, threadn);
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
  int remainthreads = atoi(mainargv[1]);
  queue_init(&deadthreads);
  queue_init(&groups);

  if (mainargc == 3) {
    grouprate = round(atoi(mainargv[2])/2.0);
    srandom(0);
  }
  else if (mainargc == 4) {
    grouprate = round(atoi(mainargv[2])/2.0);
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
      remainthreads--;
    }
  }

  /* Join remaining threads */
  while (remainthreads != 0) {
    if (!queue_isEmpty(&deadthreads)) {
      pthread_join(ids[queue_remove(&deadthreads)], &retval);
      remainthreads--;
    }
  }

  pthread_mutex_destroy(&mutex1);  // Not needed, but here for completeness
  return 0;
}

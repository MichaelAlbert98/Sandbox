

#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef N
#define N 5
#endif

long global_data = 0;
long global_jackets = 10;
struct queue groups;
const char* global_craft[] = {"kayak", "canoe", "sailboat"};
const long craft_jackets[] = {1, 2, 3};
pthread_mutex_t mutex1;

/* Queue implementation by Phil Nelson */
struct node {
  int data;
  struct node *next;
};

struct queue {
  struct node *head;
  struct node *tail;
};

void queue_init (struct queue *queue) {
  queue->head = NULL;
  queue->tail = NULL;
}

bool queue_isEmpty (struct queue *queue) {
  return queue->head == NULL;
}

/*int queue_size (struct queue *queue) {
  int size = 0;
  struct node pos = queue.head;
  while (pos.next != NULL) {
    size++;
    pos = pos.next;
  }
  return size;
}*/

void queue_insert (struct queue* queue, int value) {
  struct node *tmp = malloc(sizeof(struct node));
  if (tmp == NULL) {
    fputs ("malloc failed\n", stderr);
    exit(1);
  }

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

void fatal (long n) {
  printf ("Fatal error, lock or unlock error, thread %ld.\n", n);
  exit(n);
}

void * thread_body ( void *arg ) {
  long threadn = (long) arg;
  long craft = random() % 3;
  long ix;

  /* Print group info */
  printf("Group num is %ld, craft is %s, number of jackets is %ld\n"
         , threadn, global_craft[craft], craft_jackets[craft]);
  /* Check if jackets available */
  if (global_jackets < craft_jackets[craft]) {
    /* Queue is not full, join and wait */
    if (10 < 5) {

    }
    /* Queue is full, exit */
    else {
      printf("Group num %ld will not wait.\n", threadn);
      pthread_exit((void *) 0);
    }
  }
  if (pthread_mutex_lock(&mutex1)) { fatal(threadn); }
  global_jackets -= craft_jackets[craft];
  if (pthread_mutex_unlock(&mutex1)) { fatal(threadn); }
  pthread_exit((void *)craft_jackets[craft]);
}


int main (int mainargc, char **mainargv) {
  pthread_t ids[atoi(mainargv[1])];
  int err;
  long i;
  void *retval;
  long grouprate = 10;
  queue_init(&groups);

  srandom(0);
  pthread_mutex_init(&mutex1, NULL);

  for (i = 0; i < atoi(mainargv[1]); i++) {
    sleep(random() % grouprate);
    err = pthread_create (&ids[i], NULL, thread_body, (void *)i);
    if (err) {
      fprintf (stderr, "Can't create thread %ld\n", i);
      exit (1);
    }
    //if (!pthread_tryjoin_np(ids[i], &retval)) {
      global_jackets += (long) retval;
    //}

  }

  for (i=0; i < atoi(mainargv[1]); i++) {
    pthread_join(ids[i], &retval);
  }

  pthread_mutex_destroy(&mutex1);  // Not needed, but here for completeness
  return 0;
}

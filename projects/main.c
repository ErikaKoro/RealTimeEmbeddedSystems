/*
 *	File	: pc.c
 *
 *	Title	: Demo Producer/Consumer.
 *
 *	Short	: A solution to the producer consumer problem using
 *		pthreads.
 *
 *	Long 	:
 *
 *	Author	: Andrae Muys
 *
 *	Date	: 18 September 1997
 *
 *	Revised	:
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define QUEUESIZE 10
#define LOOP 100000
#define PRODUCER 16
#define CONSUMER 512

struct timeval times[QUEUESIZE];
double sum = 0.0;

void *producer (void *args);
void *consumer (void *args);

typedef struct {
    void * (*work)(void *);
    void * arg;
}workFunction;


typedef struct {
    workFunction buf[QUEUESIZE];
    long head, tail;
    int full, empty;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
} queue;


void *work(void *workArg){
    printf("THE NUMBER IS: %d", *(int *)workArg);
    return NULL;
}

queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction in);
void queueDel (queue *q, workFunction *out);

int main ()
{
    queue *fifo;
    pthread_t pro[PRODUCER], con[CONSUMER];

    fifo = queueInit();
    if (fifo ==  NULL) {
        fprintf (stderr, "main: Queue Init failed.\n");
        exit (1);
    }

    for(int i = 0; i < PRODUCER; i++) {
        pthread_create(&pro[i], NULL, producer, fifo);
    }
    for(int j = 0; j < CONSUMER; j++) {
        pthread_create(&con[j], NULL, consumer, fifo);
    }
    for(int k = 0; k < PRODUCER; k++) {
        pthread_join(pro[k], NULL);
    }
    float average;
    average = sum / (PRODUCER * LOOP);
    printf("\n\nthe average is %.3f\n", average);

    queueDelete (fifo);

    return 0;
}

void *producer (void *q)
{
    queue *fifo;
    workFunction item;
    int i;

    fifo = (queue *)q;

    for (i = 0; i < LOOP; i++) {
        pthread_mutex_lock (fifo->mut);
        while (fifo->full) {
            //printf ("producer: queue FULL.\n");
            pthread_cond_wait (fifo->notFull, fifo->mut);
        }
        int *p = (int *)malloc(sizeof(int));  // Allocate memory so that the value *p will change and not point to the same previous address
        *p = i;
        item.arg = p;
        item.work = work;
        queueAdd (fifo, item);
        pthread_mutex_unlock (fifo->mut);
        pthread_cond_signal (fifo->notEmpty);  // wake up sleeping or waiting thread
    }

    return (NULL);
}

void *consumer (void *q)
{
    queue *fifo;
    int i;
    workFunction del;

    fifo = (queue *)q;

    while(1) {
        pthread_mutex_lock (fifo->mut);
        while (fifo->empty) {
            //printf ("consumer: queue EMPTY.\n");
            pthread_cond_wait (fifo->notEmpty, fifo->mut);
        }
        queueDel (fifo, &del);
        del.work(del.arg);
        free(del.arg);
        pthread_mutex_unlock (fifo->mut);
        pthread_cond_signal (fifo->notFull);
    }

    return (NULL);
}

/*
  typedef struct {
  int buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
  } queue;
*/

queue *queueInit (void)
{
    queue *q;

    q = (queue *)malloc (sizeof (queue));
    if (q == NULL) return (NULL);

    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
    pthread_mutex_init (q->mut, NULL);
    q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (q->notFull, NULL);
    q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (q->notEmpty, NULL);

    return (q);
}


void queueDelete (queue *q)
{
    pthread_mutex_destroy (q->mut);
    free (q->mut);
    pthread_cond_destroy (q->notFull);
    free (q->notFull);
    pthread_cond_destroy (q->notEmpty);
    free (q->notEmpty);
    free (q);
}

void queueAdd (queue *q, workFunction in)
{
    gettimeofday(&times[q->tail], NULL);
    q->buf[q->tail] = in;
    q->tail++;
    if (q->tail == QUEUESIZE)
        q->tail = 0;
    if (q->tail == q->head)
        q->full = 1;
    q->empty = 0;

    return;
}

void queueDel (queue *q, workFunction *out)
{

    struct timeval endTime;
    *out = q->buf[q->head];
    gettimeofday(&endTime, NULL);
    sum += endTime.tv_sec * 1000000 - times[q->head].tv_sec * 1000000 + endTime.tv_usec - times[q->head].tv_usec;

    q->head++;
    if (q->head == QUEUESIZE)
        q->head = 0;
    if (q->head == q->tail)
        q->empty = 1;
    q->full = 0;

    return;
}

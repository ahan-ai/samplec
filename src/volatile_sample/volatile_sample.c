#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define QUEUE_SIZE 10

void *thread1(void*);
void *thread2(void*);

int i = 1; //global

static int queue_size = QUEUE_SIZE;
static int queue[QUEUE_SIZE];
static int volatile head = 0;
static int volatile tail = 0;

void *producer(void *junk) {
    int *q = (int*)junk;
    while (1)
    {
        int count = (tail + queue_size - head) % queue_size;
        if (count < queue_size - 1)
        {
            q[tail] = tail;
            printf("produce %d\n", q[tail]);
            tail++;
        }
        else
        {
            printf("queue is full\n");
        }
        sleep(1);
    }
}

void *consumer(void *junk) {
    int *q = (int*)junk;
    while(1)
    {
        int count = (tail + queue_size - head) % queue_size;
        if (count > 0)
        {
            printf("consume %d\n", q[head]);
            head = (head + 1) % queue_size;
            
        }
        else
        {
            printf("queue is empty\n");
        }
        sleep(1);
    }
}

int main(void) {
    pthread_t t_a;
    pthread_t t_b; //two thread

    pthread_create(&t_a, NULL, producer, (void*) queue);
    pthread_create(&t_b, NULL, consumer, (void*) queue); //Create thread

    pthread_join(t_a, NULL); // wait t_a thread end
    pthread_join(t_b, NULL); //wait t_b thread end
    exit(0);
}


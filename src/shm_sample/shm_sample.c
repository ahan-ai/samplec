#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <assert.h>

#define BILLION 1000000000L

void *thread1(void*);
void *thread2(void*);

typedef struct shm_queue
{
    void *buf;
    size_t len; // max capacity is len -1
    int head;
    int tail;
} shm_queue_t;

typedef struct double_shm_queue
{
    shm_queue_t *q1;
    shm_queue_t *q2;
} double_shm_queue_t;

int read_shm(shm_queue_t *q, void *buf, size_t len)
{
    int ret = 0;
    size_t queue_size = q->len;
    int count = 0;
    int tail = q->tail;
    int head = q->head;
    int t = 0;

    count = tail -head;
    if (count < 0)
    {
        count += queue_size;
    }
    if (count >= len)
    {
        if (head + len <= queue_size)
        {
            memcpy(buf, q->buf + head, len);
        }
        else
        {
            t = queue_size - head;
            memcpy(buf, q->buf + head, t);
            memcpy(buf + t, q->buf, len - t);
        }
        head += len;
        if (head >= queue_size)
        {
            head -= queue_size;
        }
        __asm__ __volatile ("mfence" ::: "memory");
        q->head = head;
        ret = len;
    }
    else
    {
        ret = -1;
        // printf("queue is no enough to read\n");
        goto l_out;
    }

l_out:
    return ret;
}

int write_shm(shm_queue_t *q, void *buf, size_t len)
{
    int ret = 0;
    int queue_size = q->len;
    int head = q->head;
    int tail = q->tail;
    int count = 0;
    int t = 0;

    count = tail - head;
    if (count < 0)
    {
        count += queue_size;
    }
    if (count + len < queue_size)
    {
        if (tail + len <= queue_size)
        {
            memcpy(q->buf + tail, buf, len);
        }
        else
        {
            t = queue_size - tail;
            memcpy(q->buf + tail, buf, t);
            memcpy(q->buf, buf + t, len - t);
        }
        tail += len;
        if (tail >= queue_size)
        {
            tail -= queue_size;
        }
        __asm__ __volatile ("mfence" ::: "memory");
        q->tail = tail;
        ret = len;
    }
    else
    {
        ret = -1;
        // printf("queue not enough space to write\n");
        goto l_out;
    }

l_out:
    return ret;
}

/**
 * init a share memory queue with specific length
 */
int init_shm(shm_queue_t *queue, size_t len)
{
    int ret = 0;
    void *buf = NULL;

    buf = malloc(len);
    if (NULL == buf)
    {
        ret = -1;
        goto l_out;
    }

    queue->head = 0;
    queue->tail = 0;
    queue->buf = buf;
    queue->len = len;

l_out:
    return ret;
}

void destory_shm(shm_queue_t *queue)
{
    if (queue != NULL && queue->buf != NULL)
    {
        free(queue->buf);
    }
}

void *producer(void *obj) {
    shm_queue_t *q = NULL;
    int data_len = 0;
    void *buf = NULL;
    long total_write_bytes = 0;
    int n = 0;

    q = (shm_queue_t *)obj;

    while (1)
    {
        data_len = rand() % (q->len);
        if (data_len == 0)
        {
            continue;
        }
        buf = malloc(data_len);
        if (!buf)
        {
            continue;
        }
        n = write_shm(q, buf, data_len);
        if (n > 0)
        {
            total_write_bytes += n;
            printf("produce %ld\n", total_write_bytes);
        }
        free(buf);
        buf = NULL;
        sleep(1);
    }
}

void *consumer(void *obj) {
    shm_queue_t *q = NULL;
    void *buf = NULL;
    int data_len = 0;
    int n = 0;
    long total_read_bytes = 0;

    q = (shm_queue_t*)obj;
    while(1)
    {
        data_len = rand() % (q->len);
        if (data_len == 0)
        {
            continue;
        }
        buf = malloc(data_len);
        if (!buf)
        {
            continue;
        }
        n = read_shm(q, buf, data_len);
        if (n > 0)
        {
            total_read_bytes += n;
            printf("consume %ld\n", total_read_bytes);
        }
        free(buf);
        buf = NULL;
        sleep(1);
    }
}

/**
 * write 1000000 times, read 1000000 times
 * evetytime 16bytes is read or write
 */
void *pingpong_thread1(void *obj)
{
    int ret = 0;
    double_shm_queue_t *queues = NULL;
    shm_queue_t *q1 = NULL;
    shm_queue_t *q2 = NULL;
    int count = 1000000;
    const len = 16;
    char buf[len];

    queues = (double_shm_queue_t *)obj;
    q1 = queues->q1;
    q2 = queues->q2;

    while (count > 0)
    {
        while (write_shm(q1, buf, len) < 0);
        while (read_shm(q2, buf, len) < 0);
        count--;
    }    

    return;
}

void *pingpong_thread2(void *obj)
{
    int ret = 0;
    double_shm_queue_t *queues = NULL;
    shm_queue_t *q1 = NULL;
    shm_queue_t *q2 = NULL;
    int count = 1000000;
    const len = 16;
    char buf[len];

    queues = (double_shm_queue_t *)obj;
    q1 = queues->q1;
    q2 = queues->q2;

    while (count > 0)
    {
        while (write_shm(q2, buf, len) < 0);
        while (read_shm(q1, buf, len) < 0);
        count--;
    }    

    return;
}

/**
 * two threads, two shm_queue_t object
 * thread1 write to queue1, and read from queue2
 * thread2 write to queue2, and read from quque1
 *
 * caculate the arg ping-pong time of 100,0000 times
 */
void pingpong_test()
{
    int ret = 0;
    pthread_t t_a, t_b;
    shm_queue_t queue1, queue2;
    double_shm_queue_t queues;
    size_t buf_len1 = 1<<20;
    size_t buf_len2 = 1<<20;
    double total_time, pingpong_time;
    int cpu_thread1 = 0;
    int cpu_thread2 = 1;
    struct timespec start, end;
    uint64_t diff;

    memset(&queue1, 0, sizeof(shm_queue_t));
    memset(&queue2, 0, sizeof(shm_queue_t));
    ret = init_shm(&queue1, buf_len1);
    if (ret < 0)
    {
        goto l_out;
    }
    ret = init_shm(&queue2, buf_len2);
    if (ret < 0)
    {
        goto l_out;
    }

    queues.q1 = &queue1;
    queues.q2 = &queue2;

    clock_gettime(CLOCK_MONOTONIC, &start);
    pthread_create(&t_a, NULL, pingpong_thread1, &queues);
    pthread_create(&t_b, NULL, pingpong_thread2, &queues); //Create thread

    cpu_set_t cs;
    CPU_ZERO(&cs);
    CPU_SET(cpu_thread1, &cs);
    assert(pthread_setaffinity_np(t_a, sizeof(cs), &cs) == 0);
    CPU_ZERO(&cs);
    CPU_SET(cpu_thread2, &cs);
    assert(pthread_setaffinity_np(t_b, sizeof(cs), &cs) == 0);
    pthread_join(t_a, NULL); // wait t_a thread end
    pthread_join(t_b, NULL); // wait t_b thread end
    clock_gettime(CLOCK_MONOTONIC, &end);
    diff = BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;

    printf("total time:%llu nanoseconds\n", (long long unsigned int)diff);
    printf("ping pong time:%llu nanoseconds\n", (long long unsigned int)diff / 1000000);

l_out:
    destory_shm(&queue1);
    destory_shm(&queue2);
    return;
}

int main(void) {
    pingpong_test();
}


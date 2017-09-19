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
#include "ring_buffer.h"

#define BILLION 1000000000L

void *thread1(void*);
void *thread2(void*);

typedef struct double_ring_buffer
{
    ring_buffer_t *q1;
    ring_buffer_t *q2;
} double_ring_buffer_t;

/**
 * write 1000000 times, read 1000000 times
 * evetytime 16 bytes is read or write
 */
void *pingpong_thread1(void *obj)
{
    int ret = 0;
    double_ring_buffer_t *queues = NULL;
    ring_buffer_t *q1 = NULL;
    ring_buffer_t *q2 = NULL;
    int count = 1000000;
    const len = 16;
    char buf[len];

    queues = (double_ring_buffer_t *)obj;
    q1 = queues->q1;
    q2 = queues->q2;

    while (count > 0)
    {
        while (write_ring_buffer(q1, buf, len) < 0);
        while (read_ring_buffer(q2, buf, len) < 0);
        count--;
    }    

    return;
}

void *pingpong_thread2(void *obj)
{
    int ret = 0;
    double_ring_buffer_t *queues = NULL;
    ring_buffer_t *q1 = NULL;
    ring_buffer_t *q2 = NULL;
    int count = 1000000;
    const len = 16;
    char buf[len];

    queues = (double_ring_buffer_t *)obj;
    q1 = queues->q1;
    q2 = queues->q2;

    while (count > 0)
    {
        while (write_ring_buffer(q2, buf, len) < 0);
        while (read_ring_buffer(q1, buf, len) < 0);
        count--;
    }    

    return;
}

/**
 * two threads, two ring_buffer_t object
 * thread1 write to queue1, and read from queue2
 * thread2 write to queue2, and read from quque1
 *
 * calculate the arg ping-pong time of 1000000 times
 */
void pingpong_test()
{
    int ret = 0;
    pthread_t t_a, t_b;
    ring_buffer_t queue1, queue2;
    double_ring_buffer_t queues;
    size_t buf_len1 = 1<<20;
    size_t buf_len2 = 1<<20;
    double total_time, pingpong_time;
    int cpu_thread1 = 0;
    int cpu_thread2 = 1;
    struct timespec start, end;
    uint64_t diff;

    memset(&queue1, 0, sizeof(ring_buffer_t));
    memset(&queue2, 0, sizeof(ring_buffer_t));
    ret = init_ring_buffer(&queue1, buf_len1);
    if (ret < 0)
    {
        goto l_out;
    }
    ret = init_ring_buffer(&queue2, buf_len2);
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
    // Make sure your test machine has at least two cores.
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
    destory_ring_buffer(&queue1);
    destory_ring_buffer(&queue2);
    return;
}

/* A data strcut, with an u32_t in it, total size is 16Byte */
typedef struct data_with_count_16B
{
    uint32_t count;
    char     pad[12];
} data_with_count_16B_t;

void *correctness_thread1(void *obj)
{
    int ret = 0;
    ring_buffer_t *q1 = NULL;
    int count = 1000000;
    const len = 16;
    data_with_count_16B_t buf;

    q1 = (ring_buffer_t *)obj;

    assert(sizeof(buf) == 16);
    while (count > 0)
    {
        buf.count = count;
        while (write_ring_buffer(q1, &buf, len) < 0);
        count--;
    }    

    return;
}

void *correctness_thread2(void *obj)
{
    int ret = 0;
    ring_buffer_t *q1 = NULL;
    int count = 1000000;
    const len = 16;
    data_with_count_16B_t buf;

    q1 = (ring_buffer_t *)obj;

    while (count > 0)
    {
        while (read_ring_buffer(q1, &buf, len) < 0);
        assert(buf.count == count);
        count--;
    }

    return;
}
/**
 * test the correctness of ring buffer implement
 *
 * two threads, one write, one read, 1000000 times.
 *
 * write thread write the count in the buf, and 
 * read thread make sure that it can read the expected count
 */
void correctness_test()
{
    int ret = 0;
    pthread_t t_a, t_b;
    ring_buffer_t queue1;
    size_t buf_len1 = 1<<20;
    size_t buf_len2 = 1<<20;
    int cpu_thread1 = 0;
    int cpu_thread2 = 1;

    memset(&queue1, 0, sizeof(ring_buffer_t));
    ret = init_ring_buffer(&queue1, buf_len1);
    if (ret < 0)
    {
        goto l_out;
    }

    pthread_create(&t_a, NULL, correctness_thread1, &queue1);
    pthread_create(&t_b, NULL, correctness_thread2, &queue1); //Create thread

    cpu_set_t cs;
    CPU_ZERO(&cs);
    CPU_SET(cpu_thread1, &cs);
    // Make sure your test machine has at least two cores.
    //assert(pthread_setaffinity_np(t_a, sizeof(cs), &cs) == 0);
    CPU_ZERO(&cs);
    CPU_SET(cpu_thread2, &cs);
    // assert(pthread_setaffinity_np(t_b, sizeof(cs), &cs) == 0);
    pthread_join(t_a, NULL); // wait t_a thread end
    pthread_join(t_b, NULL); // wait t_b thread end


l_out:
    destory_ring_buffer(&queue1);
    return;

}

int main(void) {
    //pingpong_test();
    correctness_test();
}


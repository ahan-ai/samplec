#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void *thread1(void*);
void *thread2(void*);

typedef struct shm_queue
{
    void *buf;
    size_t len; // max capacity is len -1
    int head;
    int tail;
} shm_queue_t;

static shm_queue_t queue;

int read_shm(shm_queue_t *q, void *buf, size_t len)
{
    int ret = 0;
    size_t queue_size = q->len;
    int count = 0;
    int tail = q->tail;
    int head = q->head;
    int t = 0;

    count = (tail + queue_size - head) % queue_size;
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
        __asm__ __volatile ("mfence" ::: "memory");
        q->head = (head + len) % queue_size;
        ret = len;
    }
    else
    {
        ret = -1;
        printf("queue is no enough to read\n");
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
    int data_len = 0;
    int t = 0;

    count = (tail + queue_size - head) % queue_size;
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
        __asm__ __volatile ("mfence" ::: "memory");
        q->tail = (tail + len) % queue_size;
        ret = len;
    }
    else
    {
        ret = -1;
        printf("queue not enough space to write\n");
        goto l_out;
    }

l_out:
    return ret;
}

int init_shm(void *buf, size_t len)
{
    int ret = 0;

    if (!buf || len < 0)
    {
        ret = -1;
        goto l_out;
    }
    queue.buf = buf;
    queue.len = len;

l_out:
    return ret;
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

int main(void) {
    int ret = 0;
    void *buf = NULL;
    size_t buf_len = 200;
    pthread_t t_a, t_b; //two thread

    buf = malloc(buf_len);
    if (!buf)
    {
        ret = -1;
        goto l_out;
    }
    
    ret = init_shm(buf, buf_len);
    if (ret < 0)
    {
        goto l_out;
    }

    pthread_create(&t_a, NULL, producer, &queue);
    pthread_create(&t_b, NULL, consumer, &queue); //Create thread

    pthread_join(t_a, NULL); // wait t_a thread end
    pthread_join(t_b, NULL); // wait t_b thread end

l_out:
    if (buf)
    {
        free(buf);
    }
    return ret;
}


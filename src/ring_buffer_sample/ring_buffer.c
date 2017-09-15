#include <string.h>
#include <stdlib.h>
#include "ring_buffer.h"

int read_ring_buffer(ring_buffer_t *q, void *buf, size_t len)
{
    int ret = 0;
    size_t rb_size = q->len;
    int count = 0;
    int tail = q->tail;
    int head = q->head;
    int t = 0;

    count = tail -head;
    if (count < 0)
    {
        count += rb_size;
    }
    if (count >= len)
    {
        if (head + len <= rb_size)
        {
            memcpy(buf, q->buf + head, len);
        }
        else
        {
            t = rb_size - head;
            memcpy(buf, q->buf + head, t);
            memcpy(buf + t, q->buf, len - t);
        }
        head += len;
        if (head >= rb_size)
        {
            head -= rb_size;
        }
        __asm__ __volatile ("mfence" ::: "memory");
        q->head = head;
        ret = len;
    }
    else
    {
        ret = -1;
        // printf("rb is no enough to read\n");
        goto l_out;
    }

l_out:
    return ret;
}

int write_ring_buffer(ring_buffer_t *q, void *buf, size_t len)
{
    int ret = 0;
    int rb_size = q->len;
    int head = q->head;
    int tail = q->tail;
    int count = 0;
    int t = 0;

    count = tail - head;
    if (count < 0)
    {
        count += rb_size;
    }
    if (count + len < rb_size)
    {
        if (tail + len <= rb_size)
        {
            memcpy(q->buf + tail, buf, len);
        }
        else
        {
            t = rb_size - tail;
            memcpy(q->buf + tail, buf, t);
            memcpy(q->buf, buf + t, len - t);
        }
        tail += len;
        if (tail >= rb_size)
        {
            tail -= rb_size;
        }
        __asm__ __volatile ("mfence" ::: "memory");
        q->tail = tail;
        ret = len;
    }
    else
    {
        ret = -1;
        // printf("rb not enough space to write\n");
        goto l_out;
    }

l_out:
    return ret;
}

/**
 * init a ring buffer with specific length
 */
int init_ring_buffer(ring_buffer_t *rb, size_t len)
{
    int ret = 0;
    void *buf = NULL;

    buf = malloc(len);
    if (NULL == buf)
    {
        ret = -1;
        goto l_out;
    }

    rb->head = 0;
    rb->tail = 0;
    rb->buf = buf;
    rb->len = len;

l_out:
    return ret;
}

void destory_ring_buffer(ring_buffer_t *rb)
{
    if (rb != NULL && rb->buf != NULL)
    {
        free(rb->buf);
    }
}


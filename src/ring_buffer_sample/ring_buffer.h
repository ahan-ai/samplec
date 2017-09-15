#ifndef __RING_BUFFER_H_
#define __RING_BUFFER_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct ring_buffer
{
    void *buf;
    volatile size_t len; // max capacity is len -1
    volatile int head;
    volatile int tail;
} ring_buffer_t;

int read_ring_buffer(ring_buffer_t *q, void *buf, size_t len);

int write_ring_buffer(ring_buffer_t *q, void *buf, size_t len);

int init_ring_buffer(ring_buffer_t *rb, size_t len);

void destory_ring_buffer(ring_buffer_t *rb);


#ifdef __cplusplus
}
#endif

#endif

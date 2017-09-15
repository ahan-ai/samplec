all: ring_buffer_sample

.PHONY: all clean

ring_buffer_sample: ring_buffer_sample.c ring_buffer.c ring_buffer.h
	gcc -O3 -g -o ring_buffer_sample -lpthread -lrt ring_buffer_sample.c ring_buffer.c ring_buffer.h

clean:
	rm -rf ring_buffer_sample

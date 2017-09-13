all: shm_sample

.PHONY: all clean

shm_sample: shm_sample.c
	gcc -g -o shm_sample -lpthread shm_sample.c

clean:
	rm -rf shm_sample

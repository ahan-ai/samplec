all: volatile_sample

.PHONY: all clean

volatile_sample: volatile_sample.c
	gcc -o volatile_sample -lpthread volatile_sample.c

clean:
	rm -rf volatile_sample

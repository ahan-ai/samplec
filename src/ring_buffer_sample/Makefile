TARGET = ring_buffer_sample
CC = gcc
CFLAGS = -O2 -Wall -Werror -g

all: $(TARGET)

.PHONY: all clean

SRCDIR = .
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst %.c, %.o, $(SRCS))
$(warning $(OBJS))
INC = -I.
LIBS = -lpthread -lrt

$(OBJS): %.o:%.c
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INC) -o $@ $(LIBS) $(OBJS)

clean:
	rm -rf *.o
	rm -rf ring_buffer_sample

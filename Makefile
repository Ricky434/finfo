CC=gcc
CFLAGS=-Wall -g
LFLAGS=

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

TARGET = finfo

.PHONY:  all clean

all: $(TARGET)
	rm $(OBJS)

$(TARGET):  $(OBJS)
	$(CC) $(LFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(OBJS) $(TARGET)

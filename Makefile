CC=gcc
CFLAGS=-Wall
LFLAGS=

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

TARGET = finfo

.PHONY:  all clean

all: $(TARGET)

$(TARGET):  $(OBJS)
	$(CC) $(LFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(OBJS) $(TARGET)

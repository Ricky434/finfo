CC=gcc
CFLAGS=-Wall -g
LFLAGS=-lm

SRCS = $(wildcard *.c)
LIBS = $(wildcard libs/*.c)
OBJS = $(SRCS:.c=.o) $(LIBS:.c=.o)

TARGET = finfo

.PHONY:  all clean debug

all: $(TARGET)
	rm $(OBJS)

$(TARGET):  $(OBJS)
	echo $(SRCS)
	$(CC) $(LFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(OBJS) $(TARGET)

debug: CFLAGS += -DDEBUG
debug: all

TARGET = a.out
SRCS := $(wildcard *.c)
OBJS := $(subst .c,.o,$(SRCS))

CC = gcc
CFLAGS = -Wall -std=c99 -I/usr/local/include
LDFLAGS = -L/usr/local/lib -lopencv_core -lopencv_highgui

.SUFFIXES: .c .o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

.c.o: $<
	$(CC) -c $(CFLAGS) $<

clean:
	rm $(TARGET) $(OBJS)

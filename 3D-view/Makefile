TARGET = a.out
SRCS := $(wildcard *.c)
OBJS := $(subst .c,.o,$(SRCS))

CC = gcc
CFLAGS = -Wall -std=c99
LDFLAGS = -framework OpenGL -framework GLUT -framework Foundation

.SUFFIXES: .c .o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

.c.o: $<
	$(CC) -c $(CFLAGS) $<

clean:
	rm $(TARGET) $(OBJS)

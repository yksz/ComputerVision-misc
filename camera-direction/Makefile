TARGET = a.out
SRCS := $(wildcard *.cpp)
OBJS := $(subst .cpp,.o,$(SRCS))

CC = g++
CFLAGS = -Wall -I/usr/local/include
LDFLAGS = -L/usr/local/lib -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_calib3d

.SUFFIXES: .cpp .o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

.cpp.o: $<
	$(CC) -c $(CFLAGS) $<

clean:
	rm $(TARGET) $(OBJS)

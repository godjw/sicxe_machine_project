CC = gcc
OBJS = 20161643.o
TARGET = 20161643.out

.SUFFIXES : .c .o

all : $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -Wall -o  $@ $(OBJS)

clean :
	rm -f $(OBJS) $(TARGET) *lst intermediate

20161643.out : 20161643.c 20161643.h

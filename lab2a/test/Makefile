# Makefile za testne programe lab2a

CC = gcc
CFLAGS = -Wall -I..
TARGETS = reader writer

all: $(TARGETS)

reader: reader.c shofer_user.h
	$(CC) $(CFLAGS) -o reader reader.c

writer: writer.c shofer_user.h
	$(CC) $(CFLAGS) -o writer writer.c

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean

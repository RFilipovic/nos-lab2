CC = gcc
CFLAGS = -Wall -O2

TARGETS = reader writer

all: $(TARGETS)

reader: reader.c
	$(CC) $(CFLAGS) -o reader reader.c

writer: writer.c
	$(CC) $(CFLAGS) -o writer writer.c

clean:
	rm -f $(TARGETS)

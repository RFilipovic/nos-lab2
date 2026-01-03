CC = gcc
CFLAGS = -Wall -O2

TARGETS = citac pisac

all: $(TARGETS)

citac: citac.c
	$(CC) $(CFLAGS) -o citac citac.c

pisac: pisac.c
	$(CC) $(CFLAGS) -o pisac pisac.c

clean:
	rm -f $(TARGETS)

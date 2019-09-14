CFLAGS := -Wall -g 

CC := gcc
CFILES = mysort.c
OBJS= ${CFILES:.c=.o}

all: mysort

mysort: mysort.o
	$(CC) $(CFLAGS)  $(OBJS) -o $@

mysort.o: mysort.c
	$(CC) $(CFLAGS)  mysort.c -c 

clean:
	-rm -f *.o
	-rm -f mysort
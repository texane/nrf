CC := $(CROSS_COMPILE)gcc
CFLAGS := -Wall -O2 -I../common -I.

SRCS := snrf.c serial.c
OBJS := $(SRCS:.c=.o)

all: libsnrf.a

libsnrf.a: $(OBJS)
	ar rc $@ $(OBJS)
	ranlib $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm $(OBJS)

fclean:	clean
	-rm libsnrf.a

main: libsnrf.a main.o
	$(CC) -Wall -O2 -o main main.o -L. -lsnrf

.PHONY: all clean fclean

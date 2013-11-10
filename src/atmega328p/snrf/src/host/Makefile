CC := gcc
CFLAGS := -Wall -O2 -I../common

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

.PHONY: all clean fclean

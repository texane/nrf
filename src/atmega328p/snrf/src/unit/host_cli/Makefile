CC := gcc
CFLAGS := -Wall -O2 -I../../common -I../../host -I.

SRCS := main.c
OBJS := $(SRCS:.c=.o)

all: a.out

../../host/libsnrf.a:
	cd ../../host && make

a.out:	../../host/libsnrf.a $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -L../../host -lsnrf

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm $(OBJS)

fclean:	clean
	-rm a.out

.PHONY: all clean fclean ../../host/libsnrf.a


SRCS= main.c
OBJS= main.o
PROG= pppw

all: $(PROG)

$(PROG): $(OBJS)
	cc -o $(PROG) $(OBJS)

$(OBJS): $(SRCS)

.c.o:
	cc -c $*.c

install:
	install -c -s -o root -g wheel -m 4750 $(PROG) /usr/local/sbin

clean:
	rm -f $(PROG) $(OBJS)


SRCS= do_connection.c main.c
OBJS= do_connection.o main.o
LIBS= ../common/libmodem.a
INCLUDES= -I../common
PROG= dialup

all: $(PROG)

$(PROG): $(OBJS)
	cc -o $(PROG) $(OBJS) $(LIBS)

$(OBJS): $(SRCS)

.c.o:
	cc -c $(INCLUDES) $*.c

install:
	install -c -s -o root -g wheel -m 4750 $(PROG) /usr/local/sbin

clean:
	rm -f $(PROG) $(OBJS)

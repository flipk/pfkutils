
SRCS= icmp_ping.c line_status.c direct.c durham.c main.c
OBJS= icmp_ping.o line_status.o direct.o durham.o main.o
PROG= autoppp
INCLUDES= -I../common
LIBS= ../common/libmodem.a

all: $(PROG)

$(PROG): $(OBJS)
	cc -o $(PROG) $(OBJS) $(LIBS)

$(OBJS): $(SRCS)

.c.o:
	cc -c $(INCLUDES) $*.c

install:
	install -c -s -o root -g wheel -m 4750 $(PROG) /usr/local/sbin

clean:
	rm -f $(PROG) $(OBJS) *~ *.core core

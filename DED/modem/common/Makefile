
SRCS= detach.c serial_port.c sig_handler.c
OBJS= detach.o serial_port.o sig_handler.o
LIB= libmodem.a

all: $(LIB)

$(LIB): $(OBJS)
	ar cq $(LIB) $(OBJS)

$(OBJS): $(SRCS)

.c.o:
	cc -c $*.c

install:
	@echo Nothing to do!

clean:
	rm -f $(OBJS) $(LIB)

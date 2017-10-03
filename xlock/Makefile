
OBJS= xlock.o hsbramp.o usleep.o resource.o \
      swarm.o rotor.o pyro.o flame.o worm.o
LIBS= -L/usr/X11R6/lib -lXext -lX11 -lm

CFLAGS= -O6
CPPDEFS= -I/usr/X11R6/include

all: libxlock.a

libxlock.a: $(OBJS)
	ar cq libxlock.a $(OBJS)

.c.o:
	gcc $(CFLAGS) $(CPPDEFS) $(INCLUDES) -c $*.c

clean:
	rm -f *.o libxlock.a

depend:
# nothing


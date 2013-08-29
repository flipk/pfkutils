
PROG= t

all: $(PROG)

$(PROG): redmalloc.o test.o
	gcc -g3 redmalloc.o test.o -o $(PROG)

.c.o:
	gcc -Wall -Werror -g3 -c $*.c

clean:
	rm -f *~ *.o $(PROG)

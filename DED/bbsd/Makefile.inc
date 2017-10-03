
#
# define the following variables before including
# this file:
#
#    CFILES is a list of all C files.
#    HFILES is a list of all H files.
#    PROG is the program to produce.
#

SRCS= $(CFILES) $(HFILES)
OBJS= $(CFILES:.C=.o)
CC= cc -O -g3 -c

all: xmakefile
	@make -f xmakefile _all

xmakefile: Makefile $(SRCS)
	rm -f xmakefile
	cp Makefile xmakefile
	chmod u+w xmakefile
	gcc -M $(CFILES) >> xmakefile

_all: $(PROG)

$(PROG): $(OBJS)
	cc -o $(PROG) $(OBJS)
	rm -f $(PROG).core

.C.o:
	$(CC) $*.C

clean:
	rm -f $(PROG) $(OBJS) xmakefile


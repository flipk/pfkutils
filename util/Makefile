
TARG= libutil.a
CSRCS= base64.c pk-md5.c regex.c checksize.c crnl.c fmtsdiff.c genpw.c \
	he.c inet_aton.c m.c random-hex.c waitpid.c xtermbar.c
CHDRS= base64.h pk-md5.h regex.h m.h
CPPSRCS= fd_mgr.C rmstar.C
CPPHDRS= ../h/circular_buffer.H fd_mgr.H lognew.H 

# magic.H

OBJS= $(CPPSRCS:.C=.o) $(CSRCS:.c=.o)
CC=  gcc
CPP= g++
INCS=  -I../dll2 -I../h
DEFS= -D_FILE_OFFSET_BITS=64
LIBS= 
OPTS= -O3 -Wall -Werror

all: xmakefile
	@make -f xmakefile $(TARG)

xmakefile: Makefile $(CSRCS) $(CPPSRCS) $(CHDRS) $(CPPHDRS)
	rm -f xmakefile tmp
	cat Makefile > tmp
	echo '' >> tmp
	$(CC) $(INCS) $(DEFS) -M $(CSRCS) $(CPPSRCS) >> tmp
	mv tmp xmakefile

$(TARG): $(OBJS)
	rm -f $(TARG)
	ar cq $(TARG) $(OBJS)

.C.o:
#	$(CC) $(INCS) $(DEFS) $(OPTS) -E $*.C > $*.i
	$(CC) $(INCS) $(DEFS) $(OPTS) -c $*.C

genpw.o: genpw.c
	$(CC) -c $*.c

.c.o:
	$(CC) $(INCS) $(DEFS) $(OPTS) -c $*.c

depend: xmakefile

clean:
	rm -f xmakefile tmp *~ *.o *.a *.obj *.i

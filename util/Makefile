
TARG= libutil.a
CSRCS= base64.c pk-md5.c regex.c checksize.c crnl.c fmtsdiff.c genpw.c \
	he.c inet_aton.c m.c random-hex.c waitpid.c xtermbar.c
CHDRS= base64.h pk-md5.h regex.h m.h
CPPSRCS= fd_mgr.C pk_msg.C rmstar.C
CPPHDRS= circular_buffer.H fd_mgr.H lognew.H pk_msg.H pk_msg_int.H magic.H

OBJS= $(CPPSRCS:.C=.o) $(CSRCS:.c=.o)
CC=  gcc
CPP= g++
INCS=  -I../dll2
DEFS= 
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

depend: xmakefile

clean:
	rm -f xmakefile tmp *~ *.o *.a *.obj *.i

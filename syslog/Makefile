
TARG= libsyslog.a
SRCS= config_file.c process_line.c strip_chars.c \
	syslog-postprocess.c syslogd.c
HDRS= rules.h

OBJS= $(SRCS:.c=.o)
CC=  gcc
INCS= -I../util
DEFS= 
OPTS= -O3 -Wall -Werror

all: xmakefile
	@make -f xmakefile $(TARG)

xmakefile: Makefile $(SRCS) $(HDRS)
	rm -f xmakefile tmp
	cat Makefile > tmp
	echo '' >> tmp
	$(CC) $(INCS) $(DEFS) -M $(SRCS) >> tmp
	mv tmp xmakefile

$(TARG): $(OBJS)
	rm -f $(TARG)
	ar cq $(TARG) $(OBJS)

.C.o:
#	$(CPP) $(INCS) $(DEFS) $(OPTS) -E $*.C > $*.i
	$(CPP) $(INCS) $(DEFS) $(OPTS) -c $*.C

.c.o:
#	$(CC) $(INCS) $(DEFS) $(OPTS) -E $*.c > $*.i
	$(CC) $(INCS) $(DEFS) $(OPTS) -c $*.c

depend: xmakefile

clean:
	rm -f xmakefile tmp *~ *.o *.a *.obj *.i

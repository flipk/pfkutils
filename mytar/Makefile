
TARG= libmytar.a
SRCS= file_db.C update_file.C
HDRS= file_db.H
COPTS= -g3 -Wall -Werror
INCS= -I .. -I ../dll2 -I ../bt
DEFS= 

_OBJS= $(SRCS:.C=.o)
OBJS= $(_OBJS:.c=.o)

all: xmakefile
	@make -f xmakefile $(TARG) t

xmakefile: Makefile $(SRCS) $(HDRS)
	rm -f xmakefile
	cat Makefile > xmakefile.tmp
	gcc -M $(INCS) $(DEFS) $(SRCS) >> xmakefile.tmp
	mv xmakefile.tmp xmakefile

$(TARG): $(OBJS)
	rm -f $(TARG)
	ar cq $(TARG) $(OBJS)

LIBS= -L ../bt -lbt ../pk-md5.o ../dll2/libdll2.a -lz

t: test.C $(TARG)
	g++ $(INCS) -g3 test.C $(TARG) $(LIBS) -o t

.C.o:
	g++ -c $(COPTS) $(INCS) $*.C

clean:
	rm -f *.[ao] xmakefile t *~

#DEPENDENCIES


TARG= libmytar.a
SRCS= file_db.C update_file.C extract_file.C update_dir.C main.C
HDRS= file_db.H update_file.H extract_file.H update_dir.H
COPTS= -O3 -Wall -Werror
INCS= -I .. -I ../dll2 -I ../bt
DEFS= 

_OBJS= $(SRCS:.C=.o)
OBJS= $(_OBJS:.c=.o)

all: xmakefile
	@make -f xmakefile $(TARG)

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
	g++ $(INCS) -O3 test.C $(TARG) $(LIBS) -o t

mt: main.C $(TARG)
	g++ $(INCS) -O3 main.C $(TARG) $(LIBS) -o mt

.C.o:
	g++ -c $(COPTS) $(INCS) $*.C

clean:
	rm -f *.[ao] xmakefile mt t *~

depend:
	@echo nothing to do

#DEPENDENCIES

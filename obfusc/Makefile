
PROG= obfuc
HDRS= myputs.h tokens.h translate.h
SRCS= header.C main.C translate.C 
c_SRCS= myputs.c tokens.c
LIBS= -ll
INCS= -I .. -I ../dll2 -I ../bt
DEFS= -DDLL2_INCLUDE_LOGNEW=0 -DDLL2_INCLUDE_BTREE=0
FLAGS= -g3 $(INCS) $(DEFS)
OBJS= $(SRCS:.C=.o) $(c_SRCS:.c=.o) btree.o recnoblock.o


all: xmakefile
	@ make -f xmakefile $(PROG)

xmakefile: Makefile $(SRCS) $(HDRS)
	ln -s ../bt/btree.C
	ln -s ../bt/recnoblock.C
	cat Makefile > tmpfile
	gcc -M $(INCS) $(DEFS) $(SRCS) >> tmpfile
	mv tmpfile xmakefile

$(PROG): $(OBJS)
	g++ -o $(PROG) $(FLAGS) $(OBJS) $(LIBS)

tokens.c: tokens.l
	flex tokens.l
	mv lex.yy.c tokens.c

.c.o:
	gcc -c $(FLAGS) $*.c

.C.o:
	g++ -c $(FLAGS) $*.C

clean:
	rm -f $(PROG) $(OBJS) tokens.c btree.C recnoblock.C
	rm -f  depend.make *core obfuscated_symbols.db
	rm -f xmakefile

# DEPENDENCIES below this line

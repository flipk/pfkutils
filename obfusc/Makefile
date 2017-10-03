
PROG= t
SRCS= main.C translate.C header.C tokens.c myputs.c btree.C recnoblock.C
OBJS= main.o translate.o header.o tokens.o myputs.o btree.o recnoblock.o
LIBS= -ll
FLAGS= -g3

all: $(PROG)

$(PROG): $(OBJS)
	g++ -o $(PROG) $(FLAGS) $(OBJS) $(LIBS)

tokens.c: tokens.l
	flex tokens.l
	mv lex.yy.c tokens.c

.c.o:
	gcc -c $(FLAGS) $*.c

.C.o:
	g++ -c $(FLAGS) $*.C

depend: tokens.c
	gcc -M $(SRCS) > depend.make

clean:
	rm -f $(PROG) $(OBJS) tokens.c
	rm -f  depend.make *core obfuscated_symbols.db
	touch depend.make

include depend.make

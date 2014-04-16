
PROG= t
OBJS= lex.yy.o y.tab.o tuple.o main.o
LIBS=

YACC= yacc
LEX= flex
CC= gcc

OPTDEBUG= -g3

all: $(PROG)

$(PROG): $(OBJS)
	gcc $(OPTDEBUG) -o $(PROG) $(OBJS) $(LIBS)

.c.o:
	$(CC) $(OPTDEBUG) -Wall -c $*.c

y.tab.c y.tab.h: parser.y
	$(YACC) -d parser.y

lex.yy.c: tokenizer.l y.tab.h
	$(LEX) tokenizer.l

clean:
	rm -f *.o y.tab.* lex.yy.* $(PROG)

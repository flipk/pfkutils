
PROG= t
OBJS= condition.o lex.yy.o y.tab.o main.o
LIBS= 
LEX= flex
YACC= yacc
CC= gcc -g3

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) -o $(PROG) $(OBJS) $(LIBS)

y.tab.c y.tab.h : parser.y
	$(YACC) -d parser.y

lex.yy.c : y.tab.h tokenizer.l
	$(LEX) tokenizer.l

.c.o:
	$(CC) -c $*.c

clean:
	rm -f $(OBJS) $(PROG) y.tab.* lex.yy.* *~

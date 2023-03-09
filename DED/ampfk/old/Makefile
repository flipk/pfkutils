
PROG= t
OBJS= condition.o lex.yy.o y.tab.o automake.o utils.o main.o
LIBS= 
LEX= flex
YACC= yacc
CC= gcc -g3
CFLAGS= -Wall -Werror

all: $(PROG)

-include .condition.Po
-include .lex.yy.Po
-include .y.tab.Po
-include .automake.Po
-include .utils.Po
-include .main.Po

$(PROG): $(OBJS)
	$(CC) -o $(PROG) $(OBJS) $(LIBS)

y.tab.c y.tab.h : parser.y
	$(YACC) -d parser.y

lex.yy.c : y.tab.h tokenizer.l
	$(LEX) tokenizer.l

.c.o:
	@$(CC) $(CFLAGS) -MT $@ -MD -MP -MF $*.Tpo -c $< -o $@
	@echo $(CC) -c $< -o $@
	@mv $*.Tpo .$*.Po

clean:
	rm -f $(OBJS) $(PROG) y.tab.* lex.yy.* *~
	@rm -f .*.Po

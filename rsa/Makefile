
SRCS= keyfile.c timing.c decrypt.c encrypt.c genrsa.c
HDRS= keyfile.h timing.h

GEN= genrsa.o  keyfile.o timing.o
ENC= encrypt.o keyfile.o timing.o
DEC= decrypt.o keyfile.o timing.o

 FLAGS= -g3
#FLAGS= -O3 -ftest-coverage -fprofile-arcs
#FLAGS= -O3 -fbranch-probabilities

LIB= gmp-4.1.3/.libs/libgmp.a
INC= -Igmp-4.1.3

all: xmakefile
	@make -f xmakefile _all

_all: g e d

g: $(GEN)
	gcc $(FLAGS) $(GEN) $(LIB) -o g
#	strip g

e: $(ENC)
	gcc $(FLAGS) $(ENC) $(LIB) -o e
#	strip e

d: $(DEC)
	gcc $(FLAGS) $(DEC) $(LIB) -o d
#	strip d

.c.o:
	gcc $(FLAGS) $(INC) -c $*.c

test:
	./g 1024
	./e < 0 > 1
	./d < 1 > 2
	ls -l 0 1 2
	cmp 0 2

clean:
	rm -f d e g 1 2 *.o rsa.*
	rm -f xmakefile
	rm -f *.bb *.bbg *.da

xmakefile: $(SRCS) $(HDRS) Makefile
	cat Makefile > xmakefile.tmp
	gcc $(INC) -M $(SRCS) >> xmakefile.tmp
	mv xmakefile.tmp xmakefile

#


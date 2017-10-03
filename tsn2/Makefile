
CPP_SRCS= btree.C main.C pk_tcp_msg.C recnoblock.C scan.C trade.C
C_SRCS=	do_connect.c pk-md5.c

SRCS=	$(CPP_SRCS) $(C_SRCS)
OBJS=	$(C_SRCS:.c=.o) $(CPP_SRCS:.C=.o)
INCS=	-I .. -I ../bt -I../dll2
DEFS=	-DDLL2_INCLUDE_LOGNEW=0 -DDLL2_INCLUDE_BTREE=0 \
	-DFBN_RECORD_SIZE=16 -DFBN_PAGE_SIZE=4096
OPTS=	-g3
TARG=	s

all: xmakefile
	@[ ! -d logs ] && mkdir logs || true
	@make -f xmakefile $(TARG)

xmakefile: Makefile $(SRCS)
	cat Makefile > tempfile
	gcc -M $(SRCS) $(DEFS) $(INCS) >> tempfile
	mv tempfile xmakefile

$(TARG): $(OBJS)
	g++ $(OBJS) -o $(TARG)

.C.o:
	@gcc -E $(INCS) $(DEFS) $*.C > logs/$*.C
	@gcc -E $(INCS) $(DEFS) $*.C > logs/$*.C2
	g++ $(INCS) $(DEFS) -c $*.C

.c.o:
	gcc $(INCS) $(DEFS) -c $*.c

clean:
	rm -f $(OBJS) $(TARG)
	rm -rf logs

# dependencies added below this line.


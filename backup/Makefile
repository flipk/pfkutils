
TARG= pfkbak
SRCS= main.C ops.C
HDRS= params.H database_elements.H

OBJS= $(SRCS:.C=.o)
CC= g++
INCS= -I ../h -I ../util -I ../dll2 -I ../FileBlock 
DEFS= -D_FILE_OFFSET_BITS=64
OPTS= -g3

LIBS= ../FileBlock/libFileBlock.a ../dll2/libdll2.a

all: xmakefile
	@make -f xmakefile $(TARG)

xmakefile: Makefile $(SRCS) $(HDRS)
	rm -f xmakefile tmp
	cat Makefile > tmp
	echo '' >> tmp
	$(CC) $(INCS) $(DEFS) -M $(SRCS) >> tmp
	mv tmp xmakefile

$(TARG): $(OBJS) $(LIBS)
	$(CC) $(OBJS) $(LIBS) -o $(TARG)

.C.o:
	$(CC) $(INCS) $(DEFS) $(OPTS) -c $*.C

doc::
	doxygen

depend: xmakefile

clean:
	rm -f $(TARG) test*.db xmakefile tmp *~ *.o *.a *.obj *.i
	rm -rf html

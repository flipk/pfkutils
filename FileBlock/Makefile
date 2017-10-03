
TARG=	libFileBlock.a
SRCS=	PageIO.C PageCache.C BlockCache.C ExtentMap.C \
	FileBlockLocalMap.C FileBlockLocal.C
HDRS=	PageIO.H PageCache.H PageCache_internal.H \
	BlockCache.H BlockCache_internal.H \
	FileBlock_iface.H \
	FileBlockLocal.H FileBlockLocal_internal.H \
	ExtentMap.H

OBJS=	$(SRCS:.C=.o)
CC=	g++
INCS=	-I../h -I../dll2
DEFS=	-D_FILE_OFFSET_BITS=64 \
	-DDLL2_INCLUDE_BTREE=1 -DDLL2_CHECKSUMS=0 -DDLL2_STRICT_WALKING=0
OPTS=	-Wall -Werror -O6
LIBS=	../dll2/dll2_hash.o
TESTOBJS= testFileBlock.o

all: xmakefile
	@make -f xmakefile $(TARG) t1


xmakefile: Makefile $(SRCS) $(HDRS)
	rm -f xmakefile tmp
	cat Makefile > tmp
	echo '' >> tmp
	$(CC) $(INCS) $(DEFS) -M $(SRCS) >> tmp
	mv tmp xmakefile

$(TARG): $(OBJS)
	rm -f $(TARG)
	ar cq $(TARG) $(OBJS)

t1: $(TARG) $(TESTOBJS)
	$(CC) $(TESTOBJS) $(TARG) $(LIBS) -o t1

doc::
	doxygen

.C.o:
	$(CC) $(INCS) $(DEFS) $(OPTS) -c $*.C

depend: xmakefile

clean:
	rm -f $(TARG) testfile.db xmakefile tmp *~ *.o *.a *.obj *.i t1
	rm -rf doc

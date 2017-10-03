
TARG=	libFileBlock.a
SRCS=	PageIO.C PageCache.C BlockCache.C \
	FileBlock_iface.C Btree.C BtreeNode.C \
	FileBlockLocal.C FileBlockLocalAllocFree.C FileBlockLocalAuid.C \
	FileBlockLocalAun.C FileBlockLocalBucket.C FileBlockLocalCompact.C \
	FileBlockLocalDIB.C FileBlockLocalGetRel.C FileBlockLocalValidate.C
HDRS=	PageIO.H PageCache.H PageCache_internal.H \
	BlockCache.H BlockCache_internal.H \
	FileBlock_iface.H \
	FileBlockLocal.H \
	Btree.H

OBJS=	$(SRCS:.C=.o)
CC=	g++
INCS=	-I../h -I../dll2
DEFS=	-D_FILE_OFFSET_BITS=64 \
	-DDLL2_INCLUDE_BTREE=1 -DDLL2_CHECKSUMS=0 -DDLL2_STRICT_WALKING=0
OPTS=	-Wall -Werror -g3
LIBS=	../dll2/dll2_hash.o
TESTSRCS= testFileBlock.C testBtree.C

all: xmakefile
	@make -f xmakefile $(TARG) t1 t2


xmakefile: Makefile $(TESTSRCS) $(SRCS) $(HDRS)
	rm -f xmakefile tmp
	cat Makefile > tmp
	echo '' >> tmp
	$(CC) $(INCS) $(DEFS) -M $(TESTSRCS) $(SRCS) >> tmp
	mv tmp xmakefile

$(TARG): $(OBJS)
	rm -f $(TARG)
	ar cq $(TARG) $(OBJS)
	ranlib $(TARG)

t1: $(TARG)   testFileBlock.o
	$(CC) testFileBlock.o $(TARG) $(LIBS) -o $@

t2: $(TARG)   testBtree.o
	$(CC) testBtree.o     $(TARG) $(LIBS) -o $@

doc::
	doxygen

.C.o:
	$(CC) $(INCS) $(DEFS) $(OPTS) -c $*.C

depend: xmakefile

clean:
	rm -f $(TARG) testfile.db xmakefile tmp *~ *.o *.a *.obj *.i t1 t2
	rm -rf doc

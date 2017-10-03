
TARG=	t
SRCS=	PageIO.C PageCache.C BlockCache.C ExtentMap.C \
	FileBlockLocalMap.C FileBlockLocal.C \
	main.C
HDRS=	PageIO.H PageCache.H PageCache_internal.H \
	BlockCache.H BlockCache_internal.H \
	FileBlock_iface.H \
	FileBlockLocal.H FileBlockLocal_internal.H \
	ExtentMap.H


OBJS=	$(SRCS:.C=.o)
CC=	g++
INCS=	-I../h -I../dll2
DEFS=	-D_FILE_OFFSET_BITS=64
# -DDLL2_CHECKSUMS=0 -DDLL2_STRICT_WALKING=0
OPTS=	-Wall -Werror -g3
LIBS=	../dll2/dll2_hash.o

all: xmakefile
	@make -f xmakefile $(TARG)

xmakefile: Makefile $(SRCS) $(HDRS)
	rm -f xmakefile tmp
	cat Makefile > tmp
	echo '' >> tmp
	$(CC) $(INCS) $(DEFS) -M $(SRCS) >> tmp
	mv tmp xmakefile

$(TARG): $(OBJS)
	rm -f $(TARG)
	$(CC) $(OBJS) $(LIBS) -o $(TARG)

doc::
	doxygen DOXYCONFIG

.C.o:
	$(CC) $(INCS) $(DEFS) $(OPTS) -c $*.C

depend: xmakefile

clean:
	rm -f $(TARG) testfile.db xmakefile tmp *~ *.o *.a *.obj *.i
	rm -rf doc

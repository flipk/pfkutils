
TARG= treesync

TSSRCS= FileList.C analyze.C copy_file.C create_dirs.C delete_file.C \
	main.C md5.C open_ts_db.C update_db.C
TSHDRS= FileList.H db.H macros.H protos.H
FBSRCS= BlockCache.C Btree.C BtreeNode.C FileBlockLocal.C \
	FileBlockLocalAllocFree.C FileBlockLocalAuid.C FileBlockLocalAun.C \
	FileBlockLocalBucket.C FileBlockLocalCompact.C FileBlockLocalDIB.C \
	FileBlockLocalGetRel.C FileBlockLocalValidate.C FileBlock_iface.C \
	PageCache.C PageIO.C PageIOFileDescriptor.C PageIONetworkTCPServer.C
FBHDRS= BlockCache.H BlockCache_internal.H Btree.H BtreeDbClasses.H \
	Btree_internal.H FileBlockLocal.H FileBlock_iface.H PageCache.H \
	PageCache_internal.H PageIO.H
MISCSRCS= pk-md5.c
MISCHDRS= pk-md5.h
COMHDRS= bst.H types.H
DLL2SRCS= dll2_hash.C
DLL2HDRS= dll2.H
ROOTHDR= version.h

SRCS= $(TSSRCS) $(FBSRCS) $(MISCSRCS)            $(DLL2SRCS)
HDRS= $(TSHDRS) $(FBHDRS) $(MISCSRCS) $(COMHDRS) $(DLL2HDRS) $(ROOTHDR)

_OBJS= $(SRCS:.C=.o)
OBJS= $(_OBJS:.c=.o)
INCS= -I.
DEFS= -D_FILE_OFFSET_BITS=64 -Dtreesync_main=main
OPTS= -O3
LIBS=
CC= g++

all: xmakefile
	@make -f xmakefile $(TARG)

copy:
	@./copy-if-changed ../../FileBlock $(FBSRCS) $(FBHDRS)
	@./copy-if-changed ../../util      $(MISCSRCS) $(MISCHDRS)
	@./copy-if-changed ../../dll2      $(DLL2SRCS) $(DLL2HDRS)
	@./copy-if-changed ../../h         $(COMHDRS)
	@./copy-if-changed ../..           $(ROOTHDR)
	@./copy-if-changed ..              $(TSSRCS) $(TSHDRS)

xmakefile: Makefile $(CPPSRCS) $(CSRCS) $(HDRS)
	rm -f xmakefile tmp
	cat Makefile > tmp
	echo '' >> tmp
	$(CC) $(INCS) $(DEFS) -M $(SRCS) >> tmp
	mv tmp xmakefile

$(TARG): $(OBJS)
	rm -f $(TARG)
	$(CC) $(OBJS) $(LIBS) -o $(TARG)

.C.o:
	$(CC) $(INCS) $(DEFS) $(OPTS) -c $*.C

.c.o:
	$(CC) $(INCS) $(DEFS) $(OPTS) -c $*.c

depend: xmakefile

clean:
	rm -f *.C *.H *.c *.h $(OBJS) $(TARG) xmakefile tmp


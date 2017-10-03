
TARG= pfkbak
EXTRAS= fbcompact

FBSRCS=  BlockCache.C Btree.C BtreeNode.C FileBlockLocal.C \
        FileBlockLocalAllocFree.C FileBlockLocalAuid.C FileBlockLocalAun.C \
        FileBlockLocalBucket.C FileBlockLocalCompact.C FileBlockLocalDIB.C \
        FileBlockLocalGetRel.C FileBlockLocalValidate.C FileBlock_iface.C \
	PageCache.C PageIO.C PageIOFileDescriptor.C PageIONetworkTCPServer.C
FBCOMPSRCS= fbcompact.C
FBHDRS= BlockCache.H BlockCache_internal.H Btree.H BtreeDbClasses.H \
	Btree_internal.H FileBlockLocal.H FileBlock_iface.H \
	PageCache.H PageCache_internal.H PageIO.H 
MISCSRCS= pk-md5.c
MISCHDRS= pk-md5.h
COMHDRS= bst.H types.H 
TSSRCS= FileList.C 
TSHDRS= FileList.H macros.H
BAKSRCS= create_backup.C create_file.C \
	delete_backup.C delete_gen.C delete_gens.C \
	extract.C extract_list.C find_backup.C \
	get_info.C list_backups.C list_files.C main.C md5buffer.C \
	update_backup.C
BAKHDRS= database_elements.H params.H protos.H
DLL2SRCS= dll2_hash.C
DLL2HDRS= dll2.H
ROOTHDR= version.h

SRCS= $(FBSRCS) $(MISCSRCS) $(TSSRCS)            $(BAKSRCS) $(DLL2SRCS)
HDRS= $(FBHDRS) $(MISCHDRS) $(TSHDRS) $(COMHDRS) $(BAKHDRS) $(DLL2HDRS) $(ROOTHDR)

_OBJS= $(SRCS:.C=.o)
OBJS= $(_OBJS:.c=.o)
FBOBJS= $(FBSRCS:.C=.o)
FBCOMPOBJS= $(FBCOMPSRCS:.C=.o)
INCS= -I.
DEFS= -D_FILE_OFFSET_BITS=64 -Dpfkbak_main=main -Dfbcompact_main=main
OPTS= -O3
LIBS= -lz
CC= g++

FBCOMPACT_OBJS= $(FBOBJS) $(FBCOMPOBJS) dll2_hash.o

all: xmakefile
	@make -f xmakefile $(TARG)  $(EXTRAS)

copy:
	@./copy-if-changed ../../FileBlock $(FBSRCS) $(FBCOMPSRCS) $(FBHDRS)
	@./copy-if-changed ../../treesync  $(TSSRCS) $(TSHDRS)
	@./copy-if-changed ../../util      $(MISCSRCS) $(MISCHDRS)
	@./copy-if-changed ../../dll2      $(DLL2SRCS) $(DLL2HDRS)
	@./copy-if-changed ../../h         $(COMHDRS)
	@./copy-if-changed ../..           $(ROOTHDR)
	@./copy-if-changed ..              $(BAKSRCS) $(BAKHDRS)

xmakefile: Makefile $(CPPSRCS) $(CSRCS) $(HDRS)
	rm -f xmakefile tmp
	cat Makefile > tmp
	echo '' >> tmp
	$(CC) $(INCS) $(DEFS) -M $(SRCS) >> tmp
	mv tmp xmakefile

$(TARG): $(OBJS)
	rm -f $(TARG)
	$(CC) $(OBJS) $(LIBS) -o $(TARG)

fbcompact: $(FBCOMPACT_OBJS) 
	$(CC) $(FBCOMPACT_OBJS) -o fbcompact

.C.o:
	$(CC) $(INCS) $(DEFS) $(OPTS) -c $*.C

.c.o:
	$(CC) $(INCS) $(DEFS) $(OPTS) -c $*.c

depend: xmakefile

clean:
	rm -f *.C *.H *.c *.h $(OBJS) $(FBCOMPACT_OBJS) $(TARG) $(EXTRAS) xmakefile tmp


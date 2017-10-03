
TARG=	libpfkbak.a
SRCS=	create_backup.C create_file.C \
	delete_backup.C delete_gens.C delete_gen.C \
	extract.C extract_list.C \
	get_info.C update_backup.C \
	list_backups.C list_files.C \
	find_backup.C main.C md5buffer.C
HDRS=	database_elements.H params.H protos.H

OBJS= $(SRCS:.C=.o)
CC= g++
INCS= -I.. -I ../h -I ../util -I ../treesync -I ../dll2 -I ../FileBlock 
DEFS= -D_FILE_OFFSET_BITS=64
OPTS= -g3
LIBS= ../FileBlock/libFileBlock.a ../treesync/libtreesync.a \
	../dll2/libdll2.a -lz

TESTSRCS= t.C
EXTRAS= t

all: xmakefile
	@make -f xmakefile $(TARG) $(EXTRAS)

xmakefile: Makefile $(SRCS) $(HDRS)
	rm -f xmakefile tmp
	cat Makefile > tmp
	echo '' >> tmp
	$(CC) $(INCS) $(DEFS) -M $(SRCS) >> tmp
	mv tmp xmakefile

$(TARG): $(OBJS)
	rm -f $(TARG)
	ar cq $(TARG) $(OBJS)
	ranlib $(TARG)

t:  t.o $(TARG) $(LIBS)
	$(CC) t.o $(TARG) $(LIBS) -o t

.C.o:
	$(CC) $(INCS) $(DEFS) $(OPTS) -c $*.C

doc::
	doxygen

copydoc::
	cp backup-database-design.gif pfkbak/
	tar cf pfkbak-doc.tar pfkbak
	scp pfkbak-doc.tar fitpc:.
	ssh fitpc 'tar -C public_html -xf pfkbak-doc.tar ; rm -f pfkbak-doc.tar'
	rm -f pfkbak-doc.tar

depend: xmakefile

clean:
	rm -f $(TARG) test*.db xmakefile tmp
	rm -f *~ *.o *.a *.obj *.i
	rm -f $(EXTRAS)
	rm -rf html

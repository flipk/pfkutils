
TARG=	libtreesync.a
SRCS=	FileList.C analyze.C copy_file.C create_dirs.C \
	main.C md5.C open_ts_db.C update_db.C delete_file.C
HDRS=	FileList.H db.H protos.H macros.H

OBJS=   $(SRCS:.C=.o)
CC=     g++
INCS=   -I../h -I../dll2 -I../util -I../FileBlock
DEFS=	-D_FILE_OFFSET_BITS=64
OPTS=	-Wall -Werror -g3
LIBS=	../FileBlock/libFileBlock.a ../dll2/libdll2.a ../util/libutil.a
TESTSRCS= t.C
EXTRAS=	t


all: xmakefile
	@make -f xmakefile $(TARG) $(EXTRAS)

xmakefile: Makefile $(SRCS) $(HDRS)
	rm -f xmakefile tmp
	cat Makefile > tmp
	echo '' >> tmp
	$(CC) $(INCS) $(DEFS) -M $(TESTSRCS) $(SRCS) >> tmp
	mv tmp xmakefile

$(TARG): $(OBJS)
	rm -f $(TARG)
	ar cq $(TARG) $(OBJS)
	ranlib $(TARG)

t:            t.o $(TARG) $(LIBS) 
	$(CC) t.o $(TARG) $(LIBS) -o t

.C.o:
	$(CC) $(INCS) $(DEFS) $(OPTS) -c $*.C

depend: xmakefile

clean:
	rm -f $(TARG) xmakefile tmp
	rm -f *~ *.o *.a *.obj *.i
	rm -f $(EXTRAS)
	rm -rf doc


TARG= libteld.a
SRCS= keys.C pick.C main_keygen.C main_keypick.C main_keyresp.C main_pfkteld.C
HDRS= keys.H pick.H

OBJS= $(SRCS:.C=.o)

all: xmakefile
	@make -f xmakefile $(TARG)

xmakefile: Makefile $(SRCS) $(HDRS)
	@cat Makefile > tmpfile
	@gcc -M $(INCS) $(SRCS) >> tmpfile
	@mv tmpfile xmakefile

$(TARG): $(OBJS)
	rm -f $(TARG)
	ar cq $(TARG) $(OBJS)
	ranlib $(TARG)

.C.o:
	g++ -Wall -Werror -c $*.C

clean:
	rm -f *.o $(TARG) xmakefile *~ pfktelkeys*

depend: xmakefile

# DEPENDENCIES automatically appended below this line
# in the xmakefile

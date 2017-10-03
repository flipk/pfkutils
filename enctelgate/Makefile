
TARG=	libetg.a
c_SRCS= base64.c 
cpp_SRCS= packet_decoder.C packet_encoder.C packet.C \
	  adm_gate.C adm_hookup.C tunnel.C d3encdec.C d3des.C \
	  main_genkey.C main_proxy.C main_worker.C
INCS=	-I .. -I ../dll2
CFLAGS= -O6 $(INCS)

SRCS= $(c_SRCS) $(cpp_SRCS)
OBJS= $(c_SRCS:.c=.o) $(cpp_SRCS:.C=.o)

all: xmakefile
	@make -f xmakefile $(TARG)

xmakefile: Makefile $(SRCS)
	@echo calculating dependencies
	@cat Makefile > tmpfile
	@gcc -M $(INCS) $(SRCS) >> tmpfile
	@mv tmpfile xmakefile

$(TARG): $(OBJS) $(LIBDEP)
	@rm -f $(TARG)
	ar cq $(TARG) $(OBJS)

.c.o:
	gcc $(CFLAGS) -c $*.c

.C.o:
	g++ $(CFLAGS) -c $*.C

clean:
	@echo cleaning
	@rm -f *.o *.L *~
	@rm -f $(TARG) xmakefile *core

depend:
	@true

# DEPENDENCIES automatically appended below this line
# in the xmakefile

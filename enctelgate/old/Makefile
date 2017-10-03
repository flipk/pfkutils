
TARG=	t
HDRS=	base64.h adm_gate.H adm_hookup.H fd_mgr.H \
	packet.H packet_decoder.H packet_encoder.H \
	pipe_mgr.H adm_pkt_io.H
c_SRCS= base64.c 
cpp_SRCS= adm_gate.C adm_hookup.C fd_mgr.C main.C \
	packet_decoder.C packet_encoder.C pipe_mgr.C \
	adm_pkt_io.C
INCS=	-I ../pkutils -I ../pkutils/dll2
CFLAGS= -g3 $(INCS)

SRCS= $(c_SRCS) $(cpp_SRCS)
OBJS= $(c_SRCS:.c=.o) $(cpp_SRCS:.C=.o)


all: xmakefile
	@[ ! -d logs ] && mkdir logs || true
	@make -f xmakefile $(TARG)

xmakefile: Makefile $(SRCS) $(HDRS)
	@echo calculating dependencies
	@cat Makefile > tmpfile
	@gcc -M $(INCS) $(SRCS) >> tmpfile
	@mv tmpfile xmakefile

$(TARG): $(OBJS)
	@echo linking program $(TARG)
	@g++ $(CFLAGS) $(OBJS) -o $(TARG)

.c.o:
	@echo compiling $*.c
	@gcc $(CFLAGS) -c -Wa,-a=logs/$*.L $*.c

.C.o:
	@echo compiling $*.C
	@gcc $(CFLAGS) -c -Wa,-a=logs/$*.Lx $*.C
	@c++filt < logs/$*.Lx > logs/$*.L ; rm -f logs/$*.Lx

clean:
	@echo cleaning
	@rm -f *.o *.L *~
	@rm -f $(TARG) xmakefile *core
	@rm -rf logs

# DEPENDENCIES automatically appended below this line
# in the xmakefile

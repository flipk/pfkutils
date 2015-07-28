
SERVER_PROG= wsProxyServer
SERVER_SRCS= proxyServerMain.cc proxyServerConn.cc

CLIENT_PROG= wsProxyClient
CLIENT_SRCS= proxyClientMain.cc proxyClientConn.cc proxyClientTcpAcceptor.cc

PROTOS= proxyMsgs.proto

#

OBJDIR= objs
PROGS= $(OBJDIR)/$(SERVER_PROG) $(OBJDIR)/$(CLIENT_PROG)
INCS= -I.. -I../../libpfkutil -I../../libpfkdll2 -I$(OBJDIR)

GENSRCS= $(PROTOS:%.proto=$(OBJDIR)/%.pb.cc)
GENHDRS= $(PROTOS:%.proto=$(OBJDIR)/%.pb.h)
GENOBJS= $(GENSRCS:.cc=.o)

SRCS= $(SERVER_SRCS) $(CLIENT_SRCS)
SERVER_OBJS= $(SERVER_SRCS:%.cc=$(OBJDIR)/%.o)
CLIENT_OBJS= $(CLIENT_SRCS:%.cc=$(OBJDIR)/%.o)

PFKLIBDIR= $(HOME)/pfk/$(PFKARCH)/lib

LIBS= -Wl,-rpath,$(PFKLIBDIR) -L$(PFKLIBDIR) \
	-lWebAppServer -lpfkutil -lprotobuf -lpthread 

CFLAGS= -O3

ifeq ($(VERBOSE),)
Q=@
else
Q=
endif

all: $(OBJDIR) $(OBJDIR)/xmakefile
	$(Q)+make -f $(OBJDIR)/xmakefile $(PROGS)

$(OBJDIR):
	$(Q)echo making OBJDIR=$(OBJDIR)
	$(Q)mkdir -p $(OBJDIR)

$(OBJDIR)/xmakefile: Makefile $(PROTOS) $(SRCS) $(GENSRCS)
	$(Q)echo depending
	$(Q)rm -f $(OBJDIR)/xmakefile $(OBJDIR)/x
	$(Q)cat Makefile > $(OBJDIR)/x
	$(Q)($(foreach f,   $(SRCS),g++ -M $(f) $(INCS) \
		-MT $(OBJDIR)/$(f:.cc=.o);)) >> $(OBJDIR)/x
	$(Q)($(foreach f,$(GENSRCS),g++ -M $(f) $(INCS) \
		-MT           $(f:.cc=.o);)) >> $(OBJDIR)/x
	$(Q)mv $(OBJDIR)/x $(OBJDIR)/xmakefile

$(GENSRCS): $(OBJDIR)/%.pb.cc: %.proto
	$(Q)echo making protobuf c++
	$(Q)protoc --cpp_out=$(OBJDIR) $<

$(GENOBJS): %.o: %.cc
	$(Q)echo compiling $@
	$(Q)g++ -c $(INCS) $< -o $@

$(SERVER_OBJS) $(CLIENT_OBJS): $(OBJDIR)/%.o: %.cc
	$(Q)echo compiling $@
	$(Q)g++ -c $(INCS) $< -o $@

$(OBJDIR)/$(SERVER_PROG): $(GENOBJS) $(SERVER_OBJS)
	$(Q)echo linking $@
	$(Q)g++ $(SERVER_OBJS) $(GENOBJS) $(LIBS) -o $@

$(OBJDIR)/$(CLIENT_PROG): $(GENOBJS) $(CLIENT_OBJS)
	$(Q)echo linking $@
	$(Q)g++ $(CLIENT_OBJS) $(GENOBJS) $(LIBS) -o $@

install:
	cp $(PROGS) $(HOME)/pfk/$(PFKARCH)/bin

clean:
	$(Q)echo cleaning
	$(Q)rm -rf $(OBJDIR)

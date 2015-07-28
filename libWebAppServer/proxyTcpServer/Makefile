
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

all: $(OBJDIR) $(OBJDIR)/xmakefile
	+make -f $(OBJDIR)/xmakefile $(PROGS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/xmakefile: Makefile $(PROTOS) $(SRCS) $(GENSRCS)
	rm -f $(OBJDIR)/xmakefile $(OBJDIR)/x
	cat Makefile > $(OBJDIR)/x
	for f in $(SRCS) ; do \
		g++ -M $$f $(INCS) \
			-MT $(OBJDIR)/$${f%.cc}.o \
			-MF $(OBJDIR)/xdeps ; \
		cat $(OBJDIR)/xdeps >> $(OBJDIR)/x ; \
		rm -f $(OBJDIR)/xdeps ; \
	done
	for f in $(GENSRCS) ; do \
		g++ -M $$f $(INCS) \
			-MT $${f%.cc}.o \
			-MF $(OBJDIR)/xdeps ; \
		cat $(OBJDIR)/xdeps >> $(OBJDIR)/x ; \
		rm -f $(OBJDIR)/xdeps ; \
	done
	mv $(OBJDIR)/x $(OBJDIR)/xmakefile

$(GENSRCS): $(OBJDIR)/%.pb.cc: %.proto
	protoc --cpp_out=$(OBJDIR) $<

$(GENOBJS): %.o: %.cc
	g++ -c $(INCS) $< -o $@

$(SERVER_OBJS) $(CLIENT_OBJS): $(OBJDIR)/%.o: %.cc
	g++ -c $(INCS) $< -o $@

$(OBJDIR)/$(SERVER_PROG): $(SERVER_OBJS) $(GENOBJS)
	g++ $(SERVER_OBJS) $(GENOBJS) $(LIBS) -o $@

$(OBJDIR)/$(CLIENT_PROG): $(CLIENT_OBJS) $(GENOBJS)
	g++ $(CLIENT_OBJS) $(GENOBJS) $(LIBS) -o $@

install:
	cp $(PROGS) $(HOME)/pfk/$(PFKARCH)/bin

clean:
	rm -rf $(OBJDIR)

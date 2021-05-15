
OBJDIR = obj
PFKU_MISC = $(HOME)/proj/pfkutils/misc
SIMPLEJSONINC = -I$(PFKU_MISC)/json
SIMPLEJSONLIB = $(PFKU_MISC)/json/obj/libsimplejson.a
PROTOPARSERLIB = $(PFKU_MISC)/protobuf/obj/libprotoparser.a
JSON_CONVERTER_MAKER = $(PFKU_MISC)/jsonproto/obj/makeJsonProtoConverter
JSON_CONVERTER_TARGETS = \
	$(SIMPLEJSONLIB) $(PROTOPARSERLIB) $(JSON_CONVERTER_MAKER)

LIB_TARGETS += lsblk_output

lsblk_output_TARGET = $(OBJDIR)/liblsblk_output.a
lsblk_output_PROTOSRCS = lsblk-output.proto
lsblk_output_CXXSRCS = \
	run_lsblk.cc \
	$(OBJDIR)/lsblk_converter.cc \
	../libpfkutil/childprocessmanager.cc
lsblk_output_INCS = \
	-I. -I$(OBJDIR) \
	$(SIMPLEJSONINC) \
	-I../libpfkutil
lsblk_output_DEFS = \
	-DLSBLK_OUTPUT_PROTO_HDR=\"$(lsblk_output_lsblk-output.proto_HDR)\"

PROG_TARGETS += test_run_lsblk

test_run_lsblk_TARGET = $(OBJDIR)/test_run_lsblk
test_run_lsblk_CXXSRCS = test_run_lsblk.cc
test_run_lsblk_DEFS = -DLSBLK_OUTPUT_PROTO_HDR=\"$(lsblk_output_lsblk-output.proto_HDR)\"
test_run_lsblk_DEPLIBS = $(lsblk_output_TARGET) $(SIMPLEJSONLIB)
test_run_lsblk_LIBS = -lprotobuf -lpthread

include ../Makefile.inc

$(OBJDIR)/lsblk_converter.cc: \
	$(OBJDIR)/lsblk_converter-stamp $(OBJDIR)/jsontools-stamp
$(OBJDIR)/lsblk_converter.h : \
	$(OBJDIR)/lsblk_converter-stamp $(OBJDIR)/jsontools-stamp
$(OBJDIR)/lsblk_converter-stamp : lsblk-output.proto $(JSON_CONVERTER_MAKER)
	rm -f $@
	$(JSON_CONVERTER_MAKER) . $< \
		$(OBJDIR)/lsblk_converter.h $(OBJDIR)/lsblk_converter.cc
	sed -e s,\"lsblk-output.pb.h\",LSBLK_OUTPUT_PROTO_HDR, \
		-i.orig $(OBJDIR)/lsblk_converter.h
	touch $@

$(JSON_CONVERTER_TARGETS) : $(OBJDIR)/jsontools-stamp
$(OBJDIR)/jsontools-stamp: $(JSON_CONVERTER_TARGETS)
	rm -f $@
	cd $(PFKU_MISC)/json && make
	cd $(PFKU_MISC)/protobuf && make
	cd $(PFKU_MISC)/jsonproto && make
# dont touch, this will make it rebuild those dirs every time.
#	touch $@

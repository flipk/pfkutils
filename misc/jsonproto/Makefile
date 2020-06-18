
ifneq ($(wildcard $(HOME)/.pfkutils_config.$(CONFIG)),)
CONFIG_FILE := $(HOME)/.pfkutils_config.$(CONFIG)
else ifneq ($(wildcard $(HOME)/.pfkutils_config),)
CONFIG_FILE := $(HOME)/.pfkutils_config
endif

ifneq ($(CONFIG_FILE),)
include $(CONFIG_FILE)
endif

OBJDIR = obj

PROTO_LIBS += -lprotobuf

LIB_TARGETS = jsonproto

jsonproto_TARGET = $(OBJDIR)/libjsonproto.a
jsonproto_CXXSRCS = jsonproto.cc
jsonproto_INCS = -I../protobuf

PROG_TARGETS = tmpl2c makeJsonProtoConverter testConverter

tmpl2c_TARGET = $(OBJDIR)/template_to_c
tmpl2c_CXXSRCS = template_to_c.cc

makeJsonProtoConverter_TARGET = $(OBJDIR)/makeJsonProtoConverter
makeJsonProtoConverter_CXXSRCS = \
	makeJsonProtoConverter.cc \
	obj/converter_template.cc \
	template_patterns.cc
makeJsonProtoConverter_INCS = -I../protobuf -I.
makeJsonProtoConverter_DEPLIBS = \
	$(jsonproto_TARGET) ../protobuf/obj/libprotoparser.a

testConverter_TARGET = $(OBJDIR)/testConverter
testConverter_CXXSRCS = \
	testConverter.cc \
	obj/test1_converter.cc \
	obj/test2_converter.cc \
	obj/test1.pb.cc \
	obj/test2.pb.cc
testConverter_INCS = -I../json $(PROTOBUF_INC) -I.
testConverter_LIBS = $(PROTOBUF_LIB) ../json/obj/libsimplejson.a

include $(HOME)/proj/pfkutils/Makefile.inc

obj/converter_template.cc: obj/converter_template.stamp
obj/converter_template.h: obj/converter_template.stamp
obj/converter_template.stamp: converter.tmpl $(tmpl2c_TARGET)
	rm -f $@
	$(tmpl2c_TARGET) converter.tmpl obj/converter_template.cc obj/converter_template.h
	touch $@

obj/test1_converter.cc : obj/test1_converter.stamp
obj/test1_converter.h : obj/test1_converter.stamp
obj/test1_converter.stamp : $(makeJsonProtoConverter_TARGET) ../protobuf/test1.proto obj/test2_converter.stamp obj/test1.pb.stamp obj/test2.pb.stamp
	rm -f $@
	$(makeJsonProtoConverter_TARGET) ../protobuf ../protobuf/test1.proto obj/test1_converter.h obj/test1_converter.cc
	touch $@

obj/test2_converter.cc : obj/test2_converter.stamp
obj/test2_converter.h : obj/test2_converter.stamp
obj/test2_converter.stamp : $(makeJsonProtoConverter_TARGET) ../protobuf/test2.proto obj/test1.pb.stamp obj/test2.pb.stamp
	rm -f $@
	$(makeJsonProtoConverter_TARGET) ../protobuf ../protobuf/test2.proto obj/test2_converter.h obj/test2_converter.cc
	touch $@

obj/test1.pb.cc: obj/test1.pb.stamp
obj/test1.pb.h: obj/test1.pb.stamp
obj/test1.pb.stamp: ../protobuf/test1.proto
	rm -f $@
	protoc --proto_path=../protobuf --cpp_out=obj ../protobuf/test1.proto
	touch $@

obj/test2.pb.cc: obj/test2.pb.stamp
obj/test2.pb.h: obj/test2.pb.stamp
obj/test2.pb.stamp: ../protobuf/test2.proto
	rm -f $@
	protoc --proto_path=../protobuf --cpp_out=obj ../protobuf/test2.proto
	touch $@

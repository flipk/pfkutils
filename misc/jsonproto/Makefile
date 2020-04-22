
OBJDIR = obj

PROTO_LIBS += -lprotobuf

LIB_TARGETS = json protobuf jsonproto

json_TARGET = $(OBJDIR)/libsimplejson.a
json_CXXSRCS = ../json/simple_json.cc ../json/json_tokenizer.cc ../json/json_parser.cc

protobuf_TARGET = $(OBJDIR)/libprotoparser.a
protobuf_CXXSRCS = ../protobuf/protobuf_tokenizer.cc ../protobuf/protobuf_parser.cc

jsonproto_TARGET = $(OBJDIR)/libjsonproto.a
jsonproto_CXXSRCS = jsonproto.cc
jsonproto_INCS = -I../protobuf

PROG_TARGETS = tmpl2c testParser testConverter

tmpl2c_TARGET = $(OBJDIR)/template_to_c
tmpl2c_CXXSRCS = template_to_c.cc

testParser_TARGET = $(OBJDIR)/testParser
testParser_CXXSRCS = testParser.cc converter_template.cc template_patterns.cc
testParser_INCS = -I../protobuf
testParser_DEPLIBS = $(jsonproto_TARGET) $(json_TARGET) $(protobuf_TARGET)

testConverter_TARGET = $(OBJDIR)/testConverter
testConverter_CXXSRCS = testConverter.cc test1_converter.cc test2_converter.cc obj/test1.pb.cc obj/test2.pb.cc
testConverter_INCS = -I../json $(PROTO_INCS)
testConverter_LIBS = $(PROTO_LIBS) ../json/obj/libsimplejson.a

EXTRA_CLEAN = my_clean

include $(HOME)/proj/pfkutils/Makefile.inc

converter_template.cc: converter.tmpl $(tmpl2c_TARGET)
	$(tmpl2c_TARGET) converter.tmpl converter_template.cc converter_template.h

test1_converter.cc test2_converter.cc: $(testParser_TARGET) ../protobuf/test1.proto ../protobuf/test2.proto
	$(testParser_TARGET)
	make protoc

protoc:
	protoc --proto_path=../protobuf --cpp_out=obj ../protobuf/test1.proto
	protoc --proto_path=../protobuf --cpp_out=obj ../protobuf/test2.proto

my_clean:
	rm -f converter_template.cc converter_template.h test1_converter.cc test1_converter.h

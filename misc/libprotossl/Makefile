
SRCS= libprotossl.cc test.cc

all: xmakefile
	+make -f xmakefile libprotossl.a t

xmakefile: test_proto.pb.h Makefile $(SRCS)
	rm -f tmp
	cat Makefile > tmp
	echo '' >> tmp
	g++ -M $(SRCS) >> tmp
	mv tmp xmakefile

test_proto.pb.cc test_proto.pb.h: test_proto.proto
	protoc --cpp_out=. test_proto.proto

libprotossl.a: libprotossl.o
	rm -f libprotossl.a
	ar cq libprotossl.a libprotossl.o

t: libprotossl.a test_proto.pb.o test.o 
	g++ test_proto.pb.o test.o libprotossl.a -o t -lprotobuf -lpolarssl

test.o: test.cc

test_proto.pb.o: test_proto.pb.cc

.cc.o:
	g++ -c -Wall -Werror $*.cc

clean:
	rm -f libprotossl.a libprotossl.o test.o test_proto.pb.cc test_proto.pb.h test_proto.pb.o t xmakefile



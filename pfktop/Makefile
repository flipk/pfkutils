
ifeq ($(TARGET),)
OBJDIR = obj
else
ifeq ($(TARGET),L1C8)
OBJDIR = obj.l1c8

cross=/shared/mh_tools/L1C8/1.1/sysroots/x86_64-pokysdk-linux/usr/bin/armv7a-vfp-neon-poky-linux-gnueabi/arm-poky-linux-gnueabi-
cflags=-mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=softfp --sysroot=/shared/mh_tools/L1C8/1.1/sysroots/armv7a-vfp-neon-poky-linux-gnueabi/

else
$(error target not supported)
endif
endif

CC = $(cross)gcc $(cflags)
CXX = $(cross)g++ $(cflags)
AR = $(cross)ar
RANLIB = $(cross)ranlib

PROG_TARGETS= pfktop pfkrenice

pfktop_TARGET = $(OBJDIR)/pfktop
pfktop_CXXSRCS = pfktop.cc \
	pidlist.cc options.cc screen.cc \
	fileparser.cc tiddb.cc tidentry.cc
pfktop_DEFS = -Dpfktop_main=main
pfktop_INCS = -I../libpfkutil
pfktop_LIBS= -lpthread -lrt

pfkrenice_TARGET = $(OBJDIR)/pfkrenice
pfkrenice_CXXSRCS = pfkrenice.cc
pfkrenice_DEFS = -Dpfkrenice_main=main

include ../Makefile.inc

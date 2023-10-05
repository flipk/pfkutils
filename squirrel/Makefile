
SQL3GEN = ../../sqlite3gen/build_native/sql3gen

OBJDIR = obj

# DEBUG_H_MACRO_GENERATION=y

LIB_TARGETS += squirrel
squirrel_TARGET = $(OBJDIR)/libsquirrel.a
squirrel_DEFS = \
	-DSQUIRREL_DB_PROTO_HDR=\"$(squirrel_obj/squirrel_db.proto_HDR)\"
squirrel_INCS = -I../libpfkutil
squirrel_PROTOSRCS = \
	$(OBJDIR)/squirrel_db.proto
squirrel_CXXSRCS = \
	squirrel_log.cc \
	squirrel_log_utilities.cc \
	../libpfkutil/thread_slinger.cc \
	../libpfkutil/signal_backtrace.cc \
	$(OBJDIR)/squirrel_db.cc


PROG_TARGETS += test_squirrel
test_squirrel_TARGET = $(OBJDIR)/test_squirrel
test_squirrel_DEFS = \
	-DSQUIRREL_DB_PROTO_HDR=\"$(squirrel_obj/squirrel_db.proto_HDR)\"
test_squirrel_DEPLIBS = $(squirrel_TARGET)
test_squirrel_LIBS = -lsqlite3 -lprotobuf
test_squirrel_INCS = -I../libpfkutil
test_squirrel_LDFLAGS = -rdynamic
test_squirrel_CXXSRCS = \
	squirrel_test.cc


alltargets: $(OBJDIR)/squirrel_db.stamp all

include ../Makefile.inc

$(OBJDIR)/squirrel_db.stamp: squirrel_db.schema
	$(Q)rm -f $(OBJDIR)/squirrel_db.stamp
	$(Q)$(SQL3GEN) squirrel_db.schema \
		squirrel_db.cc squirrel_db.h squirrel_db.proto
	$(Q)mkdir -p $(OBJDIR)
	$(Q)sed -e s,squirrel_db.pb.h,squirrel-obj_squirrel_db.pb.h, \
		< squirrel_db.h > $(OBJDIR)/squirrel_db.h
	$(Q)rm -f squirrel_db.h
	$(Q)mv squirrel_db.cc squirrel_db.proto $(OBJDIR)
	$(Q)touch $(OBJDIR)/squirrel_db.stamp

$(OBJDIR)/squirrel_db.cc: $(OBJDIR)/squirrel_db.stamp
$(OBJDIR)/squirrel_db.h: $(OBJDIR)/squirrel_db.stamp
$(OBJDIR)/squirrel_db.proto: $(OBJDIR)/squirrel_db.stamp

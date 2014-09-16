
CSCOPE= cscope-15.7a
CTWM= ctwm-3.8a

all-local:
if PFK_AM_BUILD_cscope
	if [ ! -f $(builddir)/$(CSCOPE)/Makefile ] ; then \
		touch $(abs_srcdir)/$(CSCOPE)/{Makefile.in,config.h.in,configure} ; \
	fi
	if [ ! -d $(builddir)/$(CSCOPE) ] ; then \
		mkdir $(builddir)/$(CSCOPE) ; \
	fi
	set -e ; if [ ! -f $(CSCOPE)/Makefile ] ; then \
		cd $(builddir)/$(CSCOPE) && $(abs_srcdir)/$(CSCOPE)/configure --prefix=$(exec_prefix) ; \
	fi
	cd $(builddir)/$(CSCOPE) ; make all
endif
if PFK_AM_BUILD_ctwm
	if [ ! -d $(builddir)/$(CTWM) ] ; then \
		cd $(abs_srcdir) ; \
		dirs=`find $(CTWM) -type d -print` ; \
		files=`find $(CTWM) -type f -print` ; \
		echo dirs = $$dirs ; \
		echo files = $$files ; \
		cd $(abs_builddir) ; \
		mkdir -p $$dirs ; \
		for f in $$files ; do \
			ln -s $(abs_srcdir)/$$f $$f ; \
		done ; \
	fi
	set -e ; if [ ! -f $(CTWM)/Makefile ] ; then \
		cd $(abs_builddir)/$(CTWM) && xmkmf ; \
	fi
	cd $(abs_builddir)/$(CTWM) ; make all
endif

install-exec-local:
	mkdir -p $(DESTDIR)$(bindir) || true
if PFK_AM_BUILD_cscope
	cp $(CSCOPE)/src/cscope $(DESTDIR)$(bindir)/cscope.new
	chmod 755 $(DESTDIR)$(bindir)/cscope.new
	$(STRIP) $(DESTDIR)$(bindir)/cscope.new
	if [ -f $(DESTDIR)$(bindir)/cscope ] ; then \
		mv $(DESTDIR)$(bindir)/cscope $(DESTDIR)$(bindir)/cscope.old ;\
	fi
	mv $(DESTDIR)$(bindir)/cscope.new $(DESTDIR)$(bindir)/cscope
	rm -f $(DESTDIR)$(bindir)/cscope.old
endif
if PFK_AM_BUILD_ctwm
	cp $(CTWM)/ctwm $(DESTDIR)$(bindir)/ctwm.new
	chmod 755 $(DESTDIR)$(bindir)/ctwm.new
	$(STRIP) $(DESTDIR)$(bindir)/ctwm.new
	if [ -f $(DESTDIR)$(bindir)/ctwm ] ; then \
		mv $(DESTDIR)$(bindir)/ctwm $(DESTDIR)$(bindir)/ctwm.old ; \
	fi
	mv $(DESTDIR)$(bindir)/ctwm.new $(DESTDIR)$(bindir)/ctwm
	rm -f $(DESTDIR)$(bindir)/ctwm.old
endif

clean-local:
if PFK_AM_BUILD_cscope
	set -e ; if [ -f $(CSCOPE)/Makefile ] ; then \
		cd $(CSCOPE) ; make clean ; \
	fi
endif
if PFK_AM_BUILD_ctwm
	set -e ; if [ -f $(CTWM)/Makefile ] ; then \
		cd $(CTWM) ; make clean ; \
	fi
	rm -f $(CTWM)/Makefile
endif

distclean-local:
if PFK_AM_BUILD_cscope
	set -e ; if [ -f $(CSCOPE)/Makefile ] ; then \
		cd $(CSCOPE) ; make distclean ; \
	fi
endif
if PFK_AM_BUILD_ctwm
	set -e ; if [ -f $(CTWM)/Makefile ] ; then \
		cd $(CTWM) ; make clean ; \
	fi
	rm -f $(CTWM)/Makefile
endif


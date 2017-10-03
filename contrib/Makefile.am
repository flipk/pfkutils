
CSCOPE= cscope-15.7a
PDKSH= pdksh-5.2.14
CTWM= ctwm-3.8a

all-local:
if PFK_AM_BUILD_cscope
	if [ ! -d $(builddir)/$(CSCOPE) ] ; then \
		mkdir $(builddir)/$(CSCOPE) ; \
	fi
	set -e ; if [ ! -f $(CSCOPE)/Makefile ] ; then \
		cd $(builddir)/$(CSCOPE) && $(abs_srcdir)/$(CSCOPE)/configure --prefix=$(exec_prefix) ; \
	fi
	make -C $(builddir)/$(CSCOPE) all
endif
if PFK_AM_BUILD_pdksh
	if [ ! -d $(builddir)/$(PDKSH) ] ; then \
		mkdir $(builddir)/$(PDKSH) ; \
	fi
	set -e ; if [ ! -f $(PDKSH)/Makefile ] ; then \
		cd $(builddir)/$(PDKSH) && $(abs_srcdir)/$(PDKSH)/configure --prefix=$(exec_prefix) ; \
	fi
	make -C $(builddir)/$(PDKSH) all
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
	make -C $(abs_builddir)/$(CTWM) all
endif

install-exec-local:
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
if PFK_AM_BUILD_pdksh
	cp $(PDKSH)/ksh $(DESTDIR)$(bindir)/ksh.new
	chmod 755 $(DESTDIR)$(bindir)/ksh.new
	$(STRIP) $(DESTDIR)$(bindir)/ksh.new
	if [ -f $(DESTDIR)$(bindir)/ksh ] ; then \
		mv $(DESTDIR)$(bindir)/ksh $(DESTDIR)$(bindir)/ksh.old ; \
	fi
	mv $(DESTDIR)$(bindir)/ksh.new $(DESTDIR)$(bindir)/ksh
	rm -f $(DESTDIR)$(bindir)/ksh.old
	cd $(DESTDIR)$(bindir) && rm -f pdksh && ln ksh pdksh
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
		make -C $(CSCOPE) clean ; \
	fi
endif
if PFK_AM_BUILD_pdksh
	set -e ; if [ -f $(PDKSH)/Makefile ] ; then \
		make -C $(PDKSH) clean ; \
	fi
endif
if PFK_AM_BUILD_ctwm
	set -e ; if [ -f $(CTWM)/Makefile ] ; then \
		make -C $(CTWM) clean ; \
	fi
	rm -f $(CTWM)/Makefile
endif

distclean-local:
if PFK_AM_BUILD_cscope
	set -e ; if [ -f $(CSCOPE)/Makefile ] ; then \
		make -C $(CSCOPE) distclean ; \
	fi
endif
if PFK_AM_BUILD_pdksh
	set -e ; if [ -f $(PDKSH)/Makefile ] ; then \
		make -C $(PDKSH) distclean ; \
	fi
endif
if PFK_AM_BUILD_ctwm
	set -e ; if [ -f $(CTWM)/Makefile ] ; then \
		make -C $(CTWM) clean ; \
	fi
	rm -f $(CTWM)/Makefile
endif


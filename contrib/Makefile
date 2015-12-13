
all: $(PROGS)

CSCOPE_DIR= cscope-15.7a

cscope:
	@set -e ; if [ ! -d $(OBJDIR)/$(CSCOPE_DIR) ] ; then \
		echo configuring cscope ; \
		tar cf - $(CSCOPE_DIR) | tar -C $(OBJDIR) -xf - ; \
		cd $(OBJDIR)/$(CSCOPE_DIR) ; \
		./configure --prefix=$(HOME)/pfk/$(PFKARCH) ; \
	else \
		FILELIST="" ; \
		for f in `find $(CSCOPE_DIR) -type f` ; do \
			if [ $$f -nt $(OBJDIR)/$$f ] ; then \
				FILELIST="$$FILELIST $$f" ; \
			fi ; \
		done ; \
		if [ "x$$FILELIST" != "x" ] ; then \
			echo updating $$FILELIST ; \
			tar cf - $$FILELIST | tar -C $(OBJDIR) -xvf - ; \
		fi ; \
	fi
	@+make -C $(OBJDIR)/$(CSCOPE_DIR)

CTWM_DIR= ctwm-3.8a

# no "+" on this make rule because dependencies
# are wrong on some yacc thing; it won't build with parallel jobs
ctwm:
	@set -e ; if [ ! -d $(OBJDIR)/$(CTWM_DIR) ] ; then \
		echo configuring ctwm ; \
		tar cf - $(CTWM_DIR) | tar -C $(OBJDIR) -xf - ; \
		cd $(OBJDIR)/$(CTWM_DIR) ; \
		xmkmf ; \
	else \
		FILELIST="" ; IMAKE=0 ; \
		for f in `find $(CTWM_DIR) -type f` ; do \
			if [ $$f -nt $(OBJDIR)/$$f ] ; then \
				FILELIST="$$FILELIST $$f" ; \
				[ $${f#*/} == "Imakefile" ] && IMAKE=1 ; \
				[ $${f#*/} == "Imakefile.local" ] && IMAKE=1 ; \
			fi ; \
		done ; \
		if [ "x$$FILELIST" != "x" ] ; then \
			echo updating $$FILELIST ; \
			tar cf - $$FILELIST | tar -C $(OBJDIR) -xvf - ; \
		fi ; \
		if [ $$IMAKE -eq 1 ] ; then \
			echo configuring ctwm ; \
			cd $(OBJDIR)/$(CTWM_DIR) ; \
			xmkmf ; \
		fi ; \
	fi
	@make -C $(OBJDIR)/$(CTWM_DIR)




all: $(PROGS)

install: $(foreach prog,$(PROGS),$(prog)-install )

RXVT_VERSION= pfk-2016-0410
FLUXBOX_VERSION= pfk-1_3_7
FISH_VERSION= 2.4.0

######################################################

clone-https:
	rm -rf rxvt-unicode fluxbox fish-shell
	git clone -n https://github.com/flipk/rxvt-unicode.git
	git clone -n https://github.com/flipk/fluxbox.git
	git clone -n https://github.com/fish-shell/fish-shell.git
	cd rxvt-unicode && git checkout $(RXVT_VERSION)
	cd fluxbox      && git checkout $(FLUXBOX_VERSION)
	cd fish-shell   && git checkout $(FISH_VERSION)

clone-me:
	rm -rf rxvt-unicode fluxbox
	git clone -n git@github.com:flipk/rxvt-unicode.git
	git clone -n git@github.com:flipk/fluxbox.git
	git clone -n https://github.com/fish-shell/fish-shell.git
	cd rxvt-unicode && git checkout $(RXVT_VERSION)
	cd fluxbox      && git checkout $(FLUXBOX_VERSION)
	cd fish-shell   && git checkout $(FISH_VERSION)

######################################################

CSCOPE_DIR= cscope-15.7a

cscope-pfk:
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
	@+make -C $(OBJDIR)/$(CSCOPE_DIR) $(PFK_CONFIG_contrib_makejobs)

cscope-pfk-install:
	@echo installing cscope to $(INSTALL_BIN_DIR)
	@cp $(OBJDIR)/$(CSCOPE_DIR)/src/cscope $(INSTALL_BIN_DIR)

######################################################

CTWM_DIR= ctwm-3.8a

# no "+" on this make rule because dependencies
# are wrong on some yacc thing; it won't build with parallel jobs
ctwm-pfk:
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

ctwm-pfk-install:
	@echo installing ctwm to $(INSTALL_BIN_DIR)
	@cp $(OBJDIR)/$(CTWM_DIR)/ctwm $(INSTALL_BIN_DIR)

######################################################

rxvt-pfk:
	@echo making rxvt-unicode
	chmod u+x build-rxvt-unicode.sh
	./build-rxvt-unicode.sh

rxvt-pfk-install:
	@echo installing rxvt-unicode
	chmod u+x install-rxvt-unicode.sh
	./install-rxvt-unicode.sh

######################################################

fluxbox-pfk:
	@echo making fluxbox
	chmod u+x build-fluxbox.sh
	./build-fluxbox.sh

fluxbox-pfk-install:
	@echo installing fluxbox
	chmod u+x install-fluxbox.sh
	./install-fluxbox.sh

######################################################

fish-pfk:
	@echo making fish

fish-pfk-install:
	@echo installing fish

######################################################

clean:
	cd $(CSCOPE_DIR) && make clean || true
	cd $(CTWM_DIR) && make clean || true
	cd fluxbox && make clean || true
	cd rxvt-unicode && make clean || true


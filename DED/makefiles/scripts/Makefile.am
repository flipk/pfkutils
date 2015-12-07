
PFK_SCRIPT_FILES = \
	architecture \
	changed \
	doxtrms \
	ec \
	ec1 \
	ec2 \
	ec3 \
	ec4 \
	ec5 \
	ec6 \
	ec7 \
	ec8 \
	pfkterm \
	make_linux_cscope \
	mcs \
	myemacs-checkout-helper \
	myemacs-cscope-rebuild-helper \
	git-diff-helper.sh \
	git-merge-helper.sh \
	set-background.sh \
	refresh

PFK_SCRIPT_FILES_GENERATED = \
	cs \
	emacs-lone \
	myemacs-cscope-helper \
	myshell \
	myshell-launcher \
	pf_frontend

install-data-local:
	cd $(abs_srcdir) && cp $(PFK_SCRIPT_FILES) $(prefix)/bin
	cd $(abs_builddir) && cp $(PFK_SCRIPT_FILES_GENERATED) $(prefix)/bin
	cd $(prefix)/bin && \
		chmod 755 $(PFK_SCRIPT_FILES) $(PFK_SCRIPT_FILES_GENERATED)
	$(DESTDIR)$(bindir)/setup-links-sh

diffdotfiles:
	@for f in $(PFK_SCRIPT_FILES) ; do \
		diff -u $(prefix)/bin/$$f $(abs_srcdir)/$$f ; \
	done
	@for f in $(PFK_SCRIPT_FILES_GENERATED) ; do \
		diff -u $(prefix)/bin/$$f $(abs_builddir)/$$f ; \
	done

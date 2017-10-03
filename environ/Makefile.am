
if PFK_ENVIRON

DOT_FILES = \
	Xresources ctwmrc hexdump_format \
	screenrc xinitrc xsession

ELISP_FILES = emacs-lone.el emacs.el

KSHRC_LOCALS = kshrc.local.atd kshrc.local.blade kshrc.local.isd

KSHRC_LOCAL = kshrc.local.@PFK_ENVIRON@

SETUP_SH = setup-links-sh

install-data-local:
	mkdir -p $(DESTDIR)$(sysconfdir) || true
	mkdir -p $(DESTDIR)$(prefix)/bin || true
	chmod 755 $(DESTDIR)$(sysconfdir) $(DESTDIR)$(prefix)/bin
	cd $(srcdir) && \
		cp $(DOT_FILES) $(ELISP_FILES) $(KSHRC_LOCALS) \
			$(DESTDIR)$(sysconfdir)
	cd $(abs_builddir) && cp kshrc profile $(DESTDIR)$(sysconfdir)
	cd $(DESTDIR)$(sysconfdir) && \
		chmod 644 $(DOT_FILES) $(ELISP_FILES) $(KSHRC_LOCALS)
	sed \
		-e "s,@SHELL_PATH@,@PFK_SHELL_PATH@," \
		-e "s,@ENVIRON@,@PFK_ENVIRON@," \
		-e "s,@ETCDIR@,$(sysconfdir)," \
		-e "s,@KSHRC_LOCAL@,$(KSHRC_LOCAL)," \
		-e "s,@DOT_FILES@,$(DOT_FILES)," \
		-e "s,@ELISP_FILES@,$(ELISP_FILES)," \
		-e "s,@SCRIPTS_DIR@,$(prefix)/bin," \
		-e "s,@PFK_BIN_DIR@,$(bindir)," \
		< $(srcdir)/setup-links-sh \
		> $(DESTDIR)$(bindir)/setup-links-sh
	chmod 744 $(DESTDIR)$(bindir)/setup-links-sh

endif

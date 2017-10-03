
MAKEFILES= Makefile.etg_proxy Makefile.etg_config

all:
	@for d in $(MAKEFILES) ; do \
		make -f $$d all ; \
	done
	@ls -l etg*

clean:
	@for d in $(MAKEFILES) ; do \
		make -f $$d clean ; \
	done

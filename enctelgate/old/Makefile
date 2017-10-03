
MAKEFILES= Makefile.etg_proxy

all:
	@for d in $(MAKEFILES) ; do \
		make -f $$d all ; \
	done

clean:
	@for d in $(MAKEFILES) ; do \
		make -f $$d clean ; \
	done


all: $(PROGS)

CSCOPE_DIR= cscope-15.7a

cscope:
	@set -e ; if [ ! -d $(OBJDIR)/$(CSCOPE_DIR) ] ; then \
		echo configuring cscope ; \
		tar cf - $(CSCOPE_DIR) | tar -C $(OBJDIR) -xf - ; \
		cd $(OBJDIR)/$(CSCOPE_DIR) ; \
		./configure --prefix=$(HOME)/pfk/$(PFKARCH) ; \
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
	fi
	@make -C $(OBJDIR)/$(CTWM_DIR)

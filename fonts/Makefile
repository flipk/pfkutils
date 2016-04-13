
FONTDIR= $(HOME)/pfk/fonts
ALIAS= fonts.alias

BDF_FILES= 5x7.bdf 6x13.bdf 10x20.bdf
PCFGZ_FILES= $(BDF_FILES:.bdf=.pcf.gz)

all: $(PCFGZ_FILES)

$(PCFGZ_FILES):  %.pcf.gz: %.bdf
	bdftopcf -t < $< | gzip > $@

clean:
	rm -f $(PCFGZ_FILES)

install:
	mkdir -p $(FONTDIR)
	cp $(PCFGZ_FILES) $(ALIAS) $(FONTDIR)
	cd $(FONTDIR) && mkfontdir .
	[ x$(DISPLAY) != x ] && xset +fp $(FONTDIR) || true
	tar -C $(HOME) -jxvf dot-fonts.tar.bz2

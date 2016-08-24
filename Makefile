# -*- Mode:makefile-gmake; tab-width:8 -*-

ifeq ($(CONFIG),)

##############################################

KNOWN_CONFIGS= adler bilskirnir blade cygwin droid motlap atdsrv

all:
	@echo please specify CONFIG= from config/ subdir
	@echo or do 'make known_config' where known_config is
	@echo one of: $(KNOWN_CONFIGS)

define PER_CONFIG_RULES
$(config):
	$(Q)+make CONFIG=$(config)

$(config)-cscope:
	$(Q)make CONFIG=$(config) cscope

$(config)-install:
	$(Q)make CONFIG=$(config) install

$(config)-clean:
	$(Q)make CONFIG=$(config) clean

$(config)-diffdotfiles:
	$(Q)make CONFIG=$(config) diffdotfiles

endef

$(eval $(foreach config,$(KNOWN_CONFIGS),$(PER_CONFIG_RULES)))

clean:
	rm -rf obj.*
	make -C contrib clean

##############################################

else # $CONFIG

##############################################

PFKARCH := $(shell ./scripts/architecture)
OBJDIR= obj.$(PFKARCH).$(CONFIG)

ifeq ($(DISABLE_RDYNAMIC),)
LDFLAGS += -rdynamic # for backtrace
endif

INCLUDE_MAKEFILES= config/$(CONFIG) config/always

include Makefile.inc

##############################################

endif # $CONFIG

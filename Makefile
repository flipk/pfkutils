# -*- Mode:makefile-gmake; tab-width:8 -*-

ifeq ($(CONFIG),)

##############################################

KNOWN_CONFIGS= blade adler droid

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

endef

$(eval $(foreach config,$(KNOWN_CONFIGS),$(PER_CONFIG_RULES)))

clean:
	rm -rf obj.*

##############################################

else # $CONFIG

##############################################

PFKARCH := $(shell ./scripts/architecture)
OBJDIR= obj.$(PFKARCH).$(CONFIG)

LDFLAGS = -rdynamic # for backtrace

INCLUDE_MAKEFILES= config/$(CONFIG) config/always

include Makefile.inc

##############################################

endif # $CONFIG

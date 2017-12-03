# -*- Mode:makefile-gmake; tab-width:8 -*-

# This is free and unencumbered software released into the public domain.
# 
# Anyone is free to copy, modify, publish, use, compile, sell, or
# distribute this software, either in source code form or as a compiled
# binary, for any purpose, commercial or non-commercial, and by any
# means.
# 
# In jurisdictions that recognize copyright laws, the author or authors
# of this software dedicate any and all copyright interest in the
# software to the public domain. We make this dedication for the benefit
# of the public at large and to the detriment of our heirs and
# successors. We intend this dedication to be an overt act of
# relinquishment in perpetuity of all present and future rights to this
# software under copyright law.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
# 
# For more information, please refer to <http://unlicense.org>

ifeq ($(CONFIG),)
CONFIG_FILE := $(wildcard $(HOME)/.pfkutils_config)
ifneq ($(CONFIG_FILE),)
CONFIG := $(shell cat $(CONFIG_FILE))
endif
endif

ifeq ($(CONFIG),)

##############################################

KNOWN_CONFIGS= adler bilskirnir blade cygwin droid motlap atdsrv

all:
	@echo please specify CONFIG= from config/ subdir
	@echo or do 'make known_config' where known_config is
	@echo one of: $(KNOWN_CONFIGS)
	@echo or create $(HOME)/.pfkutils_config
	@echo containing one of those values.

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
	rm -rf obj.* dox
	make -C contrib clean

##############################################

else # $CONFIG

##############################################

PFKARCH := $(shell ./scripts/architecture)
OBJDIR := obj.$(PFKARCH).$(CONFIG)

ifeq ($(DISABLE_RDYNAMIC),)
LDFLAGS += -rdynamic # for backtrace
endif

INCLUDE_MAKEFILES= config/$(CONFIG) config/always

include Makefile.inc

##############################################

endif # $CONFIG

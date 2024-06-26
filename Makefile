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

NODENAME := $(shell scripts/architecture)

ifneq ($(wildcard $(HOME)/pfk/etc/pfkutils.config.$(NODENAME)),)

CONFIG_FILE := $(HOME)/pfk/etc/pfkutils.config.$(NODENAME)

else ifneq ($(wildcard $(HOME)/pfk/etc/pfkutils.config.$(CONFIG)),)

CONFIG_FILE := $(HOME)/pfk/etc/pfkutils.config.$(CONFIG)

else ifneq ($(wildcard $(HOME)/pfk/etc/pfkutils.config),)

CONFIG_FILE := $(HOME)/pfk/etc/pfkutils.config

endif

ifeq ($(CONFIG_FILE),)

##############################################

all:
	@echo please create $(HOME)/pfk/etc/pfkutils.config using
	@echo templates found in config/

clean:
	rm -rf obj.* dox
	make -C contrib clean

##############################################

else

##############################################

include $(CONFIG_FILE)

# NOTE OBJDIR can be overriden in the CONFIG_FILE.
# but also note no matter what it can be overridden
# on the command line too "make OBJDIR=/tmp/$USER/pfkutils-obj"
PFKARCH := $(shell ./scripts/architecture)
OBJDIR ?= obj.$(CONFIG)

ifeq ($(DISABLE_RDYNAMIC),)
LDFLAGS += -rdynamic # for backtrace
endif

INCLUDE_MAKEFILES= config/rules

include Makefile.inc

##############################################

endif # $CONFIG

bundle := pfkutils-$(shell date +%Y-%m%d-%H%M ).bundle

bundle:
	@echo making bundle
	git bundle create $(bundle) --all && \
	  git bundle verify $(bundle) > $(bundle).txt

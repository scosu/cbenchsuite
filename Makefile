
.PHONY: all directories clean libs menuconfig modules/Kconfig
.DEFAULT_GOAL := all



BUILDDIR := build
OBJDIR := $(BUILDDIR)/objs
OBJMODDIR := $(BUILDDIR)/mod-objs


BUILD_CONFIG := .config
include $(BUILD_CONFIG)

ifeq ($(CONFIG_BUILD_DEBUG),y)
CFLAGS_TYPE := -Wall -g
else
CFLAGS_TYPE := -O2
endif

CC := gcc
CFLAGS := $(CFLAGS_TYPE) $(patsubst "%",%,$(CONFIG_CFLAGS)) -Iinclude -Ilibs/klib/include -Igen/include -Ilibs -std=gnu99 -D_BSD_SOURCE -D_GNU_SOURCE
LDFLAGS :=
MODULE_CFLAGS := $(CFLAGS) -shared -fPIC -I.
MODULE_LDFLAGS :=
CORE_CFLAGS := $(CFLAGS)
CORE_LDFLAGS := $(LDFLAGS) -luuid -ldl -lpthread -lrt -lm



core_hdr := $(shell find include/ -type f -name '*.h')
libs := libs/sha256-gpl/sha256.o libs/klib/libklib.a

generate_config_h := ./scripts/gen_config.sh .config gen/include/cbench/config.h

include src/Makefile

objs-core := $(patsubst %,$(OBJDIR)/%,$(objs-core))
objs-storage := $(patsubst %,$(OBJDIR)/%,$(objs-storage))
objs := $(objs-core) $(objs-storage)

#
# Static module variables
#

MOBJDIR := $(OBJDIR)/modules
MODULE_BUILD_SOURCES := $(OBJMODDIR)/core/option.o $(OBJMODDIR)/core/util.o
MODULE_BUILD_DEPS := $(core_hdr) gen/include/cbench/config.h $(MODULE_BUILD_SOURCES)

module_dirs := $(wildcard modules/*)
module_makefiles := $(patsubst %,%/CBench.mk,$(module_dirs))


#
# Dynamic per module makefile variables, evaluated for every module
#

TOP = modules/
BDIR = $(MOBJDIR)/$(MODULE_NAME)
MODULE_TARGET_DIR = $(BUILDDIR)/modules
MODULE_TARGET = $(MODULE_TARGET_DIR)/$(MODULE_NAME)/module.so
MODULE_NAME_UPPER = $(shell echo $(MODULE_NAME) | tr a-z A-Z)



modules-y :=
-include $(module_makefiles)

module_tgts := $(patsubst %,$(BUILDDIR)/modules/%/module.so,$(modules-y))


all: directories $(module_tgts) $(BUILDDIR)/cbench

$(OBJMODDIR)/%.o: src/%.c $(core_hdr) gen/include/cbench/config.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

$(OBJDIR)/%.o: src/%.c $(core_hdr) gen/include/cbench/config.h
	$(CC) $(CORE_CFLAGS) -c -o $@ $<

$(BUILDDIR)/cbench: $(objs) $(core_hdr) $(libs)
	$(CC) $(CORE_CFLAGS) -o $@ $(objs) $(libs) $(CORE_LDFLAGS)

libs/klib/libklib.a:
	cd libs/klib/ && $(MAKE) $(MFLAGS)

gen/include/cbench/config.h: .config
	$(generate_config_h)

ifeq ($(wildcard .config),)
.config: menuconfig
endif

menuconfig: libs/kconfig-frontends/inst/bin/kconfig-mconf modules/Kconfig
	./libs/kconfig-frontends/inst/bin/kconfig-mconf Kconfig
	$(generate_config_h)

oldconfig: libs/kconfig-frontends/inst/bin/kconfig-mconf modules/Kconfig
	./libs/kconfig-frontends/inst/bin/kconfig-conf --oldconfig Kconfig
	$(generate_config_h)

libs/kconfig-frontends/inst/bin/kconfig-mconf:
	cd libs/kconfig-frontends/ && ./configure --prefix=`pwd`/inst && $(MAKE) $(MFLAGS) && $(MAKE) $(MFLAGS) install

libs/sha256-gpl/sha256.o: libs/sha256-gpl/sha256.c libs/sha256-gpl/sha256.h
	$(CC) $(CFLAGS) -c -o libs/sha256-gpl/sha256.o libs/sha256-gpl/sha256.c

modules/Kconfig:
	./scripts/gen_modules_kconfig.sh modules modules/Kconfig

directories:
	mkdir -p $(OBJDIR)/core $(OBJDIR)/storage $(OBJMODDIR)/core
	mkdir -p $(patsubst %,$(BUILDDIR)/modules/%,$(modules-y))
	mkdir -p gen/include/cbench/

clean:
	rm -rf build
	cd libs/klib/ && $(MAKE) $(MFLAGS) clean
	cd libs/kconfig-frontends/ && $(MAKE) $(MFLAGS) clean
	rm -rf libs/kconfig-frontends/inst/
	rm -rf gen
	rm -f $(libs)

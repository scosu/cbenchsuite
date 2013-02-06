
.PHONY: all directories clean libs menuconfig modules/Kconfig
.DEFAULT_GOAL := all

CC := gcc
CFLAGS := -Wall -Iinclude -Ilibs/klib/include -std=gnu99 -D_BSD_SOURCE -g


BUILDDIR := build
OBJDIR := $(BUILDDIR)/objs


BUILD_CONFIG := .config
include $(BUILD_CONFIG)


core_src := $(shell find src/ -type f -name '*.c')
core_hdr := $(shell find include/ -type f -name '*.h')
libs := libs/sha256-gpl/sha256.o libs/klib/libklib.a

#
# Static module variables
#

MOBJDIR := $(OBJDIR)/modules
MODULE_BUILD_DEPS := $(core_src) $(core_hdr) include/config.h
MODULE_BUILD_SOURCES :=
MODULE_CFLAGS := -shared -fPIC -I.

module_dirs := $(wildcard modules/*)
module_makefiles := $(patsubst %,%/CBench.mk,$(module_dirs))


#
# Dynamic per module makefile variables, evaluated for every module
#

TOP = $(dir $(lastword $(MAKEFILE_LIST)))
BDIR = $(MOBJDIR)/$(MODULE_NAME)
MODULE_TARGET = $(BUILDDIR)/modules/$(MODULE_NAME)/module.so
MODULE_NAME_UPPER = $(shell echo $(MODULE_NAME) | tr a-z A-Z)



modules-y :=
-include $(module_makefiles)

module_tgts := $(patsubst %,$(BUILDDIR)/modules/%/module.so,$(modules-y))


all: directories $(module_tgts) $(BUILDDIR)/cbench


$(BUILDDIR)/cbench: $(core_src) $(core_hdr) ${libs} include/config.h
	$(CC) $(CFLAGS) -o $@ $(core_src) ${libs} -ldl -lpthread -lrt

libs/klib/libklib.a:
	cd libs/klib/ && $(MAKE) $(MFLAGS)

include/config.h: .config
	./scripts/gen_config.sh .config include/config.h

ifeq ($(wildcard .config),)
.config: menuconfig
endif

menuconfig: libs/kconfig-frontends/inst/bin/kconfig-mconf modules/Kconfig
	./libs/kconfig-frontends/inst/bin/kconfig-mconf Kconfig

libs/kconfig-frontends/inst/bin/kconfig-mconf:
	cd libs/kconfig-frontends/ && ./configure --prefix=`pwd`/inst && $(MAKE) $(MFLAGS) && $(MAKE) $(MFLAGS) install

libs/sha256-gpl/sha256.o: libs/sha256-gpl/sha256.c libs/sha256-gpl/sha256.h
	$(CC) $(CFLAGS) -c -o libs/sha256-gpl/sha256.o libs/sha256-gpl/sha256.c

modules/Kconfig:
	./scripts/gen_modules_kconfig.sh modules modules/Kconfig

directories:
	mkdir -p $(patsubst %,$(OBJDIR)/modules/%,$(modules-y))
	mkdir -p $(patsubst %,$(BUILDDIR)/modules/%,$(modules-y))

clean:
	rm -rf build
	cd libs/klib/ && $(MAKE) $(MFLAGS) clean
	cd libs/kconfig-frontends/ && $(MAKE) $(MFLAGS) clean
	rm -rf libs/kconfig-frontends/inst/
	rm -f include/config.h

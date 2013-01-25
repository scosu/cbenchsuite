
.PHONY: all directories clean
.DEFAULT_GOAL := all

CC := gcc
CFLAGS := -Wall -Iinclude -std=c99


BUILDDIR := build
OBJDIR := $(BUILDDIR)/objs


BUILD_CONFIG := .config
include $(BUILD_CONFIG)


#
# Static module variables
#

MOBJDIR := $(OBJDIR)/modules
MODULE_BUILD_DEPS := 
MODULE_CFLAGS := -shared
MODULE_FILE_EXTENSION := .so

module_dirs := $(wildcard modules/*)
module_makefiles := $(patsubst %,%/CBench.mk,$(module_dirs))


#
# Dynamic per module makefile variables, evaluated for every module
#

TOP = $(dir $(lastword $(MAKEFILE_LIST)))
BDIR = $(MOBJDIR)/$(MODULE_NAME)
MODULE_TARGET = $(BUILDDIR)/$(MODULE_NAME)$(MODULE_FILE_EXTENSION)
MODULE_NAME_UPPER = $(shell echo $(MODULE_NAME) | tr a-z A-Z)



modules-y :=
-include $(module_makefiles)

module_tgts := $(patsubst %,$(BUILDDIR)/%.so,$(modules-y))


all: directories $(module_tgts)

directories:
	mkdir -p $(patsubst %,$(OBJDIR)/%,$(module_dirs))

clean:
	rm -rf build

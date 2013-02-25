

MODULE_NAME := kernel


modules-$(CONFIG_$(MODULE_NAME_UPPER)) += $(MODULE_NAME)

$(MODULE_TARGET): $(MODULE_BUILD_DEPS) $(TOP)/kernel/kernel.c $(TOP)/kernel/compile.c
	$(CC) $(CFLAGS) $(MODULE_CFLAGS) -o $@ $(TOP)/kernel/kernel.c $(MODULE_BUILD_SOURCES)


# If you need to change something in this makefile, please be sure to install
# all scripts and so on to $(MODULE_TARGET_DIR) and make their targets a dependency
# of the above defined one. Of course you can also change the above rule
# but make sure to keep a target $(MODULE_TARGET) and add the module name
# to 'modules-y' depending on some arbitrary config option
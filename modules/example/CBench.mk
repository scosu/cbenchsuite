
# You have to set this to the module/dir name of your module
MODULE_NAME := example

# This line requires you to have an appropriate config option
modules-$(CONFIG_$(MODULE_NAME_UPPER)) += $(MODULE_NAME)

# Do not use MODULE_NAME or any dynamicly generated variable because
# they are interpreted when needed and may have another value then.
# So do it as demonstrated here, with &(TOP)/example/example.c
$(MODULE_TARGET): $(MODULE_BUILD_DEPS) $(TOP)/example/example.c
	$(CC) $(CFLAGS) $(MODULE_CFLAGS) -o $@ $(TOP)/example/example.c $(MODULE_BUILD_SOURCES)


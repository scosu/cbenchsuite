
# You have to set this to the module/dir name of your module
MODULE_NAME := math

# This line requires you to have an appropriate config option
modules-$(CONFIG_$(MODULE_NAME_UPPER)) += $(MODULE_NAME)

# Do not use MODULE_NAME or any dynamicly generated variable because
# they are interpreted when needed and may have another value then.
# So do it as demonstrated here, with &(TOP)/example/example.c
$(MODULE_TARGET): $(MODULE_BUILD_DEPS) $(TOP)/math/math.c $(TOP)/math/bench_dc_sqrt.c $(TOP)/math/bench_dhry.c $(TOP)/math/bench_linpack.c $(TOP)/math/bench_whet.c
	$(CC) $(CFLAGS) $(MODULE_CFLAGS) -o $@ $(TOP)/math/math.c $(MODULE_BUILD_SOURCES)
	cp $(TOP)/math/sqrt.dc $(MODULE_TARGET_DIR)/math/sqrt.dc
	$(CC) -O2 -DTIME -o $(MODULE_TARGET_DIR)/math/dhry $(TOP)/math/dhry_1.c $(TOP)/math/dhry_2.c
	$(CC) -O2 -o $(MODULE_TARGET_DIR)/math/linpack $(TOP)/math/linpacknew.c
	$(CC) -O2 -o $(MODULE_TARGET_DIR)/math/whetstone $(TOP)/math/whetstone.c -lm

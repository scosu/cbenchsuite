set(KERNEL_MIRROR "ftp://ftp.kernel.org/pub/linux/kernel/v3.x/" CACHE STRING "Kernel download mirror")
add_definitions(-DCONFIG_KERNEL_MIRROR="${KERNEL_MIRROR}")

cbench_module(kernel kernel.c)


#include <cbench/version.h>

struct kernel_version {
	char *version;
	char *name;
	char *file_name;
	char *url;
};

#define KERNEL_VERSION(maj, min) KERNEL_##maj##_##min,
enum kernel_versions {
#include "versions.h"
};
#undef KERNEL_VERSION

#define KERNEL_VERSION(maj, min) \
	{ .version = #maj "." #min, .file_name = "linux-" #maj "." #min ".tar.gz",\
	  .name = "linux-" #maj "." #min,\
	  .url = CONFIG_KERNEL_MIRROR "/linux-" #maj "." #min ".tar.gz" },

static struct kernel_version kernels[] = {
#include "versions.h"
	{ /* Sentinel */ }
};
#undef KERNEL_VERSION

#define KERNEL_VERSION(maj, min) {\
	{ .name = "kernel", .version = #maj "." #min },\
	{ }\
},
__maybe_unused static struct comp_version kernel_comp_versions[][2] = {
#include "versions.h"
};
#undef KERNEL_VERSION

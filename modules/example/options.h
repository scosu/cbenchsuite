#ifndef _MODULES_EXAMPLE_OPTIONS_H_
#define _MODULES_EXAMPLE_OPTIONS_H_


struct example_options {
	char sample_str[128];
	int sample_int;
	int sample_bool;
};

struct example_options example_options_defaults = {
	.sample_str = "sample string",
	.sample_int = 42,
	.sample_bool = 1,
};

#endif  /* _MODULES_EXAMPLE_OPTIONS_H_ */

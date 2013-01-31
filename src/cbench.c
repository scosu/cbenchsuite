

#include <klib/printk.h>

#include <core/module_manager.h>

#include <plugin.h>

int main(void)
{
	struct mod_mgr mm;
	__attribute__((unused)) struct plugin *plug;
	const char *vers[] = {"!=1.1", NULL};
	printk_set_log_level(7);

	mod_mgr_init(&mm, "build/modules");

	plug = mod_mgr_plugin_create(&mm, "example.example-bench", vers);

	return 0;
}

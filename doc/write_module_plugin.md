Write your own module/plugin/benchsuite
=======================================

Every plugin or benchsuite has to be within a module. Each module can contain
an arbitrary number of plugins or benchsuites. Modules can be used in cbenchsuite
without changing any cbenchsuite source files. It will be autodetected by cbenchsuite
Makefiles.

In order to get your module registered, you need three files:

	your_module/your_module.c      Module c file
	your_module/CBench.mk          Makefile to compile your module
	your_module/Kconfig            Compiletime configurations for your module

The structure and content of these files is described in the [example
module](../modules/example/).

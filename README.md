
cbenchsuite - High accuracy C benchmark suite for Linux
=================================================

cbenchsuite offers high accuracy performance measurements. It is written in C to
minimize the impact of the suite itself on the running benchmarks. Cbench is
also highly configurable and extendable, offering extensive interfaces for
additional plugins or storage backends. By default cbenchsuite uses sqlite3 as
storage backend.

Features
--------

- Efficient benchmark framework
	- written in C
	- unloads unnecessary modules before benchmarking
- High accuracy
	- Efficient framework implementation
	- Setting scheduling priorities to get accurate monitor information
	- Warmup benchmark execution to remove measurements of cache heatup
	- Data is stored with the number of preceding benchmark executions to
	  be able to determine the cache hotness after the benchmarks
	- Repeat benchmarks until the standard error drops below a given
	  threshold
	- Plugins to reset Linux kernel caches/swap etc.
	- Store system information and plugin component versions to seperate
	  different benchmark environments
- Extendable
	- Extensive module/plugin/benchsuite interface that can be used without changing any core source files
	- Storage backend interface
- Configurable
	- Many configuration options like minimum/maximum runs/runtime per
	  benchmark
	- Execute combinations of plugins defined via command line arguments or
	  as a seperate benchsuite inside a module
	- Plugins can have options and different versions


Write your own module/plugin/benchsuite
---------------------------------------

Every plugin or benchsuite has to be within a module. Each module can contain
an arbitrary number of plugins or benchsuites. Modules can be used in cbenchsuite
without changing any cbenchsuite source files. It will be autodetected by cbenchsuite
Makefiles.

In order to get your module registered, you need three files:

	your_module/your_module.c      Module c file
	your_module/CBench.mk          Makefile to compile your module
	your_module/Kconfig            Compiletime configurations for your module

The structure and content of these files is described in the example
module module/example/.

Requirements
------------
- gcc
- libuuid
- sqlite3

Installation
------------

1. First clone cbenchsuite.

		git clone https://github.com/scosu/cbenchsuite.git

2. Configure cbenchsuite.

		make menuconfig

3. Compile cbenchsuite.

		make

4. You now can directly execute the benchsuite.

		./build/cbenchsuite


Help
----

	By default, cbenchsuite interpretes arguments without '-' at the beginning as
	benchsuite names and executes them.

	Commands:
		--list,-l		List available Modules/Plugins. All other
					command line arguments are parsed as module
					identifiers. If non is given, it lists all
					modules. This command will show more information
					with the verbose option.
		--plugins,-p		Parse all arguments as plugin identifiers
					and execute them. Please see
					doc/identifier_specifications for more
					information.
		--help,-h		Displays this help and exits.

	Options:
		--log-level,-g N 	Log level used, from 1 to 7(debugging)
		--storage,-s STR	Storage backend to use. Currently available are:
						sqlite3 and csv
		--db-path,-db DB	Database directory. Default: /tmp/cbenchsuite/
		--verbose,-v		Verbose output. (more information, but not the
					same as log-level)
		--module-dir,-m PATH	Module directory.
		--work_dir,-w PATH	Working directory. IMPORTANT! Depending on the
					location of this work directory, the benchmark
					results could vary. Default: /tmp/cbenchsuite/work
		--sysinfo,-i INFO	Additional system information. This will
					seperate the results of the executed benchmarks
					from others. For example you can use any code
					revision or simple names for different test
					conditions.
		--min-runs N		Minimum number of runs executed.
		--max-runs N		Maximum number of runs executed.
		--min-runtime SECONDS	Minimum runtime per independent result value.
		--max-runtime SECONDS	Maximum runtime per independent result value.
		--warmup-runs N		Number of warmup runs before the actual
					measurements.
		--stderr N		Percent of the standard error that need to be
					reached within the other runtime bounds. (float)

License
-------
	/*
	 * Cbench - A C benchmarking suite for Linux benchmarking.
	 * Copyright (C) 2013  Markus Pargmann
	 *
	 * This program is free software; you can redistribute it and/or
	 * modify it under the terms of the GNU General Public License
	 * as published by the Free Software Foundation; either version 2
	 * of the License, or (at your option) any later version.
	 *
	 * This program is distributed in the hope that it will be useful,
	 * but WITHOUT ANY WARRANTY; without even the implied warranty of
	 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	 * GNU General Public License for more details.
	 *
	 * You should have received a copy of the GNU General Public License
	 * along with this program; if not, write to the Free Software
	 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
	 */

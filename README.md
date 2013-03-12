
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
- matplotlib for python >= 3 (for plotter)
- python >= 3 (for plotter)

Installation
------------

1. First clone cbenchsuite.

		git clone https://github.com/scosu/cbenchsuite.git
		cd cbenchsuite

2. Configure cbenchsuite.

		make menuconfig

	![menuconfig screenshot](http://allfex.org/files/cbenchsuite/0.1/menuconfig.png)

	Here you can configure some options of cbenchsuite. However,
	most settings only define the default values, so it is not
	important to configure anything here.  Simply choose
	exit and save the configuration file.

3. Compile cbenchsuite.

		make

	There is no install target at the moment avaiable. After the build
	everything is located where it is needed and you can directly start
	using it.

4. Now the benchsuite is ready to use.

		./cbenchsuite

Usage
-----

1. Let's have a look at all the plugins.

		./cbenchsuite -l

	This will give you a list of available modules and plugins:

		kernel
		    compile              Linux kernel compile benchmark. It measures how fast the kernel can be compiled

		compression
		    7zip-bench           p7zip's integrated benchmark. The performance is measured in compression/decompression-speed.

		cpusched
		    yield-bench          This benchmark stresses the cpu kernel scheduler by repeating sched_yield calls for a given time.

		linux_perf
		    hackbench            Benchmark that spawns a number of groups that internally send/receive packets. Also known within the linux kernel perf tool as sched-messaging.
		    sched-pipe           Benchmarks the pipe performance by spawning two processes and sending data between them.
		...

	If you want more information about some modules, you can simply append
	them to the commandline.

		./cbenchsuite -l -v kernel

	`-v` increases the verbosity of the list command. The command above will
	give you verbose information about the kernel module:

		kernel
		    compile              Linux kernel compile benchmark. It measures how fast the kernel can be compiled
		      Version
			1.0
			  Components: kernel-3.8
			  Options
			    threads (Default: 16)

	As you can see, cbenchsuite keeps track of different versions and supports
	options for plugins. To get even more information about a module, you
	can append another `-v`.

		kernel
		  shared object 'build/modules/kernel/module.so'
		    compile              Linux kernel compile benchmark. It measures how fast the kernel can be compiled
		      Versions
			1.0
			  Components: kernel-3.8
			  Independent values: 1
			  Options
			    threads (Default: 16)
			1.0
			  Components: kernel-3.7
			  Independent values: 1
			  Options
			    threads (Default: 16)

	This is obviously even more verbose. It will show you all available
	versions of plugins and much more detailed information. Of course, the
	different verbosity levels can also be used without specifying a special
	module.

2. Execute

	The easiest way to execute some benchmarks and get results is to use a
	benchsuite. Benchsuites are also listed with the `-l` command. In the
	default mode, cbenchsuite will try to interprete all commandline arguments
	as benchsuite identifiers.

		./cbenchsuite kernel.example-benchsuite

	This will for example execute a benchsuite called `example-benchsuite`
	defined in the `kernel` module. (Just an example, that benchsuite
	does not exist)

	Another possibility is the execution of single plugins:

		./cbenchsuite -p cpusched.yield-bench

	This executes plugin `yield-bench` in module `cpusched`. It will
	automatically repeatedly execute that benchmark and store the results
	in a sqlite3 database at `~/.cbenchsuite/db/db.sqlite`.

		INFO     : Running plugins: cpusched.yield-bench
		INFO     :      Runtime without warmup between 00h05 and 00h30
		INFO     :      Installing plugins
		INFO     :      Warmup run 1
		INFO     :      Warmup run 2
		INFO     :      Execution run 1 uuid:'fc7a9525-6188-4321-933c-7ae6909c193a'
		INFO     :              Minimum runtime not reached, continuing
		INFO     :      Execution run 2 uuid:'671e8f34-7e47-4d26-9611-ed4f2cb2803b'
		INFO     :              Minimum runtime not reached, continuing

		...

		INFO     :      Execution run 10 uuid:'bb0b6f5f-5e7c-48f5-81d5-76814a998aae'
		INFO     :              Necessary standard error reached, stopping
		INFO     :      Uninstalling plugins

	You can see that there are some warmup-runs. They are used to minimize
	the probability to measure with cold caches.

	By defining multiple commandline arguments, the plugins are benchmarked
	one after another, not in parallel.

		./cbenchsuite cpusched.yield-bench kernel.compile

	Executes first yield-bench and after that kernel.compile.

	To execute plugins in parallel, you have to seperate the plugins with
	`;`. Don't forget to add quoutes around the whole argument then.

		./cbenchsuite "cpusched.yield-bench;cooldown.sleep"

	This will execute yield-bench and sleep in parallel. Be aware that you
	should not execute multiple benchmark plugins in parallel. Executing
	a benchmark in parallel with a helper or some other plugin is no problem.

	There is a number of function slots, that each plugin may implement. The
	main function slot for benchmarks is `run`, all other function slots are
	used for other stuff. Most helper plugins will offer you a set of options
	where you can define in which function slot the plugin should be executed.

	In our example we can execute the `cooldown.sleep` plugin in the `init`
	slot. This is basically done by setting an option for that plugin. Options
	are seperated by `:` from the plugin identifier.

		./cbenchsuite "cpusched.yield-bench;cooldown.sleep:init=20"

	Before each execution of the yield benchmark, the `cooldown.sleep` will
	sleep for 20 seconds in the init slot with the result to give the system
	some time to make delayed actions or similar. `init=20` will set the option
	`init` to `20`. You may define multiple options:

		./cbenchsuite "cpusched.yield-bench;cooldown.sleep:init=20:exit=10"

	Also you can define versions of the plugin you want to use:

		./cbenchsuite "kernel.compile@kernel>=3.8;cooldown.sleep@==1.0:init=20:exit=10"

	This will choose a `kernel.compile` version with a higher or equal to 3.8 kernel version
	and the `cooldown.sleep` plugin at version 1.0. See doc/identifier_specification
	a complete identifier grammar.

	cbenchsuite stores all information about an execution in the database, including
	system information. However, the saved information are probably not enough
	to characterise the execution, so there is a option `--sysinfo,-i` that you
	should use to note what special system setup or testscenario you are using.
	Do not describe the combination of plugins, versions or options here because
	cbenchsuite stores those information. Instead use it to for software revisions,
	kernel revisions or similar, e.g.:

		./cbenchsuite -p -i "branch scheduler_test" kernel.compile


3. Plot

	19:05
	The following command will produce results used to describe the creation
	of charts in this section.

		for pipe in 0 1; do for process in 0 1; do for groups in 5 10 15 20; do ./cbenchsuite --warmup-runs 0 --max-runs 10 --min-runtime 30 -p linux_perf.hackbench:pipe=${pipe}:process=${process}:groups=${groups}; done; done; done

	Now that we have a big set of results, lets start the plotter command
	line interface.

		./plotter.py

	This command line interface has two modes.
	- `data` In this mode you can select which data you want to plot by
		adding filters.
	- `plots` The data is grouped into plots and you can change the final
		figures.

	To switch between those modes, you can use `plots_create` and `plots_reset`.
	But be careful with `plots_reset` because it discards all the changes you made.

	This guide will only show a simple way from results to the final figures.
	If you want to do something else, all commands are documented, simply
	execute `help COMMAND` or `help`.

	Let us assume you executed the above command on multiple systems or with
	different kernels. Then you probably want to select which systems to use.

		data  > select system
		    1: CPUs:  8 Mem: 16785Mb Kernel: 3.6.8 Arch: x86_64 Info:  nr runs: 108
		Select systems:

	This command shows all available systems. You can select multiple systems.
	`nr_runs` gives you some information about the number of datasets that
	remain if you select only that system. `Info: ` contains the custom information
	you passed to cbenchsuite with `--sysinfo,-i`. After selecting a system,
	an appropriate filter is created.

		data  > filters
		   1: system selection (system: system.system_sha = '828c8c38f436ced0ba5b58bd2e29e8f215f35b4eb3b512f29ed1409b434f66bd')

	After creating a system filter, all future select calls, will already
	apply this filter. To remove this filter you could type

		data  > filter_rm 1

	Then you can select a system again. You can also add a custom filter
	with `filter_add`. But that requires some knowledge about the database
	structure. It is much easier to only use select commands.

	When you are done with filtering, you can switch to `plots` mode.

		data  > plots_create
		Group 1
		1.1  linux_perf.hackbench
		    Chart type: bar
		    Properties:
		      plot-depth: 3
		      title: linux_perf.hackbench
		    Number of different configurations: 16
		    Parameter graph: (Everything right of '||' will be in the figure)
		      - data
			 \ runtime
			  `- Groups(option.groups)
			     |    Properties: ylabel=Runtime (s)
			    |\ 15
			    | `- Loops(option.loops)
			    |    \ 10000
			    |     `--||- Pipe(option.pipe)
			    |        || |\ 1
			    |        || | `- Process(option.process)
			    |        || |   |\ 1
			    |        || |   | `- system
			    |        || |   |     `-  8 CPUs Kernel 3.6.8(828c8c38f436ced0ba5b58bd2e29e8f215f35b4eb3b512f29ed1409b434f66bd)
			    |        || |    \ 0
			    |        || |     `- system
			    |        || |         `-  8 CPUs Kernel 3.6.8(828c8c38f436ced0ba5b58bd2e29e8f215f35b4eb3b512f29ed1409b434f66bd)
			    |        ||  \ 0
			    |        ||   `- Process(option.process)
			    |        ||     |\ 0
			    |        ||     | `- system
			    |        ||     |     `-  8 CPUs Kernel 3.6.8(828c8c38f436ced0ba5b58bd2e29e8f215f35b4eb3b512f29ed1409b434f66bd)
			    |        ||      \ 1
			    |        ||       `- system
			    |        ||           `-  8 CPUs Kernel 3.6.8(828c8c38f436ced0ba5b58bd2e29e8f215f35b4eb3b512f29ed1409b434f66bd)

		...

	As you can see `plots` will show you a huge list and trees of plots that
	were created. Beginning at the top of the output, there is a `Group 1`.
	One group in this representation is equivalent to an parallel execution
	of plugins. `1.1` denotes the exact number of this plot. Then there are
	some information about the plot itself:
		- Chart type: May be `bar` or `line`
		- Properties: Properties for all figures created out of this plot.
			It may be overwritten by properties attached to nodes in
			the tree. Properties can be title, xlabel, etc.

	After that the parameter graph follows. It shows you the different
	parameters of the raw data that were detected. All nodes that are right
	of the `||` line, will end up in one plot. Everything left of it will be
	represented as directory hierarchy. If there is no such line, then everyhing
	will be in one figure.

	Now as you may noticed, there are some parameters that have only one possible value,
	for example the system parameter. It is really easy to remove all those
	parameters from the tree:

		plots > plot_autoremove *
		plots > plots
		Group 1
		1.1  linux_perf.hackbench
		    Chart type: bar
		    Properties:
		      plot-depth: 3
		      title: linux_perf.hackbench
		    Number of different configurations: 16
		    Same run parameters:
		      data = runtime
		      Loops(option.loops) = 10000
		      system =  8 CPUs Kernel 3.6.8(828c8c38f436ced0ba5b58bd2e29e8f215f35b4eb3b512f29ed1409b434f66bd)
		    Parameter graph: (Everything right of '||' will be in the figure)
		      - Groups(option.groups)
			 |    Properties: ylabel=Runtime (s)
			|\ 15
			| `- Pipe(option.pipe)
			|   |\ 1
			|   | `- Process(option.process)
			|   |    |`- 1
			|   |     `- 0
			|    \ 0
			|     `- Process(option.process)
			|        |`- 0
			|         `- 1
			|\ 5
			| `- Pipe(option.pipe)
			|   |\ 0
			|   | `- Process(option.process)
			|   |    |`- 0
			|   |     `- 1
			|    \ 1
			|     `- Process(option.process)
			|        |`- 0
			|         `- 1
		...

	As you can see, the tree is much smaller now. All parameters with one value
	are listed in "Same run parameters". As there is no `||` line, you know
	that the plotter can get all the data into a single figure. If that
	line is still visible or you want to change what is displayed, there are
	some other commands that help you to alter the tree, e.g. `plot_autosquash` and `plot_squash`.

	You can set properties for plotting, like title, xlabel, fontsizes, etc.
	with `plot_set_property`.

	Finally to get a nice figure out of this plot, simply execute:

		plots > plot_generate *
		Generated /tmp/cbenchsuite/linux_perf.hackbench/linux_perf.hackbench

	This command will generate all figures for all plots in parallel. As you
	can see the figure was generated and stored at the location shown. This
	is what the plotter created:

	![Example figure generated by cbenchsuite](http://allfex.org/files/cbenchsuite/0.1/example_plot.svg)

FAQ
---

1. Why are there `NOTICE: ... failed to set priority ...` messages?

	You are probably running cbenchsuite not as root. cbenchsuite is designed
	to create results that are very accurate, so it tries to set the priority
	for some threads. Execute the cbenchsuite as root to get rid of those
	messages. Also be aware that some plugins can not be executed as normal
	user. If you are not able to start cbenchsuite as root, you can ignore
	those messages without a problem.

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

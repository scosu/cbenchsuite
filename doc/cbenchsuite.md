
cbenchsuite executable usage
============================

Modules and Plugins
-------------------

A module contains one or more plugins that have something in common. A module
is build to a single shared library that is loaded by the cbenchsuite
executable.

A plugin is a set of functions to fullfill an arbitrary purpose. There are many
different types you could want to use, e.g. benchmarks, background load,
system monitors, cleanup plugins etc. The type of plugin is described in its
description.

- **List all modules**

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

- **List selected modules**

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

Benchsuites
-----------

A benchsuite is a execution description of plugins. It does not contain
benchsuites. A benchsuite should be a collection of plugins to stress some
parts of your system or the system as a whole. Of course this is not limited to
this. Benchsuites are also listed by the `cbenchsuite -l` command as described
above.

If you do not use a predefined benchsuite for execution, a benchsuite is
created by defining plugins to execute as commandline arguments. cbenchsuite
does not save the name or anything else of the executed benchsuite because the
combination of plugins is stored and much more important. So you can select
some plugin groups to execute manually and finally compare them to results
created by an execution of a benchsuite.

Execution
---------

- **Execute a benchsuite**

	The easiest way to execute some benchmarks and get results is to use a
	benchsuite. Benchsuites are also listed with the `-l` command. In the
	default mode, cbenchsuite will try to interprete all commandline arguments
	as benchsuite identifiers.

		./cbenchsuite kernel.example-benchsuite

	This will for example execute a benchsuite called `example-benchsuite`
	defined in the `kernel` module. (Just an example, that benchsuite
	does not exist)

- **Execute a plugin**

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

- **Execute multiple plugins**

	By defining multiple commandline arguments, the plugins are benchmarked
	one after another, not in parallel.

		./cbenchsuite -p cpusched.yield-bench kernel.compile

	Executes first yield-bench and after that kernel.compile.

- **Execute plugin groups**

	To execute plugins in parallel, you have to seperate the plugins with
	`;`. Don't forget to add quoutes around the whole argument then.

		./cbenchsuite -p "cpusched.yield-bench;cooldown.sleep"

	This will execute yield-bench and sleep in parallel. Be aware that you
	should not execute multiple benchmark plugins in parallel. Executing
	a benchmark in parallel with a helper or some other plugin is no problem.

- **Execute with defined options**

	There is a number of function slots, that each plugin may implement. The
	main function slot for benchmarks is `run`, all other function slots are
	used for other stuff. Most helper plugins will offer you a set of options
	where you can define in which function slot the plugin should be executed.

	In our example we can execute the `cooldown.sleep` plugin in the `init`
	slot. This is basically done by setting an option for that plugin. Options
	are seperated by `:` from the plugin identifier.

		./cbenchsuite -p "cpusched.yield-bench;cooldown.sleep:init=20"

	Before each execution of the yield benchmark, the `cooldown.sleep` will
	sleep for 20 seconds in the init slot with the result to give the system
	some time to make delayed actions or similar. `init=20` will set the option
	`init` to `20`. You may define multiple options:

		./cbenchsuite -p "cpusched.yield-bench;cooldown.sleep:init=20:exit=10"

- **Execute specific versions**

	Also you can define versions of the plugin you want to use:

		./cbenchsuite -p "kernel.compile@kernel>=3.8;cooldown.sleep@==1.0:init=20:exit=10"

	This will choose a `kernel.compile` version with a higher or equal to 3.8 kernel version
	and the `cooldown.sleep` plugin at version 1.0. See doc/identifier_specification
	a complete identifier grammar.

- **Execution system information**

	cbenchsuite stores all information about an execution in the database, including
	system information. However, the saved information are probably not enough
	to characterise the execution, so there is a option `--sysinfo,-i` that you
	should use to note what special system setup or testscenario you are using.
	Do not describe the combination of plugins, versions or options here because
	cbenchsuite stores those information. Instead use it to for software revisions,
	kernel revisions or similar, e.g.:

		./cbenchsuite -p -i "branch scheduler_test" kernel.compile


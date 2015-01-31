
cbenchsuite - High accuracy C benchmark suite for Linux
=================================================

cbenchsuite offers high accuracy performance measurements. It is written in C to
minimize the impact of the suite itself on the running benchmarks. Cbench is
also highly configurable and extendable, offering extensive interfaces for
additional plugins or storage backends. By default cbenchsuite uses sqlite3 as
storage backend.

Features
--------

- Complete framework
	- Benchmarking systems
	- Storage in sqlite3 databases
	- Automatically generate figures and websites for the results
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


Example figures
---------------

- **Results as stacked bar chart**
	![Example stacked bar chart with 3 levels of data for hackbench](http://allfex.org/files/cbenchsuite/0.1/results/20130403_kernel_comparison/cooldown.sleep__cpusched.latency-monitor__linux_perf.hackbench__sysctl.drop-caches__sysctl.monitor-meminfo__sysctl.monitor-schedstat__sysctl.monitor-stat__sysctl.swap-reset/linux_perf.hackbench.svg)

- **Results as simple bar chart**
	![Example simple bar chart with 2 levels of data for kernel build](http://allfex.org/files/cbenchsuite/0.1/results/20130403_kernel_comparison/cooldown.sleep__cpusched.latency-monitor__kernel.compile__sysctl.drop-caches__sysctl.monitor-meminfo__sysctl.monitor-schedstat__sysctl.monitor-stat__sysctl.swap-reset/kernel.compile.svg)

- **Monitor data in line charts**
	![Example monitor line chart with 1 level of data](http://allfex.org/files/cbenchsuite/0.1/results/20130403_kernel_comparison/cooldown.sleep__cpusched.latency-monitor__kernel.compile__sysctl.drop-caches__sysctl.monitor-meminfo__sysctl.monitor-schedstat__sysctl.monitor-stat__sysctl.swap-reset/sysctl.monitor-meminfo/data__Cached/kernel.compile_Threads__4.svg)
	![Example monitor line chart with 1 level of data](http://allfex.org/files/cbenchsuite/0.1/results/20130403_kernel_comparison/cooldown.sleep__cpusched.latency-monitor__kernel.compile__sysctl.drop-caches__sysctl.monitor-meminfo__sysctl.monitor-schedstat__sysctl.monitor-stat__sysctl.swap-reset/sysctl.monitor-meminfo/data__Committed_AS/kernel.compile_Threads__2.svg)
	![Example monitor line chart with 1 level of data](http://allfex.org/files/cbenchsuite/0.1/results/20130403_kernel_comparison/cooldown.sleep__cpusched.latency-monitor__kernel.compile__sysctl.drop-caches__sysctl.monitor-meminfo__sysctl.monitor-schedstat__sysctl.monitor-stat__sysctl.swap-reset/sysctl.monitor-schedstat/data__schedule_calls/kernel.compile_Threads__2.svg)

Example website
---------------

- **[Example hackbench website](http://allfex.org/files/cbenchsuite/0.1/results/20130403_kernel_comparison/cooldown.sleep__cpusched.latency-monitor__linux_perf.hackbench__sysctl.drop-caches__sysctl.monitor-meminfo__sysctl.monitor-schedstat__sysctl.monitor-stat__sysctl.swap-reset/)**
- **[Example kernel compile website](http://allfex.org/files/cbenchsuite/0.1/results/20130403_kernel_comparison/cooldown.sleep__cpusched.latency-monitor__kernel.compile__sysctl.drop-caches__sysctl.monitor-meminfo__sysctl.monitor-schedstat__sysctl.monitor-stat__sysctl.swap-reset/)**
- [Some other websites generated](http://allfex.org/files/cbenchsuite/0.1/results/20130403_kernel_comparison/)

Installation
------------

1. First clone cbenchsuite.

		git clone https://github.com/scosu/cbenchsuite.git
		cd cbenchsuite

2. Configure

		mkdir build
		cd build
		cmake ..

3. Compile cbenchsuite.

		make
4. Install

		make install

	This will install cbenchsuite to <PREFIX>/bin/cbenchsuite and the
	benchmark modules to <PREFIX>/lib/cbenchsuite_modules.

4. Now the benchsuite is ready to use.

		./cbenchsuite

Requirements
------------
- gcc (>= 4.6, I tested 4.5.3 and it did not compile due to union problems)
- libuuid
- sqlite3
- ncurses
- gperf
- matplotlib for python >= 3 (for plotter)
- python >= 3 (for plotter)

Documentation
-------------

- **[cbenchsuite usage](doc/cbenchsuite.md)**
- **[Plotter usage](doc/plotter.md)**
- [Identifier specification](doc/identifier_specifications.md)
- [Write a module/plugin](doc/write_module_plugin.md)
- [Database](doc/database.md)

FAQ
---

1. **Why are there `NOTICE: ... failed to set priority ...` messages?**

	You are probably running cbenchsuite not as root. cbenchsuite is designed
	to create results that are very accurate, so it tries to set the priority
	for some threads. Execute the cbenchsuite as root to get rid of those
	messages. Also be aware that some plugins can not be executed as normal
	user. If you are not able to start cbenchsuite as root, you can ignore
	those messages without a problem.

Acknowledgements
----------------

- [kconfig-frontends](http://ymorin.is-a-geek.org/projects/kconfig-frontends) (Build configuration)
- [Linux](http://kernel.org) (Some datastructures like list etc.)
- [Matplotlib](http://matplotlib.org/)
- [sha256 C implementation](https://github.com/lentinj/u-boot/commit/b571afde0295b007a45055ee49f8822c753a5651)
- [statistics.py](https://github.com/nitsanw/javanetperf/blob/master/statistics.py)

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

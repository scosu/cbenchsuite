
Plot results
============

cbenchsuite contains a tool to plot results from a sqlite database gathered by
the cbenchsuite executable. This tool is written in python3 and requires
matplotlib. Beside images, it is able to produce a website containing all
important information, like options, versions and a legend.

The plotter has a command line interface to control the plot process.

Automagic plotting
------------------

To create plots really fast and without much commands, just start the plotter
with a database:

	./plotter.py -d db.sqlite

There is a simple 'automagic' command you can use to produce a basic set of
plots and html websites.

	This is a plotter CLI. Use help for more information
	data  > automagic

Done.

This is most likely not the best representation of your data, so if you want to
plot differently, please go through the rest of this document.

Use the plotter
---------------

- **Produce some sample data**
	The following command will produce results used to describe the creation
	of charts in this section.

		for pipe in 0 1; do for process in 0 1; do for groups in 5 10 15 20; do ./cbenchsuite --warmup-runs 0 --max-runs 10 --min-runtime 30 -p linux_perf.hackbench:pipe=${pipe}:process=${process}:groups=${groups}; done; done; done

- **Start the plotter**

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

- **Filter datasets**

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

- **Create plots**

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

- **Generate figures**

	Finally to get a nice figure out of this plot, simply execute:

		plots > plot_generate *
		Generated /tmp/cbenchsuite/linux_perf.hackbench/linux_perf.hackbench

	This command will generate all figures for all plots in parallel. As you
	can see the figure was generated and stored at the location shown. This
	is what the plotter created:

	![Example stacked bar chart with 3 levels of data for hackbench](http://allfex.org/files/cbenchsuite/0.1/results/20130403_kernel_comparison/cooldown.sleep__cpusched.latency-monitor__linux_perf.hackbench__sysctl.drop-caches__sysctl.monitor-meminfo__sysctl.monitor-schedstat__sysctl.monitor-stat__sysctl.swap-reset/linux_perf.hackbench.svg)

	As you can see, bar charts can automatically plot up to 3 hierarchy levels
	of data. You can customize the levels via the properties.

	With large datasets, this command may take a lot of time.

- **Generate website**

	To get everything summarized in a nice website, just execute the
	following command and by default, the html will be generated in the
	same tree as the figures. This does not generate the figures itself, so
	if you select different plots as in the `plot_generate` command, you
	will have websites with missing images. Each directory created, will
	contain a index.html. The root index.html file may get very big because
	of the big amount of data stored, so it is sometimes better to link to
	index files in subdirectories.

		plots > html_generate *

	The website makes some use of javascript to get a better overview over
	the results and reduce the svg rendering and traffic impact of the
	figures.

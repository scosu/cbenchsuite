
#include <cbench/benchsuite.h>

#define SUITE_SCHED_UTILS \
	{ .name = "cpusched.latency-monitor", },\
	{ .name = "sysctl.monitor-stat", },\
	{ .name = "sysctl.monitor-meminfo", },\
	{ .name = "sysctl.monitor-schedstat", },\
	{ .name = "sysctl.swap-reset", .options = "init_pre=1", },\
	{ .name = "sysctl.drop-caches", .options = "init=1", },\
	{ .name = "cooldown.sleep", .options = "init_post=20", }\

#define SUITE_SCHED_BENCHMARK(benchname, benchoptions) \
	{ { .name = benchname, .options = benchoptions } , SUITE_SCHED_UTILS, { }}

#define SUITE_SCHED_SIZE 9

static struct plugin_link suite_sched_kernel_grps[][SUITE_SCHED_SIZE] = {
	SUITE_SCHED_BENCHMARK("kernel.compile", "threads=1"),
	SUITE_SCHED_BENCHMARK("kernel.compile", "threads=2"),
	SUITE_SCHED_BENCHMARK("kernel.compile", "threads=4"),
	SUITE_SCHED_BENCHMARK("kernel.compile", "threads=8"),
	SUITE_SCHED_BENCHMARK("kernel.compile", "threads=16"),
	SUITE_SCHED_BENCHMARK("kernel.compile", "threads=24"),
	SUITE_SCHED_BENCHMARK("kernel.compile", "threads=32"),
	SUITE_SCHED_BENCHMARK("kernel.compile", "threads=48"),
	SUITE_SCHED_BENCHMARK("kernel.compile", "threads=64"),
};

static struct plugin_link suite_sched_hackbench_grps[][SUITE_SCHED_SIZE] = {
	SUITE_SCHED_BENCHMARK("linux_perf.hackbench", "groups=10:pipe=0:process=1"),
	SUITE_SCHED_BENCHMARK("linux_perf.hackbench", "groups=10:pipe=1:process=1"),
	SUITE_SCHED_BENCHMARK("linux_perf.hackbench", "groups=30:pipe=0:process=1"),
	SUITE_SCHED_BENCHMARK("linux_perf.hackbench", "groups=30:pipe=1:process=1"),
	SUITE_SCHED_BENCHMARK("linux_perf.hackbench", "groups=50:pipe=0:process=1"),
	SUITE_SCHED_BENCHMARK("linux_perf.hackbench", "groups=50:pipe=1:process=1"),
	SUITE_SCHED_BENCHMARK("linux_perf.hackbench", "groups=70:pipe=0:process=1"),
	SUITE_SCHED_BENCHMARK("linux_perf.hackbench", "groups=70:pipe=1:process=1"),
	SUITE_SCHED_BENCHMARK("linux_perf.hackbench", "groups=90:pipe=0:process=1"),
	SUITE_SCHED_BENCHMARK("linux_perf.hackbench", "groups=90:pipe=1:process=1"),
	SUITE_SCHED_BENCHMARK("linux_perf.hackbench", "groups=110:pipe=0:process=1"),
	SUITE_SCHED_BENCHMARK("linux_perf.hackbench", "groups=110:pipe=1:process=1"),
	SUITE_SCHED_BENCHMARK("linux_perf.hackbench", "groups=130:pipe=0:process=1"),
	SUITE_SCHED_BENCHMARK("linux_perf.hackbench", "groups=130:pipe=1:process=1"),
};

static struct plugin_link suite_sched_pipe_grps[][SUITE_SCHED_SIZE] = {
	SUITE_SCHED_BENCHMARK("linux_perf.sched-pipe", NULL),
};

static struct plugin_link suite_sched_fork_grps[][SUITE_SCHED_SIZE] = {
	SUITE_SCHED_BENCHMARK("cpusched.fork-bench", "threads=1"),
	SUITE_SCHED_BENCHMARK("cpusched.fork-bench", "threads=2"),
	SUITE_SCHED_BENCHMARK("cpusched.fork-bench", "threads=4"),
	SUITE_SCHED_BENCHMARK("cpusched.fork-bench", "threads=8"),
	SUITE_SCHED_BENCHMARK("cpusched.fork-bench", "threads=16"),
	SUITE_SCHED_BENCHMARK("cpusched.fork-bench", "threads=24"),
	SUITE_SCHED_BENCHMARK("cpusched.fork-bench", "threads=32"),
	SUITE_SCHED_BENCHMARK("cpusched.fork-bench", "threads=48"),
	SUITE_SCHED_BENCHMARK("cpusched.fork-bench", "threads=64"),
};

static struct plugin_link suite_sched_7zip[][SUITE_SCHED_SIZE] = {
	SUITE_SCHED_BENCHMARK("compression.7zip-bench", "threads=1"),
	SUITE_SCHED_BENCHMARK("compression.7zip-bench", "threads=2"),
	SUITE_SCHED_BENCHMARK("compression.7zip-bench", "threads=4"),
	SUITE_SCHED_BENCHMARK("compression.7zip-bench", "threads=8"),
	SUITE_SCHED_BENCHMARK("compression.7zip-bench", "threads=16"),
	SUITE_SCHED_BENCHMARK("compression.7zip-bench", "threads=24"),
	SUITE_SCHED_BENCHMARK("compression.7zip-bench", "threads=32"),
	SUITE_SCHED_BENCHMARK("compression.7zip-bench", "threads=48"),
	SUITE_SCHED_BENCHMARK("compression.7zip-bench", "threads=64"),
};

static struct plugin_link suite_sched_math[][SUITE_SCHED_SIZE] = {
	SUITE_SCHED_BENCHMARK("math.dc-sqrt", NULL),
	SUITE_SCHED_BENCHMARK("math.dhrystone", NULL),
	SUITE_SCHED_BENCHMARK("math.whetstone", NULL),
	SUITE_SCHED_BENCHMARK("math.linpack", NULL),
};

static struct plugin_link *suite_sched_groups[] = {
	suite_sched_kernel_grps[0],
	suite_sched_kernel_grps[1],
	suite_sched_kernel_grps[2],
	suite_sched_kernel_grps[3],
	suite_sched_kernel_grps[4],
	suite_sched_hackbench_grps[0],
	suite_sched_hackbench_grps[1],
	suite_sched_hackbench_grps[2],
	suite_sched_hackbench_grps[3],
	suite_sched_hackbench_grps[4],
	suite_sched_hackbench_grps[5],
	suite_sched_hackbench_grps[6],
	suite_sched_hackbench_grps[7],
	suite_sched_hackbench_grps[8],
	suite_sched_hackbench_grps[9],
	suite_sched_pipe_grps[0],
	suite_sched_fork_grps[0],
	suite_sched_fork_grps[1],
	suite_sched_fork_grps[2],
	suite_sched_fork_grps[3],
	suite_sched_fork_grps[4],
	suite_sched_7zip[0],
	suite_sched_7zip[1],
	suite_sched_7zip[2],
	suite_sched_7zip[3],
	suite_sched_7zip[4],
	suite_sched_math[0],
	suite_sched_math[1],
	suite_sched_math[2],
	suite_sched_math[3],
	NULL
};

static struct plugin_link *suite_sched_medium_groups[] = {
	suite_sched_kernel_grps[0],
	suite_sched_kernel_grps[1],
	suite_sched_kernel_grps[2],
	suite_sched_kernel_grps[3],
	suite_sched_kernel_grps[4],
	suite_sched_kernel_grps[5],
	suite_sched_kernel_grps[6],
	suite_sched_kernel_grps[7],
	suite_sched_kernel_grps[8],
	suite_sched_hackbench_grps[0],
	suite_sched_hackbench_grps[1],
	suite_sched_hackbench_grps[2],
	suite_sched_hackbench_grps[3],
	suite_sched_hackbench_grps[4],
	suite_sched_hackbench_grps[5],
	suite_sched_hackbench_grps[6],
	suite_sched_hackbench_grps[7],
	suite_sched_hackbench_grps[8],
	suite_sched_hackbench_grps[9],
	suite_sched_hackbench_grps[10],
	suite_sched_hackbench_grps[11],
	suite_sched_hackbench_grps[12],
	suite_sched_hackbench_grps[13],
	suite_sched_hackbench_grps[14],
	suite_sched_hackbench_grps[15],
	suite_sched_pipe_grps[0],
	suite_sched_fork_grps[0],
	suite_sched_fork_grps[1],
	suite_sched_fork_grps[2],
	suite_sched_fork_grps[3],
	suite_sched_fork_grps[4],
	suite_sched_fork_grps[5],
	suite_sched_fork_grps[6],
	suite_sched_fork_grps[7],
	suite_sched_fork_grps[8],
	suite_sched_7zip[0],
	suite_sched_7zip[1],
	suite_sched_7zip[2],
	suite_sched_7zip[3],
	suite_sched_7zip[4],
	suite_sched_7zip[5],
	suite_sched_7zip[6],
	suite_sched_7zip[7],
	suite_sched_7zip[8],
	suite_sched_math[0],
	suite_sched_math[1],
	suite_sched_math[2],
	suite_sched_math[3],
	NULL
};

const struct benchsuite_id suite_normal = {
	.name = "suite-normal",
	.plugin_grps = suite_sched_groups,
	.version = { .version = "0.2", },
	.description = "A collection of scheduler benchmarks, while monitoring the scheduler behavior for normal computers",
};

const struct benchsuite_id suite_medium = {
	.name = "suite-medium",
	.plugin_grps = suite_sched_medium_groups,
	.version = { .version = "0.2", },
	.description = "A collection of scheduler benchmarks for medium sized servers, while monitoring the scheduler behavior for normal computers",
};

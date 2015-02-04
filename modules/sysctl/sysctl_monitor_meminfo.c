/*
 * Cbench - A C benchmarking suite for Linux benchmarking.
 * Copyright (C) 2013  Markus Pargmann <mpargmann@allfex.org>
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

#include <inttypes.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>

#include <cbench/data.h>
#include <cbench/requirement.h>
#include <cbench/version.h>
#include <cbench/plugin.h>

static const char monitor_meminfo_path[] = "/proc/meminfo";

static struct requirement plugin_monitor_meminfo_requirements[] = {
	{
		.name = monitor_meminfo_path,
		.description = "This plugin uses /proc/memninfo to monitor the systyem.",
		.found = 0,
	},
	{ }
};

static struct version plugin_monitor_meminfo_versions[] = {
	{
		.version = "0.1",
		.requirements = plugin_monitor_meminfo_requirements,
	}, {
		/* Sentinel */
	}
};

struct monitor_meminfo {
	uint64_t memtotal;
	uint64_t memfree;
	uint64_t buffers;
	uint64_t cached;
	uint64_t swapcached;
	uint64_t active;
	uint64_t inactive;
	uint64_t active_anon;
	uint64_t inactive_anon;
	uint64_t active_file;
	uint64_t inactive_file;
	uint64_t unevictable;
	uint64_t mlocked;
	uint64_t swaptotal;
	uint64_t swapfree;
	uint64_t dirty;
	uint64_t writeback;
	uint64_t anonpages;
	uint64_t mapped;
	uint64_t shmem;
	uint64_t slab;
	uint64_t sreclaimable;
	uint64_t sunreclaim;
	uint64_t kernelstack;
	uint64_t pagetables;
	uint64_t nfs_unstable;
	uint64_t bounce;
	uint64_t writebacktmp;
	uint64_t commitlimit;
	uint64_t committed_as;
	uint64_t vmalloctotal;
	uint64_t vmallocused;
	uint64_t vmallocchunk;
	uint64_t hugepages_total;
	uint64_t hugepages_free;
	uint64_t hugepages_rsvd;
	uint64_t hugepages_surp;
	uint64_t hugepagesize;
	uint64_t directmap4k;
	uint64_t directmap2m;
};

struct monitor_meminfo_data {
	char *buf;
	size_t buf_len;
	int first;

	double start_time;
};

static int monitor_meminfo_install(struct plugin *plug)
{
	struct monitor_meminfo_data *d = malloc(sizeof(*d));

	if (!d)
		return -1;

	d->buf = NULL;

	plugin_set_data(plug, d);

	return 0;
}

static int monitor_meminfo_uninstall(struct plugin *plug)
{
	struct monitor_meminfo_data *d = plugin_get_data(plug);

	if (d->buf) {
		free(d->buf);
		d->buf = NULL;
	}

	free(d);

	plugin_set_data(plug, NULL);
	return 0;
}

static int monitor_meminfo_init(struct plugin *plug)
{
	struct monitor_meminfo_data *d = plugin_get_data(plug);

	d->first = 1;
	return 0;
}

#define MEMINFO_PARSE(buf, name, ptr, varname) \
	ret = sscanf(buf, name ": %" PRIu64 " ", &ptr->varname);\
	if (ret == 1)\
		continue;

static int monitor_meminfo_get(struct plugin *plug,
		struct monitor_meminfo *meminfo)
{
	FILE *f;
	struct monitor_meminfo_data *d = plugin_get_data(plug);

	f = fopen(monitor_meminfo_path, "r");
	if (!f) {
		printf("Error: Could not open file %s\n", monitor_meminfo_path);
		return -1;
	}

	memset(meminfo, 0, sizeof(*meminfo));

	while (0 < getline(&d->buf, &d->buf_len, f)) {
		int ret;

		MEMINFO_PARSE(d->buf, "MemTotal", meminfo, memtotal)
		MEMINFO_PARSE(d->buf, "MemFree", meminfo, memfree)
		MEMINFO_PARSE(d->buf, "Buffers", meminfo, buffers)
		MEMINFO_PARSE(d->buf, "Cached", meminfo, cached)
		MEMINFO_PARSE(d->buf, "SwapCached", meminfo, swapcached)
		MEMINFO_PARSE(d->buf, "Active", meminfo, active)
		MEMINFO_PARSE(d->buf, "Inactive", meminfo, inactive)
		MEMINFO_PARSE(d->buf, "Active(anon)", meminfo, active_anon)
		MEMINFO_PARSE(d->buf, "Inactive(anon)", meminfo, inactive_anon)
		MEMINFO_PARSE(d->buf, "Active(file)", meminfo, active_file)
		MEMINFO_PARSE(d->buf, "Inactive(file)", meminfo, inactive_file)
		MEMINFO_PARSE(d->buf, "Unevictable", meminfo, unevictable)
		MEMINFO_PARSE(d->buf, "Mlocked", meminfo, mlocked)
		MEMINFO_PARSE(d->buf, "SwapTotal", meminfo, swaptotal)
		MEMINFO_PARSE(d->buf, "SwapFree", meminfo, swapfree)
		MEMINFO_PARSE(d->buf, "Dirty", meminfo, dirty)
		MEMINFO_PARSE(d->buf, "Writeback", meminfo, writeback)
		MEMINFO_PARSE(d->buf, "AnonPages", meminfo, anonpages)
		MEMINFO_PARSE(d->buf, "Mapped", meminfo, mapped)
		MEMINFO_PARSE(d->buf, "Shmem", meminfo, shmem)
		MEMINFO_PARSE(d->buf, "Slab", meminfo, slab)
		MEMINFO_PARSE(d->buf, "SReclaimable", meminfo, sreclaimable)
		MEMINFO_PARSE(d->buf, "SUnreclaim", meminfo, sunreclaim)
		MEMINFO_PARSE(d->buf, "KernelStack", meminfo, kernelstack)
		MEMINFO_PARSE(d->buf, "PageTables", meminfo, pagetables)
		MEMINFO_PARSE(d->buf, "NFS_Unstable", meminfo, nfs_unstable)
		MEMINFO_PARSE(d->buf, "Bounce", meminfo, bounce)
		MEMINFO_PARSE(d->buf, "WritebackTmp", meminfo, writebacktmp)
		MEMINFO_PARSE(d->buf, "CommitLimit", meminfo, commitlimit)
		MEMINFO_PARSE(d->buf, "Committed_AS", meminfo, committed_as)
		MEMINFO_PARSE(d->buf, "VmallocTotal", meminfo, vmalloctotal)
		MEMINFO_PARSE(d->buf, "VmallocUsed", meminfo, vmallocused)
		MEMINFO_PARSE(d->buf, "VmallocChunk", meminfo, vmallocchunk)
		MEMINFO_PARSE(d->buf, "HugePages_Total", meminfo, hugepages_total)
		MEMINFO_PARSE(d->buf, "HugePages_Free", meminfo, hugepages_free)
		MEMINFO_PARSE(d->buf, "HugePages_Rsvd", meminfo, hugepages_rsvd)
		MEMINFO_PARSE(d->buf, "HugePages_Surp", meminfo, hugepages_surp)
		MEMINFO_PARSE(d->buf, "Hugepagesize", meminfo, hugepagesize)
		MEMINFO_PARSE(d->buf, "DirectMap4k", meminfo, directmap4k)
		MEMINFO_PARSE(d->buf, "DirectMap2M", meminfo, directmap2m)
	}

	fclose(f);

	return 0;
}

#undef MEMINFO_PARSE

static int monitor_meminfo_mon(struct plugin *plug)
{
	struct monitor_meminfo_data *data = plugin_get_data(plug);
	struct monitor_meminfo meminfo;
	struct data *d;
	int ret;
	struct timespec time;
	double now;

	clock_gettime(CLOCK_MONOTONIC, &time);

	ret = monitor_meminfo_get(plug, &meminfo);
	if (ret)
		return ret;

	d = data_alloc(DATA_TYPE_MONITOR, 41);
	if (!d)
		return -1;

	now = time.tv_sec + time.tv_nsec / 1000000000.0;

	if (data->first) {
		data->start_time = now;
		data->first = 0;
	}

	now -= data->start_time;

	data_add_double(d, now);

	data_add_int64(d, meminfo.memtotal);
	data_add_int64(d, meminfo.memfree);
	data_add_int64(d, meminfo.buffers);
	data_add_int64(d, meminfo.cached);
	data_add_int64(d, meminfo.swapcached);
	data_add_int64(d, meminfo.active);
	data_add_int64(d, meminfo.inactive);
	data_add_int64(d, meminfo.active_anon);
	data_add_int64(d, meminfo.inactive_anon);
	data_add_int64(d, meminfo.active_file);
	data_add_int64(d, meminfo.inactive_file);
	data_add_int64(d, meminfo.unevictable);
	data_add_int64(d, meminfo.mlocked);
	data_add_int64(d, meminfo.swaptotal);
	data_add_int64(d, meminfo.swapfree);
	data_add_int64(d, meminfo.dirty);
	data_add_int64(d, meminfo.writeback);
	data_add_int64(d, meminfo.anonpages);
	data_add_int64(d, meminfo.mapped);
	data_add_int64(d, meminfo.shmem);
	data_add_int64(d, meminfo.slab);
	data_add_int64(d, meminfo.sreclaimable);
	data_add_int64(d, meminfo.sunreclaim);
	data_add_int64(d, meminfo.kernelstack);
	data_add_int64(d, meminfo.pagetables);
	data_add_int64(d, meminfo.nfs_unstable);
	data_add_int64(d, meminfo.bounce);
	data_add_int64(d, meminfo.writebacktmp);
	data_add_int64(d, meminfo.commitlimit);
	data_add_int64(d, meminfo.committed_as);
	data_add_int64(d, meminfo.vmalloctotal);
	data_add_int64(d, meminfo.vmallocused);
	data_add_int64(d, meminfo.vmallocchunk);
	data_add_int64(d, meminfo.hugepages_total);
	data_add_int64(d, meminfo.hugepages_free);
	data_add_int64(d, meminfo.hugepages_rsvd);
	data_add_int64(d, meminfo.hugepages_surp);
	data_add_int64(d, meminfo.hugepagesize);
	data_add_int64(d, meminfo.directmap4k);
	data_add_int64(d, meminfo.directmap2m);

	plugin_add_results(plug, d);

	return 0;
}

static const struct header *monitor_meminfo_data_hdr(struct plugin *plug)
{
	static const struct header hdr[] = {
		{
			.name = "time",
			.unit = "s",
			.description = "Time of this measurement.",
		}, {
			.name = "MemTotal",
			.unit = "kB",
			.description = "Total usable ram (i.e. physical ram minus a few reserved bits and the kernel binary code)",
		}, {
			.name = "MemFree",
			.unit = "kB",
			.description = "The sum of LowFree+HighFree",
		}, {
			.name = "Buffers",
			.unit = "kB",
			.description = "Relatively temporary storage for raw disk blocks shouldnt get tremendously large (20MB or so)",
		}, {
			.name = "Cached",
			.unit = "kB",
			.description = "in-memory cache for files read from the disk (the pagecache).  Doesnt include SwapCached",
		}, {
			.name = "SwapCached",
			.unit = "kB",
			.description = "Memory that once was swapped out, is swapped back in but still also is in the swapfile (if memory is needed it doesnt need to be swapped out AGAIN because it is already in the swapfile. This saves I/O)",
		}, {
			.name = "Active",
			.unit = "kB",
			.description = "Memory that has been used more recently and usually not reclaimed unless absolutely necessary.",
		}, {
			.name = "Inactive",
			.unit = "kB",
			.description = "Memory which has been less recently used.  It is more eligible to be reclaimed for other purposes",
		}, {
			.name = "Active(anon)",
			.unit = "kB",
		}, {
			.name = "Inactive(anon)",
			.unit = "kB",
		}, {
			.name = "Active(file)",
			.unit = "kB",
		}, {
			.name = "Inactive(file)",
			.unit = "kB",
		}, {
			.name = "Unevictable",
			.unit = "kB",
		}, {
			.name = "Mlocked",
			.unit = "kB",
		}, {
			.name = "SwapTotal",
			.unit = "kB",
			.description = "total amount of swap space available",
		}, {
			.name = "SwapFree",
			.unit = "kB",
			.description = "Memory which has been evicted from RAM, and is temporarily on the disk",
		}, {
			.name = "Dirty",
			.unit = "kB",
			.description = "Memory which is waiting to get written back to the disk",
		}, {
			.name = "Writeback",
			.unit = "kB",
			.description = "Memory which is actively being written back to the disk",
		}, {
			.name = "AnonPages",
			.unit = "kB",
			.description = "Non-file backed pages mapped into userspace page tables",
		}, {
			.name = "Mapped",
			.unit = "kB",
			.description = "files which have been mmaped, such as libraries",
		}, {
			.name = "Shmem",
			.unit = "kB",
		}, {
			.name = "Slab",
			.unit = "kB",
			.description = "in-kernel data structures cache",
		}, {
			.name = "SReclaimable",
			.unit = "kB",
			.description = "Part of Slab, that might be reclaimed, such as caches",
		}, {
			.name = "SUnreclaim",
			.unit = "kB",
			.description = "Part of Slab, that cannot be reclaimed on memory pressure",
		}, {
			.name = "KernelStack",
			.unit = "kB",
		}, {
			.name = "PageTables",
			.unit = "kB",
			.description = "amount of memory dedicated to the lowest level of page tables.",
		}, {
			.name = "NFS_Unstable",
			.unit = "kB",
			.description = "NFS pages sent to the server, but not yet committed to stable storage",
		}, {
			.name = "Bounce",
			.unit = "kB",
			.description = "Memory used for block device bounce buffers",
		}, {
			.name = "WritebackTmp",
			.unit = "kB",
			.description = "Memory used by FUSE for temporary writeback buffers",
		}, {
			.name = "CommitLimit",
			.unit = "kB",
		}, {
			.name = "Committed_AS",
			.unit = "kB",
			.description = "The amount of memory presently allocated on the system.",
		}, {
			.name = "VmallocTotal",
			.unit = "kB",
			.description = "total size of vmalloc memory area",
		}, {
			.name = "VmallocUsed",
			.unit = "kB",
			.description = "amount of vmalloc area which is used",
		}, {
			.name = "VmallocChunk",
			.unit = "kB",
			.description = "largest contiguous block of vmalloc area which is free",
		}, {
			.name = "HugePages_Total",
		}, {
			.name = "HugePages_Free",
		}, {
			.name = "HugePages_Rsvd",
		}, {
			.name = "HugePages_Surp",
		}, {
			.name = "Hugepagesize",
			.unit = "kB",
		}, {
			.name = "DirectMap4k",
			.unit = "kB",
		}, {
			.name = "DirectMap2M",
			.unit = "kB",
		}, {
			/* Sentinel */
		}
	};

	return hdr;
}

static int monitor_meminfo_mod_init(struct module *mod,
				    const struct plugin_id *plug)
{
	if (!access(monitor_meminfo_path, F_OK | R_OK))
		plugin_monitor_meminfo_requirements[0].found = 1;

	return 0;
}

const struct plugin_id plugin_meminfo = {
	.name = "monitor-meminfo",
	.description = "Monitor plugin to keep track of different values shown in /proc/meminfo.",
	.module_init = monitor_meminfo_mod_init,
	.install = monitor_meminfo_install,
	.uninstall = monitor_meminfo_uninstall,
	.init = monitor_meminfo_init,
	.monitor = monitor_meminfo_mon,
	.versions = plugin_monitor_meminfo_versions,
	.data_hdr = monitor_meminfo_data_hdr,
};

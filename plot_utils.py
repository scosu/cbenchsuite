#!/usr/bin/python3

# Cbenchsuite - A C benchmarking suite for Linux benchmarking.
# Copyright (C) 2013  Markus Pargmann <mpargmann@allfex.org>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


import matplotlib.pyplot as pyplt
import statistics
import operator
import re

properties_default = {
		'confidence': 0.95,
		'legend': True,
		'xsize': 16,
		'ysize': 9,
		'dpi': 300,
		'legendfontsize': 16,
		'xlabelfontsize': 17,
		'ylabelfontsize': 17,
		'ytickfontsize': 15,
		'xtickfontsize': 15,
		'barfontsize': 15,
		'titlefontsize': 20,
		'watermark': 'Powered by cbenchsuite (http://cbench.allfex.org)',
		'watermarkfontsize': 13,
		'file-type': 'svg'}

def property_get(properties, key):
	if properties and key in properties:
		return properties[key]
	if key in properties_default:
		return properties_default[key]
	return None

def mean_confidence_interval(data, confidence=0.95):
	mean, _, _, _, _, h = statistics.stats(data, 1-confidence)
	return mean, h

class parameter_sort:
	def _part_parameter(self, par):
		slash = par.split('/')
		equal = [i.split('=') for i in slash]

		ret = []

		for i in equal:
			ret += [j.split(' ') for j in i]
		return ret
	def __init__(self, obj, *args):
		if isinstance(obj, tuple):
			obj = obj[0]
		self.cmp_list = self._part_parameter(obj)
	def _rec_cmp(self, a, b):
		if isinstance(a, int):
			if a < b:
				return -1
			if a > b:
				return 1
			return 0
		if isinstance(a, str):
			mstr = "([^0-9]*)([0-9]+(.[0-9]+|))(.*)"
			am = re.match(mstr, a)
			bm = re.match(mstr, b)
			if not am or not bm:
				if a < b:
					return -1
				if a == b:
					return 0
				return 1

			if am.group(1) < bm.group(1):
				return -1
			if am.group(1) > bm.group(1):
				return 1
			ai = float(am.group(2))
			bi = float(bm.group(2))
			if ai < bi:
				return -1
			if ai > bi:
				return 1

			if am.group(4) < bm.group(4):
				return -1
			if am.group(4) > bm.group(4):
				return 1
			return 0

		for i in range(len(a)):
			if i == len(b):
				return -1
			ret = self._rec_cmp(a[i], b[i])
			if ret != 0:
				return ret

		if len(a) < len(b):
			return -1
		return 0
	def _compare(self, opt):
		a = self.cmp_list
		b = opt.cmp_list
		return self._rec_cmp(a, b)
	def __lt__(self, obj):
		return self._compare(obj) < 0
	def __gt__(self, obj):
		return self._compare(obj) > 0
	def __le__(self, obj):
		return self._compare(obj) <= 0
	def __ge__(self, obj):
		return self._compare(obj) >= 0
	def __eq__(self, obj):
		return self._compare(obj) == 0
	def __ne__(self, obj):
		return self._compare(obj) != 0
def parameter_sort_create(obj):
	return parameter_sort(obj)

def _sort_keys(keys):
	return sorted(keys, key=parameter_sort_create)

def _create_figure(properties):
	xsize = property_get(properties, 'xsize')
	ysize = property_get(properties, 'ysize')
	dpi = property_get(properties, 'dpi')
	fontsize = property_get(properties, 'fontsize')
	return pyplt.figure(figsize=(xsize, ysize), dpi=dpi)

def _plot_stuff(fig, ax, properties, max_x = None):
	wm = property_get(properties, 'watermark')
	fs = property_get(properties, 'watermarkfontsize')
	fig.text(1, 0, wm, fontsize=fs, color='black', ha='right', va='bottom', alpha=0.7)
	fs = property_get(properties, 'xtickfontsize')
	ax.tick_params(axis='x', which='major', labelsize=fs)
	ax.tick_params(axis='x', which='minor', labelsize=fs)

	fs = property_get(properties, 'ytickfontsize')
	ax.tick_params(axis='y', which='major', labelsize=fs)
	ax.tick_params(axis='y', which='minor', labelsize=fs)

	title = property_get(properties, 'title')
	if title != None:
		fs = property_get(properties, 'titlefontsize')
		ax.set_title(title, fontsize=fs)
	x_label = property_get(properties, 'xlabel')
	if x_label != None:
		fs = property_get(properties, 'xlabelfontsize')
		ax.set_xlabel(x_label, fontsize=fs)
	y_label = property_get(properties, 'ylabel')
	if y_label != None:
		fs = property_get(properties, 'ylabelfontsize')
		ax.set_ylabel(y_label, fontsize=fs)
	no_legend = property_get(properties, 'no-legend')
	if not no_legend and max_x != None:
		fs = property_get(properties, 'legendfontsize')
		handles, labels = ax.get_legend_handles_labels()
		by_label = dict(zip(labels, handles))
		labels = []
		for k,v in by_label.items():
			labels.append((k,v))
		hl = sorted(labels, key=parameter_sort_create)
		labels, handles = zip(*hl)
		max_label = 0
		for k in labels:
			max_label = max(len(k), max_label)

		est_len = (max_label * fs) / 30
		ax.set_xlim(xmax = max_x * max(1.01, 0.00155 * est_len ** 2 - 0.0135 * est_len + 1.08))
		ax.legend(handles, labels, loc='center right', bbox_to_anchor=(1.13, 0.5), fancybox=True, shadow=True, fontsize=fs)


# Takes something like this:
#data_line = {
#		'x': [(0,1), (1,3), (2,2), (3,5)],
#		'y': [(0,1), (2,1), (3,2), (4,5)],
#		'z': [(0,[1,2,3,2,2,2,2]), (1,[2,3,2,2,3,3,3,3]), (2,2), (3,5)]
#}
def plot_line_chart(data, path, properties=None, fmts_arg = None, x_keys = None):
	fig = _create_figure(properties)
	ax = fig.add_subplot(111)
	if fmts_arg == None:
		fmts = [
				{'color': '#348ABD', 'linestyle': '-', 'marker': ' '},
				{'color': '#A60628', 'linestyle': '-', 'marker': ' '},
				{'color': '#467821', 'linestyle': '-', 'marker': ' '},
				{'color': '#CF4457', 'linestyle': '-', 'marker': ' '},
				{'color': '#188487', 'linestyle': '-', 'marker': ' '},
				{'color': '#E24A33', 'linestyle': '-', 'marker': ' '},
				{'color': '#348ABD', 'linestyle': '--', 'marker': ' '},
				{'color': '#A60628', 'linestyle': '--', 'marker': ' '},
				{'color': '#467821', 'linestyle': '--', 'marker': ' '},
				{'color': '#CF4457', 'linestyle': '--', 'marker': ' '},
				{'color': '#188487', 'linestyle': '--', 'marker': ' '},
				{'color': '#E24A33', 'linestyle': '--', 'marker': ' '},
				{'color': '0.4', 'linestyle': '-', 'marker': ' '},
				{'color': '0.4', 'linestyle': '--', 'marker': ' '},
			]
	else:
		fmts = fmts_arg
	fmtsid = 0
	def _next_fmt():
		nonlocal fmtsid
		fmt = fmts[fmtsid%len(fmts)]
		fmtsid += 1
		return fmt

	if x_keys == None:
		x_keys = _sort_keys(data.keys())

	max_overall_x = None
	min_overall_x = None
	for k in x_keys:
		if k not in data:
			continue
		fmt = _next_fmt()
		if isinstance(data[k][0][0], list) or isinstance(data[k][0][0], tuple):
			dats = data[k]
		else:
			dats = [data[k]]
		for v in dats:
			xs = []
			ys = []
			yerrs = []
			yerr_not_zero = False
			for pt in v:
				xs.append(pt[0])
				if max_overall_x == None:
					max_overall_x = pt[0]
					min_overall_x = pt[0]
				else:
					max_overall_x = max(max_overall_x, pt[0])
					min_overall_x = min(min_overall_x, pt[0])
				if isinstance(pt[1], list):
					m, h = mean_confidence_interval(pt[1])
					yerrs.append(h)
					ys.append(m)
					yerr_not_zero = True
				else:
					ys.append(pt[1])
					yerrs.append(0.0)
			if yerr_not_zero:
				ax.errorbar(xs, ys, yerrs, label=k, linestyle=fmt['linestyle'], marker=fmt['marker'], color=fmt['color'])
			else:
				ax.plot(xs, ys, label=k, linestyle=fmt['linestyle'], marker=fmt['marker'], color=fmt['color'])
	rang = max_overall_x - min_overall_x
	ax.set_xlim(xmin = (min_overall_x - rang*0.05))
	_plot_stuff(fig, ax, properties, max_x=(max_overall_x + rang*0.05))
	path += '.' + property_get(properties, 'file-type')
	fig.savefig(path)
	print("Generated figure " + path)


# Works with all those structures:
#data = {'a':1, 'b':4, 'c':2}
#data = {
#		'a': {'1': 2, '2': 1, 'dummy': 4},
#		'b': {'1': 3, '2': 1, 'dummy': 3},
#		'c': {'1': 4, '2': 2, 'dummy': 1}
#}
#data = {
#		'x': {'1': {'a': 4, 'e': [1, 2, 3], 'c': 3}, '2': {'d':5, 'e': 2}, 'dummy': {'e':3}},
#		'y': {'1': {'a': 6, 'e': [1, 2, 2, 3], 'c': 1}, '2ab': {'d':5}, 'dummy': {'e':3}},
#		'z': {'1': {'a': 2, 'eaasd': [1, 2, 2, 2, 3], 'c': 4}, '2': {'d':5}, 'dummy': {'e':3}}
#}
##
# @brief Plot a bar chart with data of up to 3 dimensions
#
# @param data maximal 3 level depth of dictionary with lists of floats or floats
# as lowest children. Lists will produce errorbars. The keys of the dictionary
# are used to sort the stuff.
# @param l1_keys Level 1 keys, this specifies the order
# @param l2_keys Level 2 keys, this specifies the order
# @param l3_keys Level 3 keys, this specifies the order
# @param colors_arg
# @param confidence_arg
#
# @return
def plot_bar_chart(data, path, properties = None, l1_keys = None, l2_keys = None, l3_keys = None, colors_arg = None):
	fig = _create_figure(properties)
	ax = fig.add_subplot(111)
	if l1_keys == None:
		l1_keys = _sort_keys(data.keys())
	if l2_keys == None:
		l2_keys = set()
		for k1, v1 in data.items():
			if not isinstance(v1, dict):
				continue
			for k2, v2 in v1.items():
				l2_keys.add(k2)
		l2_keys = _sort_keys(list(l2_keys))
	if l3_keys == None:
		l3_keys = set()
		for k1, v1 in data.items():
			if not isinstance(v1, dict):
				continue
			for k2, v2 in v1.items():
				if not isinstance(v2, dict):
					continue
				for k3, v3 in v2.items():
					l3_keys.add(k3)
		l3_keys = _sort_keys(list(l3_keys))
	xxticks = []
	xticks = []
	confidence = property_get(properties, 'confidence')
	label_colors = {}
	if colors_arg == None:
		colors = ['#348ABD', '#A60628', '#467821', '#CF4457', '#188487', '#E24A33', '0.3', '0.5', '0.7']
	else:
		colors = colors_arg
	colorid = 0
	x = 1
	max_overall = 0
	def _xtick(label, x_val = None):
		nonlocal x
		if x_val == None:
			x_val = x
		xxticks.append(x_val)
		xticks.append(label)
	def _color_get(label):
		nonlocal colorid
		if label in label_colors:
			return False, label_colors[label]
		else:
			label_colors[label] = colors[colorid % len(colors)]
			colorid += 1
			return True, label_colors[label]
	def _plt_bar(x, y, label):
		nonlocal confidence
		set_label, color = _color_get(label)
		if isinstance(y, list):
			nonlocal max_overall
			m,h = mean_confidence_interval(y, confidence)
			max_overall = max(m, max_overall)
			if label == None or not set_label:
				ax.bar(x, m, align="center", color = color, ecolor='r', yerr = h)
			else:
				ax.bar(x, m, align="center", color = color, ecolor='r', label = label, yerr = h)
		else:
			max_overall = max(max_overall, y)
			if label == None or not set_label:
				ax.bar(x, y, align="center", color = color)
			else:
				ax.bar(x, y, align="center", color = color, label = label)

	levels = 0
	for l1_key in l1_keys:
		l1_val = data[l1_key]
		levels = 1
		if not isinstance(l1_val, dict):
			_xtick(l1_key)
			_plt_bar(x, l1_val, l1_key)
			x += 1
		else:
			_xtick(l1_key, x + len(l1_val.keys()) / 2 - 0.5)
			levels = 2
			for l2_key in l2_keys:
				if l2_key not in l1_val:
					continue
				l2_val = l1_val[l2_key]
				levels = 3
				if not isinstance(l2_val, dict):
					_plt_bar(x, l2_val, l2_key)
					max_val = 0
					x += 1

				else:
					max_val = 0
					for l3_key in l3_keys:
						if l3_key not in l2_val:
							continue
						l3_val = l2_val[l3_key]
						if isinstance(l3_val, list):
							max_val = max(sum(l3_val)/len(l3_val), max_val)
						else:
							max_val = max(l3_val, max_val)
						_plt_bar(x, l3_val, l3_key)

					fs = property_get(properties, 'barfontsize')
					rot = min((len(l2_key) - 1)*30, 90)
					ax.text(x, max_val + max_overall * 0.02, l2_key, horizontalalignment='center', verticalalignment='bottom', rotation=rot, fontsize = fs)
					x += 1
			x += 1
	if levels == 3:
		ax.set_ylim(ymin = 0, ymax=1.2*max_overall)
	else:
		ax.set_ylim(ymin = 0)
	ax.set_xticks(xxticks)
	ax.set_xticklabels(xticks)
	_plot_stuff(fig, ax, properties, max_x = x-0.5)

	path += '.' + property_get(properties, 'file-type')
	fig.savefig(path)
	print("Generated figure " + path)



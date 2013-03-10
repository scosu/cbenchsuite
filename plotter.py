#!/usr/bin/python

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

import argparse
import cmd
import sqlite3
import os
import plot_utils
import threading
import json

parser = argparse.ArgumentParser()

parser.add_argument('-d', '--database', help="Sqlite3 database file, default: ~/.cbenchsuite/db/db.sqlite", default="~/.cbenchsuite/db/db.sqlite")
parser.add_argument('-o', '--outdir', help="Output directory for plots. Default: /tmp/cbenchsuite/", default="/tmp/cbenchsuite/")


parsed = parser.parse_args()

db = sqlite3.connect(os.path.expanduser(parsed.database));
db.row_factory = sqlite3.Row

def indent(nr):
	s = ''
	for i in range(nr):
		s += '  '
	return s

def translate(name, replacements, append_original = False):
	if isinstance(name, list) or isinstance(name,tuple):
		n = name
	else:
		n = [name]
	ret = []
	for i in n:
		if i in replacements:
			if append_original:
				ret.append(replacements[str(i)] + '(' + str(i) + ')')
			else:
				ret.append(replacements[str(i)])
		else:
			ret.append(str(i))
	if isinstance(name, list) or isinstance(name, tuple):
		return ret
	else:
		return ret[0]

class filter:
	def __init__(self, table, condition, desc = None):
		self.table = table
		self.condition = condition
		self.desc = desc
		self.properties = {}
	def name(self):
		f_query = self.table + ': ' + self.condition
		if self.desc:
			return self.desc + ' (' + f_query + ')'
		else:
			return f_query
	def to_sql(self):
		return self.condition

class diff_tree_edge:
	def __init__(self, vals, ptr):
		self.vals = vals
		self.ptr = ptr
class diff_tree:
	def __init__(self):
		self.name = []
		self.childs = []
		self.combo = None
		self.properties = {}
	def set_name(self, name):
		self.name = [name]
	def _add_edge(self, childs, values):
		for c in childs:
			if len(values) != len(c.vals):
				continue
			match = True
			for i in range(len(values)):
				if c.vals[i] != values[i]:
					match = False
					break
			if match:
				return c
		tmp_edge = diff_tree_edge(values, diff_tree())
		childs.append(tmp_edge)
		return tmp_edge
	def depth(self):
		if len(self.name) == 0:
			return 0
		depth = 0
		for e in self.childs:
			depth = max(depth, e.ptr.depth())
		return depth + 1
	def plot_dict(self, plot_dict_cb, node_path, translations):
		if len(self.name) == 0 or len(self.childs) == 0:
			return plot_dict_cb(node_path, self)
		ret = {}
		for e in self.childs:
			if len(self.name) == 1 and (self.name[0] == 'system' or self.name[0] == 'data'):
				k = '/'.join(translate(e.vals, translations))
			else:
				k = '/'.join(translate(self.name, translations)) + '=' + '/'.join(translate(e.vals, translations))
			ret[k] = e.ptr.plot_dict(plot_dict_cb, node_path + [(self, e)], translations)
		return ret
	def plot(self, plot_cb, max_depth, node_path):
		if self.depth() <= max_depth:
			plot_cb(node_path, self)
			return
		for e in self.childs:
			e.ptr.plot(plot_cb, max_depth, node_path + [(self, e)])
	def add_child(self, value):
		return self._add_edge(self.childs, [value]).ptr
	def squash_names(self, names):
		one_match = False
		for i in self.name:
			if i in names:
				one_match = True
				break
		if not one_match:
			for e in self.childs:
				e.ptr.squash_names(names)
			return
		for n in names:
			if n in self.name:
				continue
			for e in self.childs:
				e.ptr.pull_name_here(n)
			self.name += self.childs[0].ptr.name
			new_childs = []
			for e in self.childs:
				for prop, val in e.ptr.properties:
					self.properties[prop] = val
				for ce in e.ptr.childs:
					new_childs.append(diff_tree_edge(e.vals[:] + ce.vals[:], ce.ptr))
			self.childs = new_childs
	def autosquash(self, max_levels):
		max_levels.reverse()
		full_path = []
		lvl = 0
		while 1:
			possible_values = set()
			lvl_nodes = self.get_nodes_level(lvl)
			lvl += 1
			if len(lvl_nodes) == 0:
				break
			for i in lvl_nodes:
				if len(i.name) == 0:
					break
				for e in i.childs:
					possible_values.add(tuple(e.vals))
			if len(lvl_nodes[0].name) == 0:
				break
			full_path.append((lvl_nodes[0].name[0], possible_values))
		full_path.reverse()
		lvl = 0
		to_squash = []
		gathered = 1
		for (name, vals) in full_path:
			if gathered * len(vals) > max_levels[lvl]:
				if len(to_squash) > 1:
					self.squash_names(to_squash)
				gathered = 1
				to_squash = []
				lvl += 1
				if lvl >= len(max_levels):
					break
			gathered *= len(vals)
			to_squash.append(name)
		if len(to_squash) > 1:
			to_squash.reverse()
			self.squash_names(to_squash)
	def find_nodes(self, name):
		if name in self.name:
			return [self]
		ret = []
		for e in self.childs:
			ret += e.ptr.find_nodes(name)
		return ret
	def swap_with_next(self):
		new_childs = []
		new_properties = {}
		for e in self.childs:
			for prop,val in e.ptr.properties.items():
				new_properties[prop] = val
			for ce in e.ptr.childs:
				ne = self._add_edge(new_childs, ce.vals)
				nn = ne.ptr
				nn.properties = self.properties.copy()
				nn.name = self.name[:]
				nne = self._add_edge(nn.childs, e.vals)
				nne.ptr = ce.ptr
		self.name = self.childs[0].ptr.name[:]
		self.childs = new_childs
		self.properties = new_properties
	def pull_name_here(self, name):
		if name in self.name:
			return True
		if len(self.name) == 0 or len(self.childs) == 0:
			return False
		failed = False
		for e in self.childs:
			ret = e.ptr.pull_name_here(name)
			if not ret:
				failed = True
				break
		if failed:
			return False
		self.swap_with_next()
		return True
	def get_nodes_level(self, level):
		if level == 0:
			return [self]
		nodes = []
		for e in self.childs:
			nodes += e.ptr.get_nodes_level(level-1)
		return nodes
	def print(self, replacements, prefix, first_prefix, plot_depth):
		if len(self.name) == 0:
			if len(self.properties) != 0:
				print(first_prefix + '    Properties: ', end='')
		else:
			print(first_prefix + ' ' + '/'.join(translate(self.name, replacements, True)))
			if len(self.properties) != 0:
				print(prefix + ' |    Properties: ', end='')
		if len(self.properties) > 0:
			first = True
			for prop in sorted(self.properties.keys()):
				if first:
					first = False
				else:
					print(', ', end='')
				print(prop + '=' + str(self.properties[prop]), end='')
			print('')
		depth = self.depth()
		if depth - 1 == plot_depth:
			dpre = ' || '
			dpre_first = '-||-'
		else:
			dpre = ''
			dpre_first = ''
		for i in range(len(self.childs)):
			if len(self.childs[i].ptr.name) == 0 and len(self.childs[i].ptr.properties) == 0:
				if i == len(self.childs) - 1:
					print(prefix + ' `- ', end='')
				else:
					print(prefix + '|`- ', end='')
			else:
				if i == len(self.childs) - 1:
					print(prefix + ' \\ ', end='')
				else:
					print(prefix + '|\\ ', end='')
			e = self.childs[i]
			first = True
			print('/'.join(translate(e.vals, replacements, True)))
			if i == len(self.childs) - 1:
				e.ptr.print(replacements, prefix + '    ' + dpre, prefix + '  `-' + dpre_first, plot_depth)
			else:
				e.ptr.print(replacements, prefix + '|   ' + dpre, prefix + '| `-' + dpre_first, plot_depth)

	def print_flattened(self, replacements, plot_depth):
		lvl = 0
		while 1:
			s = None
			possible_values = set()
			lvl_nodes = self.get_nodes_level(lvl)
			lvl += 1
			if len(lvl_nodes) == 0:
				break
			remaining_depth = lvl_nodes[0].depth()
			for i in lvl_nodes:
				if len(i.name) == 0:
					return
				if not s:
					s = indent(3) + '/'.join(translate(i.name, replacements, True))
				for e in i.childs:
					possible_values.add(tuple(e.vals))
			if remaining_depth == plot_depth:
				print(indent(3) + "----------- Figure parameters ------------")
			print(s)
			for val in sorted([ '/'.join(translate(i, replacements, True)) for i in possible_values]):
				print(indent(4) + val)
	def remove_name(self, name):
		if self.name == name:
			child = self.childs[0].ptr
			self.name = child.name
			self.childs = child.childs
			self.combo = child.combo
			self.properties = child.properties
			return
		for e in self.childs:
			e.ptr.remove_name(name)
		return
	def remove_singleval_childs(self):
		removed_values = {}
		lvl = 0
		properties = {}
		while 1:
			possible_values = set()
			lvl_nodes = self.get_nodes_level(lvl)
			if len(lvl_nodes) == 0:
				break
			for i in lvl_nodes:
				if not i.name:
					return removed_values, properties
				for e in i.childs:
					possible_values.add(tuple(e.vals))
			if len(possible_values) == 1:
				if lvl_nodes[0].name != None and list(possible_values)[0] != None:
					for i in range(len(lvl_nodes[0].name)):
						removed_values[lvl_nodes[0].name[i]] = list(possible_values)[0][i]
					#removed_values[tuple(lvl_nodes[0].name)] = list(possible_values)[0]
					for k,v in lvl_nodes[0].properties.items():
						properties[k] = v
				self.remove_name(lvl_nodes[0].name)
				continue
			lvl += 1
		return removed_values, properties

class plot:
	known_properties = {
			'xlabel': str,
			'ylabel': str,
			'title': str,
			'confidence': float,
			'legend': bool,
			'xsize': float,
			'ysize': float,
			'dpi': int,
			'legendfontsize': float,
			'xlabelfontsize': float,
			'ylabelfontsize': float,
			'ytickfontsize': float,
			'xtickfontsize': float,
			'barfontsize': float,
			'titlefontsize': float,
			'watermark': str,
			'watermarkfontsize': float}
	def __init__(self, db, filters, plugin_sha, combos, levels, base_path, replacerules = None):
		super(plot, self).__init__()
		self.plugin_sha = plugin_sha
		self.combos = combos
		self.db = db
		self.threaddb = None
		self.filters = filters
		self.properties = {}
		self.removed_values = {}
		self.removed_properties = {}
		self.thread = None
		if not replacerules:
			self.replacerules = {}
		else:
			self.replacerules = replacerules.copy()

		res = db.execute('''SELECT plugin_table, plugin_opt_table, plugin_comp_vers_table, module, name
				FROM plugin
				WHERE plugin_sha = ''' + "'" + plugin_sha + "';")
		row = res.fetchone()
		if row[0] == '':
			self.dummy = True
		else:
			self.dummy = False
		self.tables = {'results' : row[0],
				'options' : row[1],
				'versions' : row[2]}
		self.module_name = row[3]
		self.name = row[4]
		self.base_path = os.path.join(base_path, self.module_name + '.' + self.name)

		self.properties['title'] = self.module_name + '.' + self.name

		if not self.dummy:
			res = db.execute('''SELECT COUNT(run_uuid) as nr_results
					FROM "''' + self.tables['results'] + '''"
					GROUP BY run_uuid;''')
			row = res.fetchone()
			if row[0] > 1:
				self.barchart = False
				self.linechart = True
				self.levels = levels['line']
			else:
				self.barchart = True
				self.linechart = False
				self.levels = levels['bar']
		self.root = None
		self.rebuild_tree()
	def _plot_db_cb(self, nodepath, lastnode):
		data_field = None
		for (node,edge) in nodepath:
			for i in range(len(node.name)):
				if node.name[i] == 'data':
					data_field = edge.vals[i]
					break
		if data_field == None:
			if 'data' not in self.removed_values:
				return {}
			data_field = self.removed_values['data']
		query = 'SELECT "' + self.tables['results'] + '"."' + data_field + '''"
			FROM
				unique_run,
				"''' + self.tables['results'] + '''" USING(run_uuid)
			WHERE
				system_sha = ''' + "'" + lastnode.combo['system'] + """' and
				plugin_group_sha = '""" + lastnode.combo['group'] + """';"""
		res = self.threaddb.execute(query)
		data = []
		for row in res:
			data.append(row[0])
		return data
	def _plot_cb(self, nodepath, last_node):
		path = self.base_path
		for n in nodepath:
			path = os.path.join(path, '#'.join(translate(n[0].name, self.replacerules)) + '__' + '#'.join(translate(n[1].vals, self.replacerules)))

		try:
			os.makedirs(os.path.dirname(path))
		except:
			pass

		data = last_node.plot_dict(self._plot_db_cb, nodepath[:], self.replacerules)

		props = self.properties.copy()
		for k,v in self.removed_properties.items():
			props[k] = v
		for n in nodepath:
			for k,v in n[0].properties.items():
				props[k] = v
		for k,v in last_node.properties.items():
			props[k] = v

		if isinstance(data, list):
			data = {'result': data}

		if self.barchart:
			plot_utils.plot_bar_chart(data, path, props)
		else:
			plot_utils.plot_line_chart(data, path, props)
		del data
		print("Generated " + path)
	def _plot(self):
		self.threaddb = sqlite3.connect(os.path.expanduser(parsed.database));
		if self.barchart:
			self.root.plot(self._plot_cb, 3, [])
		else:
			self.root.plot(self._plot_cb, 1, [])
		self.threaddb.close()
		self.threaddb = None
	def plot(self):
		if self.dummy:
			return
		self.thread = threading.Thread(target=self._plot)
		self.thread.start()
	def plot_wait(self):
		if self.dummy:
			return
		self.thread.join()
		self.thread = None
	def update_plot_depth(self):
		nodes = self.root.find_nodes('data')
		if self.barchart:
			depth = 3
		else:
			depth = 1
		d = self.root.depth()
		if d < depth:
			depth = d
		for i in nodes:
			d = i.depth() - 1
			if d < depth:
				depth = d
		self.properties['plot-depth'] = depth
	def rebuild_tree(self):
		if self.dummy:
			return
		root = diff_tree()
		for combo in self.combos:
			ptrs = [root]
			for level in self.levels:
				if level == 'system':
					for pind in range(len(ptrs)):
						ptrs[pind].set_name('system')
						ptrs[pind] = ptrs[pind].add_child(combo['system'])
				elif level == 'option':
					if self.tables['options'] == '':
						continue
					res = self.db.execute('SELECT * FROM "' + self.tables['options'] + '" WHERE plugin_opts_sha = \'' + combo['options'] + '\';')
					row = res.fetchone()
					for k in sorted(row.keys()):
						v = row[k]
						if k == "plugin_opts_sha":
							continue
						self.replacerules['option.' + k] = k.capitalize()
						for pind in range(len(ptrs)):
							ptrs[pind].set_name('option.' + k)
							ptrs[pind] = ptrs[pind].add_child(v)
				elif level == 'version':
					if self.tables['versions'] == '':
						continue
					res = self.db.execute('SELECT * FROM "' + self.tables['versions'] + '" WHERE plugin_comp_vers_sha = \'' + combo['versions'] + '\';')
					row = res.fetchone()
					for k in sorted(row.keys()):
						v = row[k]
						if k == "plugin_comp_vers_sha":
							continue
						self.replacerules['version.' + k] = k.capitalize()
						for pind in range(len(ptrs)):
							ptrs[pind].set_name('version.' + k)
							ptrs[pind] = ptrs[pind].add_child(v)
				elif level == 'data':
					for pind in range(len(ptrs)):
						ptrs[pind].set_name('data')
					res = self.db.execute("SELECT * FROM plugin_data_meta WHERE plugin_sha = '" + self.plugin_sha + "';")
					new_ptrs = []
					for row in res:
						for pind in range(len(ptrs)):
							tmp_child = ptrs[pind].add_child(row['name'])
							tmp_child.properties['ylabel'] = row['name'].capitalize()
							if row['unit'] != '':
								tmp_child.properties['ylabel'] += ' (' + row['unit'] + ')'
							new_ptrs.append(tmp_child)
					ptrs = new_ptrs
			for ptr in ptrs:
				ptr.combo = combo
		self.root = root
		self.update_plot_depth()
	def print(self):
		print(indent(1) + "{:s}.{:s}".format(self.module_name, self.name))
		if self.dummy:
			print(indent(2) + "Plugin without data")
		else:
			if self.barchart:
				print(indent(2) + "Chart type: bar")
			if self.linechart:
				print(indent(2) + "Chart type: line")
			if len(self.properties) > 0:
				print(indent(2) + "Properties:")
				for k in sorted(self.properties.keys()):
					print(indent(3) + k + ": " + str(self.properties[k]))
			print(indent(2) + "Number of different configurations: " + str(len(self.combos)))
			if len(self.removed_values) > 0:
				print(indent(2) + "Same run parameters:")
				for k in sorted(self.removed_values.keys()):
					print(indent(3) + translate(k, self.replacerules, True) + " = " + translate(self.removed_values[k], self.replacerules, True))
			print(indent(2) + "Parameter graph: (Everything right of '||' will be in the figure)")
			self.root.print(self.replacerules, indent(4), indent(3) + '-', self.properties['plot-depth'])
			#self.root.print_flattened(self.replacerules, self.properties['plot-depth'])
	def autoremove(self):
		if self.dummy:
			return
		self.removed_values, self.removed_properties = self.root.remove_singleval_childs()
		self.update_plot_depth()
	def squash_levels(self, names):
		self.root.squash_names(names)
		self.update_plot_depth()
	def autosquash_levels(self, max_values):
		self.root.autosquash(max_values)
		self.update_plot_depth()
	def line_autosquash(self, max_values):
		if self.linechart:
			self.autosquash_levels(max_values)
	def bar_autosquash(self, max_values):
		if self.barchart:
			self.autosquash_levels(max_values)
		self.update_plot_depth()
	def add_replace_rule(self, src, tgt):
		self.replacerules[src] = tgt
	def add_property(self, prop, val):
		if prop not in self.known_properties:
			print("Unknown property " + prop)
			return
		try:
			self.properties[prop] = self.known_properties[prop](val)
		except:
			print("Failed to parse property")

def db_create_filters(filters, tables):
	s = None
	for tab in tables:
		for f in filters:
			if f.table == tab:
				if s == None:
					s = '(' + f.to_sql() + ')'
				else:
					s += ' AND (' + f.to_sql() + ')'
	return s

def db_generic_run_exec(db, filters, select, group_by = None, order_by = None):
	involved_tables = ['unique_run', 'system', 'system_cpu', 'cpu_type', 'plugin_group', 'plugin']
	filter_query = db_create_filters(filters, involved_tables)
	query = '''
		SELECT plugin_opt_table,plugin_comp_vers_table
		FROM
			unique_run,
			plugin_group USING(plugin_group_sha),
			plugin USING(plugin_sha)'''
	if filter_query:
		query += ' WHERE ' + filter_query
	query += ' GROUP BY plugin_sha;'
	res = db.execute(query)

	plugin_tables = ''
	for plug in res:
		if plug['plugin_opt_table'] != '':
			plugin_tables += ' LEFT JOIN '
			plugin_tables += '"' + plug['plugin_opt_table'] + '" ON plugin_group.plugin_opts_sha = "' + plug['plugin_opt_table'] + '".plugin_opts_sha'
			involved_tables.append(plug['plugin_opt_table'])
		if plug['plugin_comp_vers_table'] != '':
			plugin_tables += ' LEFT JOIN '
			plugin_tables += '"' + plug['plugin_comp_vers_table'] + '" ON plugin_group.plugin_comp_vers_sha = "' + plug['plugin_comp_vers_table'] + '".plugin_comp_vers_sha'
			involved_tables.append(plug['plugin_comp_vers_table'])

	filter_query = db_create_filters(filters, involved_tables)

	if select:
		query = 'SELECT ' + select + ','
	else:
		query = 'SELECT '
	query += '''COUNT(DISTINCT run_uuid) as number_runs
			FROM
				unique_run,
				system USING(system_sha),
				system_cpu USING(cpus_sha),
				cpu_type USING(cpu_type_sha),
				plugin_group USING(plugin_group_sha),
				plugin USING(plugin_sha) '''
	query += plugin_tables
	if filter_query:
		query += 'WHERE ' + filter_query
	if group_by:
		query += ' GROUP BY ' + group_by
	if order_by:
		query += ' ORDER BY ' + order_by
	return db.execute(query)

def db_get_systems(db, filters):
	return db_generic_run_exec(db, filters, "system.*", group_by = 'system_sha')

def db_get_plugin_groups(db, filters):
	return db_generic_run_exec(db, filters, "plugin_group.*,plugin.*,system.*",
				group_by = 'plugin_sha,plugin_group_sha',
				order_by = 'plugin_group_sha,plugin_sha')

def match_line(cmpls, text, line, begidx, endidx):
	offs = len(line[:endidx].split()) - 2
	if text == '':
		offs += 1
	if len(cmpls) <= offs:
		return []
	matches = []
	for i in cmpls[offs]:
		if i.startswith(text):
			matches.append(i)
	return matches

def input_get_selected(prompt, select_list, max_ct):
	dat = input(prompt)
	selected = []
	try:
		for i in dat.split(' '):
			dec = int(i)
			if dec < 1 or dec >= max_ct:
				print("Selection is out of range: " + i)
				return []
			selected.append(select_list[dec-1])
		return selected
	except:
		print("Can't parse selection " + i)
		return []
def plotgrps_generate(db, filters, levels):
	default_replacements = {}
	res = db_generic_run_exec(db, filters, "plugin_group_sha, plugin_sha, system_sha, plugin_group.plugin_opts_sha, plugin_group.plugin_comp_vers_sha, system.custom_info, system.nr_cpus_on, system.kernel, module, name",
			group_by = "plugin_group_sha, plugin_sha, system_sha")

	grp_sys_rows = {}
	for row in res:
		k = row['plugin_group_sha'] + '/' + row['system_sha']

		if k not in grp_sys_rows:
			grp_sys_rows[k] = []
		grp_sys_rows[k].append(row)
		default_replacements[row['system_sha']] = row['custom_info'] + ' ' + str(row['nr_cpus_on']) + ' CPUs Kernel ' + row['kernel']

	grps = []
	for gs in grp_sys_rows.values():
		group_added = False
		for gc in range(len(grps)):
			match = True
			grp_keys = [j['plugin_sha'] for j in grps[gc][0]]
			if len(gs) != len(grp_keys):
				continue
			for i in gs:
				if i['plugin_sha'] not in grp_keys:
					match = False
					break
			if match:
				grps[gc].append(gs)
				group_added = True
				continue
		if not group_added:
			grps.append([gs])

	# grps is a list of lists of row objects
	# level2: list of row objects included in same plugin configuration.

	plot_grps = []

	path = parsed.outdir
	for combo in grps:
		plots = []

		plugs = {}
		for g in combo:
			for inst in g:
				k = inst['plugin_sha']

				if k not in plugs:
					plugs[k] = []
				print(inst['plugin_group_sha'])
				plugs[k].append({'system': inst['system_sha'],
						'options': inst['plugin_opts_sha'],
						'versions': inst['plugin_comp_vers_sha'],
						'group': inst['plugin_group_sha'],
						'name': inst['module'] + '.' + inst['name']})

		for psha,p in plugs.items():
			names = set()
			for i in p:
				names.add(i['name'])
			local_path = os.path.join(path, '#'.join(sorted(list(names))))
			plots.append(plot(db, filters, psha, p, levels, local_path, default_replacements))
		if len(plots) > 0:
			plot_grps.append(plots)
	return plot_grps


class cmdline(cmd.Cmd):
	def __init__(self, db):
		super(cmdline, self).__init__()
		self.prompt = "data  > "
		self.intro = "This is a plotter CLI. Use help for more information"
		self.filters = []
		self.db = db
		self.plotmode = False
		self.plotgrps = []
		self.default_levels = {'bar': ['data', 'version', 'option', 'system'],
					'line': ['data', 'version', 'option', 'system']}
	def check_plotmode(self):
		if self.plotmode:
			print("You can not use this command in plot mode, you first have to call plots_reset")
			return True
		return False
	def do_filters(self, arg):
		'''Usage: filters

List all filters used. No arguments expected
'''
		ct = 1
		if len(self.filters) == 0:
			print("No filters defined")
		for f in self.filters:
			print(indent(1) + "{:2d}: {:s}".format(ct, f.name()))
			ct += 1
	def do_filter_add(self, arg):
		'''Usage: filter_add <sqlite table> <sqlite filter expression>

Add a custom filter to the list of used filters.

Arguments:
	sqlite table: sqlite table that this filter is using.
	sqlite filter expression: A filter expression used in the WHERE section
		of the sql query. You should use complete field identifiers
		e.g. TABLE.FIELD
'''
		if self.check_plotmode():
			return
		table, _, condition = arg.partition(' ')
		if condition == '':
			print('Requiring more than 2 arguments, see help')
			return

		new_filter = filter(table, condition)
		try:
			res = db_generic_run_exec(self.db, self.filters + [new_filter], None)
			row = res.fetchone()
			print("Number of unique runs filtered: " + str(row['number_runs']))
		except:
			print("Filter contains an SQL error, did not added this filter")
	def complete_filter_add(self, text, line, begidx, endidx):
		cmpls = [['system', 'system_cpu', 'cpu_type', 'plugin', 'plugin_group', 'unique_run']]
		return match_line(cmpls, text, line, begidx, endidx)
	def do_filter_rm(self, arg):
		'''Usage: filter_rm <selected filter>

Remove a filter.

Arguments:
	selected filter: A number assigned to the filter. Use 'filters' before
		to find the correct number.
'''
		if self.check_plotmode():
			return
		ind = int(arg) - 1
		if ind < 0 or ind >= len(self.filters):
			print('No valid filter: ' + str(ind + 1))
			return
		del self.filters[ind]
	def do_select(self, arg):
		'''Usage: select <system|plugingroup>

Select specific datasets from the database. This command will show you a list
of possibilities from which you can choose multiple items. The selected items
will then be added to the filters and can be removed with filter_rm.

Arguments:
	system: Select systems that should be used.
	plugingroup: Select specific plugin groups. A plugin group contains all
		plugins that ran at the same time.
'''
		if self.check_plotmode():
			return
		if arg == 'system':
			res = db_get_systems(self.db, self.filters)
			ct = 1
			systems = []
			for row in res:
				sys_desc = "CPUs:{:3d} Mem:{:6.0f}Mb Kernel: {:s} Arch: {:s} Info: {:s}".format(
						row['nr_cpus'],
						row['mem_total'] / 1000000.0,
						row['kernel'], row['machine'],
						row['custom_info'])
				print(indent(1) + "{:3d}: ".format(ct) + sys_desc + " nr runs: {:3d}".format(row['number_runs']))
				systems.append(row['system_sha'])
				ct += 1
			selected = input_get_selected("Select systems: ", systems, ct)
			if len(selected) > 0:
				self.filters.append(filter("system", ' OR '.join(["system.system_sha = '" + i + "'" for i in selected]), desc = "system selection"))
		elif arg == 'plugingroup':
			res = db_get_plugin_groups(self.db, self.filters)
			grps = {}
			for row in res:
				ind = row['plugin_group_sha']
				if ind not in grps:
					grps[ind] = ''
				grps[ind] += ' ' + row['module'] + '.' + row['name'] + '@' + row['version']

			ct = 1
			shas = []
			for k,v in grps.items():
				print(indent(1) + "{:3d}: ".format(ct) + v)
				shas.append(k)
				ct += 1
			selected = input_get_selected("Select plugin groups: ", shas, ct)
			if len(selected) > 0:
				self.filters.append(filter("plugin_group", ' OR '.join(["plugin_group.plugin_group_sha = '" + i + "'" for i in selected]), desc = "plugin group selection"))
	def complete_select(self, text, line, begidx, endidx):
		cmpls = [['system', 'plugingroup']]#TODO for future release: , 'system_cpu', 'cpu', 'option', 'pluginversion']]
		return match_line(cmpls, text, line, begidx, endidx)
	def do_nr_runs(self, arg):
		'''Usage: nr_runs

Display the number of runs selected. This is not equivalent to the number of
results, number of runs with results or number of plots.
'''
		res = db_generic_run_exec(self.db, self.filters, None)
		row = res.fetchone()
		print("Number of unique runs filtered: " + str(row['number_runs']))

	def do_line_chart_def_levels(self, arg):
		'''Usage: line_chart_def_levels <level1> <level2> <level3> <level4>

This command sets the default hierarchy of different run options. level1 is at
the top of the hierarchy, level4 at the bottom. It is more likely that level1
is represented as directory hierarchy, while level4 will most likely be
represented through different colors in the plot. But the specific plots will
be determined after plots_create.

This command does not have any influence on generated plots.

Arguments: Each level can be one value of the following:
	system: Different systems will divide the hierarchy here.
	option: All options, typical multiple levels.
	version: All component versions.
	data: Different data fields.
'''
		levels = arg.split(' ')
		if len(levels) != 4:
			print("Please define all 4 levels.")
			return
		options = ['system', 'option', 'version', 'data']
		for l in levels:
			if l not in options:
				print('Unknown level "' + l + '"')
				return
		self.barplotlevels = levels
	def complete_line_chart_def_levels(self, text, line, begidx, endidx):
		cmpls = [['system', 'option', 'version', 'data'] for i in range(3)]
		return match_line(cmpls, text, line, begidx, endidx)
	def do_bar_chart_def_levels(self, arg):
		'''Usage: bar_chart_def_levels <level1> <level2> <level3> <level4>

This command sets the default hierarchy of different run options. level1 is at
the top of the hierarchy, level4 at the bottom. It is more likely that level1
is represented as directory hierarchy, while level4 will most likely be
represented through different colors in the plot. But the specific plots will
be determined after plots_create.

In difference to line charts, bar charts can represent up to 3 levels in one
plot.

This command does not have any influence on generated plots.

Arguments: Each level can be one value of the following:
	system: Different systems will divide the hierarchy here.
	option: All options, typical multiple levels.
	version: All component versions.
	data: Different data fields.
'''
		levels = arg.split(' ')
		if len(levels) != 4:
			print("Please define all 4 levels.")
			return
		options = ['system', 'option', 'version', 'data']
		for l in levels:
			if l not in options:
				print('Unknown level "' + l + '"')
				return
		self.lineplotlevels = levels
	def complete_bar_chart_def_levels(self, text, line, begidx, endidx):
		cmpls = [['system', 'option', 'version', 'data'] for i in range(3)]
		return match_line(cmpls, text, line, begidx, endidx)
	def do_plots_reset(self, arg):
		'''Usage: plots_reset

Reset all generated plots. This will discard all changes you made to the plots.
'''
		self.plotmode = False
		self.plotgrps = []
		self.prompt = "data  > "
	def do_plots_create(self, arg):
		'''Usage: plots_create

Generate all plots based on the filters. This generator will group together same
plugin groups with different versions, options and systems. This tool is not
able to group different plugin groups in one plot.

After using this command, changing filters will have no effect.
'''
		if self.check_plotmode():
			return
		self.plotmode = True
		self.plotgrps = plotgrps_generate(self.db, self.filters, self.default_levels)
		self.prompt = "plots > "
	def do_plots(self, arg):
		'''Usage: plots

List all plots with their options.

This command does not take any arguments.

Information printed:
	Properties (Can be changed with plot_set_property)
		xtitle: Title of the X-Axis
		ytitle: Title of the Y-Axis
		title: Title of the plot
	Parameter path (Can be altered with plot_autoremove and plot_squash)
		The parameter path shows the different levels of hierarchy from
		the root to the leafs. Each parameter can have multiple
		different values, which are shown indented below the parameter.
		All parameters with only one possible value can be removed
		by using plot_autoremove.
		bar charts can show up to 3 levels, beginning at the leafs,
		line charts can only show 1 level.
		You can use plot_squash and plot_autosquash to reduce the
		number of levels and increase the amount of data in one chart.
		All levels above that will be represented as file/directory
		hierarchy. If you do not remove the one-value parameters, the
		resulting plots/directories will also contain this single line.
'''
		for i in range(len(self.plotgrps)):
			print("Group " + str(i+1))
			for j in range(len(self.plotgrps[i])):
				print(str(i+1) + '.' + str(j+1),end = "")
				self.plotgrps[i][j].print()
	def get_selected_plots(self, args):
		if args == None or len(args) == 0:
			print("No plot selected")
			return []
		if '*' in args:
			r = []
			for grp in self.plotgrps:
				r += grp
			return r
		selected = set()
		for i in args:
			grpid,_,plotid = i.partition('.')
			if grpid == '' or plotid == '':
				print("Failed to parse " + i)
				return []
			try:
				grpid = int(grpid) - 1
			except:
				print("Failed to parse " + i)
				return []

			if grpid < 0 or grpid >= len(self.plotgrps):
				print("Groupid out of range: " + i)
				return []

			if plotid == '*':
				selected |= self.plotgrps[grpid]
				continue

			try:
				plotid = int(plotid) - 1
			except:
				print("Failed to parse " + i)
				return []
			if plotid < 0 or plotid >= len(self.plotgrps[grpid]):
				print("Plot id out of range: " + i)
				return []
			selected.add(self.plotgrps[grpid][plotid])
		return list(selected)
	def do_plot_autoremove(self, arg):
		'''Usage: plot_autoremove <plot id>

Autoremove parameters from the partameter hierarchy with only one value each.
It is recommended to use this command because it removes unnecessary levels
from the hierarchy.

Arguments:
	plot id: An identifier for a plot. Possible is: '*', 'plotgroup.*' 
		and 'plotgroup.plot', where plotgroup is the number of the group
		and plot the number of the plot within a group. See 'plots' for
		the numbers.
'''
		args = arg.split()
		sel = self.get_selected_plots(args)
		for i in sel:
			i.autoremove()
	def do_plot_squash(self, arg):
		'''Usage: plot_squash <plot id> <levelname1> <levelname2> [<levelname3> ...]

Squash multiple levels of the parameter hierarchy into one. All levelname
arguments will afterwards be represented in only one level.

Arguments:
	plot id: An identifier for a plot. Possible is: '*', 'plotgroup.*' 
		and 'plotgroup.plot', where plotgroup is the number of the group
		and plot the number of the plot within a group. See 'plots' for
		the numbers.
	levelnameX: A name of a level. One level can have multiple names after a
		squash, so you only have to define one of them here.
'''
		args = arg.split()
		sel = self.get_selected_plots([args[0]])
		for i in sel:
			i.squash_levels(args[1:])
	def do_plot_autosquash(self, arg):
		'''Usage: plot_autosquash <plot id> <level#values> [<level#values> ...]

Automatically try to squash levels of the parameter hierarchy beginning at the
leafs. It will not change the order of the parameter hierarchy.

Note: plot_autoremove should be called before this command. Else there may be
integrated parameters with only one value that can't be removed.

Arguments:
	plot id: An identifier for a plot. Possible is: '*', 'plotgroup.*',
		'plotgroup.plot', 'all_line', 'all_bar', where plotgroup
		is the number of the group and plot the number of the
		plot within a group. See 'plots' for the numbers. 'all_line' and
		'all_bar' will apply this for all bar or line charts.
	level#values: Maximum number of values created through a squash. This
		is equivalent to the number of colors/bars etc. of your plot,
		depending on the chart type.
'''
		args = arg.split()
		if len(args) == 0:
			print("Arguments required")
			return
		if args[0] in ['all_line', 'all_bar']:
			sel = self.get_selected_plots('*')
		else:
			sel = self.get_selected_plots([args[0]])
		try:
			max_values = [ int(i) for i in args[1:]]
		except:
			print("Not a number")
			return
		if len(max_values) == 0:
			print("Missing level#values argument")
			return
		for i in sel:
			if args[0] == 'all_bar':
				i.bar_autosquash(max_values)
			elif args[0] == 'all_line':
				i.line_autosquash(max_values)
			else:
				i.autosquash_levels(max_values)
	def do_plot_replace_rule(self, arg):
		'''Usage: plot_replace_rule <plot id> <parameter id> <replacement>

Setup a replacement rule for names and values. Those replacement will be
applied when creating the plots.

Arguments:
	plot id: An identifier for a plot. Possible is: '*', 'plotgroup.*' 
		and 'plotgroup.plot', where plotgroup is the number of the group
		and plot the number of the plot within a group. See 'plots' for
		the numbers.
	parameter id: Original parameter name/value as displayed in 'plots'.
	replacement: Arbitrary string that may include whitespaces. Do not use
		any quotes to include whitespaces.
'''
		args = arg.split()
		if len(args) == 0:
			print("Arguments required")
			return
		sel = self.get_selected_plots([args[0]])
		for i in sel:
			i.add_replace_rule(args[1], ' '.join(args[2:]))
	def do_plot_set_property(self, arg):
		'''Usage: plot_set_property <plot id> <property> <value>

Set a plot property.

Arguments:
	plot id: An identifier for a plot. Possible is: '*', 'plotgroup.*' 
		and 'plotgroup.plot', where plotgroup is the number of the group
		and plot the number of the plot within a group. See 'plots' for
		the numbers.
	property: The property you want to set.
	value: Arbitrary string
'''
		args = arg.split()
		if len(args) == 0:
			print("Arguments required")
			return
		sel = self.get_selected_plots([args[0]])
		for i in sel:
			i.add_property(args[1], ' '.join(args[2:]))
	def do_plot_generate(self, arg):
		'''Usage: plot_generate <plot id> <property> <value>

Generate and save one or multiple plots. This will create the final file and
put it in the appropriate directory. All plots are created in parallel.

Arguments:
	plot id: An identifier for a plot. Possible is: '*', 'plotgroup.*' 
		and 'plotgroup.plot', where plotgroup is the number of the group
		and plot the number of the plot within a group. See 'plots' for
		the numbers.
'''
		args = arg.split()
		if len(args) == 0:
			print("Arguments required")
			return
		sel = self.get_selected_plots([args[0]])
		for i in sel:
			i.plot()
		for i in sel:
			i.plot_wait()

	def do_EOF(self, arg):
		return True

cmdline(db).cmdloop()

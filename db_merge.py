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
import os
import sqlite3
import sys

parser = argparse.ArgumentParser()

parser.add_argument('outdb')
parser.add_argument('indb', nargs='+')
parser.add_argument('-f', '--force', default=False, action="store_true", help="Force overwriting of outdb if it is existing. Data in outdb will be lost")

parsed = parser.parse_args()

if os.path.exists(parsed.outdb):
	if parsed.force:
		os.unlink(parsed.outdb)
	else:
		print("Error: Target database " + parsed.outdb + " already exists and overwrite was not forced.")
		sys.exit(-1)

db = sqlite3.connect(parsed.outdb)
db.row_factory = sqlite3.Row

for i in parsed.indb:
#	db.execute("BEGIN TRANSACTION;")
	db.execute("ATTACH '" + i + "' AS tomerge;")

	res = db.execute("SELECT name, sql FROM tomerge.sqlite_master WHERE type = 'table' AND name not in (SELECT name FROM sqlite_master WHERE type='table');")
	for row in res:
		print(row['name'])
		db.execute(row['sql'])

	res = db.execute("SELECT name FROM tomerge.sqlite_master WHERE type = 'table';")
	for row in res:
		db.execute('INSERT OR IGNORE INTO "' + row['name'] + '" SELECT * FROM tomerge."' + row['name'] + '";')

	db.execute("DETACH tomerge;")
#	db.execute("END TRANSACTION;")

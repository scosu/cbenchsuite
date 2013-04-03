
Database
========

All information are stored within a sqlite3 database. It is easy to extend the
cbenchsuite executable to other database backends, but the python plotter
exclusively works with sqlite at the moment.

Merge databases
---------------

When executing cbenchsuite on different systems, there will be created multiple
databases. To plot all data from multiple databases, you first have to merge
them. A merge is easy to do with the generic sqlite3 database merger.

	./db_merge.py target_db.sqlite source1.sqlite source2.sqlite

This command will merge databases `source1.sqlite` and `source2.sqlite` into a
new database `target_db.sqlite`. The target database file must not exist. The
command will never overwrite any database without `--force`. Be aware that
`--force` will first delete a possibly existing database and then create a
fresh one.

The merged database is then ready to be used for plotting.

Database structure
------------------

TODO

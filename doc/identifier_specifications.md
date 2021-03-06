Identifier specification
========================

There are different identifiers used in pbenchsuite.

Versions may not contain any of these character strings:
'__', ':', ';', '@'

Names additionally may not have:
'.'


Version string
--------------

Version strings should be mapped to numbers. If pbenchsuite cannot
parse .-splitted strings as integers, it will fallback to compare
strings by their alphabetical order. It will not try to guess the
version hierarchy.

In general version strings are:
MAJOR.MINOR.BUGFIX.[...]

	VERSION ::= VERSION_LIMITER VERSION
	VERSION ::= VERSION_LIMITER
	VERSION_LIMITER ::= '@' VERSION_COMPONENT '<' VERSION_STRING
	VERSION_LIMITER ::= '@' VERSION_COMPONENT '<=' VERSION_STRING
	VERSION_LIMITER ::= '@' VERSION_COMPONENT '>' VERSION_STRING
	VERSION_LIMITER ::= '@' VERSION_COMPONENT '>=' VERSION_STRING
	VERSION_LIMITER ::= '@' VERSION_COMPONENT '!=' VERSION_STRING
	VERSION_LIMITER ::= '@' VERSION_COMPONENT '=' VERSION_STRING
	VERSION_COMPONENT is the plugin internal component name for the version
		limiter. This may be the version of used programs like bzip2, gzip
		and so on.
	VERSION_STRING is the version propagated by the plugins. pbenchsuite
		processes all version comparisons from left to right. That means you can
		specify a major version for example this way: '=3', instead of '>=3.0.0.0;<4.0.0.0'

Plugin identifier
-----------------

	PLUGIN_ID ::= MODULE_NAME '.' PLUGIN_NAME PLUGIN_VERSION
	PLUGIN_ID ::= PLUGIN_NAME # Use this with care. There may be several roles a plugin can have
	PLUGIN_VERSION ::= ''
	PLUGIN_VERSION ::= VERSION

Option
------

	OPTIONS ::= OPTIONS ':' OPTION
	OPTIONS ::= OPTION
	OPTION ::= OPTION_NAME '=' OPTION_VALUE

Plugin run context
------------------

	RUNCTXT ::= PLUGIN_ID ':' OPTIONS
	RUNCTXT ::= PLUGIN_ID

Plugin runcombination
---------------------

Plugins executed in parallel.

	RUNCOMBO ::= RUNCOMBO ';' RUNCTXT
	RUNCOMBO ::= RUNCTXT

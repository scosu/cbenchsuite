#!/bin/bash

if [ $# -ne 2 ]
then
	echo "USAGE: source_config target_header"
	exit
fi

config=$1

mkdir -p `dirname $2`

(
echo "#ifndef _CBENCH_CONFIG_H_"
echo "#define _CBENCH_CONFIG_H_"
cat "$config" | sed -e "s/^#/\/\//g;s/^CONFIG_/#define CONFIG_/g;s/=/ /g"
echo "#endif  /* _CBENCH_CONFIG_H_ */"
) > "$2"

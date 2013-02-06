#!/bin/bash

mod_dir=$1
tgt_file=$2

mods=`find "$mod_dir" -mindepth 2 -maxdepth 2 -type f -name Kconfig`

(
for i in $mods
do
	echo "source \"$i\""
done
) > "$tgt_file"


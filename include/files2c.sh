#!/bin/sh
filename=$1
shift
prefix=$1
shift

echo "// AUTOMATICALLY GENERATED FILE" >$filename

for a in $*; do
	buf=$(echo $a|cut -f 1 -d '.')
	name=$(echo $buf|sed "s/_/ /g")
    echo "// GENERATED FROM $a" >>$filename
	./file2c $a "static unsigned char const "$buf"[]={" "};"  >>$filename
	echo >>$filename
	names="$names \"$name\","
	bufs="$bufs $buf,"
done

if test -n "$prefix"; then
    echo "static unsigned char const *"$prefix"_pointers[]={$bufs NULL};" >>$filename
    echo "static char const *"$prefix"_names[]={$names NULL};" >>$filename
fi
echo >>$filename

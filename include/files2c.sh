#!/bin/sh
filename=$1
shift
prefix=$1
shift

echo >$filename

for a in $*; do
	buf=$(echo $a|cut -f 1 -d '.')
	name=$(echo $buf|sed "s/_/ /g")
	./file2c $a "static guint8 "$buf"[]={" "};"  >>$filename
	echo >>$filename
	names="$names \"$name\","
	bufs="$bufs $buf,"
done

if test -n "$prefix"; then
    echo "static const guint8 *"$prefix"_pointers[]={$bufs NULL};" >>$filename
    echo "static const char *"$prefix"_names[]={$names NULL};" >>$filename
fi
echo >>$filename

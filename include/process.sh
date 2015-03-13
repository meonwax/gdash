#!/bin/sh
echo > levels.h

for a in $*; do
	buf=$(echo $a|cut -f 1 -d '.')
	name=$(echo $buf|sed "s/_/ /g")
	./file2c $a "guint8 $buf [] = {" "};"  >>levels.h
	echo >>levels.h
	names="$names \"$name\","
	bufs="$bufs $buf,"
done

echo "static const guint8 *level_pointers[]={$bufs NULL};" >>levels.h
echo "static const char *level_names[]={$names NULL};" >>levels.h


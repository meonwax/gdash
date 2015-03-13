#!/bin/bash
SUBDIRS=$(find . -type d -printf "%f\n"|grep -v "\.$")
echo "# Automatically generated Makefile.am! Check $0" >Makefile.am
echo >>Makefile.am

for a in $SUBDIRS; do
    CAVES=$(find $a -type f -name \*.gds)
    echo ${a}dir = '$(pkgdatadir)'/${a} >> Makefile.am
    echo ${a}_CAVES = $CAVES >>Makefile.am
    DIST=$DIST\ \$\(${a}_CAVES\)
    echo ${a}_DATA = \$\(${a}_CAVES\) >>Makefile.am
    echo >>Makefile.am
done
echo EXTRA_DIST = $0 $DIST >>Makefile.am


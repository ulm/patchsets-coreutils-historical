#!/bin/bash

if [[ $# -lt 2 ]] && [[ $# -gt 3 ]] ; then
	echo "Usage: $0 <coreutils ver> <patch ver>"
	exit 1
fi
cver=$1
pver=$2

if [[ ! -d ./${cver} ]] ; then
	echo "Error: ${cver} is not a valid coreutils ver"
	exit 1
fi

rm -rf tmp
rm -f coreutils-${cver}-*.tar.bz2

mkdir -p tmp/patch/excluded
cp -r ${cver}/* tmp/patch/
find tmp/patch -name CVS -type d | xargs rm -r

tar -jcf coreutils-${cver}-patches-${pver}.tar.bz2 \
	-C tmp patch || exit 1
rm -rf tmp

du -b coreutils-${cver}-*.tar.bz2
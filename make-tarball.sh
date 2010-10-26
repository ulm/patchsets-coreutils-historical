#!/bin/bash

if [[ $# -ne 2 && $# -ne 1 ]] ; then
	echo "Usage: $0 <coreutils ver> [patch ver]"
	exit 1
fi
cver=$1
pver=$2

if [[ ! -d ./${cver} ]] ; then
	echo "Error: ${cver} is not a valid coreutils ver"
	exit 1
fi

if [[ -z ${pver} ]] ; then
	if [[ ! -e ./${cver}/README.history ]] ; then
		echo "Error: README.history does not exist for ${cver}"
		exit 1
	fi
	pver=$(awk '{print $1; exit}' ${cver}/README.history)
	if [[ -n ${pver} ]] ; then
		echo "Autoguessing patch ver as '${pver}' from ${cver}/README.history"
	else
		echo "Error: cannot extract version from ${cver}/README.history"
		exit 1
	fi
fi

rm -rf tmp
rm -f coreutils-${cver}-*.tar.xz

mkdir -p tmp/patch/excluded
cp -r ../README* ${cver}/* tmp/patch/
find tmp/patch -name CVS -type d | xargs rm -rf

tar -C tmp patch -cf - | xz > coreutils-${cver}-patches-${pver}.tar.xz || exit 1
rm -rf tmp

du -b coreutils-${cver}-*.tar.xz

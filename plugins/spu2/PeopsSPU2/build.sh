#!/bin/sh

curdir=`pwd`

echo ------------------
echo Building PeopsSPU2
echo ------------------

make $@

if [ $? -ne 0 ]
then
exit 1
fi

cp libspu2Peops*.so* ${PCSX2PLUGINS}

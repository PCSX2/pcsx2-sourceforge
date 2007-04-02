#!/bin/sh

echo ---------------
echo Building Pcsx2 
echo ---------------

if [ $# -gt 0 ] && [ $1 = "all" ]
then

aclocal
automake
autoconf
./configure ${PCSX2OPTIONS}
make clean
make install

else

make $@

#if [ $? -ne 0 ]
#then
#exit 1
#fi

#if [ $# -eq 0 ] || [ $1 != "clean" ]
#then
#make install
#fi

fi

if [ $? -ne 0 ]
then
exit 1
fi

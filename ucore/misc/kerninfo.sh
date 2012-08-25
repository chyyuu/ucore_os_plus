#!/bin/sh

UNAME=`uname`
if [ "$UNAME" = "Linux" ]; then
	size=$(stat -c%s "${T_OBJ}/kern-bin")
elif [ "$UNAME" = "FreeBSD" ]; then
	size=$(stat -f %z "${T_OBJ}/kern-bin")
else
	echo "Unsupported platform!"
	exit 1
fi

kern_sect_size=$(echo "(( $size + 511 ) / 512)" | bc)
echo $kern_sect_size > ${T_OBJ}/kern-sect_size
rm -f ${T_OBJ}/bootloader-*

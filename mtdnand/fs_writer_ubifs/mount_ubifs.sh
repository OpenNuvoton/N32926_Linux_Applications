#!/bin/sh

PAGESIZE=2048
#UBIFSIMGPATH=../../../imgmaker_x86/ubifs/foo_ubifs.img
UBIFSIMGPATH=./foo_ubifs.img

echo "mtdutils/flash_erase"                             
./mtdutils/flash_erase /dev/mtd3 0 0
echo "mtdutils/ubiformat"                             
if ./mtdutils/ubiformat /dev/mtd3 -O $PAGESIZE -s $PAGESIZE -f $UBIFSIMGPATH > /dev/null 2>&1; then
	echo "mtdutils/ubiattach"
	if ./mtdutils/ubiattach /dev/ubi_ctrl -m 3 > /dev/null 2>&1; then
		echo "mtdutils/ubimkvol"
		mkdir -p /mnt/ubifs
		echo "mount"
		if mount -t ubifs ubi0:foo /mnt/ubifs > /dev/null 2>&1; then
			echo "Success!!"
			cat /mnt/ubifs/helloworld.txt
			exit 0
		fi
	fi
fi

./mtdutils/ubidetach -m 3

#!/bin/sh

mkdir /mnt/yaffs2
./mtdutils/flash_erase /dev/mtd3 0 0
if ./mtdutils/nandwrite -a -m /dev/mtd3 foo_yaffs2.img > /dev/null; then
	if mount -t yaffs2 -o"inband-tags" /dev/mtdblock3 /mnt/yaffs2; then
		echo "Success"
		cat /mnt/yaffs2/hi.txt
		exit 0
	fi
fi

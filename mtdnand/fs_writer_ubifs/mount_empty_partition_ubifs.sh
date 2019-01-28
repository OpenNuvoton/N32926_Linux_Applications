#!/bin/sh

PAGESIZE=2048
PartitionSize=60MiB

./mtdutils/flash_erase /dev/mtd3 0 0
echo "mtdutils/ubiformat"                             
if ./mtdutils/ubiformat /dev/mtd3 -O $PAGESIZE -s $PAGESIZE > /dev/null 2>&1; then
	echo "mtdutils/ubiattach"
	if ./mtdutils/ubiattach /dev/ubi_ctrl -m 3 > /dev/null 2>&1; then
		echo "mtdutils/ubimkvol"
		if ./mtdutils/ubimkvol /dev/ubi0 -N test_volume -s $PartitionSize > /dev/null 2>&1; then
			if [ ! -d "/mnt/ubifs" ]; then
				mkdir /mnt/ubifs
			fi
			if mount -t ubifs ubi0:test_volume /mnt/ubifs > /dev/null 2>&1; then
				echo "Success!!"
				exit 0
			fi
		fi
	fi
fi
./mtdutils/ubidetach -m 3
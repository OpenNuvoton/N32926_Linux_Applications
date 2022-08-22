#!/bin/sh

#This is en example for 2K+64 nand flash
#dev:    size   erasesize  name
#mtd0: 00080000 00020000 "SYSTEM"	(  512 KB )
#mtd1: 00f80000 00020000 "EXECUTE"	( 15.5 MB )
#mtd2: 02000000 00020000 "NAND1-1"	(   32 MB )
#mtd3: 05000000 00020000 "NAND1-2"	(   80 MB )

PATH_SYSIMG=/tmp/system.img

if ./mtdinfo /dev/mtd0 > /tmp/system.info; then
   if cat /tmp/system.info | grep SYSTEM; then
        BLOCKSIZE=`awk '{if ($1=="Eraseblock") {print $3}}' /tmp/system.info`
        PAGESIZE=`awk '{if ($1=="Minimum") {print $5}}' /tmp/system.info`
        OOBSIZE=`awk '{if ($1=="OOB") {print $3}}' /tmp/system.info`

        echo "BS: " $BLOCKSIZE 
        echo "PS: " $PAGESIZE 
        echo "OS: " $OOBSIZE
   else
        echo "Can't found MTD-SYSTEM INFO"
	exit 1
   fi
else
	echo "FAILED ON MAKING MTD-SYSTEM INFO"
	exit 1
fi

if ! ./mtdinfo /dev/mtd1 > /tmp/execute.info; then
	echo "FAILED ON MAKING MTD-EXECUTE INFO"
	exit 1
fi

if ! cat /tmp/execute.info | grep EXECUTE; then
        echo "Can't found MTD-EXECUTE INFO"
	exit 1
fi


echo "./mn_writer -x $PATH_SYSIMG -f ./NANDWRITER.ini -t ./TurboWriter_U6DN.ini -b $BLOCKSIZE -p $PAGESIZE -o $OOBSIZE"
if ./mn_writer -x $PATH_SYSIMG -f ./NANDWRITER.ini -t ./TurboWriter_U6DN.ini -b $BLOCKSIZE -p $PAGESIZE -o $OOBSIZE; then
	echo "Success!!"
else
	echo "Failed on writing images"
	exit 1
fi
exit 0

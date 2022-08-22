#!/bin/bash

# 128 MB, 2048+64, page per block: 64, block size :128KB
# MTD NAND Partition layout
# Partition 0: 4 Block		offset: 0, 			size: 0x80000
# Partition 1: (16MB)		offset: 0x80000,	size: 0x1000000
# Partition 2: (32MB)		offset: 0x1080000,	size: 0x2000000
# Partition 3: (to FULL)	offset: 0x3080000,	size: 0x4F80000
# Partition 4: (1MB)		offset: 0xF00000,	size: 0x100000


#[Usage] ./GoldenImgMaker
#	-v ....: version
#	-O ....: Output folder path
#	-p ....: page size
#	-o ....: OOB size
#	-s ....: page per block
#	-b ....: block number
#	-t ....: TurboWriter.ini path
#	-P "<ImgFilePath> [parameters]"

#	[parameters]
#		-t ....: Image type. Data:0, Exec:1, ROMFS:2, Sys:3, Logo:4
#		-o ....: Redundent area size.(option)\n"
#		-S ....: MTD partition entry size.
#		-O ....: MTD partition entry offset.
#		-j ....: Won't determine parity code if all 0xff in a page.
#		-b ....: BCH error bits(4, 8, 12, 15, 24).
#		-e ....: Execution address.

#[Example] ./GoldenImgMaker -o ./skyeye_golden -p 2048 -o 64 -s 64 -b 1024
#				-P "./NandLoader_240.bin -t 3 -S 0x80000 -O 0 -e 0x900000 -j 0 " 
#				-P "./conprog.bin -t 1 -O 0x80000 -S 0x1000000 -e 0x0 -j 0 " 
#				-P "./mtdnand_ubi_nand1.img -t 0 -O 0x1080000 -S 0x1000000 -e 0x0 -j 1" 


if ./Src/GoldenImgMaker -O ../golden_imgmaker_x86/golden_ubi -t ../golden_imgmaker_x86/TurboWriter_U6DN.ini -p 2048 -o 64 -s 64 -b 1024 \
                        -P "../golden_imgmaker_x86/N32926_NANDLoader_240MHz_Fast_Logo.bin -t 3 -b 4 -O 0 -S 0x80000 -e 0x900000" \
			-P "../golden_imgmaker_x86/NuvotonLogo_320x240.bin -t 4 -b 8 -O 0xF00000 -S 0x100000 -e 0x500000" \
                        -P "../../../image/conprog.bin -t 1 -b 8 -O 0x80000 -S 0x1000000" \
                        -P "../fs_writer_ubifs/example_image/foo_ubifs.img -t 0 -b 8 -O 0x1080000 -S 0x2000000 -j"; then

	echo "========================================================================="
	echo "All golden images are created."
	echo "All parity code in spare area are produced by GoldenImgMaker."
	echo "Now, Please put these files into memory by your NAND programmer."
	echo "========================================================================="
fi

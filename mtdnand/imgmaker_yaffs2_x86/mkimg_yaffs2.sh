#!/bin/sh

#mkyaffs2image: image building tool for YAFFS2 built Jan  2 2014
#Missing <dir> and/or <image.yaffs2> argument
#Usage: mkyaffs2image [Options] <dir> <image_file>
#           <dir>          the directory tree to be converted
#           <image.yaffs2> the output file to hold the image
#Options:
#  -c | --convert          produce a big-endian image from a little-endian machine
#  -p | --page-size        Page size of target NAND device [2048]
#  -o | --oob-size         OOB size of target NAND device [64]
#  -b | --block-size       Block size of target NAND device [0x20000]
#  -i | --inabnd-tags      Use Inband Tags
#  -n | --no-pad           Do not pad to end of block

if [ ! -d ./foo ]; then
	mkdir -p ./foo
fi

echo "hello world" > ./foo/helloworld.txt

# Make an YAFFS2 image with inband-tags (blocksize=128KB, pagesize=2048, oobsize=64)
./mkyaffs2image -i ./foo ./foo_yaffs2.img

# Make an YAFFS2 image with inband-tags (blocksize=1MB, pagesize=4096, oobsize=224)
# ./mkyaffs2image -b 0x100000 -p 4096 -o 224 -i ./foo ./foo_yaffs2.4k.img

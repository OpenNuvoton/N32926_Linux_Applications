#!/bin/bash

if [ ! -f "../../../image/conprog.bin" ]; then
	echo "The conprog.bin is non-exist."
        echo "Please make sure the file existence."
	exit 1
fi

cd Src
make
cd ..
if ! ./GoldenImgMaker_UBI.sh; then
       echo "Fail on producing UBI golden images."
else
	ls golden_ubi -al
fi

if ! ./GoldenImgMaker_YAFFS2.sh; then
       echo "Fail on producing YAFFS2 golden images."
else
	ls golden_yaffs2 -al
fi

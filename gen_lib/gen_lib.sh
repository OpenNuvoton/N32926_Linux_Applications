#!/bin/sh

genromfs -f lib_romfs.bin -d lib

/bin/cp lib_romfs.bin ../../image/


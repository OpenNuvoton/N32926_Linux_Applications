Date: 2018/9/27

tslib-1.1 is from the website
https://github.com/kergoth/tslib

To build in arm-linux environment,
1. $ ./configure --host=arm-linux --enable-shared=no --enable-static --enable-input=static --enable-linear=static --enable-dejitter=static --enable-pthres=static --enable-variance=static --disable-linear-h2200 --disable-cy8mrln-palmpre --disable-corgi --disable-collie --disable-h3600 --disable-arctic2 --disable-dmc --disable-mk712 --disable-tatung --disable-touchkit --disable-ucb1x00 --disable-galax --prefix=$PWD/_install

2. $ make AM_LDFLAGS=-all-static. To build a totally statically linked executables, need to build with libtool by passing -all-static via AM_LDFLAGS.

3. $ make install-strip. The files will be copied to path specified by "--prefix" in configure. But actually only ts_calibrate, ts_XXXX in bin and ts.conf in etc are required.

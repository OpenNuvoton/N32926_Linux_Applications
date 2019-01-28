The sample code for faraday H.264 codec:

1. Modify h264.h to assign the proper value
2. "make clean" and "make" to build h264dec & h264enc application

===============================================

pattern/xxx.264   : H.264 bitstream 
info/xxx.txt      : xxx.264 frame length list

===============================================
Usage:

#./h264enc          // Encode VideoIn port-1(QVGA) bitstream
#./h264dec          // playback H.264 bitstream in ..\pattern folder

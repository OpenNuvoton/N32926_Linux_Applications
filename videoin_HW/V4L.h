/* V4L.h
 *
 *
 * Copyright (c)2008 Nuvoton technology corporation
 * http://www.nuvoton.com
 *
 * V4L header file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#ifndef __V4L_H__
#define __V4L_H__

#include <stdint.h>

// Defined Error Code in here
#ifndef ERRCODE
#define ERRCODE int32_t
#endif

typedef struct{
	unsigned int u32RemainBufSize;
	unsigned int u32RemainBufPhyAdr;
}S_BUF_INFO;

typedef struct 
{
	int32_t i32PipeBufNo;
	int32_t i32PipeBufSize;
	int32_t i32CurrPipePhyAddr;
	int32_t i32CurrBufIndex;
}S_PIPE_INFO;

#define V4L_ERR					0x80400000							
#define ERR_V4L_SUCCESS				0
#define ERR_V4L_OPEN_DEV			(V4L_ERR | 0x1)
#define ERR_V4L_VID_GET_CAP			(V4L_ERR | 0x2)
#define ERR_V4L_VID_GRAB_DEV			(V4L_ERR | 0x3)
#define ERR_V4L_VID_SET_PICT			(V4L_ERR | 0x4)
#define ERR_V4L_VID_SET_WIN			(V4L_ERR | 0x5)
#define ERR_AVDEV_VID_CAPTURE			(V4L_ERR | 0x6)
#define ERR_V4L_MMAP				(V4L_ERR | 0x7)
#define ERR_V4L_VID_MCAPTURE			(V4L_ERR | 0x8)
#define ERR_V4L_VID_SYNC			(V4L_ERR | 0x9)
#define ERR_V4L_VID_CAPTURE			(V4L_ERR | 0xA)
#define ERR_V4L_VID_QUERY_UID		(V4L_ERR | 0xB)


/* Sensor CTRL */
#define REG_SDE_CTRL		0x6A
#define REG_BRIGHT		0x9B


#define VIDIOCGCAPTIME				_IOR('v',30,struct v4l2_buffer)      /*Get Capture time */
#define VIDIOCSPREVIEW  			_IOR('v',43, int)

#define VIDIOC_G_DIFF_OFFSET		_IOR('v',48, int)		/* Get diff offset address and size */ 
#define VIDIOC_G_DIFF_SIZE			_IOR('v',49, int)
#define VIDIOC_S_MOTION_THRESHOLD	_IOW('v',50, int)		/* Set motion detection threshold */

#define VIDIOC_QUERY_SENSOR_ID		_IOR('v',51, int)
#define	VIDIOC_G_BUFINFO			_IOR('v',52, S_BUF_INFO)

#define VIDIOC_G_PACKET_INFO		_IOR('v',53, S_PIPE_INFO)			/* Get Packet offset address and size */ 
#define VIDIOC_G_PLANAR_INFO		_IOR('v',54, S_PIPE_INFO)			/* Get Planar offset address and size */ 
#define VIDIOC_S_MAP_BUF			_IOW('v',55, int)					/* Specified the mapping buffer */



#define VIDEO_START	0
#define VIDEO_STOP	1

ERRCODE
InitV4LDevice(
	E_IMAGE_RESOL eImageResol
);

ERRCODE
ReadV4LPicture(
	uint8_t **ppu8PicPtr,
	uint64_t *pu64TimeStamp,
	uint32_t *pu32BufPhyAddr
);

ERRCODE
TriggerV4LNextFrame(void);

void FinializeV4LDevice();

ERRCODE
StartV4LCapture(void);

ERRCODE
StopV4LCapture(void);

ERRCODE
SetV4LViewWindow(int lcmwidth, int lcmheight, int prewidth, int preheight);

ERRCODE
SetV4LEncode(int frame, int width, int height, int palette);


ERRCODE
StartPreview(void);

ERRCODE
StopPreview(void);

ERRCODE
QueryV4LUserControl(void);
ERRCODE
ChangeV4LUserControl_Brigness(int32_t ctrl);

ERRCODE
QueryV4LZoomInfo(struct v4l2_cropcap *psVideoCropCap, 
			struct v4l2_crop *psVideoCrop);
ERRCODE
QueryV4LSensorID(int32_t* pi32SensorId);

ERRCODE
Zooming(struct v4l2_cropcap *psVIDEOCROPCAP, 
	struct v4l2_crop *psVIDEOCROP,
	int32_t ctrl);

ERRCODE
MotionDetV4LSetThreshold(uint32_t u32Threshold);
ERRCODE
MotionDetV4LDumpDiffBuffer(void);

ERRCODE QueryV4LPacketInformation(S_PIPE_INFO* ps_packetInfo);
ERRCODE QueryV4LPlanarInformation(S_PIPE_INFO* ps_planarInfo);
//int32_t ChangeEncodeV4LResoultion(int32_t* pi32_width, int32_t* pi32_height);
void menuChangeParameter(int i32FBFd, 
						 	E_IMAGE_RESOL* pePreviewImgResol, 
							E_IMAGE_RESOL* peEncodeImgResol);

int dumpV4LBuffer(void);
#endif


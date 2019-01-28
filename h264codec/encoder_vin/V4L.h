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
Zooming(struct v4l2_cropcap *psVIDEOCROPCAP, 
	struct v4l2_crop *psVIDEOCROP,
	int32_t ctrl);

ERRCODE
MotionDetV4LSetThreshold(uint32_t u32Threshold);
ERRCODE
MotionDetV4LDumpDiffBuffer(void);


#endif


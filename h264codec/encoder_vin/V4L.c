/* V4L.c
 *
 *
 * Copyright (c)2008 Nuvoton technology corporation
 * http://www.nuvoton.com
 *
 * video for linux function
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <unistd.h>  
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <linux/videodev.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <errno.h>
#include <inttypes.h>


#include "Misc.h"
#include "V4L.h"

#define VID_DEVICE "/dev/video0"

#define VIDIOCGCAPTIME	_IOR('v',30,struct v4l2_buffer)      /*Get Capture time */
#define VIDIOCSPREVIEW                  _IOR('v',43, int)

#define VIDEO_START	0
#define VIDEO_STOP	1


typedef struct {
    int32_t i32VidFD;
    int32_t i32FrameFmt; /* see VIDEO_PALETTE_xxx */
    int32_t i32UseMMap;
    int32_t i32FrameSize;
    struct video_window sVidWin;
    uint8_t *pu8VidBuf;
    struct video_mbuf sVidMBufs; // gb_buffers;
    struct video_mmap sVidMMap; //gb_buf;
    int32_t i32VidFrame; //gb_frame;
} S_V4L_VID_DATA;

static S_V4L_VID_DATA s_sVidData = {
	.i32VidFD = -1,
	.i32UseMMap = 0,
	.i32VidFrame = 0,
};

static uint64_t s_u64PrevTS = 0;

ERRCODE
InitV4LDevice(
	E_IMAGE_RESOL eImageResol
)
{
	int i32VidFD;
	ERRCODE err = ERR_V4L_SUCCESS;
	struct video_capability sVidCap;
	struct video_picture sPict;
	int  i32Width, i32Height;



	i32VidFD = open(VID_DEVICE, O_RDWR);
	if (i32VidFD < 0) {
		ERR_PRINT("Can not open video device %s\n", strerror(errno));
		err = ERR_V4L_OPEN_DEV;
		goto fail;
	}
	if (ioctl(i32VidFD, VIDIOCGCAP, &sVidCap) < 0) {
		DEBUG_PRINT("VIDIOCGCAP: %s\n", strerror(errno));
		err = ERR_V4L_VID_GET_CAP;
		goto fail;
	}

	if (!(sVidCap.type & VID_TYPE_CAPTURE)) {
		DEBUG_PRINT("Fatal: grab device does not handle capture\n");
		err = ERR_V4L_VID_GRAB_DEV;
		goto fail;
	}

	if(eImageResol == eIMAGE_VGA){
		i32Width = 640;
		i32Height = 480;
	}
	else if (eImageResol == eIMAGE_WQVGA){
		i32Width = 480;
		i32Height = 272;
	}
	else{	//QVGA
		i32Width = 320;
		i32Height = 240;
	}

	ioctl(i32VidFD, VIDIOCGPICT, &sPict);
	sPict.palette = VIDEO_PALETTE_YUV422;
	sPict.depth= 16;
	if(ioctl(i32VidFD, VIDIOCSPICT, &sPict) < 0) {
		DEBUG_PRINT("VIDIOCSPICT: %s\n", strerror(errno));
		err = ERR_V4L_VID_SET_PICT;
		goto fail;
	}

	if (ioctl(i32VidFD, VIDIOCGMBUF, &s_sVidData.sVidMBufs) < 0) {
		/* try to use read based access */
		int val;

		s_sVidData.sVidWin.width = i32Width;
		s_sVidData.sVidWin.height = i32Height;
		s_sVidData.sVidWin.x = 0;
		s_sVidData.sVidWin.y = 0;
		s_sVidData.sVidWin.chromakey = -1;
		s_sVidData.sVidWin.flags = 0;

		if (ioctl(i32VidFD, VIDIOCSWIN, &s_sVidData.sVidWin) < 0) {
			DEBUG_PRINT("VIDIOCSWIN: %s\n", strerror(errno));
			err = ERR_V4L_VID_SET_WIN;
			goto fail;
		}

		s_sVidData.i32FrameFmt = VIDEO_PALETTE_YUV420P;

		val = 1;
		if (ioctl(i32VidFD, VIDIOCCAPTURE, &val) < 0) {
			DEBUG_PRINT("VIDIOCCAPTURE: %s\n", strerror(errno));
			err = ERR_AVDEV_VID_CAPTURE;
			goto fail;
		}

		s_sVidData.i32UseMMap = 0;
	} 
	else {
		s_sVidData.pu8VidBuf = mmap(0, s_sVidData.sVidMBufs.size, PROT_READ|PROT_WRITE, MAP_SHARED, i32VidFD, 0);
		if ((unsigned char*)-1 == s_sVidData.pu8VidBuf) {
			s_sVidData.pu8VidBuf = mmap(0, s_sVidData.sVidMBufs.size, PROT_READ|PROT_WRITE, MAP_PRIVATE, i32VidFD, 0);
			if ((unsigned char*)-1 == s_sVidData.pu8VidBuf) {
				DEBUG_PRINT("mmap: %s\n", strerror(errno));
				err = ERR_V4L_MMAP;
				goto fail;
			}
		}
		s_sVidData.i32VidFrame = 0;

		/* start to grab the first frame */
		s_sVidData.sVidMMap.frame = s_sVidData.i32VidFrame % s_sVidData.sVidMBufs.frames;
		s_sVidData.sVidMMap.height = i32Height;
		s_sVidData.sVidMMap.width = i32Width;
		s_sVidData.sVidMMap.format = VIDEO_PALETTE_YUV420P;

		if (ioctl(i32VidFD, VIDIOCMCAPTURE, &s_sVidData.sVidMMap) < 0) {
			if (errno != EAGAIN) {
				DEBUG_PRINT("VIDIOCMCAPTURE: %s\n", strerror(errno));
			} 
			else {
				DEBUG_PRINT("Fatal: grab device does not receive any video signal\n");
			}
			err = ERR_V4L_VID_MCAPTURE;
			goto fail;
		}
		s_sVidData.i32FrameFmt = s_sVidData.sVidMMap.format;
		s_sVidData.i32UseMMap = 1;
	}
	s_sVidData.i32FrameSize = s_sVidData.sVidMMap.width  * s_sVidData.sVidMMap.height* 2;
	s_sVidData.i32VidFD = i32VidFD;
	return err;
fail:
	if (i32VidFD >= 0)
		close(i32VidFD);
	return err;
}


ERRCODE
ReadV4LPicture(
	uint8_t **ppu8PicPtr,
	uint64_t *pu64TimeStamp,
	uint32_t *pu32BufPhyAddr
)
{
	int32_t i32TryCnt = 0;
	unsigned char *pchVidInBuf;
	struct v4l2_buffer sV4L2Buff;
	
	*ppu8PicPtr = NULL;

	while (ioctl(s_sVidData.i32VidFD, VIDIOCSYNC, &s_sVidData.i32VidFrame) < 0 &&
		(errno == EAGAIN || errno == EINTR)){
		usleep(10000);
		i32TryCnt ++;
		if(i32TryCnt >= 100){
			ERR_PRINT(": V4L fail\n");
			return ERR_V4L_VID_SYNC;
		}
	}

	sV4L2Buff.index = s_sVidData.i32VidFrame;
	ioctl(s_sVidData.i32VidFD, VIDIOCGCAPTIME, &sV4L2Buff);
	*pu64TimeStamp = (uint64_t)sV4L2Buff.timestamp.tv_sec * 1000000 + (uint64_t)sV4L2Buff.timestamp.tv_usec;
	*pu32BufPhyAddr = sV4L2Buff.m.userptr;	/* encode physical address */
    
	if(s_u64PrevTS != 0){
		if((*pu64TimeStamp - s_u64PrevTS) > 300000)
			DEBUG_PRINT(": V4L get raw picture over %"PRId64" us\n", (*pu64TimeStamp - s_u64PrevTS));			
	}
	s_u64PrevTS = *pu64TimeStamp;		
        
	pchVidInBuf = s_sVidData.pu8VidBuf + s_sVidData.sVidMBufs.offsets[s_sVidData.i32VidFrame];

	*ppu8PicPtr = pchVidInBuf;
	

#if defined (STATISTIC)

	if(s_u32VidInFrames == 0){
		s_u32VidInFirstTS = s_u64PrevTS;
	}

	s_u32VidInFrames ++;
	
	if(s_u32VidInFrames == 300){
		DEBUG_PRINT(": Vin frame rate %"PRId64" fps \n", s_u32VidInFrames/((s_u64PrevTS - s_u32VidInFirstTS)/1000000)); 
		s_u32VidInFrames = 0;
	}
#endif

	return ERR_V4L_SUCCESS;
}


ERRCODE
TriggerV4LNextFrame(void)
{
	int32_t i32TryCnt = 0;

	while (ioctl(s_sVidData.i32VidFD, VIDIOCSYNC, &s_sVidData.i32VidFrame) < 0 &&
		(errno == EAGAIN || errno == EINTR)){
		usleep(10000);
		i32TryCnt ++;
		if(i32TryCnt >= 100){
			ERR_PRINT("TriggerV4LNextFrame: V4L fail\n");
			return ERR_V4L_VID_SYNC;
		}
	}

	s_sVidData.sVidMMap.frame = s_sVidData.i32VidFrame;
	if (ioctl(s_sVidData.i32VidFD, VIDIOCMCAPTURE, &s_sVidData.sVidMMap) < 0) {
		if (errno == EAGAIN)
			DEBUG_PRINT(": Cannot Sync\n");
		else
			DEBUG_PRINT(": VIDIOCMCAPTURE %s\n", strerror(errno));
		return ERR_V4L_VID_MCAPTURE;
	}

	/* This is now the grabbing frame */
	s_sVidData.i32VidFrame = (s_sVidData.i32VidFrame + 1) % s_sVidData.sVidMBufs.frames;

	return ERR_V4L_SUCCESS;
}

ERRCODE
StartV4LCapture(void)
{
	uint32_t u32Capture = VIDEO_START;
	printf("User Start Capture\n");
	if (ioctl(s_sVidData.i32VidFD, VIDIOCCAPTURE, &u32Capture) < 0) {
		return ERR_V4L_VID_CAPTURE;
	}
	s_sVidData.i32VidFrame = 0;
	return ERR_V4L_SUCCESS;
}

ERRCODE
StopV4LCapture(void)
{
	uint32_t u32Capture = VIDEO_STOP;
	if (ioctl(s_sVidData.i32VidFD, VIDIOCCAPTURE, &u32Capture) < 0) {
		return ERR_V4L_VID_CAPTURE;
	}
	return ERR_V4L_SUCCESS;
}

ERRCODE
StartPreview(void)
{
	if (ioctl(s_sVidData.i32VidFD, VIDIOCSPREVIEW, 1) < 0) {
		return ERR_V4L_VID_CAPTURE;
	}
	return ERR_V4L_SUCCESS;
}

ERRCODE
StopPreview(void)
{
	if (ioctl(s_sVidData.i32VidFD, VIDIOCSPREVIEW, 0) < 0) {
		return ERR_V4L_VID_CAPTURE;
	}
	return ERR_V4L_SUCCESS;
}



ERRCODE
SetV4LViewWindow(int lcmwidth, int lcmheight, int prewidth, int preheight)
{
	ERRCODE err = ERR_V4L_SUCCESS;
	s_sVidData.sVidWin.width = prewidth;
	s_sVidData.sVidWin.height = preheight;
	s_sVidData.sVidWin.x = (lcmwidth-prewidth)/2;		//Depend on the VPOST 
	s_sVidData.sVidWin.y = (lcmheight-preheight)/2;		
	s_sVidData.sVidWin.chromakey = -1;
	s_sVidData.sVidWin.flags = 0;
	
	printf("User Packet View Win Width , Height = %d, %d\n", s_sVidData.sVidWin.width, s_sVidData.sVidWin.height);
	printf("User Packet View Win Position X , Y = %d, %d\n", s_sVidData.sVidWin.x, s_sVidData.sVidWin.y);

	if (ioctl(s_sVidData.i32VidFD, VIDIOCSWIN, &s_sVidData.sVidWin) < 0) {
		DEBUG_PRINT("VIDIOCSWIN: %s\n", strerror(errno));
		printf("User VIDIOCSWIN error\n");
		err = ERR_V4L_VID_SET_WIN;
		goto fail;
	}
	return err;
fail:
	if (s_sVidData.i32VidFD >= 0)
		close(s_sVidData.i32VidFD);
	return err;
}

ERRCODE
SetV4LEncode(int frame, int width, int height, int palette)
{
	ERRCODE err = ERR_V4L_SUCCESS;
	s_sVidData.sVidMMap.frame = frame; //s_sVidData.i32VidFrame;
	s_sVidData.sVidMMap.format = palette; //VIDEO_PALETTE_YUV422P or VIDEO_PALETTE_YUV420P
	s_sVidData.sVidMMap.height =  height;
	s_sVidData.sVidMMap.width = width;
	s_sVidData.i32FrameSize = s_sVidData.sVidMMap.width  * s_sVidData.sVidMMap.height* 2;
	if (ioctl(s_sVidData.i32VidFD, VIDIOCMCAPTURE, &s_sVidData.sVidMMap) < 0) {
		if (errno == EAGAIN)
			DEBUG_PRINT(": Cannot Sync\n");
		else
			DEBUG_PRINT(": VIDIOCMCAPTURE %s\n", strerror(errno));
		return ERR_V4L_VID_MCAPTURE;
	}
	return err;
}


#define MAX_USER_CTRL 	16
static unsigned int ctrl_id = 0;
struct v4l2_queryctrl PrivateUserCtrl[MAX_USER_CTRL];

ERRCODE
QueryV4LUserControl(void)
{
	struct v4l2_queryctrl queryctrl;
	ERRCODE err = ERR_V4L_SUCCESS;
	unsigned int id;

	memset (&queryctrl, 0, sizeof (queryctrl));
	printf ("QueryV4LUserControl standard start \n");
	for (id = V4L2_CID_BASE;
		id < V4L2_CID_LASTP1;
		id=id+1) {
			queryctrl.id =id;
			printf ("QueryV4LUserControl %d \n", queryctrl.id);
			if (0 == ioctl (s_sVidData.i32VidFD, VIDIOC_QUERYCTRL, &queryctrl)) {
				if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED){
					printf ("V4L2_CTRL_FLAG_DISABLED %d \n", queryctrl.id);
					continue;
				}
			}else {
				if (errno == EINVAL){
					printf ("EINVAL %d \n", queryctrl.id);
					continue;
				}	
				printf ("VIDIOC_QUERYCTRL");
				err = ERR_V4L_VID_QUERY_UID;	
			}
	}
	printf ("QueryV4LUserControl private start \n");
	for (id = V4L2_CID_PRIVATE_BASE; ;id=id+1) {	
			queryctrl.id =id;
			if (0 == ioctl (s_sVidData.i32VidFD, VIDIOC_QUERYCTRL, &queryctrl)) {
				if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
					continue;	
				if(ctrl_id>MAX_USER_CTRL){
					err = ERR_V4L_VID_QUERY_UID;	
					break;
				}					
				memcpy(&PrivateUserCtrl[ctrl_id], &queryctrl, sizeof(queryctrl)); 
				printf ("********************************************\n");
				printf ("id %d\n", PrivateUserCtrl[ctrl_id].id);
				printf ("name %s\n", PrivateUserCtrl[ctrl_id].name);
				printf ("minimum %d\n", PrivateUserCtrl[ctrl_id].minimum);
				printf ("maximum %d\n", PrivateUserCtrl[ctrl_id].maximum);	
				printf ("step %d\n", PrivateUserCtrl[ctrl_id].step);	
				ctrl_id = ctrl_id +1;
				
			} else {
				if (errno == EINVAL)
					break;
				printf("VIDIOC_QUERYCTRL");
				err = ERR_V4L_VID_QUERY_UID;	
			}
	}
	return err;
}
/*
	OV7725 Sensor as example 
	Contrast/Brightness Enable : 0xA6[2]  = REG_SDE_CTRL
	Brightness control register : 0x9B[7:0] = REG_BRIGHT
*/
static  struct v4l2_queryctrl* ctrl_by_name(char* name)
{
	unsigned int i;

	for (i = 0; i < MAX_USER_CTRL; i++)
		if( strcmp((char*)PrivateUserCtrl[i].name, name)==0 )
			return PrivateUserCtrl+i;
	return NULL;
}

ERRCODE
ChangeV4LUserControl_Brigness(int32_t ctrl)
{
	ERRCODE err = ERR_V4L_SUCCESS;
	struct v4l2_control control;
	struct v4l2_queryctrl* 		i2c_set_addr_ctrl;
	struct v4l2_queryctrl* 		i2c_write;
	struct v4l2_queryctrl* 		i2c_read;
	printf("!!!Remmember that the function only works for OV7725 Sesnor\n");
	
	i2c_set_addr_ctrl = ctrl_by_name("i2c_set_addr");
	i2c_write = ctrl_by_name("i2c_write");
	i2c_read= ctrl_by_name("i2c_read");
	
	/* Enable Brightness Adjustment */
	control.id = i2c_set_addr_ctrl->id;
	control.value = REG_SDE_CTRL;
	if (0 != ioctl (s_sVidData.i32VidFD, VIDIOC_S_CTRL, &control)) {//Set I2C Reg Addr
		printf("VIDIOC_G_CTRL fail\n");
		return -1; 
	}
	control.id = i2c_read->id;
	if (0 != ioctl (s_sVidData.i32VidFD, VIDIOC_G_CTRL, &control)) {//Read back 
		printf("VIDIOC_G_CTRL fail\n");
		return -1; 
	}
	control.id = i2c_write->id;
	control.value = control.value | 0x04;
	if (0 != ioctl (s_sVidData.i32VidFD, VIDIOC_S_CTRL, &control)) {//Enable brightness/constrast fail
		printf("VIDIOC_S_CTRL fail\n");
		return -1; 
	}

	/*  Brightness Ajustment */
	control.id = i2c_set_addr_ctrl->id;
	control.value =  REG_BRIGHT;
	if (0 != ioctl (s_sVidData.i32VidFD, VIDIOC_S_CTRL, &control)) {//Set I2C Reg Addr
		printf("VIDIOC_G_CTRL fail\n");
		return -1; 
	}
	control.id = i2c_read->id;
	if (0 != ioctl (s_sVidData.i32VidFD, VIDIOC_G_CTRL, &control)) {
		printf("VIDIOC_G_CTRL fail\n");
		return -1; 
	}
	control.id = i2c_write->id;	
	if(ctrl>=0){	
		control.value = control.value+1;
		if (0 != ioctl (s_sVidData.i32VidFD, VIDIOC_S_CTRL, &control)) {//Enable brightness/constrast fail 
			printf("VIDIOC_S_CTRL fail\n");
			return -1; 
		}
	}else{
		control.value = control.value-1;
		if (0 != ioctl (s_sVidData.i32VidFD, VIDIOC_S_CTRL, &control)) {//Enable brightness/constrast fail 
			printf("VIDIOC_S_CTRL fail\n");
			return -1; 
		}
	}	
	return err;	
}




/*
	ioctl VIDIOC_CROPCAP
		
	Name:	
		VIDIOC_CROPCAP— Information about the video cropping and scaling abilities
	Synopsis:	
		int ioctl(int fd, int request, struct v4l2_cropcap *argp);
	Arguments:
		fd			File descriptor returned by open().
		request			VIDIOC_CROPCAP
		argp
	Description
		Applications use this function to query the cropping limits, the pixel aspect of images and to
		calculate scale factors. They set the type field of a v4l2_cropcap structure to the respective buffer
		(stream) type and call the VIDIOC_CROPCAP ioctl with a pointer to this structure. Drivers fill the rest
		of the structure. The results are constant except when switching the video standard. Remember this
		switch can occur implicit when switching the video input or output.
	-------------------------------------------------------------------------------------------		
	struct v4l2_cropcap
		struct v4l2_cropcap {
				enum v4l2_buf_type type;
				struct v4l2_rect bounds;
				struct v4l2_rect defrect;
				struct v4l2_fract pixelaspect;
			};

		enum v4l2_buf_type type 		Type of the data stream, set by the application.Only these types are valid here:
							V4L2_BUF_TYPE_VIDEO_CAPTURE,
							V4L2_BUF_TYPE_VIDEO_OUTPUT,
							V4L2_BUF_TYPE_VIDEO_OVERLAY, and custom(driver defined) types with code
							V4L2_BUF_TYPE_PRIVATE and higher.

		struct v4l2_rect bounds			The cropping rectangle cannot exceed these limits. 
							Width and height are defined in pixels,

		struct v4l2_rect defrect		Each capture device has a default source rectangle,	

		﻿struct v4l2_fract pixelaspect 	 	This is the pixel aspect (y / x) when no scaling is applied, the ratio of the actual sampling
							frequency and the frequency required to get square pixels.
							**** We us the structure for zooming step. 
							**** It means the zooming step will be (16, 12)	for VGA sensor. The aspect ratio is 4:3   						
							
	-------------------------------------------------------------------------------------------				
		struct v4l2_rect {
			__s32 left;
			__s32 top;
			__s32 width;
			__s32 height;
		};
		
		struct v4l2_fract {
			__u32 numerator;
			__u32  denominator;
		};
		

*/

ERRCODE
QueryV4LZoomInfo(struct v4l2_cropcap *psVideoCropCap, 
			struct v4l2_crop *psVideoCrop)
{
	ERRCODE err = ERR_V4L_SUCCESS;

	if (0 != ioctl (s_sVidData.i32VidFD, VIDIOC_CROPCAP, psVideoCropCap)) {//Get cropping window capacity 
		printf("VIDIOC_CROPCAP fail\n");
		return -1; 
	}
	
	if (0 != ioctl (s_sVidData.i32VidFD, VIDIOC_G_CROP, psVideoCrop)) {//Get current cropping window
			printf("VIDIOC_G_CROP fail\n");
			return -1; 
	}
	return err;
}


ERRCODE
Zooming(struct v4l2_cropcap *psVideoCropCap, 
	struct v4l2_crop *psVideoCrop,
	int32_t ctrl)
{
	ERRCODE err = ERR_V4L_SUCCESS;

	if (QueryV4LZoomInfo(psVideoCropCap, psVideoCrop)!=ERR_V4L_SUCCESS)
	{
		printf("Query zoom info fail\n");
			return -1; 	
	}

	if(ctrl==1)
	{//zoom in 				
		if( ((psVideoCrop->c.width - psVideoCropCap->pixelaspect.denominator) >= psVideoCropCap->defrect.width/8) &&
		     ((psVideoCrop->c.height - psVideoCropCap->pixelaspect.numerator) >= psVideoCropCap->defrect.height/8) )
		{//Limits the JPEG upscale to 8X.
			printf("Zooming in\n");	
			psVideoCrop->c.width = psVideoCrop->c.width - psVideoCropCap->pixelaspect.denominator;
			psVideoCrop->c.height = psVideoCrop->c.height - psVideoCropCap->pixelaspect.numerator;
			psVideoCrop->c.left = psVideoCrop->c.left + psVideoCropCap->pixelaspect.denominator/2;
			psVideoCrop->c.top = psVideoCrop->c.top + psVideoCropCap->pixelaspect.numerator/2;
			if (0 != ioctl (s_sVidData.i32VidFD, VIDIOC_S_CROP, psVideoCrop)) {//Set current cropping window
				printf("VIDIOC_S_CROP fail\n");
				return -1; 
			}	
		}		
	}
	else 
	{//zoom out
		if( ((psVideoCrop->c.width + psVideoCropCap->pixelaspect.denominator) <= psVideoCropCap->defrect.width) &&
		     ((psVideoCrop->c.height + psVideoCropCap->pixelaspect.numerator) <= psVideoCropCap->defrect.height) ) 
		{//cropping dimension must be less or equal defrect. 
			printf("Zooming out\n");
			psVideoCrop->c.width = psVideoCrop->c.width + psVideoCropCap->pixelaspect.denominator;
			psVideoCrop->c.height = psVideoCrop->c.height + psVideoCropCap->pixelaspect.numerator;
			psVideoCrop->c.left = psVideoCrop->c.left - psVideoCropCap->pixelaspect.denominator/2;
			psVideoCrop->c.top = psVideoCrop->c.top - psVideoCropCap->pixelaspect.numerator/2;
			if (0 != ioctl (s_sVidData.i32VidFD, VIDIOC_S_CROP, psVideoCrop)) {//Set current cropping window
				printf("VIDIOC_S_CROP fail\n");
				return -1; 
			}	
		}		
	}
	printf("cropcap.defrect.left =%d\n",psVideoCropCap->defrect.left);			/* default window information */
	printf("cropcap.defrect.top =%d\n",psVideoCropCap->defrect.top);
	printf("cropcap.defrect.width =%d\n",psVideoCropCap->defrect.width);
	printf("cropcap.defrect.height =%d\n",psVideoCropCap->defrect.height);
	printf("cropcap.pixelaspect.numerator =%d\n",psVideoCropCap->pixelaspect.numerator);	/* constraint for step in y direct */
	printf("cropcap.pixelaspect.denominator =%d\n",psVideoCropCap->pixelaspect.denominator);/* constraint for step in x direct */

	printf("crop.c.left =%d\n",psVideoCrop->c.left);
	printf("crop.c.top =%d\n",psVideoCrop->c.top);
	printf("crop.c.width =%d\n",psVideoCrop->c.width);
	printf("crop.c.height =%d\n",psVideoCrop->c.height);
	
	return err;
}

void FinializeV4LDevice()
{
	if (s_sVidData.i32UseMMap)
		munmap(s_sVidData.pu8VidBuf, s_sVidData.sVidMBufs.size);
	close(s_sVidData.i32VidFD);
	s_sVidData.i32VidFD = -1;
}





/* JpegEnc.c
 *
 *
 * Copyright (c)2008 Nuvoton technology corporation
 * http://www.nuvoton.com
 *
 * JPEG encode function
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <linux/videodev.h>
#include "JpegEnc.h"


static uint8_t *s_pu8JpegEncBuf = MAP_FAILED;
static int32_t s_i32JpegFd = -1;
static uint32_t s_u32JpegBufSize;
static jpeg_param_t s_sJpegParam;
static jpeg_info_t *s_pJpegInfo = NULL;	

#define DBG_POINT()		printf("Function:%s. Line: %d\n", __FUNCTION__, __LINE__);


extern void create_simple_EXIF(char *Buffer, int thumbnail_offset,int thumbnail_size);

int 
InitJpegDevice(
	E_IMAGE_RESOL eImageResol
)
{
	int32_t i;
	char achDevice[] = "/dev/video1";

	for(i=0; i < RETRY_NUM; i++){
		s_i32JpegFd = open(achDevice, O_RDWR);         //maybe /dev/video1, video2, ...
		if (s_i32JpegFd  < 0){
			ERR_PRINT("open %s error\n", achDevice);
			achDevice[10]++;
			continue;       
		}
		/* allocate memory for JPEG engine */
		if((ioctl(s_i32JpegFd, JPEG_GET_JPEG_BUFFER, &s_u32JpegBufSize)) < 0){
			close(s_i32JpegFd);
			achDevice[10]++;
			DEBUG_PRINT("Try to open %s\n",achDevice);
		}
		else{       
			DEBUG_PRINT("jpegcodec is %s\n",achDevice);
			break;                  
		}
	}
	if(s_i32JpegFd < 0)
		return s_i32JpegFd; 

	s_pJpegInfo = malloc(sizeof(jpeg_info_t) + sizeof(__u32));
	if(s_pJpegInfo == NULL){
		ERR_PRINT("Allocate JPEG info failed\n");
		close(s_i32JpegFd);
		return -1;		
	}

	s_pu8JpegEncBuf = mmap(NULL, s_u32JpegBufSize, PROT_READ|PROT_WRITE, MAP_SHARED, s_i32JpegFd, 0);
	
	if(s_pu8JpegEncBuf == MAP_FAILED){
		ERR_PRINT("JPEG Map Failed!\n");
		close(s_i32JpegFd);
		return -1;
	}

	memset((void*)&s_sJpegParam, 0, sizeof(s_sJpegParam));


	s_sJpegParam.encode = 1;						/* Encode */
	if(eImageResol == eIMAGE_VGA){
		s_sJpegParam.encode_width = 640;			/* JPEG width */
		s_sJpegParam.encode_height = 480;			/* JPGE Height */
	}
	else if(eImageResol == eIMAGE_WQVGA){
		s_sJpegParam.encode_width = 480;			/* JPEG width */
		s_sJpegParam.encode_height = 272;			/* JPGE Height */
	}
	else{
		s_sJpegParam.encode_width = 320;			/* JPEG width */
		s_sJpegParam.encode_height = 240;			/* JPGE Height */
	}


	s_sJpegParam.encode_image_format = DRVJPEG_ENC_PRIMARY_YUV422;
	//s_sJpegParam.encode_image_format = DRVJPEG_ENC_PRIMARY_YUV420;

	s_sJpegParam.buffersize = 0;		/* only for continuous shot */
	s_sJpegParam.buffercount = 1;		

	s_sJpegParam.scale = 0;		/* Scale function is disabled when scale 0 */	
	s_sJpegParam.qscaling = 7;			
	s_sJpegParam.qadjust = 0xf;			
	printf("JPEG file descriptor = %d\n", s_i32JpegFd);
	return s_i32JpegFd;
}

int 
JpegEncFromVIN(E_IMAGE_RESOL eEncodeImgResol,
	uint32_t u32PlanarYBufPhyAddr,	/* src pattern */
	uint8_t **ppu8JpegEncBuf,
	uint32_t *pu32JpegEncLen,
	S_JPEG_ENC_FEAT *psJpegEncFeat,
	struct v4l2_crop *psVideoCrop
)
{
	uint32_t u32Width, u32Height;	
	uint8_t *pu8EncBitStmAddr;
	uint8_t *pu8ReservedAddr = NULL;
	int32_t i32Len;
	uint32_t u32PlanarUBufPhyAddr, u32PlanarVBufPhyAddr;

	if(eEncodeImgResol == eIMAGE_WQVGA){
		u32Width = 480;				
		u32Height = 272;					
	}else if(eEncodeImgResol == eIMAGE_VGA){
		u32Width = 640;
		u32Height = 480;		
	}else if(eEncodeImgResol == eIMAGE_SVGA){
		u32Width = 800;
		u32Height = 600;
	}else if(eEncodeImgResol == eIMAGE_HD720){
		u32Width = 1280;
		u32Height = 720;
	}else{
		u32Width = 320;				
		u32Height = 240;				
	} 


	DBG_POINT();
#if 1
		if(psVideoCrop->c.width>u32Width){//Cropping window bigger than encode dimension, 
			u32PlanarUBufPhyAddr = 	u32PlanarYBufPhyAddr + u32Width*u32Height;
	#ifdef _PLANAR_YUV420_
			//YUV420
			u32PlanarVBufPhyAddr = 	u32PlanarUBufPhyAddr + u32Width*u32Height/4;		
	#else
			//YUV422
			u32PlanarVBufPhyAddr = 	u32PlanarUBufPhyAddr + u32Width*u32Height/2;	
	#endif	
		}
		else{
			u32PlanarUBufPhyAddr = 	u32PlanarYBufPhyAddr + psVideoCrop->c.width*psVideoCrop->c.height;
	#ifdef _PLANAR_YUV420_
			//YUV420
			u32PlanarVBufPhyAddr = 	u32PlanarUBufPhyAddr + psVideoCrop->c.width*psVideoCrop->c.height/4;		
	#else
			//YUV422
			u32PlanarVBufPhyAddr = 	u32PlanarUBufPhyAddr + psVideoCrop->c.width*psVideoCrop->c.height/2;	
	#endif				
		}
		if((ioctl(s_i32JpegFd, JPEG_SET_ENC_USER_YADDRESS, u32PlanarYBufPhyAddr)) < 0){
			ERR_PRINT("Set jpeg Y address failed:%d\n",errno);
			return -1;
		}
		if((ioctl(s_i32JpegFd, JPEG_SET_ENC_USER_UADDRESS, u32PlanarUBufPhyAddr)) < 0){
			ERR_PRINT("Set jpeg U address failed:%d\n",errno);
			return -1;
		}
		if((ioctl(s_i32JpegFd, JPEG_SET_ENC_USER_VADDRESS, u32PlanarVBufPhyAddr)) < 0){
			ERR_PRINT("Set jpeg V address failed:%d\n",errno);
			return -1;
		}	
#endif 
	printf("Y Addr = 0x%x\n", u32PlanarYBufPhyAddr);	
	printf("U Addr = 0x%x\n", u32PlanarUBufPhyAddr);	
	printf("V Addr = 0x%x\n", u32PlanarVBufPhyAddr);	
	s_sJpegParam.encode_image_format = DRVJPEG_ENC_PRIMARY_YUV422; 
	s_sJpegParam.encode_source_format = DRVJPEG_ENC_SRC_PLANAR;	/* Source is Planar Format */	

	pu8EncBitStmAddr = s_pu8JpegEncBuf;

	if(psJpegEncFeat->eJpegEncThbSupp){
	/* Set reserved size for exif */
		int32_t i32EncReservedSize = 0x84;
		pu8ReservedAddr = pu8EncBitStmAddr + 6;
		memset((char *)pu8ReservedAddr, 0xFF, i32EncReservedSize);
		ioctl(s_i32JpegFd, JPEG_SET_ENCOCDE_RESERVED, i32EncReservedSize); 
	}


	if((psVideoCrop->c.width <  u32Width) ||
		(psVideoCrop->c.height < u32Height) )
	{//Encode upscale 
		s_sJpegParam.scaled_width = u32Width;	 
		s_sJpegParam.scaled_height = u32Height;
		s_sJpegParam.encode_width = psVideoCrop->c.width;
		s_sJpegParam.encode_height = psVideoCrop->c.height;					
		s_sJpegParam.scale = 1;				
		printf("JPEG encode source WxH= %d x %d\n", s_sJpegParam.encode_width, s_sJpegParam.encode_height);
		printf("JPEG encode target WxH= %d x %d\n", u32Width, u32Height);	
		/* Set encode source stride (Must calling after IOCTL - JPEG_S_PARAM) */
		ioctl(s_i32JpegFd, JPEG_SET_ENC_STRIDE, psVideoCrop->c.width>u32Width);
	}	
	else
	{//
		s_sJpegParam.scale = 0; 
		s_sJpegParam.encode_width = u32Width;
		s_sJpegParam.encode_height = u32Height;
		/* Set encode source stride (Must calling after IOCTL - JPEG_S_PARAM) */
		ioctl(s_i32JpegFd, JPEG_SET_ENC_STRIDE, u32Width);

		printf("JPEG encode target WxH= %d x %d, stride = %d\n", u32Width, u32Height, u32Width);
	}
		
	DBG_POINT();
	/* Set operation property to JPEG engine */
	if((ioctl(s_i32JpegFd, JPEG_S_PARAM, &s_sJpegParam)) < 0){
		ERR_PRINT("Set jpeg parame failed:%d\n",errno);
		return -1;
	}
	DBG_POINT();
	
	/* Set Encode Source from VideoIn */
	//ioctl(s_i32JpegFd,JPEG_SET_ENC_SRC_FROM_VIN,NULL);	

	if(psJpegEncFeat->eJpegEncThbSupp){

		if(psJpegEncFeat->eJpegEncThbSupp == eJPEG_ENC_THB_QQVGA){
			ioctl(s_i32JpegFd,JPEG_SET_ENC_THUMBNAIL, 1);	
		}
		else{
			ioctl(s_i32JpegFd,JPEG_SET_ENC_THUMBNAIL, 0);	
		}

	}
	DBG_POINT();

	
	
	/* Trigger JPEG engine */
	if((ioctl(s_i32JpegFd, JPEG_TRIGGER, NULL)) < 0){
		ERR_PRINT("Trigger jpeg failed:%d\n",errno);
		return -1;
	}
	DBG_POINT();
	i32Len = read(s_i32JpegFd, s_pJpegInfo, sizeof(jpeg_info_t) + sizeof (__u32));
	
	if(i32Len < 0){
		ERR_PRINT("No data can get\n");
		if(s_pJpegInfo->state == JPEG_MEM_SHORTAGE)
			ERR_PRINT("Memory Stortage\n");	
		return -1;
	}

	if(psJpegEncFeat->eJpegEncThbSupp){
		int32_t i32ThumbnailSize;
		int32_t i32ThumbnailOffset;
		ioctl(s_i32JpegFd, JPEG_GET_ENC_THUMBNAIL_SIZE, &i32ThumbnailSize);
		ioctl(s_i32JpegFd, JPEG_GET_ENC_THUMBNAIL_OFFSET, &i32ThumbnailOffset);
		create_simple_EXIF((char *)pu8ReservedAddr, i32ThumbnailOffset,i32ThumbnailSize);
	}

	*ppu8JpegEncBuf = s_pu8JpegEncBuf;
	*pu32JpegEncLen = s_pJpegInfo->image_size[0];
	return 0;
}



int 
JpegEncFromVIN_Packet(E_IMAGE_RESOL eEncodeImgResol,
	uint32_t u32PacketBufPhyAddr,	/* src pattern */
	uint8_t **ppu8JpegEncBuf,
	uint32_t *pu32JpegEncLen,
	S_JPEG_ENC_FEAT *psJpegEncFeat,
	struct v4l2_crop *psVideoCrop
)
{
	uint32_t u32Width, u32Height;	
	uint8_t *pu8EncBitStmAddr;
	uint8_t *pu8ReservedAddr = NULL;
	int32_t i32Len;


	if(eEncodeImgResol == eIMAGE_WQVGA){
		u32Width = 480;				
		u32Height = 272;					
	}else if(eEncodeImgResol == eIMAGE_VGA){
		u32Width = 640;
		u32Height = 480;		
	}else if(eEncodeImgResol == eIMAGE_SVGA){
		u32Width = 800;
		u32Height = 600;
	}else if(eEncodeImgResol == eIMAGE_HD720){
		u32Width = 1280;
		u32Height = 720;
	}else{
		u32Width = 320;				
		u32Height = 240;				
	} 
	
	
	DBG_POINT();
#if 1					
	if((ioctl(s_i32JpegFd, JPEG_SET_ENC_USER_YADDRESS, u32PacketBufPhyAddr)) < 0){
		ERR_PRINT("Set jpeg Y address failed:%d\n",errno);
		return -1;
	}
	#if 0
		if((ioctl(s_i32JpegFd, JPEG_SET_ENC_USER_UADDRESS, u32PlanarUBufPhyAddr)) < 0){
			ERR_PRINT("Set jpeg U address failed:%d\n",errno);
			return -1;
		}
		if((ioctl(s_i32JpegFd, JPEG_SET_ENC_USER_VADDRESS, u32PlanarVBufPhyAddr)) < 0){
			ERR_PRINT("Set jpeg V address failed:%d\n",errno);
			return -1;
		}
	#endif	
#endif 
	s_sJpegParam.encode_image_format = DRVJPEG_ENC_PRIMARY_YUV422; /* Useless */
	s_sJpegParam.encode_source_format = DRVJPEG_ENC_SRC_PACKET;	/* Source is Packet Format */	

	/* Set encode source stride (Must calling after IOCTL - JPEG_S_PARAM) */
	ioctl(s_i32JpegFd, JPEG_SET_ENC_STRIDE, u32Width);

	pu8EncBitStmAddr = s_pu8JpegEncBuf;

	if(psJpegEncFeat->eJpegEncThbSupp){
	/* Set reserved size for exif */
		int32_t i32EncReservedSize = 0x84;
		pu8ReservedAddr = pu8EncBitStmAddr + 6;
		memset((char *)pu8ReservedAddr, 0xFF, i32EncReservedSize);
		ioctl(s_i32JpegFd, JPEG_SET_ENCOCDE_RESERVED, i32EncReservedSize); 
	}


#if 0
	if((psVideoCrop->c.width <  u32Width) ||
		(psVideoCrop->c.height < u32Height) )
	{
		s_sJpegParam.scaled_width = u32Width;	 
		s_sJpegParam.scaled_height = u32Height;
		s_sJpegParam.encode_width = psVideoCrop->c.width;
		s_sJpegParam.encode_height = psVideoCrop->c.height;					
		s_sJpegParam.scale = 1;				
		printf("JPEG encode source WxH= %d x %d\n", s_sJpegParam.encode_width, s_sJpegParam.encode_height);
		printf("JPEG encode target WxH= %d x %d\n", u32Width, u32Height);	
	}	
	else
	{
		s_sJpegParam.scale = 0; 
		s_sJpegParam.encode_width = u32Width;
		s_sJpegParam.encode_height = u32Height;
		printf("JPEG encode target WxH= %d x %d\n", u32Width, u32Height);		
	}
#else
	s_sJpegParam.scale = 0; 
	s_sJpegParam.encode_width = u32Width;
	s_sJpegParam.encode_height = u32Height;
	printf("JPEG encode target WxH= %d x %d\n", u32Width, u32Height);	
#endif		
	DBG_POINT();
	/* Set operation property to JPEG engine */
	if((ioctl(s_i32JpegFd, JPEG_S_PARAM, &s_sJpegParam)) < 0){
		ERR_PRINT("Set jpeg parame failed:%d\n",errno);
		return -1;
	}
	DBG_POINT();

	/* Set Encode Source from VideoIn */
	//ioctl(s_i32JpegFd,JPEG_SET_ENC_SRC_FROM_VIN,NULL);	

	if(psJpegEncFeat->eJpegEncThbSupp){

		if(psJpegEncFeat->eJpegEncThbSupp == eJPEG_ENC_THB_QQVGA){
			ioctl(s_i32JpegFd,JPEG_SET_ENC_THUMBNAIL, 1);	
		}
		else{
			ioctl(s_i32JpegFd,JPEG_SET_ENC_THUMBNAIL, 0);	
		}

	}
	DBG_POINT();
	/* Trigger JPEG engine */
	if((ioctl(s_i32JpegFd, JPEG_TRIGGER, NULL)) < 0){
		ERR_PRINT("Trigger jpeg failed:%d\n",errno);
		return -1;
	}
	DBG_POINT();
	i32Len = read(s_i32JpegFd, s_pJpegInfo, sizeof(jpeg_info_t) + sizeof (__u32));
	
	if(i32Len < 0){
		ERR_PRINT("No data can get\n");
		if(s_pJpegInfo->state == JPEG_MEM_SHORTAGE)
			ERR_PRINT("Memory Stortage\n");	
		return -1;
	}

	if(psJpegEncFeat->eJpegEncThbSupp){
		int32_t i32ThumbnailSize;
		int32_t i32ThumbnailOffset;
		ioctl(s_i32JpegFd, JPEG_GET_ENC_THUMBNAIL_SIZE, &i32ThumbnailSize);
		ioctl(s_i32JpegFd, JPEG_GET_ENC_THUMBNAIL_OFFSET, &i32ThumbnailOffset);
		create_simple_EXIF((char *)pu8ReservedAddr, i32ThumbnailOffset,i32ThumbnailSize);
	}

	*ppu8JpegEncBuf = s_pu8JpegEncBuf;
	*pu32JpegEncLen = s_pJpegInfo->image_size[0];
	return 0;
}



int 
JpegEncFromVIN_Planar(E_IMAGE_RESOL eEncodeImgResol,
	uint32_t u32PlanarFmt,			/* Planar YUV422 or YUV420 */	
	uint32_t u32PlanarBufPhyAddr,	/* src pattern address*/
	uint8_t **ppu8JpegEncBuf,
	uint32_t *pu32JpegEncLen,
	S_JPEG_ENC_FEAT *psJpegEncFeat,
	struct v4l2_crop *psVideoCrop
)
{
	uint32_t u32Width, u32Height;	
	uint8_t *pu8EncBitStmAddr;
	uint8_t *pu8ReservedAddr = NULL;
	int32_t i32Len;
	uint32_t u32UAddr, u32VAddr;

	if(eEncodeImgResol == eIMAGE_WQVGA){
		u32Width = 480;				
		u32Height = 272;					
	}else if(eEncodeImgResol == eIMAGE_VGA){
		u32Width = 640;
		u32Height = 480;		
	}else if(eEncodeImgResol == eIMAGE_SVGA){
		u32Width = 800;
		u32Height = 600;
	}else if(eEncodeImgResol == eIMAGE_HD720){
		u32Width = 1280;
		u32Height = 720;
	}else{
		u32Width = 320;				
		u32Height = 240;				
	} 
	
	
	DBG_POINT();
	if(u32PlanarFmt == VIDEO_PALETTE_YUV422P)
		s_sJpegParam.encode_image_format = DRVJPEG_ENC_PRIMARY_YUV422; 	/* JPEG Output format */
	else
		s_sJpegParam.encode_image_format = DRVJPEG_ENC_PRIMARY_YUV420; 	/* JPEG Output format */
	s_sJpegParam.encode_source_format = DRVJPEG_ENC_SRC_PLANAR;		/* Source is Packet Format */	

	/* Set encode source stride (Must calling after IOCTL - JPEG_S_PARAM) */
	ioctl(s_i32JpegFd, JPEG_SET_ENC_STRIDE, u32Width);

	pu8EncBitStmAddr = s_pu8JpegEncBuf;

	if(psJpegEncFeat->eJpegEncThbSupp){
	/* Set reserved size for exif */
		int32_t i32EncReservedSize = 0x84;
		pu8ReservedAddr = pu8EncBitStmAddr + 6;
		memset((char *)pu8ReservedAddr, 0xFF, i32EncReservedSize);
		ioctl(s_i32JpegFd, JPEG_SET_ENCOCDE_RESERVED, i32EncReservedSize); 
	}


#if 1
	if((psVideoCrop->c.width <  u32Width) ||
		(psVideoCrop->c.height < u32Height) )
	{
		s_sJpegParam.scaled_width = u32Width;	 
		s_sJpegParam.scaled_height = u32Height;
		s_sJpegParam.encode_width = psVideoCrop->c.width;
		s_sJpegParam.encode_height = psVideoCrop->c.height;					
		s_sJpegParam.scale = 1;				
		printf("JPEG encode source WxH= %d x %d\n", s_sJpegParam.encode_width, s_sJpegParam.encode_height);
		printf("JPEG encode target WxH= %d x %d\n", u32Width, u32Height);	
	}	
	else
	{
		s_sJpegParam.scale = 0; 
		s_sJpegParam.encode_width = u32Width;
		s_sJpegParam.encode_height = u32Height;
		printf("JPEG encode target WxH= %d x %d\n", u32Width, u32Height);		
	}
#else
	s_sJpegParam.scale = 0; 
	s_sJpegParam.encode_width = u32Width;
	s_sJpegParam.encode_height = u32Height;
	printf("JPEG encode target WxH= %d x %d\n", u32Width, u32Height);	
#endif		

#if 1						
	if((ioctl(s_i32JpegFd, JPEG_SET_ENC_USER_YADDRESS, u32PlanarBufPhyAddr)) < 0){
		ERR_PRINT("Set jpeg Y address failed:%d\n",errno);
		return -1;
	}
	u32UAddr = u32PlanarBufPhyAddr+s_sJpegParam.encode_width*s_sJpegParam.encode_height; 
	if((ioctl(s_i32JpegFd, JPEG_SET_ENC_USER_UADDRESS, u32UAddr )) < 0){
		ERR_PRINT("Set jpeg U address failed:%d\n",errno);
		return -1;
	}
	if(u32PlanarFmt == VIDEO_PALETTE_YUV422P)
		u32VAddr = u32UAddr + s_sJpegParam.encode_width * s_sJpegParam.encode_height/2;
	else
		u32VAddr = u32UAddr + s_sJpegParam.encode_width * s_sJpegParam.encode_height/4;
	if((ioctl(s_i32JpegFd, JPEG_SET_ENC_USER_VADDRESS, u32VAddr)) < 0){
		ERR_PRINT("Set jpeg V address failed:%d\n",errno);
		return -1;
	}	
	printf("New Encode Planar Y/U/V = 0x%x, 0x%x, 0x%x\n", u32PlanarBufPhyAddr, u32UAddr, u32VAddr);
#endif 
	DBG_POINT();
	/* Set operation property to JPEG engine */
	if((ioctl(s_i32JpegFd, JPEG_S_PARAM, &s_sJpegParam)) < 0){
		ERR_PRINT("Set jpeg parame failed:%d\n",errno);
		return -1;
	}
	DBG_POINT();

	/* Set Encode Source from VideoIn */
	//ioctl(s_i32JpegFd,JPEG_SET_ENC_SRC_FROM_VIN,NULL);	

	if(psJpegEncFeat->eJpegEncThbSupp){

		if(psJpegEncFeat->eJpegEncThbSupp == eJPEG_ENC_THB_QQVGA){
			ioctl(s_i32JpegFd,JPEG_SET_ENC_THUMBNAIL, 1);	
		}
		else{
			ioctl(s_i32JpegFd,JPEG_SET_ENC_THUMBNAIL, 0);	
		}

	}
	DBG_POINT();
	/* Trigger JPEG engine */
	if((ioctl(s_i32JpegFd, JPEG_TRIGGER, NULL)) < 0){
		ERR_PRINT("Trigger jpeg failed:%d\n",errno);
		return -1;
	}
	DBG_POINT();
	i32Len = read(s_i32JpegFd, s_pJpegInfo, sizeof(jpeg_info_t) + sizeof (__u32));
	
	if(i32Len < 0){
		ERR_PRINT("No data can get\n");
		if(s_pJpegInfo->state == JPEG_MEM_SHORTAGE)
			ERR_PRINT("Memory Stortage\n");	
		return -1;
	}

	if(psJpegEncFeat->eJpegEncThbSupp){
		int32_t i32ThumbnailSize;
		int32_t i32ThumbnailOffset;
		ioctl(s_i32JpegFd, JPEG_GET_ENC_THUMBNAIL_SIZE, &i32ThumbnailSize);
		ioctl(s_i32JpegFd, JPEG_GET_ENC_THUMBNAIL_OFFSET, &i32ThumbnailOffset);
		create_simple_EXIF((char *)pu8ReservedAddr, i32ThumbnailOffset,i32ThumbnailSize);
	}

	*ppu8JpegEncBuf = s_pu8JpegEncBuf;
	*pu32JpegEncLen = s_pJpegInfo->image_size[0];
	return 0;
}


int 
FinializeJpegDevice(void)
{
	if(s_pJpegInfo){
		free(s_pJpegInfo);
		s_pJpegInfo = NULL;
	}

	if(s_pu8JpegEncBuf !=  MAP_FAILED){
		munmap(s_pu8JpegEncBuf, s_u32JpegBufSize);
		s_pu8JpegEncBuf = MAP_FAILED;	
	}

	if(s_i32JpegFd > 0){
		close(s_i32JpegFd);
		s_i32JpegFd = -1;
	}
	return 0;
}


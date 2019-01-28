/* vin_demo.c
 *
 *
 * Copyright (c)2008 Nuvoton technology corporation
 * http://www.nuvoton.com
 *
 * vin_demo application
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
#include <linux/input.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/statfs.h>
#include <linux/fb.h>
#include <wchar.h>
#include <linux/videodev.h>
#include <signal.h>

#include "demo.h"
#include "FB_IOCTL.h"
#include "ROT_IOCTL.h"

void LoadROTSrcPattern(uint8_t *p8SrcBuf)
{
	FILE *fpr;
	int sz;

	
	DBG_PRINTF("The pattern \"rot_src0.pb\" and application \"rot_demo\" have to store in the same directory.\n");
	fpr = fopen("rot_src0.pb", "rb");
	fseek(fpr, 0L, SEEK_END);
	sz = ftell(fpr);
	fseek(fpr, 0L, SEEK_SET);

	if(fpr<0){		
		ERR_PRINTF("Open read file  error \n"); 
		exit(-1);
	}
	fread(p8SrcBuf, 1, sz, fpr);
	fclose(fpr);
	sync();
}





int32_t demo_rot(uint32_t u32ROTBufPhyAddr, uint32_t u32FBPhyAddr, uint32_t rot)
{
	S_ROT sRotConf;
	uint32_t u32FB0, u32FB1;
	uint32_t bIsComplete;
	uint32_t idx=0;	
	uint32_t bIsFbbuf0=1;		
	uint32_t u32width=OPT_SRC_WIDTH, u32height=OPT_SRC_HEIGHT;

	sRotConf.eRotFormat = E_ROT_PACKET_RGB565;	// Assigned the rotation format RGB888/RGB565/YUV422
	sRotConf.eBufSize = E_LBUF_4;				// Assigned the buffer size for on the fly rotation
	u32FB0=u32FBPhyAddr;
	u32FB1=u32FBPhyAddr+gsVar.xres*gsVar.yres*2;
	do
	{
		if(rot==0)
			sRotConf.eRotDir = E_ROT_ROT_R90;		// Right
		else
			sRotConf.eRotDir = E_ROT_ROT_L90;		// Left
		// Rotation Dimension  [31:16]==> Height of source pattern, [15:0]==> Width of source pattern
		sRotConf.u32RotDimHW =((u32height-idx*src_h_step*2)<<16) | (u32width- idx*src_w_step*2);		 
		// Source line offset, pixel unit		
		sRotConf.u32SrcLineOffset = idx*6;	
		// Destination line offset, pixel unit		
		sRotConf.u32DstLineOffset = idx*8;			
		// Buffer physical start address of source image
		sRotConf.u32SrcAddr = u32ROTBufPhyAddr+idx*src_h_step*gsVar.yres*2+idx*src_w_step*2;	
	
		// Buffer physical start address of rotated image
		if(bIsFbbuf0==1)
			sRotConf.u32DstAddr = u32FB0+idx*dst_h_step*gsVar.xres*2+idx*dst_w_step*2;	//Trigger to buf 0		
		else
			sRotConf.u32DstAddr = u32FB1+idx*dst_h_step*gsVar.xres*2+idx*dst_w_step*2;	//Trigger to buf 1
	
		if (ioctl(i32DevROT, ROT_IO_SET, &sRotConf) < 0) {
			ERR_PRINTF("Err ioctl ROT get dimension \n");
			close(i32DevROT);
			return -1;
		}
		bIsComplete = 0;
		if (ioctl(i32DevROT, ROT_IO_TRIGGER, NULL) < 0) {
			ERR_PRINTF("Err ioctl ROT_IO_TRIGGER \n");
			close(i32DevROT);
			return -1;
		}

		do
		{
			if (ioctl(i32DevROT, ROT_POLLING_INTERRUPT, &bIsComplete) < 0) {
				ERR_PRINTF("Err ioctl ROT_IO_TRIGGER \n");
				close(i32DevROT);
				return -1;
			}
		}while(bIsComplete==0);
		if(bIsFbbuf0==1){
			ShowResult(0);
			bIsFbbuf0=0;
		}
		else{
			ShowResult(1);
			bIsFbbuf0=1;
		}	
		idx=idx+1;	
	}while(idx<5);
	return 0;
}

int main(int argc, char **argv)
{
	//ParseOption();	
	int32_t index = 0;
	int rot_fd, fb_fd;
	uint32_t u32FBPhyAddr;
	uint32_t u32ROTBufPhyAddr;


	fb_fd = OpenFBDevice(&u32FBPhyAddr);
	if(fb_fd<0){
		ERR_PRINTF("Error open fb device\n");
		CloseFBDevice();
		return -1;
	}
	rot_fd = OpenROTDevice(&u32ROTBufPhyAddr); 						
	if(rot_fd<0){
		ERR_PRINTF("Error open rot device\n");
		CloseROTDevice();
		return -1;
	}	
	LoadROTSrcPattern(pu8MapROTBuf);	/* Load src pattern to the mapping buffer of ROT */
	while(1)
	{				
		demo_rot(u32ROTBufPhyAddr, u32FBPhyAddr, 0);		
		demo_rot(u32ROTBufPhyAddr, u32FBPhyAddr, 1);
		index = index+1;
		if(index>2)
			index = 0;
	}
	CloseFBDevice();
	CloseROTDevice();
	return 0;
}


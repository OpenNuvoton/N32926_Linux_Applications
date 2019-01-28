/************************************************************************
 *									*
 * Copyright (c) 2008 Nuvoton Technology Corp. All rights reserved.	*
 *									*
 ************************************************************************/

/************************************************************************
 *
 * FILENAME
 *	 osd.c
 *
 * VERSION
 *	 1.0
 *
 * DESCRIPTION
 *	 This file is a OSD sample program
 *
 * DATA STRUCTURES
 *	 None
 *
 * FUNCTIONS
 *	 None
 *
 * HISTORY
 *	 2011/06/10		 Ver 1.0 Created
 *
 * REMARK
 *	 None
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/fb.h>

// OSD
#define IOCTL_GET_OSD_OFFSET    _IOR('v', 60, unsigned int *)
#define OSD_SEND_CMD            _IOW('v', 160, unsigned int *)

#define LCD_RGB565_2_RGB555		_IO('v', 30)
#define LCD_RGB555_2_RGB565		_IO('v', 31)

#define LCD_ENABLE_INT		_IO('v', 28)
#define LCD_DISABLE_INT		_IO('v', 29)

#define IOCTL_FB_LOCK			_IOW('v', 64, unsigned int)	
#define IOCTL_FB_UNLOCK			_IOW('v', 65, unsigned int)	
#define IOCTL_WAIT_VSYNC		_IOW('v', 67, unsigned int)	


/* Macros about LCM */
#define CMD_DISPLAY_ON						0x3F
#define CMD_DISPLAY_OFF						0x3E
#define CMD_SET_COL_ADDR					0x40
#define	CMD_SET_ROW_ADDR					0xB8
#define CMD_SET_DISP_START_LINE		0xC0


#define CHAR_WIDTH 		5

// OSD 
typedef enum {
  // All functions return -2 on "not open"
  OSD_Close=1,    // ()
  // Disables OSD and releases the buffers (??)
  // returns 0 on success

  OSD_Open,       // (cmd + color_format)
  // Opens OSD with color format
  // returns 0 on success

  OSD_Show,       // (cmd)
  // enables OSD mode
  // returns 0 on success

  OSD_Hide,       // (cmd)
  // disables OSD mode
  // returns 0 on success

  OSD_Clear,      // (cmd )
  // clear OSD buffer with color-key color
  // returns 0 on success
  
  OSD_Fill,      // (cmd +)
  // clear OSD buffer with assigned color
  // returns 0 on success

  OSD_FillBlock,      // (cmd+X-axis)  
  // set OSD buffer with user color data (color data will be sent by "write()" function later
  // returns 0 on success
  
  OSD_SetTrans,   // (transparency{color})
  // Sets transparency color-key
  // returns 0 on success

  OSD_ClrTrans,   // (transparency{color})
  // Disable transparency color-key
  // returns 0 on success

  OSD_SetBlend,   // (blending{weight})
  // Enable alpha-blending and give weighting value, 0 - 0xFF (opaque)
  // returns 0 on success

  OSD_ClrBlend,   // (blending{weight})
  // Disable alpha-blending
  // returns 0 on success

} OSD_Command;

typedef enum {
	OSD_RGB555=0xAB08, 
  	OSD_RGB565,       
  	OSD_RGBx888,       
	OSD_RGB888x,  
	OSD_ARGB888,  	
  	OSD_Cb0Y0Cr0Y1=0xAB00,       
  	OSD_Y0Cb0Y1Cr0,       
  	OSD_Cr0Y0Cb0Y1,       
  	OSD_Y0Cr0Y1Cb0,       
  	OSD_Y1Cr0Y0Cb0,       
  	OSD_Cr0Y1Cb0Y0,       
  	OSD_Y1Cb0Y0Cr0,       
  	OSD_Cb0Y1Cr0Y0,       
} OSD_Format;


typedef struct osd_cmd_s {
	int cmd;
	int x0;	
	int y0;
	int x0_size;
	int y0_size;
	int color;		// color_format, color_key
	int alpha;		// alpha blending weight	
	int format;	
} osd_cmd_t;


/*IOCTLs*/


char *pBufNumber[10];
char fnNumber[] = "pattern/0_RGB565.bin";
#define NUMBER_NUM 10
#define NUMBER_SIZE 14*20*2

char *pBufBettery[4];
char fnBettery[] = "pattern/battery001_RGB565.bin";
#define BETTERY_NUM 4
#define BETTERY_SIZE 40*22*2

char *pBufRSSI[4];
char fnRSSI[] = "pattern/RSSI_00_RGB565.bin";
#define RSSI_NUM 4
#define RSSI_SIZE 30*20*2

unsigned char *pVideoBuffer;

static unsigned short s_u16Width;
static unsigned short s_u16Height;

int load_images()
{
	FILE *fp;
	int i;

	// load number images
	for (i=0; i<NUMBER_NUM; i++) {
		fp = fopen(fnNumber, "r");
		pBufNumber[i] = malloc(NUMBER_SIZE);
		if (fp == NULL) {
			printf("Can not open image file!\n");
			exit(0);
		}
		if (fread(pBufNumber[i], NUMBER_SIZE, 1, fp) <= 0) {
			printf("Can not load the image file!\n");
			exit(0);
		}
		fclose(fp);
		fnNumber[8]++;
	}

	// load bettery images
	for (i=0; i<BETTERY_NUM; i++) {
		fp = fopen(fnBettery, "r");
		pBufBettery[i] = malloc(BETTERY_SIZE);
		if (fp == NULL) {
			printf("Can not open image file!\n");
			exit(0);
		}
		if (fread(pBufBettery[i], BETTERY_SIZE, 1, fp) <= 0) {
			printf("Can not load the image file!\n");
			exit(0);
		}
		fclose(fp);
		fnBettery[17]++;
	}

	// load RSSI images
	for (i=0; i<RSSI_NUM; i++) {
		fp = fopen(fnRSSI, "r");
		pBufRSSI[i] = malloc(RSSI_SIZE);
		if (fp == NULL) {
			printf("Can not open image file!\n");
			exit(0);
		}
		if (fread(pBufRSSI[i], RSSI_SIZE, 1, fp) <= 0) {
			printf("Can not load the image file!\n");
			exit(0);
		}
		fclose(fp);
		fnRSSI[14]++;
	}

}

int show_osd(int fd)
{
	osd_cmd_t osd_block;		
	int i = 1;
	int retval;


	while (1) {
		printf("i = %d\n", i);

		// STEP: show bettery at right-top
		osd_block.cmd = OSD_FillBlock;
		osd_block.x0 = s_u16Width-40-20;		
		osd_block.y0 = 30;								
		osd_block.x0_size = 40;												
		osd_block.y0_size = 22;																
		ioctl(fd, OSD_SEND_CMD, (unsigned long) &osd_block);
		retval = write(fd, pBufBettery[(i-1)%4],  BETTERY_SIZE/8);   			
	//	printf("retval = 0x%x \n", retval);
		retval = write(fd, pBufBettery[(i-1)%4]+ (BETTERY_SIZE/8)*1, BETTERY_SIZE/8);   			
	//	printf("retval = 0x%x \n", retval);		
		retval = write(fd, pBufBettery[(i-1)%4]+ (BETTERY_SIZE/8)*2, BETTERY_SIZE/8);   			
	//	printf("retval = 0x%x \n", retval);		
		retval = write(fd, pBufBettery[(i-1)%4]+ (BETTERY_SIZE/8)*3, BETTERY_SIZE/8);   			
	//	printf("retval = 0x%x \n", retval);		
		retval = write(fd, pBufBettery[(i-1)%4]+ (BETTERY_SIZE/8)*4, BETTERY_SIZE/8);   			
	//	printf("retval = 0x%x \n", retval);		
		retval = write(fd, pBufBettery[(i-1)%4]+ (BETTERY_SIZE/8)*5, BETTERY_SIZE/8);   			
	//	printf("retval = 0x%x \n", retval);		
		retval = write(fd, pBufBettery[(i-1)%4]+ (BETTERY_SIZE/8)*6, BETTERY_SIZE/8);   			
	//	printf("retval = 0x%x \n", retval);		
		retval = write(fd, pBufBettery[(i-1)%4]+ (BETTERY_SIZE/8)*7, BETTERY_SIZE/8);   											
	//	printf("retval = 0x%x \n", retval);		

		// STEP: show RSSI at left-top
		osd_block.cmd = OSD_FillBlock;
		osd_block.x0 = 20;		
		osd_block.y0 = 30;								
		osd_block.x0_size = 30;												
		osd_block.y0_size = 20;																
		ioctl(fd, OSD_SEND_CMD, (unsigned long) &osd_block);
		write(fd, pBufRSSI[(i-1)%4], RSSI_SIZE/4);   			
		write(fd, pBufRSSI[(i-1)%4]+ (RSSI_SIZE/4)*1, RSSI_SIZE/4);   			
		write(fd, pBufRSSI[(i-1)%4]+ (RSSI_SIZE/4)*2, RSSI_SIZE/4);   			
		write(fd, pBufRSSI[(i-1)%4]+ (RSSI_SIZE/4)*3, RSSI_SIZE/4);   			

		// STEP: show two-digits number at left-down
		osd_block.cmd = OSD_FillBlock;
		osd_block.x0 = 20;		
		osd_block.y0 = s_u16Height-20-30;								
		osd_block.x0_size = 14;												
		osd_block.y0_size = 20;																
		ioctl(fd, OSD_SEND_CMD, (unsigned long) &osd_block);
		write(fd, pBufNumber[i/10], NUMBER_SIZE/4);   			
		write(fd, pBufNumber[i/10]+ (NUMBER_SIZE/4)*1, NUMBER_SIZE/4);   			
		write(fd, pBufNumber[i/10]+ (NUMBER_SIZE/4)*2, NUMBER_SIZE/4);   			
		write(fd, pBufNumber[i/10]+ (NUMBER_SIZE/4)*3, NUMBER_SIZE/4);   			
		
		osd_block.cmd = OSD_FillBlock;
		osd_block.x0 = 40;		
		osd_block.y0 = s_u16Height-20-30;								
		osd_block.x0_size = 14;												
		osd_block.y0_size = 20;																
		ioctl(fd, OSD_SEND_CMD, (unsigned long) &osd_block);
		write(fd, pBufNumber[i%10], NUMBER_SIZE/4);   					
		write(fd, pBufNumber[i%10]+ (NUMBER_SIZE/4)*1, NUMBER_SIZE/4);   					
		write(fd, pBufNumber[i%10]+ (NUMBER_SIZE/4)*2, NUMBER_SIZE/4);   					
		write(fd, pBufNumber[i%10]+ (NUMBER_SIZE/4)*3, NUMBER_SIZE/4);   					

//		if (++i > 99)
//			i = 1;
		if (++i > 5)
			break;
		sleep(1);
	}

	return 0;
}

int main()
{
	static struct fb_var_screeninfo var;
	static struct fb_fix_screeninfo fix;	
	int fd, i, j, k, l, buf;
	char* pbuf;
	int* puBuf;	
	unsigned long uVideoSize;
	osd_cmd_t osd_block;	
	FILE *fpVideoImg;	
	int osd_offset;
	int fb_buf_size;	
	int fbiopan_display = 0;
	int retval;	

	fd = open("/dev/fb0", O_RDWR);
	if (fd < 0) {
		printf("Can not open fb0!\n");
		return -1;
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0) {
		perror("ioctl FBIOGET_VSCREENINFO");
		close(fd);
		return -1;
	}
	uVideoSize = var.xres * var.yres * var.bits_per_pixel / 8;
	
	s_u16Width = var.xres;
	s_u16Height = var.yres;

	var.yoffset = 0;

	if (ioctl(fd, IOCTL_GET_OSD_OFFSET, &osd_offset) < 0) {
		perror("ioctl IOCTL_GET_OSD_OFFSET");
		close(fd);
		return -1;
	}

	if (ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0) {
		perror("ioctl FBIOGET_FSCREENINFO");
		close(fd);
		return -1;
	}

	pVideoBuffer = mmap(NULL, fix.smem_len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (pVideoBuffer == MAP_FAILED) {
		printf("LCD Video Map failed!\n");
		exit(0);
	}

	printf("pVideoBuffer = 0x%x\n", pVideoBuffer);
	printf("osd_offset = 0x%x\n", osd_offset);	
	printf("fix.smem_len = 0x%x\n", fix.smem_len);		

	// check RGB565 or RGB888 demo
	if (var.bits_per_pixel == 32)
		goto _ARGB888_demo;		

	// *** ARGB888+RGB888 demo ***		
	// load background
	if (uVideoSize == 320*240*2)
		fpVideoImg = fopen("pattern/320x240.dat", "r");	
	else if (uVideoSize == 480*272*2)
		fpVideoImg = fopen("pattern/480x272.dat", "r");	
	else if (uVideoSize == 800*480*2)
		fpVideoImg = fopen("pattern/800x480.dat", "r");
	else
	{
		printf("No matched background image found!\n");
		exit(0);
	}
	
	 if(fpVideoImg == NULL)
    {
    	printf("open Image FILE fail !! \n");
    	exit(0);
    }  
    
	if( fread(pVideoBuffer, uVideoSize, 1, fpVideoImg) <= 0)
	{
		printf("Cannot Read the Image File!\n");
		exit(0);
	}

	load_images();

	// STEP: set color key to white(0xFFFF)
	osd_block.cmd = OSD_SetTrans;
	osd_block.color = 0xffff;		// STEP: set color key to white(0xFFFF)
	ioctl(fd,OSD_SEND_CMD, (unsigned long) &osd_block);
	
	// STEP: clear all of OSD buffer
	osd_block.format = OSD_RGB565;
	osd_block.cmd = OSD_Open;			
	ioctl(fd,OSD_SEND_CMD, (unsigned long) &osd_block);
	osd_block.cmd = OSD_Clear;    			
	ioctl(fd,OSD_SEND_CMD, (unsigned long) &osd_block);
	
	// STEP: set left and right sides color to gray(0xEF5B)
	osd_block.cmd = OSD_Fill;
	osd_block.x0 = 0x00;		// OSD_Fill command test		
	osd_block.y0 = 0x00;								
	osd_block.x0_size = 80;												
	osd_block.y0_size = s_u16Height;																
	osd_block.color = 0xEF5B;
	ioctl(fd,OSD_SEND_CMD, (unsigned long) &osd_block);

	osd_block.cmd = OSD_Fill;
	osd_block.x0 = s_u16Width-80;		// OSD_Fill command test		
	osd_block.y0 = 0x00;								
	osd_block.x0_size = 80;												
	osd_block.y0_size = s_u16Height;																
	osd_block.color = 0xEF5B;
	ioctl(fd,OSD_SEND_CMD, (unsigned long) &osd_block);
	
	// STEP: enable OSD
	osd_block.cmd = OSD_Show;
	ioctl(fd,OSD_SEND_CMD, (unsigned long) &osd_block);

	show_osd(fd);

	// STEP: clear color key to white(0xFFFF)
	osd_block.cmd = OSD_ClrTrans;
	ioctl(fd,OSD_SEND_CMD, (unsigned long) &osd_block);
  	
	// OSD is replaced by another image 
	fclose(fpVideoImg);
	if (uVideoSize == 320*240*2)
		fpVideoImg = fopen("pattern/osd_320x240.dat", "r");		// green color rectangle in central
	else if (uVideoSize == 480*272*2)
		fpVideoImg = fopen("pattern/osd_480x272.dat", "r");		// green color rectangle in central
	else if (uVideoSize == 800*480*2)
		fpVideoImg = fopen("pattern/osd_800x480.dat", "r");		// green color rectangle in central
	else
	{
		printf("No matched osd image found!\n");
		exit(0);
	}

	 if(fpVideoImg == NULL)
    {
    	printf("open OSD FILE fail !! \n");
    	exit(0);
    }  
    
	if( fread(pVideoBuffer+osd_offset, uVideoSize, 1, fpVideoImg) <= 0)
	{
		printf("Cannot Read the OSD File!\n");
		exit(0);
	}

	fclose(fpVideoImg);
		
	// demo OSD constant alpha-blending feature
	printf("demo OSD constant alpha-blending feature !! \n");	
	i = 0;
	while(1)
	{
		printf("i = %d\n", i);
				
		if (++i > 16)
			break;
			
		osd_block.alpha = i*0x10;			
		osd_block.cmd = OSD_SetBlend;		// enable OSD alpha-blending 
		ioctl(fd,OSD_SEND_CMD, (unsigned long) &osd_block);		
		sleep(1);		
	}		
			
	osd_block.cmd = OSD_ClrBlend;		// disable OSD alpha-blending 
	ioctl(fd,OSD_SEND_CMD, (unsigned long) &osd_block);					


	// disable OSD 
	printf("disable OSD !! \n");	
	osd_block.cmd = OSD_Hide;
	ioctl(fd,OSD_SEND_CMD, (unsigned long) &osd_block);

	close(fd);

	// *** ARGB888+RGB888 demo ***
_ARGB888_demo:

	// load background 
	if (uVideoSize == 320*240*4)
		fpVideoImg = fopen("pattern/320x240_RGBx888.dat", "r");	
//	else if (uVideoSize == 480*272*2)
//		fpVideoImg = fopen("pattern/480x272_RGBx888.dat", "r");	
//	else if (uVideoSize == 800*480*2)
//		fpVideoImg = fopen("pattern/800x480_RGBx888.dat", "r");
	else
	{
		printf("No matched background image found!\n");
		exit(0);
	}
	
	 if(fpVideoImg == NULL)
    {
    	printf("open Image FILE fail !! \n");
    	exit(0);
    }  
    
	if( fread(pVideoBuffer, uVideoSize, 1, fpVideoImg) <= 0)
	{
		printf("Cannot Read the Image File!\n");
		exit(0);
	}
	fclose(fpVideoImg);	

	// display background	
	ioctl(fd,IOCTL_FB_UNLOCK);

	osd_block.format = OSD_ARGB888;
//	osd_block.format = OSD_RGBx888;
	
	osd_block.cmd = OSD_Open;			
	ioctl(fd,OSD_SEND_CMD, (unsigned long) &osd_block);

	// STEP: clear all of OSD buffer
	osd_block.cmd = OSD_Clear;    			
	ioctl(fd,OSD_SEND_CMD, (unsigned long) &osd_block);

	// STEP: disable transparent feature
	osd_block.cmd = OSD_ClrTrans;
	ioctl(fd,OSD_SEND_CMD, (unsigned long) &osd_block);

	// STEP: enable OSD
	osd_block.cmd = OSD_Show;
	ioctl(fd,OSD_SEND_CMD, (unsigned long) &osd_block);
	
	// load an image to OSD buffer
	if (uVideoSize == 320*240*4)
		fpVideoImg = fopen("pattern/osd_320x240_RGBx888.dat", "r");		// green color rectangle in central
//	else if (uVideoSize == 480*272*2)
//		fpVideoImg = fopen("pattern/osd_480x272_RGBx888.dat", "r");		// green color rectangle in central
//	else if (uVideoSize == 800*480*2)
//		fpVideoImg = fopen("pattern/osd_800x480_RGBx888.dat", "r");		// green color rectangle in central
	else
	{
		printf("No matched osd image found!\n");
		exit(0);
	}

	 if(fpVideoImg == NULL)
    {
    	printf("open OSD FILE fail !! \n");
    	exit(0);
    }  
    
	if( fread(pVideoBuffer+osd_offset, uVideoSize, 1, fpVideoImg) <= 0)
	{
		printf("Cannot Read the OSD File!\n");
		exit(0);
	}
	
	#define X_AXIS		5		// x-axis is divided into 5 parts
	#define Y_AXIS		4	
	
	pbuf = (char*)pVideoBuffer+osd_offset;
	i = 0;
	while (1) {
		puBuf = (int*)pbuf;		
		printf("i = 0x%x !!!\n", i);				
		for ( l=0; l<X_AXIS*Y_AXIS; l++)	
		{
			for ( j=0; j<var.yres/Y_AXIS; j++)
			{
				for ( k=0; k<var.xres/X_AXIS; k++)
				{
					buf = *puBuf;	
					buf += 0x80000000-i*0x10000000;
					*puBuf = buf;
					puBuf++;
				}
				puBuf+=var.xres*(X_AXIS-1)/X_AXIS;				
			}			
			if ((l+1)%X_AXIS)	// check if next row will be displayed
			{
				puBuf-=var.xres*var.yres/Y_AXIS;							
				puBuf+=var.xres/X_AXIS;										
			}				
			else
				puBuf-=var.xres*(X_AXIS-1)/X_AXIS;	// next row							
				
			sleep(1);						
		}					

		if (++i > 1)
			break;
	}

	fclose(fpVideoImg);
	close(fd);
	
	return 0;
}


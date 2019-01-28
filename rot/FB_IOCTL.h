/* FB_IOCTL.h
 *
 *
 * Copyright (c)2008 Nuvoton technology corporation
 * http://www.nuvoton.com
 *
 * frame buffer ioctl commands.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef __FB_IOCTL_H__
#define __FB_IOCTL_H__

#include <asm/ioctl.h>

//FB ioctl commands, copy from BSP/linux/include/asm-arm/arch-w55fa95/w55fa95_fb.h

#define DISPLAY_MODE_RGB555     0
#define DISPLAY_MODE_RGB565     1
#define DISPLAY_MODE_CBYCRY     4
#define DISPLAY_MODE_YCBYCR     5
#define DISPLAY_MODE_CRYCBY     6
#define DISPLAY_MODE_YCRYCB     7


#define VIDEO_ACTIVE_WINDOW_COORDINATES	_IOW('v', 22, unsigned int)	//set display-start line in display buffer
#define VIDEO_DISPLAY_ON				_IOW('v', 24, unsigned int)	//display on
#define VIDEO_DISPLAY_OFF				_IOW('v', 25, unsigned int)	//display off
#define IOCTLCLEARSCREEN				_IOW('v', 26, unsigned int)	//clear screen
#define IOCTL_LCD_BRIGHTNESS			_IOW('v', 27, unsigned int)  //brightness control		
#define IOCTLBLACKSCREEN				_IOW('v', 23, unsigned int)	//clear screen ****** changed from 28 > 23

#define IOCTL_LCD_ENABLE_INT			_IO('v', 28)
#define IOCTL_LCD_DISABLE_INT			_IO('v', 29)

#define IOCTL_LCD_RGB565_2_RGB555		_IO('v', 30)
#define IOCTL_LCD_RGB555_2_RGB565		_IO('v', 31)
#define IOCTL_LCD_GET_DMA_BASE          _IOR('v', 32, unsigned int *)
#define DUMP_LCD_REG					_IOR('v', 33, unsigned int *)
#define VIDEO_DISPLAY_LCD				_IOW('v', 38, unsigned int)	//display LCD only
#define VIDEO_DISPLAY_TV				_IOW('v', 39, unsigned int)	//display TV only 

#define VIDEO_FORMAT_CHANGE				_IOW('v', 50, unsigned int)	//change video source format between RGB565 and YUV
#define VIDEO_TV_SYSTEM					_IOW('v', 51, unsigned int)	//set TV NTSC/PAL system 


#define IOCTL_GET_OSD_OFFSET    		_IOR('v', 60, unsigned int *)

//#define IOCTL_OSD_DIRTY	    		_IOW('v', 61, unsigned int )
#define IOCTL_OSD_LOCK					_IOW('v', 62, unsigned int)	
#define IOCTL_OSD_UNLOCK				_IOW('v', 63, unsigned int)	
#define IOCTL_FB_LOCK					_IOW('v', 64, unsigned int)	
#define IOCTL_FB_UNLOCK					_IOW('v', 65, unsigned int)	
#define IOCTL_FB_LOCK_RESET				_IOW('v', 66, unsigned int)	

#define IOCTL_WAIT_VSYNC				_IOW('v', 67, unsigned int)	

#define SET_OSD_SIZE_ENABLE				_IOW('v', 70, unsigned int)	// set OSD size enable
#define SET_OSD_SIZE_DISABLE			_IOW('v', 71, unsigned int)	// set OSD size disable

#define IOCTL_GET_FB_OFFLINE   	 		_IOR('v', 80, unsigned int *)
#define IOCTL_GET_OSD_OFFLINE   		_IOR('v', 81, unsigned int *)


#define OSD_SEND_CMD            		_IOW('v', 160, unsigned int *)

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

} OSD_Command;

typedef struct osd_cmd_s {
	int cmd;
	int x0;	
	int y0;
	int x0_size;
	int y0_size;
	int color;		// color_format, color_key
} osd_cmd_t;

#endif
 

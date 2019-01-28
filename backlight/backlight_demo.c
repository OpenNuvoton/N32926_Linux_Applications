
/****************************************************************************
 *                                                                          *
 * Copyright (c) 2010 Nuvoton Technology Corp. All rights reserved.         *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 *
 * FILENAME
 *     backlight.c
 *
 * DESCRIPTION
 *     This file is a LCD backlight demo program
 *
 ****************************************************************************/

#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/ioctl.h>
#include<pthread.h>
#include<fcntl.h>
#include<linux/input.h>

#define FB_DEV "/dev/fb0"

#define IOCTL_LCD_BRIGHTNESS	_IOW('v', 27, unsigned int)	//brightness control

int main(void)
{
	struct input_event data;
	int fb_fd;
	unsigned int select = 0;
	int brightness = 100;	

	if ((fb_fd = open(FB_DEV, O_RDONLY)) < 0) {
		printf("open device %s failed", FB_DEV);
		return -1;
	}

	while (1) {
					
		printf("\n************* PWM demo ************\n");
		printf("1.Set Brightness Value Down ..\n");
   		printf("2.Set Brightness Value Up ..\n");
   		
   		scanf("%d", &select);   		
   		
   		if (select == 0x1)
			brightness -= 10;
		else if (select == 0x2)
			brightness += 10;
		else
			continue;

		if (brightness <= 0)
			brightness = 1;
		if (brightness > 100)
			brightness = 100;
		printf("brightness = %d\n", brightness);

		ioctl(fb_fd, IOCTL_LCD_BRIGHTNESS, &brightness);
		
		usleep(100000);
	}
}


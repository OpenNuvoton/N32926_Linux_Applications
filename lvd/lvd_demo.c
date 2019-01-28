
/****************************************************************************
 *                                                                          *
 * Copyright (c) 2010 Nuvoton Technology Corp. All rights reserved.         *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 *
 * FILENAME
 *     lvd_demo.c
 *
 * DESCRIPTION
 *     This program is used to do voltage detection via ADC
 *
 ****************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define TS_DEV		"/dev/input/event0"
#define ADCSYS_FILE	"/sys/devices/platform/w55fa92-adc/adc"
#define LVD_INTERVAL 4 
#define BUF_SIZE	8

int main(int argc, char *argv[])
{
	int tsfd, adcfd;
	char selection=1;	//default;
	char buf[BUF_SIZE];
	int32_t i32Opt;

	// if another application had opened touchscreen device, you can skip this step
	if ((tsfd = open(TS_DEV, O_RDONLY)) < 0) {
		printf("open device %s failed, errno = %d\n", TS_DEV, errno);
		return -1;
	}
	usleep(1);

	while((i32Opt = getopt(argc, argv, "s:")) != -1) {
		switch(i32Opt){
			case 's':
			{
				if(strcasecmp(optarg, "0") == 0){
					selection = '0';
					printf("Don't check\n");
				}
				if(strcasecmp(optarg, "1") == 0){
					selection = '1';
					printf("Check channel 1 and 2\n");
				}
				else if(strcasecmp(optarg, "2") == 0){
					selection = '2';
					printf("Check channel 3 and 4\n");
				}
				else if(strcasecmp(optarg, "3") == 0){
					selection = '3';	
					printf("Check channel 5 and 6\n");
				}
				else if(strcasecmp(optarg, "4") == 0){
					selection = '4';
					printf("Check channel 7\n");
				}
			}
			sleep(1);
			break;
		}
	}

	while (1) {
		
		// read voltage
		adcfd = open(ADCSYS_FILE, O_RDWR);
		if (adcfd < 0) {
			printf("open device %s failed, errno = %d\n", ADCSYS_FILE, errno);
			return -1;
		}
		memset(&buf[0], 0x0, BUF_SIZE);		
		buf[0]=selection; 
		write(adcfd, buf, BUF_SIZE);
		lseek(adcfd, 0, SEEK_SET);
		read(adcfd, buf, BUF_SIZE);
		printf("buf = %x, %x, %x, %x\n", buf[0], buf[1], buf[2], buf[3]);	//Higher channel is in lower word. Lower channel is in high word */
		printf("voltage = %1.2f\n", (float)(buf[3]*256 + buf[2])/4096.0*9.9);
		close(adcfd);
		sleep(LVD_INTERVAL);
	}

	return 0;
}


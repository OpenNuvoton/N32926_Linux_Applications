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

#include "ROT_IOCTL.h"
#include "demo.h"

/* ROT device */
char rot_device[] = "/dev/rot";

int32_t i32DevROT; 
int32_t i32MapROTBuf;
uint8_t* pu8MapROTBuf = NULL;		/* ROT mapping virtual address */
S_ROT_BUF sBufInfo;
int32_t OpenROTDevice(
	uint32_t* pu32RotPhysicalAddr		
)
{
	uint8_t* pu8RotBuf;

	i32DevROT = open(rot_device, O_RDWR);		
	if (i32DevROT < 0){
		ERR_PRINTF("open %s error\n", rot_device);
		return -1;	
	}	
	else
		printf("open %s successfully\n", rot_device);
	
	/* Polling the rot buffer's dimension */
	if (ioctl(i32DevROT, ROT_BUF_INFO, &sBufInfo) < 0) {
		ERR_PRINTF("Err ioctl ROT get dimension \n");
		close(i32DevROT);
		return -1;
	}

	DBG_PRINTF("ROT W/H/PW = %d, %d, %d\n", sBufInfo.u32Width, sBufInfo.u32Height, sBufInfo.u32PixelWidth);	
	DBG_PRINTF("ROT buf physical address%x\n", sBufInfo.u32PhysicalAddr);
	*pu32RotPhysicalAddr = sBufInfo.u32PhysicalAddr;

	i32MapROTBuf =  sBufInfo.u32Width*sBufInfo.u32Height*sBufInfo.u32PixelWidth;
	pu8RotBuf = mmap( NULL, i32MapROTBuf, PROT_READ|PROT_WRITE, MAP_SHARED, i32DevROT, 0 );
	
	if ((unsigned char*)-1 == pu8RotBuf) {
		ERR_PRINTF("AP mapping rotation buffer fail\n");
		close(i32DevROT);		
		return -1;
	}
	pu8MapROTBuf = pu8RotBuf;
	return i32DevROT;
}
void CloseROTDevice(void)
{
	if(pu8MapROTBuf!=NULL)
		munmap(pu8MapROTBuf, i32MapROTBuf);
}

	

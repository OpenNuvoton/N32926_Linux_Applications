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

#include "FB_IOCTL.h"
#include "demo.h"

int32_t i32DevFB; 
int32_t i32MapSizeFB;
uint8_t *pu8MapFBBuf = NULL;		/* VPOST mapping virtual address */
uint32_t gu32FBPhysicalAddr;			/* VPOST Off line buffer physical address */

struct fb_var_screeninfo gsVar;


int /* Return file description */
OpenFBDevice(
	uint32_t *pu32PhyBufAddr		/* Off line buffer physical address */
)
{
	int32_t i32FBFd;
	
	char achFBDevice[] = "/dev/fb0";
	uint8_t *pu8FBBuf = MAP_FAILED;

	i32FBFd = open(achFBDevice, O_RDWR);	
	if(i32FBFd < 0){
		printf("Open FB fail\n");
			
		return -1;
	}
	i32DevFB = i32FBFd;
	DBG_PRINTF("fb open successful\n");

	if (ioctl(i32FBFd, FBIOGET_VSCREENINFO, &gsVar) < 0) {
		ERR_PRINTF("ioctl FBIOGET_VSCREENINFO \n");
		close(i32FBFd);
		return -1;
	}

	if (ioctl(i32FBFd, IOCTL_GET_FB_OFFLINE, pu32PhyBufAddr) < 0) {
		ERR_PRINTF("Err IOCTL_GET_FB_OFFLINE \n");
		close(i32FBFd);
		return -1;
	}
	DBG_PRINTF("FB buffer physical address = %x\n", *pu32PhyBufAddr);
	gu32FBPhysicalAddr = *pu32PhyBufAddr;
	
	i32MapSizeFB = gsVar.xres*gsVar.yres*2*2; //dual bufffer
	pu8FBBuf = mmap( NULL, i32MapSizeFB, PROT_READ|PROT_WRITE, MAP_SHARED, i32FBFd, 0 );
	if ((unsigned char*)-1 == pu8FBBuf) {
		ERR_PRINTF("AP mapping frame buffer fail\n");
		close(i32FBFd);
		return -1;
	}
	pu8MapFBBuf = pu8FBBuf;

	return i32DevFB;	
}
void CloseFBDevice(void)
{
	if(pu8MapFBBuf!=NULL)
		munmap(pu8MapFBBuf, i32MapSizeFB);
}
void ShowResult(uint32_t buf)
{
	if(buf==0)
	{//buf =0		
		gsVar.yoffset = 0;
		if (ioctl(i32DevFB, FBIOPAN_DISPLAY, &gsVar) < 0) {
			printf("ioctl FBIOPAN_DISPLAY");
		}
		ioctl(i32DevFB,IOCTL_WAIT_VSYNC);
		memset((uint8_t*)((uint32_t)pu8MapFBBuf+gsVar.xres*gsVar.yres*2), 0, gsVar.xres*gsVar.yres*2);
		DBG_PRINTF("Show buffer 0\n");
	}
	else
	{//buf =1
		gsVar.yoffset = gsVar.yres;
		if (ioctl(i32DevFB, FBIOPAN_DISPLAY, &gsVar) < 0) {
			printf("ioctl FBIOPAN_DISPLAY");
		}
		ioctl(i32DevFB,IOCTL_WAIT_VSYNC);
		memset(pu8MapFBBuf, 0, gsVar.xres*gsVar.yres*2);
		DBG_PRINTF("Show buffer 1\n");
	}
}

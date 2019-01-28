#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <linux/fb.h>
#include <unistd.h>
#include "w55fa92_vpe.h"

#define DBGPRINTF(...)
/* VPE device */
char vpe_device[] = "/dev/vpe";
int vpe_fd;

/* VPOST device */
char vpost_device[] = "/dev/fb0";
int vpost_fd;
struct fb_var_screeninfo var;

#define LCD_ENABLE_INT		_IO('v', 28)
#define LCD_DISABLE_INT		_IO('v', 29)

unsigned int g_u32VpostBufMapSize = 0;	/* For munmap use */
unsigned char *pg_VpostBuffer;		/* For unmap use */
unsigned int g_u32VpostWidth=0, g_u32VpostHeight =0;
#define PAT_WIDTH		1280
#define PAT_HEIGHT		720
#define STEP_WIDTH		32
#define STEP_HEIGHT		18
#undef _USE_FBIOPAN_
#undef _NON_TEARING_
int FormatConversion(char* pSrcBuf, char* pDstBuf, int Tarwidth, int Tarheight)
{
	unsigned int volatile vworkbuf, tmpsize;
	unsigned int value, buf=0, ret =0;
	vpe_transform_t vpe_setting;
	unsigned int ptr_y, ptr_u, ptr_v, step=0;
	ptr_y = (unsigned int)pSrcBuf;
	ptr_u = ptr_y+PAT_WIDTH*PAT_HEIGHT;		/* Planar YUV422 */ 
	ptr_v = ptr_u+PAT_WIDTH*PAT_HEIGHT/2;
	// Get Video Decoder IP Register size
	if( ioctl(vpe_fd, VPE_INIT, NULL)  < 0){
		close(vpe_fd);
		printf("VPE_INIT fail\n");		
		ret = -1;
	}	
	value = 0x5a;
	if((ioctl(vpe_fd, VPE_IO_SET, &value)) < 0){
		close(vpe_fd);
		printf("VPE_IO_SET fail\n");
		ret = -1;
	}			

	value = 1;	
	ioctl(vpe_fd, VPE_SET_MMU_MODE, &value);

	vpe_setting.src_width = PAT_WIDTH;
	unsigned int test=0;
	if(vpe_setting.dest_addrPac==(unsigned int)pDstBuf){// Show buffer 1. Because VPE always filled from buffer 0. 
		var.yoffset = var.yres;
#ifdef _USE_FBIOPAN_
		if (ioctl(vpost_fd, FBIOPAN_DISPLAY, &var) < 0) {
			printf("ioctl FBIOPAN_DISPLAY");
		}
#endif
	}
	while( ((vpe_setting.src_width>=g_u32VpostWidth))&&(vpe_setting.src_height>=g_u32VpostHeight ) ){	
		vpe_setting.src_addrPacY = ptr_y + PAT_WIDTH*STEP_HEIGHT*step/2;				/* Planar YUV422 */
		vpe_setting.src_addrU = ptr_u+PAT_WIDTH/2*STEP_HEIGHT*step/2;	
		vpe_setting.src_addrV = ptr_v+PAT_WIDTH/2*STEP_HEIGHT*step/2;				
		vpe_setting.src_format = VPE_SRC_PLANAR_YUV422;
		vpe_setting.src_width = PAT_WIDTH - step*STEP_WIDTH;
		vpe_setting.src_height = PAT_HEIGHT - step*STEP_HEIGHT;

		vpe_setting.src_leftoffset = step*STEP_WIDTH/2;
		vpe_setting.src_rightoffset = step*STEP_WIDTH/2;
		DBGPRINTF("Src dimension = %d x %d\n", vpe_setting.src_width, vpe_setting.src_height);
		DBGPRINTF("Src offset(L,R) = %d , %d\n", vpe_setting.src_leftoffset, vpe_setting.src_rightoffset);
		
		if(buf==0){/* Render to the first buffer, VPOST show the 2nd buffer */
			vpe_setting.dest_addrPac = (unsigned int)pDstBuf;
		}
		else{
			vpe_setting.dest_addrPac = (unsigned int)pDstBuf+ var.xres*var.yres*var.bits_per_pixel/8;	
		}
		DBGPRINTF("Buf %x, Start Addr = 0x%x", buf, vpe_setting.dest_addrPac);
		vpe_setting.dest_format = VPE_DST_PACKET_RGB565;
		vpe_setting.dest_width = Tarwidth;
		vpe_setting.dest_height = Tarheight;
		vpe_setting.dest_leftoffset = (g_u32VpostWidth-Tarwidth)/2;
		vpe_setting.dest_rightoffset = (g_u32VpostWidth-Tarwidth)/2;
		DBGPRINTF("Dst dimension = %d x %d\n", vpe_setting.dest_width, vpe_setting.dest_height);
		DBGPRINTF("Dst offset(L,R) = %d , %d\n", vpe_setting.dest_leftoffset, vpe_setting.dest_rightoffset);
		vpe_setting.algorithm = VPE_SCALE_DDA;
		vpe_setting.rotation = VPE_OP_NORMAL;
		
		if((ioctl(vpe_fd, VPE_SET_FORMAT_TRANSFORM, &vpe_setting)) < 0){
			close(vpe_fd);
			DBGPRINTF("VPE_IO_GET fail\n");
			ret = -1;
		}				
#ifndef _USE_FBIOPAN_
#ifdef _NON_TEARING_
		ioctl(vpost_fd,LCD_DISABLE_INT);
#endif
#endif
		if((ioctl(vpe_fd, VPE_TRIGGER, NULL)) < 0){
			close(vpe_fd);
			DBGPRINTF("VPE_TRIGGER info fail\n");
			ret = -1;
		}		
		//ioctl(vpe_fd, VPE_WAIT_INTERRUPT, &value);
		while (ioctl (vpe_fd, VPE_WAIT_INTERRUPT, &value)) {
			if (errno == EINTR) {
				continue;
			}
			else {
				printf ("%s: VPE_WAIT_INTERRUPT failed: %s\n", __FUNCTION__, strerror (errno));
				ret = -1;
				break;
			}
		}

		if(vpe_setting.dest_addrPac==(unsigned int)pDstBuf){//Show buffer 0
			var.yoffset = 0;
#ifdef _USE_FBIOPAN_
			if (ioctl(vpost_fd, FBIOPAN_DISPLAY, &var) < 0) {
				printf("ioctl FBIOPAN_DISPLAY");
			}
#else
#ifdef _NON_TEARING_
			ioctl(vpost_fd,LCD_ENABLE_INT);
#endif
#endif

		}	
		else{
			var.yoffset = var.yres;
			if (ioctl(vpost_fd, FBIOPAN_DISPLAY, &var) < 0) {//Show buffer 1
				printf("ioctl FBIOPAN_DISPLAY");
			}		
		}
		step = step +1;
#ifdef _USE_FBIOPAN_
		if(buf == 0)
			buf =1;
		else
			buf =0;
#endif
	}
	
	if(vpe_setting.dest_addrPac==(unsigned int)pDstBuf)
	{//VPOST should show buffer 1 for tearing issue if restart. 
		vpe_setting.dest_addrPac = (unsigned int)pDstBuf+ var.xres*var.yres*var.bits_per_pixel/8;
#ifndef _USE_FBIOPAN_
#ifdef _NON_TEARING_
		ioctl(vpost_fd,LCD_DISABLE_INT);
#endif
#endif
		if((ioctl(vpe_fd, VPE_TRIGGER, NULL)) < 0){
			close(vpe_fd);
			DBGPRINTF("VPE_TRIGGER info fail\n");
			ret = -1;
		}		
		//ioctl(vpe_fd, VPE_WAIT_INTERRUPT, &value);
		while (ioctl (vpe_fd, VPE_WAIT_INTERRUPT, &value)) {
			if (errno == EINTR) {
				continue;
			}
		}
		var.yoffset = var.yres;
#ifdef _USE_FBIOPAN_
		if (ioctl(vpost_fd, FBIOPAN_DISPLAY, &var) < 0) {//Show buffer 1
				printf("ioctl FBIOPAN_DISPLAY");
		}			
#else
#ifdef _NON_TEARING_
		ioctl(vpost_fd,LCD_ENABLE_INT);
#endif
#endif
	}
	return ret;
	
}	

#define SRCBUFSIZE	(1843200)
#define DSTBUFSIZE	(480*272*2)
//#define SRCBUFSIZE	(640*480*2)
//#define DSTBUFSIZE	(640*480*2)
void loadSrcPattern(char *p8SrcBuf)
{
	FILE *fpr;
	int ret;

	printf("The pattern \"src.pb\" and application \"vpe_demo\" have to store in the same directory.\n");
	fpr = fopen("src.pb", "rb");
	if(fpr<0){		
		printf("Open read file  error \n"); 
		exit(-1);
	}
	fread(p8SrcBuf, 1, SRCBUFSIZE, fpr);
	fclose(fpr);
	sync();
}


int FrameBufferDimension(int fd, int* width, int* height, int* bpp) 	
{
	if (ioctl(vpost_fd, FBIOGET_VSCREENINFO, &var) < 0) {
		printf("ioctl FBIOGET_VSCREENINFO");
		close(vpost_fd);
		return -1;
	}
	*width = var.xres;
	*height = var.yres;
	*bpp = var.bits_per_pixel;
	return 0;
}

int InitVPE(void)
{
	unsigned int version;

	vpe_fd = open(vpe_device, O_RDWR);		
	if (vpe_fd < 0){
		printf("open %s error\n", vpe_device);
		return -1;	
	}	
	else
		printf("open %s successfully\n", vpe_device);
	
	return 0;
}



int InitVpost(void)
{
	int uVideoSize, width, height, bpp, bIsOsdInit=0;

	vpost_fd = open(vpost_device, O_RDWR);
	printf("Frame buffer file description = %d\n", vpost_fd);
	if (vpost_fd == -1){
		printf("Cannot open fb0!\n");
		return -1;
	}	
				
	if(FrameBufferDimension(vpost_fd, &width, &height, &bpp)<0){
		fprintf(stderr,"Get Frame Parameter Failed:%d\n", errno);
		return -1;
	}	

	printf("LCD Dimension = %d, %d\n", width, height);
	printf("LCD bpp = %d\n", bpp);	
#ifdef _USE_FBIOPAN_
	g_u32VpostBufMapSize = width*height*bpp/8*2;	/* MAP 2 buffer */
#else
	g_u32VpostBufMapSize = width*height*bpp/8*1;	/* MAP 1 buffer */
#endif
	printf("Map total buffer = %x\n", g_u32VpostBufMapSize);	
	g_u32VpostWidth = width;
	g_u32VpostHeight = height;
	
	pg_VpostBuffer = mmap(NULL, 
						g_u32VpostBufMapSize, 
						PROT_READ|PROT_WRITE, 
						MAP_SHARED, 
						vpost_fd, 
						0);
	memset(pg_VpostBuffer, 0x0, g_u32VpostBufMapSize);
	if (pg_VpostBuffer == MAP_FAILED) {
		printf("LCD Video Map failed!\n");
		exit(0);
	}
	var.yoffset = var.yres;
#ifdef _USE_FBIOPAN_
	if (ioctl(vpost_fd, FBIOPAN_DISPLAY, &var) < 0) {
		printf("ioctl FBIOPAN_DISPLAY");
		exit(0);
	}	
#endif
	return 0;
}
static void myExitHandler (int sig)
{
  	printf("Exit Signal Handler\n");	
	if((ioctl(vpe_fd, VPE_STOP, NULL)) < 0)	{
		close(vpe_fd);
		printf("VPE_STOP fail\n");
	}
	fflush(stdout);
	usleep(1000000);
	exit(-9);
}
int main()
{
	unsigned int select = 0;
	char *p8SrcBuf;

	signal (SIGINT, myExitHandler);
	//signal (SIGQUIT, myExitHandler);
 	//signal (SIGILL, myExitHandler);
  	//signal (SIGABRT, myExitHandler);
  	//signal (SIGFPE, myExitHandler);
  	signal (SIGKILL, myExitHandler);
	//signal (SIGPIPE, myExitHandler);
	signal (SIGTERM, myExitHandler);

	if(InitVpost()){
		exit(-1);	
	}
	/* Allocate buffer for 3M pixels srouce pattern */ 	
	if ( InitVPE() ){				
		exit(-1);
	}	
	p8SrcBuf = malloc(SRCBUFSIZE);	
	if(p8SrcBuf == NULL){
		printf("Allocate src buffer faile \n");
		exit(-1);
	}
	

	printf("Allocate Buf addr = 0x%x\n", (unsigned int)p8SrcBuf);
	memset (p8SrcBuf, 0x00, SRCBUFSIZE);
	
	loadSrcPattern(p8SrcBuf);
	while(1){
		FormatConversion(p8SrcBuf, pg_VpostBuffer, g_u32VpostWidth, g_u32VpostHeight);
		printf("Done\n");
	}		
	if(p8SrcBuf!=NULL)
		free(p8SrcBuf);
	
	munmap(pg_VpostBuffer, g_u32VpostBufMapSize);
	if (vpost_fd >0)
		close(vpost_fd);

	if (vpe_fd >0)
		close(vpe_fd);		
}

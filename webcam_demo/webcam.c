#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "fb_lib.h"
#include "cam_lib.h"
#include "w55fa92_vpe.h"
#include <dirent.h>
#include "jpegcodec.h"
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <asm/ioctl.h>
//#include <asm/arch/hardware.h>
#include <linux/vt.h>
#include <linux/kd.h>

int BUFFER_COUNT;
int fb_fd;
int fd_jpeg;
int32_t i32KpdFd;
unsigned char *pJpegBuffer = NULL; 
char device[] = "/dev/video0";

int image_width = 640;
int image_height = 480;
unsigned int jpeg_buffer_paddress = 0, jpeg_buffer_vaddress = 0;
unsigned int user_memorypool_enable = 0; 
int frame_data_size;


unsigned char *fb_addr;
static int 
InitKpdDevice(void)
{
	char achDevName[256];
	char achKeypad[] = "Keypad";
	char achKpdDevice[] = "/dev/input/event0";
	int32_t i32KpdFd= -1;
	uint32_t u32Result;
	int32_t i;	
	u32Result = 0;
	for (i = 0; i < RETRY_NUM; i++) {
		DEBUG_PRINT("trying to open %s ...\n", achKpdDevice);
		if ((i32KpdFd=open(achKpdDevice, O_RDONLY | O_NONBLOCK)) < 0) {
			break;
		}
		memset(achDevName, 0x0, sizeof(achDevName));
		if (ioctl(i32KpdFd, EVIOCGNAME(sizeof(achDevName)), achDevName) < 0) {
			DEBUG_PRINT("%s evdev ioctl error!\n", achKpdDevice);
		}
		else{
			if (strstr(achDevName, achKeypad) != NULL) {
				DEBUG_PRINT("Keypad device is %s\n", achKpdDevice);
				u32Result = 1;
				break;
			}
		}
		close(i32KpdFd);
		achKpdDevice[16]++;
	}

	if (u32Result == 0) {
		DEBUG_PRINT("can not find any Keypad device!\n");
		return -1;
	}

	return i32KpdFd;
}


int main(int argc, char* args[])
{
    	int cam_fd;
	int i;
    	DIR *dir;
    	int ret;
	int jpeg_v4l2_buffer_num = 0;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	long int screensize;
	int32_t i32Ret = 0;
	unsigned int xres, yres,jpeg_buffer_size;
static int              fd = -1;

	struct v4l2_buffer buf_v4l2;
	i32KpdFd = InitKpdDevice();
	if(i32KpdFd < 0){
		i32Ret = -1;
		goto out;
	}
	printf("\n************* Webcam demo ************\n");
	printf("Please select Memory mode\n");
	printf("0: Normal Mode\n");
	printf("1: User Memory Pool Mode\n");
	printf("Note: The main difference between two modes is JEPG encode performance\n");
	printf("      The frame rae depends on v4l2 buffer number\n>");
	scanf("%d", &user_memorypool_enable);

	if(user_memorypool_enable)
		printf("*Use User Memory Pool Mode\n");
	else
		printf("*Use Normal Mode\n");
	
	printf("Open jpegcodec device\n");

	/* Try to open folder "/sys/class/video4linux/video1/",
	   if the folder exists, jpegcodec is "/dev/video1", otherwises jpegcodec is "/dev/video0" */
	dir = opendir("/sys/class/video4linux/video1/");
	if(dir)
	{
		closedir(dir);		
		device[10]++;
	}	

	for(i=0;i<RETRY_NUM;i++)
	{
		fd_jpeg = open(device, O_RDWR);
		if (fd_jpeg < 0)
		{			
			continue;	
		}
		break;
	}
	if (fd_jpeg < 0)
	{
		printf(" JPEG Engine is busy\n");
		return -1; 
	}
	printf("  *jpegcodec is %s\n",device);	
/* allocate memory for JPEG engine */
	ioctl(fd_jpeg, JPEG_GET_JPEG_BUFFER, (__u32)&jpeg_buffer_size);

	pJpegBuffer = mmap(NULL, jpeg_buffer_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd_jpeg, 0);

	if(pJpegBuffer == MAP_FAILED)
	{
		printf(" JPEG Map Failed!\n");
		goto out;
	}
	else
	{
		ioctl(fd_jpeg, JPEG_GET_JPEG_BUFFER_PADDR, (__u32)&jpeg_buffer_paddress);
		
		jpeg_buffer_vaddress = (unsigned int) pJpegBuffer;

		printf("  *Jpeg engine buffer: %dKB\n",jpeg_buffer_size / 1024);
		printf("     -Physical address:    0x%08X\n", jpeg_buffer_paddress);
		printf("     -User Space address:  0x%08X\n", jpeg_buffer_vaddress);	
  	}
   	signal (SIGINT, myExitHandler);

	printf("Open frame buffer device\n");
    	if ((fb_fd = fb_open(FB_0)) < 0) {
       		printf(" fb_open failed\n");
        	return 1;
    	}

    	if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
         	printf(" fb_get_fscreeninfo failed\n");
         	return 1;
    	}

    	if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
	 	printf(" fb_get_vscreeninfo failed\n");
         	return 1;
    	}

    	screensize = vinfo.yres * finfo.line_length * 2;
    	fb_addr = (unsigned char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    	if ((int)fb_addr == -1) {
        	printf(" mmap failed\n");
        	return 1;
    	}
  
   	printf("  *Panel Size:%dx%d-%d\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
	
	printf("\nPlease input buffer number for v4l2\n");
	if(user_memorypool_enable)
	{
		jpeg_v4l2_buffer_num = (jpeg_buffer_size - BUFFER_SIZE_OTHER_USE - BUFFER_SIZE_WEBCAM_BITSTREAM) / (image_width * image_height * 2);		
		printf("Note: Maximum Buffer Number for User Memory Pool Mode is %d in the demo code\n", jpeg_v4l2_buffer_num);
	}
input_again:
	scanf("%d", &BUFFER_COUNT);
	
	if(BUFFER_COUNT <= 0)
	{
		printf("Buffer must be larger than 0. Please input again.\n>");
		goto input_again;
	}
	else
	{
		if(user_memorypool_enable)
		{
			if(BUFFER_COUNT > jpeg_v4l2_buffer_num)
			{
				printf("Buffer must be smaller than %d. Please input again.\n>",jpeg_v4l2_buffer_num+1);
				goto input_again;
			}
		}
	}

	printf("*Use %d Buffer(s) for v4l2\n\n",BUFFER_COUNT);

	printf("Open Webcam device\n");
	device[10]++;

	for(i=0;i<RETRY_NUM;i++)
	{
		cam_fd  = open(device, O_RDWR);
		if (cam_fd  < 0)
		{			
			continue;	
		}
		break;
	}
	if (cam_fd < 0)
	{
		printf("  Can't open Webcam\n");
		return -1; 
	}
	printf("  *Webcam is %s\n",device);	

   	ret = cam_start(cam_fd);
   	if (ret < 0) {
        	printf("  Camera start failed(%d)\n", ret);
       		goto out;
    	}
	
    	if ( InitVPE((char*) pJpegBuffer, (char*) fb_addr, vinfo.xres, vinfo.yres) ){
		exit(-1);
    	}

	while (1) 
	{
        	ret = cam_read(cam_fd, &buf_v4l2);
        	if (ret < 0) {
            		printf("  Camera read failed\n");
            		goto out;
        	}	
	}

out:
	close(vpe_fd);
    	munmap(fb_addr, screensize);
    	fb_close(fb_fd);
    	cam_stop(cam_fd);
    	cam_close(cam_fd);
	
	if(pJpegBuffer)
		munmap(pJpegBuffer, jpeg_buffer_size);
	close(fd_jpeg);
	
	while(1);
    	return 0;
}

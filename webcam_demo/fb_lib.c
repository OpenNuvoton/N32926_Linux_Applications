#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "fb_lib.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include "w55fa92_vpe.h"
#include "fb_lib.h"
#include "cam_lib.h"

int vpe_fd;
unsigned int frame_control = 1;
vpe_transform_t vpe_setting;

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

#define LCD_ENABLE_INT		_IO('v', 28)
#define LCD_DISABLE_INT		_IO('v', 29)

int fb_open(int fbx)
{
    int fd;

    switch (fbx) {
    case FB_0:
        fd = open("/dev/fb0", O_RDWR);
        break;
    
    case FB_1:
        fd = open("/dev/fb1", O_RDWR);
        break;
    }

    return fd;
}


int InitVPE(char* pSrcBuf, char* pDstBuf, int Tarwidth, int Tarheight)
{
	char vpe_device[] = "/dev/vpe";

	unsigned int value, ret =0;
	vpe_fd = open(vpe_device, O_RDWR);
	if (vpe_fd < 0){
		printf("Open %s error\n", vpe_device);
		return -1;
	}
	else
		printf("Open %s successfully\n", vpe_device);


	// Get Video Decoder IP Register size
	if( ioctl(vpe_fd, VPE_INIT, NULL)  < 0){
		close(vpe_fd);
		printf("  VPE_INIT fail\n");
		ret = -1;
	}
	value = 0x5a;
	if((ioctl(vpe_fd, VPE_IO_SET, &value)) < 0){
		close(vpe_fd);
		printf("  VPE_IO_SET fail\n");
		ret = -1;
	}

	value = 1;
	ioctl(vpe_fd, VPE_SET_MMU_MODE, &value);

	vpe_setting.src_addrPacY = (unsigned int)pSrcBuf;
	vpe_setting.src_addrU = 0;
	vpe_setting.src_addrV = 0;
	vpe_setting.src_format = VPE_SRC_PACKET_YUV422;
	vpe_setting.src_width = image_width;
	vpe_setting.src_height = image_height;
	vpe_setting.src_leftoffset = 0;
	vpe_setting.src_rightoffset = 0;
	vpe_setting.dest_addrPac = (unsigned int)pDstBuf;
	vpe_setting.dest_format = VPE_DST_PACKET_RGB565;
	vpe_setting.dest_width = Tarwidth;
	vpe_setting.dest_height = Tarheight;
	vpe_setting.dest_leftoffset = 0;
	vpe_setting.dest_rightoffset = 0;
	vpe_setting.algorithm = VPE_SCALE_DDA;
	vpe_setting.rotation = VPE_OP_NORMAL;

	return ret;
}

void myExitHandler (int sig)
{
  	printf("  Exit Signal Handler\n");
	if((ioctl(vpe_fd, VPE_STOP, NULL)) < 0)	{
		close(vpe_fd);
		printf("VPE_STOP fail\n");
	}
	fflush(stdout);
	usleep(1000000);
	exit(-9);
}


int FormatConvert(char* pSrcBuf, char* pDstBuf, int Tarwidth, int Tarheight)
{
	unsigned int value, ret =0;

	value = 1;
	vpe_setting.src_addrPacY = (unsigned int)pSrcBuf;

	vpe_setting.dest_addrPac = (unsigned int)pDstBuf + finfo.line_length * vinfo.yres * (frame_control & 0x01);

	if((ioctl(vpe_fd, VPE_SET_FORMAT_TRANSFORM, &vpe_setting)) < 0){
		close(vpe_fd);
		printf("VPE_IO_GET fail\n");
		ret = -1;
	}

	if((ioctl(vpe_fd, VPE_TRIGGER, NULL)) < 0){
		close(vpe_fd);
		printf("VPE_TRIGGER info fail\n");
		ret = -1;
	}
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
	vinfo.yoffset = vinfo.yres * (frame_control & 0x01);
	frame_control++;
	ioctl( fb_fd, FBIOPAN_DISPLAY, &vinfo);

	return ret;
}

int fb_render(int fd, unsigned char *img, unsigned char *map, 
	      int xoffset, int yoffset,
              int width, int height, int depth)
{
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int location;
    int x;
    int y;
    int k;

    int b;
    int g;
    int r;
    unsigned short int t;

    int xsrc;
    int ysrc;

    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
         return -1;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
         return -2;
    }

    for (y = yoffset+1, ysrc = 1; ysrc < height; y++, ysrc++) {
	if (y >= vinfo.yres) 
		break;

        switch (depth) {
        case 24:
            k = (height-ysrc)*3*width;
            break;

        case 32:
            k = (height-ysrc)*4*width;
            break;
        }

        for (x = xoffset, xsrc = 1; xsrc < width; x++, xsrc++) {
	    if (x >= vinfo.xres)
		break;

            location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8);
	    location += (y+vinfo.yoffset) * finfo.line_length;

            switch (vinfo.bits_per_pixel) {
            case 32:
                if (depth == 24) {
                    *(map + location) = img[k+xsrc*3+2]; 
                    *(map + location + 1) = img[k+xsrc*3+1]; 
                    *(map + location + 2) = img[k+xsrc*3]; 
                    *(map + location + 3) = 0; 
                } else if (depth == 32) {
                    *(map + location) = img[k+xsrc*4+2]; 
                    *(map + location + 1) = img[k+xsrc*4+1]; 
                    *(map + location + 2) = img[k+xsrc*4]; 
                    *(map + location + 3) = img[k+xsrc*4+3]; 
                }
                break;

            case 24:

                break;

            case 16:
		if (depth == 24) {
                    r = img[k+xsrc*3];
                    g = img[k+xsrc*3+1];
                    b = img[k+xsrc*3+2];
		} else if (depth == 32) {
		    r = img[k+xsrc*4];
                    g = img[k+xsrc*4+1];
                    b = img[k+xsrc*4+2];
 		}
                t = r << 11 | g << 5 | b;
                *((unsigned short int*)(map + location)) = t;
                break;
            }
        }
    }

    return 0;
}

void fb_close(int fd)
{
    close(fd);
}

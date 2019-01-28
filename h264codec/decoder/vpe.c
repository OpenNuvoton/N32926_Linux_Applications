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

#include "port.h"
#include "h264.h"

#define DBGPRINTF(...)
/* VPE device */
char vpe_device[] = "/dev/vpe";
int vpe_fd;

#define LCD_ENABLE_INT		_IO('v', 28)
#define LCD_DISABLE_INT		_IO('v', 29)


extern __u32 g_u32VpostWidth, g_u32VpostHeight, fb_bpp;
extern struct fb_var_screeninfo g_fb_var;
extern int fb_fd, fb_paddress;

int FormatConversion(void* data, int toggle, int Srcwidth, int Srcheight, int Tarwidth, int Tarheight)
{
	//unsigned int volatile vworkbuf, tmpsize;
    AVFrame             *pict=(AVFrame *)data;	
	unsigned int value;
	vpe_transform_t vpe_setting;
	unsigned int ptr_y, ptr_u, ptr_v, step=0;
	int total_x, total_y;
	int PAT_WIDTH, PAT_HEIGHT;
	unsigned int ret =0;	
	
	unsigned int pDstBuf;
	
	PAT_WIDTH = (Srcwidth+3) & ~0x03;
	PAT_HEIGHT = Srcheight;
	
	//printf("SrcWidth = %d, SrcHeight = %d, Tarwidth=%d, Tarheight = %d\n",Srcwidth, Srcheight, Tarwidth, Tarheight);
	
	total_x = (Srcwidth + 15) & ~0x0F;
	total_y = (Srcheight + 15) & ~0x0F;	
	ptr_y = (unsigned int)pict->data[0];
	ptr_u = (unsigned int)pict->data[1];		/* Planar YUV420 */ 
	ptr_v = (unsigned int)pict->data[2];
	
	
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

#if FB_PINGPONG_BUF	
       
    if (toggle)
    {
        g_fb_var.yoffset = g_fb_var.yres;
        pDstBuf = fb_paddress;         
    }
    else
    {
        g_fb_var.yoffset = 0;
        pDstBuf = fb_paddress + g_u32VpostWidth * g_u32VpostHeight * fb_bpp;  
    }
    	
	if (ioctl(fb_fd, FBIOPAN_DISPLAY,&g_fb_var) < 0) {
        printf("ioctl FB_IOPAN_DISPLAY fail\n");	    
    }	    

#else	
    pDstBuf = fb_paddress;
#endif	
	
	vpe_setting.src_width = PAT_WIDTH;
	vpe_setting.src_addrPacY = ptr_y;				/* Planar YUV420 */
	vpe_setting.src_addrU = ptr_u;	
	vpe_setting.src_addrV = ptr_v;				
	vpe_setting.src_format = VPE_SRC_PLANAR_YUV420;
	vpe_setting.src_width = PAT_WIDTH ;
	vpe_setting.src_height = PAT_HEIGHT;		
		

	vpe_setting.src_leftoffset = 0;
	vpe_setting.src_rightoffset = total_x - PAT_WIDTH;
		
	vpe_setting.dest_addrPac = (unsigned int)pDstBuf;	
	vpe_setting.dest_format = VPE_DST_PACKET_RGB565;
	vpe_setting.dest_width = Tarwidth;
	vpe_setting.dest_height = Tarheight;
	
	if (g_u32VpostHeight > Tarheight)
	{  // Put image at the center of panel
	    int offset_Y;
	    offset_Y = (g_u32VpostHeight - Tarheight)/2 * g_u32VpostWidth * 2;      // For RGB565
	    vpe_setting.dest_addrPac = (unsigned int)pDstBuf + offset_Y;		    
	}
	
	
	if (g_u32VpostWidth > Tarwidth)
	{
		vpe_setting.dest_leftoffset = (g_u32VpostWidth-Tarwidth)/2;
		vpe_setting.dest_rightoffset = (g_u32VpostWidth-Tarwidth)/2;
	}
	else
	{
	    vpe_setting.dest_width = g_u32VpostWidth;
	    vpe_setting.dest_height = g_u32VpostHeight;
		    
		vpe_setting.dest_leftoffset = 0;
		vpe_setting.dest_rightoffset = 0;		
	}
	


	vpe_setting.algorithm = VPE_SCALE_DDA;
	vpe_setting.rotation = VPE_OP_NORMAL;

		
	if((ioctl(vpe_fd, VPE_SET_FORMAT_TRANSFORM, &vpe_setting)) < 0)		
	{
		close(vpe_fd);
		printf("VPE_IO_GET fail\n");
	}		
	

	if((ioctl(vpe_fd, VPE_TRIGGER, NULL)) < 0)	
	{
		close(vpe_fd);
		printf("VPE_TRIGGER info fail\n");
	}		
	
	return ret;	
	
	
}


int InitVPE(void)
{
	unsigned int version;
	unsigned int value;	

	vpe_fd = open(vpe_device, O_RDWR);		
	if (vpe_fd < 0){
		printf("open %s error\n", vpe_device);
		return -1;	
	}	
	else
		printf("open %s successfully\n", vpe_device);
	
	// Get Video Decoder IP Register size
	if( ioctl(vpe_fd, VPE_INIT, NULL)  < 0)		
	{
		close(vpe_fd);
		printf("VPE_INIT fail\n");		
	}

	value = 0x5a;
	if((ioctl(vpe_fd, VPE_IO_SET, &value)) < 0)		
	{
		close(vpe_fd);
		printf("VPE_IO_SET fail\n");
	}			
	
	value = 0;		
	ioctl(vpe_fd, VPE_SET_MMU_MODE, &value);		
		
	return 0;
}





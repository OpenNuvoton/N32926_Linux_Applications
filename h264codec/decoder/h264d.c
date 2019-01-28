/* 
    Assume the bitstream size is known in file "test.info", 720x480 resolution, 10 frames
    sample code for H264 for bitstream input and YUV output
    This sample code is to do decode 100 rounds,10 frames/round, named "/tmp/dev0.yuv"
    #./h264d test.264 test.info
 */
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <linux/videodev.h>
#include <errno.h>
#include <linux/fb.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/time.h>
#include <dirent.h>



#include "favc_avcodec.h"
#include "favc_version.h"
#include "h264.h"

#define DECODE_OUTPUT_PACKET_YUV422     0
#define MAX_IMG_WIDTH                   1280
#define MAX_IMG_HEIGHT                  720

#define DISPLAY_MODE_CBYCRY	4
#define DISPLAY_MODE_YCBYCR	5
#define DISPLAY_MODE_CRYCBY	6

#define IOCTL_LCD_GET_DMA_BASE      _IOR('v', 32, unsigned int *)
#define VIDEO_FORMAT_CHANGE			_IOW('v', 50, unsigned int)	//frame buffer format change

#define IOCTL_LCD_ENABLE_INT		_IO('v', 28)
#define IOCTL_LCD_DISABLE_INT		_IO('v', 29)
#define IOCTL_WAIT_VSYNC		    _IOW('v', 67, unsigned int)	

#define dout_name         ".//dev0.yuv"

#define	OUTPUT_FILE		0

// VPOST related
struct fb_var_screeninfo g_fb_var;
int fb_fd=0, oneFBsize;
static char *fbdevice = "/dev/fb0";
__u32 g_u32VpostWidth, g_u32VpostHeight,fb_bpp;
unsigned int fb_paddress;

int frame_info[FRAME_COUNT+1];

FILE            *din,*din_info;
FILE            *dout;
int             favc_dec_fd=0;
int             dec_mmap_addr;
unsigned int    in_virt_buffer;


typedef struct {
    unsigned int size;
    unsigned int addr;
} _BUF_INFO;

unsigned char *pVBitStreamBuffer = NULL,*pVBitStartBuffer;
int bitstreamsize;
int decoded_img_width, decoded_img_height;
int output_vir_buf=0;
unsigned char *pVFrameBuffer;

avc_workbuf_t	outputbuf;


// VPE
extern int vpe_fd;
extern int InitVPE(void);
extern int FormatConversion(void* data, char* pDstBuf, int SrcWidth, int SrcHeight, int Tarwidth, int Tarheight);



int InitFB()
{
    
	fb_fd = open(fbdevice, O_RDWR);
	if (fb_fd == -1) {
		perror("open fbdevice fail");
		return -1;
	}

	
	if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &g_fb_var) < 0) {
		perror("ioctl FBIOGET_VSCREENINFO");
		close(fb_fd);
		fb_fd = -1;
		return -1;
	}	
	
	g_u32VpostWidth = g_fb_var.xres;
	g_u32VpostHeight = g_fb_var.yres;
	fb_bpp = g_fb_var.bits_per_pixel/8;	
	printf("FB width =%d, height = %d, bpp=%d\n",g_u32VpostWidth,g_u32VpostHeight,fb_bpp);	
	
    // Get FB Physical Buffer address
	if (ioctl(fb_fd, IOCTL_LCD_GET_DMA_BASE, &fb_paddress) < 0) {
		perror("ioctl IOCTL_LCD_GET_DMA_BASE ");
		close(fb_fd);
		return -1;
	}	
	
	//printf("LCD Video Buffer Phy address = 0x%x\n",fb_paddress);
	
	ioctl(fb_fd,IOCTL_LCD_ENABLE_INT);
	
	dec_mmap_addr = fb_paddress; 
	
	oneFBsize = g_u32VpostWidth * g_u32VpostHeight * fb_bpp;

#if FB_PINGPONG_BUF
    pVFrameBuffer = mmap(NULL, oneFBsize * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    // Copy buf0 to buf1 to keep content consistent
    memcpy (pVFrameBuffer + oneFBsize, pVFrameBuffer , oneFBsize);  
#endif    
	
#if DECODE_OUTPUT_PACKET_YUV422
	ioctl(fb_fd,VIDEO_FORMAT_CHANGE, DISPLAY_MODE_YCBYCR);   
#endif	
    return 0;
}


int h264_init(video_profile *video_setting)
{
    FAVC_DEC_PARAM      tDecParam;
    int                 ret_value;

    if(favc_dec_fd==0)
      favc_dec_fd=open(FAVC_DECODER_DEV,O_RDWR);
	
    if(favc_dec_fd <= 0)
    {
        printf("Fail to open %s\n",FAVC_DECODER_DEV);
        return -1;
    }
    
   
	if((ioctl(favc_dec_fd, FAVC_IOCTL_DECODE_GET_BSSIZE, &bitstreamsize)) < 0)		
	{
		close(favc_dec_fd);
		printf("Get vde bitstream size fail\n");
	}	
	
	if((ioctl(favc_dec_fd, FAVC_IOCTL_DECODE_GET_OUTPUT_INFO, &outputbuf)) < 0)		
	{
		close(favc_dec_fd);
		printf("Get output buffer size fail\n");
        return -1;		
	}		 
	printf("output buf addr = 0x%x, size=0x%x\n",outputbuf.addr, outputbuf.size);
	
	output_vir_buf = (unsigned int)mmap(NULL, outputbuf.size, PROT_READ|PROT_WRITE, MAP_SHARED, favc_dec_fd, 0);	
	printf("mmap output_vir_buf = 0x%x\n",output_vir_buf);
	
	if((void *)output_vir_buf == MAP_FAILED)
	{
		printf("Map Output Buffer Failed!\n");
		return -1;;
	}		
	
#if (DECODE_OUTPUT_PACKET_YUV422 ==0)
	if (outputbuf.size ==0)
	    return -1;
#endif	    
    
    memset(&tDecParam, 0, sizeof(FAVC_DEC_PARAM));
    tDecParam.u32API_version = H264VER;
    tDecParam.u32MaxWidth = video_setting->width;
    tDecParam.u32MaxHeight = video_setting->height;
//#if OUTPUT_FILE    
#if 1    
	// For file output or VPE convert planr to packet format
    tDecParam.u32FrameBufferWidth = video_setting->width;
    tDecParam.u32FrameBufferHeight = video_setting->height;
#else
	// For TV output. H.264 output packet to FB off-screen buffer
    tDecParam.u32FrameBufferWidth = 640;
    tDecParam.u32FrameBufferHeight = 480;
#endif   

#if DECODE_OUTPUT_PACKET_YUV422
	tDecParam.u32OutputFmt = 1; // 1->Packet YUV422 format, 0-> planar YUV420 foramt
#else	
	tDecParam.u32OutputFmt = 0; // 1->Packet YUV422 format, 0-> planar YUV420 foramt
#endif	

    ret_value = ioctl(favc_dec_fd,FAVC_IOCTL_DECODE_INIT,&tDecParam);		// Output : Packet YUV422 or Planar YUV420
    if(ret_value < 0)
    {
        printf("FAVC_IOCTL_DECODE_INIT: memory allocation failed\n");
        return -1;
    }
    return 0;
}


int h264_close(video_profile *video_setting)
{
   
    if (output_vir_buf)
    {
        munmap((char *)output_vir_buf, outputbuf.size);
        output_vir_buf=0;
    }        
        
    if(favc_dec_fd) {
        close(favc_dec_fd);
        favc_dec_fd = 0;
    }
    return 0;
}

int favc_decode(video_profile *video_setting, unsigned char *frame, void *data, int size)
{
    AVFrame             *pict=(AVFrame *)data;
    FAVC_DEC_PARAM      tDecParam;
    FAVC_DEC_RESULT		ptResut;    
    int  ret_value;
    
    // set display virtual addr (YUV or RGB)
//YUV422 needn't assign U,V
    memset(&tDecParam, 0, sizeof(FAVC_DEC_PARAM));
    tDecParam.pu8Display_addr[0] = (unsigned int)pict->data[0];
    tDecParam.pu8Display_addr[1] = (unsigned int)pict->data[1];
    tDecParam.pu8Display_addr[2] = (unsigned int)pict->data[2];
    tDecParam.u32Pkt_size =	(unsigned int)size;
    tDecParam.pu8Pkt_buf = frame;

    tDecParam.crop_x = 0;
    tDecParam.crop_y = 0;
    
	tDecParam.u32OutputFmt = 1; // 1->Packet YUV422 format, 0-> planar YUV420 foramt
	
	//printf("decode Y = 0x%x\n",tDecParam.pu8Display_addr[0]);

    if((ret_value = ioctl(favc_dec_fd, FAVC_IOCTL_DECODE_FRAME, &tDecParam)) != 0)    
    {
        printf("FAVC_IOCTL_DECODE_FRAME: Failed.ret=%x\n",ret_value);
        return -1;
    }

#if OUTPUT_FILE
	if (tDecParam.tResult.isDisplayOut != 0)
	{
    	fwrite((void *)pict->data[0],video_setting->width*video_setting->height,1,dout);
		if (tDecParam.u32OutputFmt == 0)
		{
		    fwrite((void *)pict->data[1],video_setting->width*video_setting->height/4,1,dout);
		    fwrite((void *)pict->data[2],video_setting->width*video_setting->height/4,1,dout);
	    }
	}
#endif
    decoded_img_width = tDecParam.tResult.u32Width;
    decoded_img_height = tDecParam.tResult.u32Height;    

	if (tDecParam.tResult.isDisplayOut ==0)
		return 0;
	else
    	return tDecParam.got_picture;
}

#define __DEBUG__	1


DecodeH264(char *src264File, char *src264info)
{
    int  i, j,totalsz = 0;
//    int  fcount = 0;
    video_profile  video_setting;
    AVFrame  pict[2], *pict_ptr;
    int runsz = 0; 
    int result,flushCount=0;
    int totaloffset=0;
    int toggle_flag=0;
    
    DIR *dir=NULL;
    struct dirent *ptr=NULL;    

    din = fopen(src264File, "rb");
    din_info = fopen(src264info, "r");    

    //set the default value
    video_setting.width = -1;
    video_setting.height = -1;
   

    //printf("din = %d, din_info=%d\n",din,din_info);
    if (din==0)
    {
        printf("Open file %s fail\n",src264File);
        goto file_close;
    }       
    printf("Open file %s is OK\n",src264File); 
    
    if (din_info ==0)
    {
        printf("Open file %s fail\n",src264info);
        goto file_close;        
    }
    
    
    memset(frame_info, 0, sizeof(frame_info));
    
    for(i=0; i< FRAME_COUNT; i++)
    {
        fscanf(din_info,"%d", &frame_info[i]);
        totalsz += frame_info[i];
        //printf("%d ", frame_info[i]);
        if (frame_info[i] == 0)
            break;
    }
    //printf("\n");
    fclose(din_info);

    //printf("malloc size = %d\n",totalsz);
    //in_virt_buffer = (unsigned int)malloc(totalsz);    
    in_virt_buffer = (unsigned int)malloc(1280*720*2);      
    if (!in_virt_buffer)
        goto file_close;
        
    //printf("APP : malloc virtual addr = 0x%x\n",in_virt_buffer);
    
    //printf("read file\n");
    //fread((void *)in_virt_buffer, totalsz, 1, din); //bitstream memory

    //printf("h264_init\n");
    if (h264_init(&video_setting) < 0)
        goto fail_h264_init;


    //YUV422 needn't assign U,V
#if DECODE_OUTPUT_PACKET_YUV422
    pict[0].data[0] = (unsigned char *)dec_mmap_addr;
    pict[0].data[1] = (unsigned char *)(dec_mmap_addr+ (video_setting.width*video_setting.height));
    pict[0].data[2] = (unsigned char *)(dec_mmap_addr+ (video_setting.width*video_setting.height*5/4));
#else    
    pict[0].data[0] = (unsigned char *)outputbuf.addr;
    pict[0].data[1] = (unsigned char *)(outputbuf.addr+ (MAX_IMG_WIDTH*MAX_IMG_HEIGHT));
    pict[0].data[2] = (unsigned char *)(outputbuf.addr+ (MAX_IMG_WIDTH*MAX_IMG_HEIGHT*5/4));
    
    pict[1].data[0] = (unsigned char *)outputbuf.addr + (outputbuf.size/2) ;
    pict[1].data[1] = (unsigned char *)(outputbuf.addr+ (outputbuf.size/2) + (MAX_IMG_WIDTH*MAX_IMG_HEIGHT));
    pict[1].data[2] = (unsigned char *)(outputbuf.addr+ (outputbuf.size/2) + (MAX_IMG_WIDTH*MAX_IMG_HEIGHT*5/4));    
#endif    

    for (i=0; i<FRAME_COUNT; i++)
    //for (i=0; i<10; i++)    
    {
        fseek(din, totaloffset, SEEK_SET);
        
        if ((frame_info[i] > bitstreamsize) || (frame_info[i] == 0))
        {
            if (frame_info[i] != 0)
                printf("Warning : Bitstream size is larger than buffer size\n");
            
            //if (frame_info[i] == 0)
            //    printf("total frame num = %d\n", i-1);    
            goto fail_h264_init;
        }     
           
        //printf("frame %d, bs lenght =%d\n",i,frame_info[i]);
        fread((void *)in_virt_buffer, frame_info[i], 1, din); //bitstream memory               
        
        totaloffset += frame_info[i];
//usleep(100);        
        if (toggle_flag)
        {
            pict_ptr = &pict[1];
            toggle_flag = 0;
        }            
        else
        {
            pict_ptr = &pict[0];     
            toggle_flag = 1;               
        }            
           
        result = favc_decode(&video_setting, (unsigned char *)(in_virt_buffer), (void *)pict_ptr, frame_info[i]);        
        if (result < 0)
        {
            printf("frame %d/(0~%d) decode FAIL!\n", i, FRAME_COUNT-1);
            goto fail_h264_init;
        }
        else if (result == 0)
        	flushCount++;
        
#if (DECODE_OUTPUT_PACKET_YUV422 ==0)
        FormatConversion((void *)pict_ptr, (char *)toggle_flag, decoded_img_width, decoded_img_height, decoded_img_width, decoded_img_height);   
        //FormatConversion((void *)pict_ptr, (char *)toggle_flag, decoded_img_width, decoded_img_height, g_u32VpostWidth, g_u32VpostHeight);       
#endif        
        	
        //printf("frame %d Decoded\n", i);
        runsz += frame_info[i];
        //getchar();
    }

    printf("Total frame %d decode OK!\n", i);

fail_h264_init:

    h264_close(&video_setting);

//    printf("Total frame %d decode OK!\n", i);

    //fflush(stdout);

//fail_h264_init:
    free((void *)in_virt_buffer);
file_close:
    fclose(din);
//    fclose(dout);
    fflush(stdout);
    
    return 0;    
}
 


#define  BS_FOLDER  "../pattern/"
#define  INFO_FOLDER "../info/"

int main(int argc, char **argv)
{
    DIR *dir264=NULL, *dir_info=NULL;
    struct dirent *ptr=NULL;
    char frame_info[256];
    char H264BS[256];//="../pattern/";
    char H264info[256];// = "../info/";
    int file_length;
    

    if (InitFB())
        exit(-1);
        
	if ( InitVPE())	
		exit(-1);

    dir264 = opendir("../pattern");  
    dir_info = opendir("../info");  

    while((ptr = readdir(dir264)) != NULL)
    {
        if ((strcmp(ptr->d_name,".")==0) || (strcmp(ptr->d_name,"..")==0)) continue;
        
        file_length = strlen(ptr->d_name);
        //printf("%s, length = %d\n",ptr->d_name,file_length);
        
        strcpy(frame_info,ptr->d_name);
        if ((strcmp(&frame_info[file_length-3],"264") ==0) || (strcmp(&frame_info[file_length-3],"jsv") ==0))
        {
            strcpy(H264BS,BS_FOLDER);
            strcpy(H264info, INFO_FOLDER);
            
            strcat(H264BS,ptr->d_name);
            strcpy(&frame_info[file_length-3],"txt");   
            strcat(H264info, frame_info);
            //printf("H264 BS = %s\n",H264BS);    
            //printf("H264 Info = %s\n",H264info);                
            
            DecodeH264(H264BS,H264info); 
        }
        else continue;
    }
    
    printf("All decode done\n");
    
    fflush(stdout);
    
   
    if (vpe_fd >0)
	    close(vpe_fd);	
	    
    if (fb_fd > 0)
    {
        ioctl(fb_fd, IOCTL_WAIT_VSYNC, NULL);
        close(fb_fd);	    
    }        
    
    closedir(dir264);
    closedir(dir_info);
        
    exit(0);
    
        
}



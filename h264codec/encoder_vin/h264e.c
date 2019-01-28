/* 
    sample code for H264 for pattern 720x480 input and bitstream output
    This sample code is to do encode 1000 stream frames named "/tmp/dev0.264"
    #./h264_main test.yuv
 */

#include <unistd.h>  
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <linux/videodev.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <errno.h>
#include <inttypes.h>

#include "favc_avcodec.h"
#include "ratecontrol.h"
#include "favc_version.h"
#include "h264.h"

#include "Misc.h"
#include "V4L.h"

#define     READ_YUV    1
#define     WRITE_FILE  1

#define     ENC_IMG_WIDTH   320
#define     ENC_IMG_HEIGHT  240


#define dout_name1         "../encQVGA.264"
#define dinfo_name        "../encQVGA.txt"


#define VIDEO_PALETTE_YUV420P_MACRO		50		/* YUV 420 Planar Macro */


unsigned char Encbitsteam[ENC_IMG_WIDTH*ENC_IMG_HEIGHT*3/2];

FILE            *din;
FILE            *dout, *dout2, *dout_info;
int             favc_enc_fd1=0;//,favc_enc_fd2=0;
//int             favc_dec_fd;
H264RateControl h264_ratec;
static int  favc_quant=0;
//int             enc_mmap_addr;
static unsigned int    out_virt_buffer1;//, out_virt_buffer2;
int dec_BS_buf_size;

avc_workbuf_t	bitstreambuf, yuvframebuf;//, framebuf,workbuf;


typedef unsigned long long uint64;


int h264_init(video_profile *video_setting)
{
    FAVC_ENC_PARAM     enc_param;
    int YUVoffset;

    // Open driver handler1
    if(favc_enc_fd1==0)
      favc_enc_fd1=open(FAVC_ENCODER_DEV,O_RDWR);
	
    if(favc_enc_fd1 <= 0)
    {
        printf("H1 Fail to open %s\n",FAVC_ENCODER_DEV);
        return -1;
    }


    //-----------------------------------------------
    //  driver handle 1
    //-----------------------------------------------    
	// Get Bitstream Buffer Information	
	if((ioctl(favc_enc_fd1, FAVC_IOCTL_ENCODE_GET_BSINFO, &bitstreambuf)) < 0)		
	{
		close(favc_enc_fd1);
		printf("Get avc Enc bitstream info fail\n");
		return -1;
	}	
	printf("H1 Get Enc BS Buffer Physical addr = 0x%x, size = 0x%x,\n",bitstreambuf.addr,bitstreambuf.size);
	
	out_virt_buffer1 = (unsigned int)mmap(NULL, bitstreambuf.size, PROT_READ|PROT_WRITE, MAP_SHARED, favc_enc_fd1, bitstreambuf.offset);

	if((void *)out_virt_buffer1 == MAP_FAILED)
	{
		printf("Map ENC Bitstream Buffer Failed!\n");
		return -1;;
	}	
	printf("H1 Mapped ENC Bitstream Buffer Virtual addr = 0x%x\n",out_virt_buffer1);

    //-----------------------------------------------
    //  Issue Encode parameter to driver handle 1 & 2
    //-----------------------------------------------  	          

    memset(&enc_param, 0, sizeof(FAVC_ENC_PARAM));

    enc_param.u32API_version = H264VER;
  
    enc_param.u32FrameWidth=video_setting->width;
    enc_param.u32FrameHeight=video_setting->height;
   
    enc_param.fFrameRate = video_setting->framerate;
    enc_param.u32IPInterval = video_setting->gop_size; //60, IPPPP.... I, next I frame interval
    enc_param.u32MaxQuant       =video_setting->qmax;
    enc_param.u32MinQuant       =video_setting->qmin;
    enc_param.u32Quant = video_setting->quant; //32
    enc_param.u32BitRate = video_setting->bit_rate;
    enc_param.ssp_output = -1;
    enc_param.intra = -1;

    enc_param.bROIEnable = 0;
    enc_param.u32ROIX = 0;
    enc_param.u32ROIY = 0;
    enc_param.u32ROIWidth = 0;
    enc_param.u32ROIHeight = 0;

#ifdef RATE_CTL
    memset(&h264_ratec, 0, sizeof(H264RateControl));
    H264RateControlInit(&h264_ratec, enc_param.u32BitRate,
        RC_DELAY_FACTOR,RC_AVERAGING_PERIOD, RC_BUFFER_SIZE_BITRATE, 
        (int)enc_param.fFrameRate,
        (int) enc_param.u32MaxQuant, 
        (int)enc_param.u32MinQuant,
        (unsigned int)enc_param.u32Quant, 
        enc_param.u32IPInterval);
#endif

    favc_quant = video_setting->quant;

    printf("APP : FAVC_IOCTL_ENCODE_INIT\n");

    if (ioctl(favc_enc_fd1, FAVC_IOCTL_ENCODE_INIT, &enc_param) < 0)
    {
        close(favc_enc_fd1);
        printf("Handler_1 Error to set FAVC_IOCTL_ENCODE_INIT\n");
        return -1;
    }


    return 0;
}



int enc_close(video_profile *video_setting)
{
    printf("Enter enc_close\n");
    //---------------------------------
    //  Close driver handle 1
    //---------------------------------    
    if(favc_enc_fd1) {
        close(favc_enc_fd1);
    }

    favc_enc_fd1 = 0;
    //printf("Enter enc_close\n");
   
    return 0;
}

int favc_encode(int fileDecriptor, video_profile *video_setting, unsigned char *frame, void * data)
{
    AVFrame             *pict=(AVFrame *)data;
    FAVC_ENC_PARAM     enc_param;

    enc_param.pu8YFrameBaseAddr = (unsigned char *)pict->data[0];   //input user continued virtual address (Y), Y=0 when NVOP
    enc_param.pu8UFrameBaseAddr = (unsigned char *)pict->data[1];   //input user continued virtual address (U)
    enc_param.pu8VFrameBaseAddr = (unsigned char *)pict->data[2];   //input user continued virtual address (V)

    enc_param.bitstream = frame;  //output User Virtual address   
    enc_param.ssp_output = -1;
    enc_param.intra = -1;
    enc_param.u32IPInterval = 0; // use default IPInterval that set in INIT

    enc_param.u32Quant = favc_quant;
    enc_param.bitstream_size = 0;

    //printf("APP : FAVC_IOCTL_ENCODE_FRAME\n");
    if (ioctl(fileDecriptor, FAVC_IOCTL_ENCODE_FRAME, &enc_param) < 0)
    {
        printf("Dev =%d Error to set FAVC_IOCTL_ENCODE_FRAME\n", fileDecriptor);
        return 0;
    }

#ifdef RATE_CTL
    if (enc_param.keyframe == 0) {
        //printf("%d %d %d\n", enc_param.u32Quant, enc_param.bitstream_size, 0);
        H264RateControlUpdate(&h264_ratec, enc_param.u32Quant, enc_param.bitstream_size , 0);
    } else  {
        //printf("%d %d %d\n", enc_param.u32Quant, enc_param.bitstream_size, 1);
        H264RateControlUpdate(&h264_ratec, enc_param.u32Quant, enc_param.bitstream_size , 1);
    }
    favc_quant = h264_ratec.rtn_quant;
    
    //printf(" favc_quant = %d\n",favc_quant);

    //H264RateControlUpdate(&h264_ratec,enc_param.bitstream_size,enc_param.frame_cost);
#endif

    video_setting->intra = enc_param.keyframe;

    return enc_param.bitstream_size;
}


int main(int argc, char **argv)
{
    int tlength=0;
    int i, length, fcount=0;
    video_profile   video_setting;
    AVFrame pict, pict2;
    unsigned int y_image_size, uv_image_size, total_image_size;
    int readbyte, writebyte; 
    
	uint8_t *pu8PicPtr;
	uint64_t u64TS;
	uint32_t u32PicPhyAdr;       

    printf("h264enc main\n");


    dout=fopen(dout_name1,"wb");
    printf("Use encoder output name %s\n",dout_name1);
    
    dout_info = fopen(dinfo_name,"w");
    printf("Use encoder output info_name %s\n",dinfo_name);

    //set the default value
    video_setting.qmax = 51;
    video_setting.qmin = 0;
#ifdef RATE_CTL    
    video_setting.quant = 26;               // Init Quant
#else
    video_setting.quant = FIX_QUANT;
#endif    
    video_setting.bit_rate = 512000;
    video_setting.width = ENC_IMG_WIDTH;
    video_setting.height = ENC_IMG_HEIGHT;
    video_setting.framerate = 30;
    video_setting.frame_rate_base = 1;
    video_setting.gop_size = IPInterval;

    y_image_size = video_setting.width*video_setting.height;
    uv_image_size = y_image_size >> 1;
    total_image_size = y_image_size + uv_image_size;
    
    
    vin_main();
    
    if (h264_init(&video_setting) < 0) 
        goto fail_h264_init;

    fcount = TEST_ROUND;
    for (i=0; i < fcount; i++)
    {
     
	    if(ReadV4LPicture(&pu8PicPtr, &u64TS, &u32PicPhyAdr) == ERR_V4L_SUCCESS){ 
		    // got the planar buffer address ==> u32PicPhyAdr
		    TriggerV4LNextFrame();
	    }  
	    //TriggerV4LNextFrame();
	    
	    //printf("VideoIn Phy Addr = 0x%x\n",u32PicPhyAdr);
	    
        y_image_size = ENC_IMG_WIDTH * ENC_IMG_HEIGHT;
        uv_image_size = y_image_size >> 1;	    
        
        pict.data[0]=(unsigned char *)u32PicPhyAdr;
        pict.data[1]=(unsigned char *)(u32PicPhyAdr + y_image_size);
        pict.data[2]=(unsigned char *)(u32PicPhyAdr + y_image_size +(y_image_size >> 2));  	    
        
        // Tirgger to Encode bitstream by Driver handle 2
        length = favc_encode(favc_enc_fd1, &video_setting,(unsigned char *)Encbitsteam,(void *)&pict);        
        fprintf(dout_info,"%d\n", length);        
        
        if (length == 0)
            break;
 

        writebyte = fwrite((void *)Encbitsteam, sizeof(char), length, dout);   
        tlength += writebyte;     
    }

    printf("Total Frame %d encode Done, total bitstream = %d \n", i, tlength);

fail_h264_init:
       
	if(out_virt_buffer1)
		munmap((void *)out_virt_buffer1, bitstreambuf.size);    
          
file_close:

    sync();
    
    fclose(dout);
    fclose(dout_info);
    
    enc_close(&video_setting);
    
    vin_exit();
    
    fflush(stdout);     
            
	            
    return 0;
}

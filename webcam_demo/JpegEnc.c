#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/mman.h>
#include <asm/ioctl.h>
//#include <asm/arch/hardware.h>
#include "jpegcodec.h"
#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/fb.h>
#include <unistd.h>
#include <dirent.h>
#include "cam_lib.h"

char filename[100];
int capture_count = 0;
jpeg_param_t jpeg_param;
jpeg_info_t *jpeginfo;	

int Write2File(char *filename, char *buffer, int len)
{
	int fd;
	unsigned long totalcnt, wlen;
	int ret = 0;
	
	fd = open(filename, O_CREAT|O_WRONLY|O_TRUNC);
	if(fd < 0)
	{
		ret = -1;
		printf("open file fail\n");
		goto out;
	}

	totalcnt = 0;
	while(totalcnt < len)
	{
		wlen = write(fd, buffer + totalcnt, len - totalcnt);
		if(wlen < 0)
		{
			printf("write file error, errno=%d, len=%x\n", errno, (__u32)wlen);
			ret = -1;
			break;
		}
		totalcnt += wlen;
	}
	close(fd);
out:	
	return ret;
}


int jpegEnc(struct v4l2_buffer *buf)
{
	int ret = 0;
	int len, jpeginfo_size;	
	printf("  *JEPG Encode from Buffer %d\n",buf->index);
	memset((void*)&jpeg_param, 0, sizeof(jpeg_param_t));
	jpeginfo_size = sizeof(jpeg_info_t) + sizeof(__u32);
	jpeginfo = malloc(jpeginfo_size);
	
	jpeg_param.encode = 1;			/* Encode */
	if(user_memorypool_enable)
	{
		jpeg_param.vaddr_src = (unsigned int) buffers[buf->index].start ;
		jpeg_param.vaddr_dst = (unsigned int) bitstream_vaddress;
		jpeg_param.paddr_src = (unsigned int) rawdata_paddress + buf->index * buf->length;
		jpeg_param.paddr_dst = (unsigned int) bitstream_paddress;
	}
	else
	{	
		memcpy((unsigned char *)rawdata_vaddress, buffers[buf->index].start, buf->bytesused);
		jpeg_param.vaddr_src = (unsigned int) rawdata_vaddress;
		jpeg_param.vaddr_dst = (unsigned int) bitstream_vaddress;
		jpeg_param.paddr_src = (unsigned int) rawdata_paddress;
		jpeg_param.paddr_dst = (unsigned int) bitstream_paddress;
	}

	jpeg_param.encode_width = image_width;	/* JPEG width */
	jpeg_param.encode_height = image_height;/* JPGE Height */
	jpeg_param.encode_source_format = DRVJPEG_ENC_SRC_PACKET;	/* DRVJPEG_ENC_SRC_PACKET/DRVJPEG_ENC_SRC_PLANAR */
	jpeg_param.encode_image_format = DRVJPEG_ENC_PRIMARY_YUV420;	/* DRVJPEG_ENC_PRIMARY_YUV420/DRVJPEG_ENC_PRIMARY_YUV422 */
	jpeg_param.buffersize = 0;		/* only for continuous shot */
   	jpeg_param.buffercount = 1;		

	/* Set operation property to JPEG engine */
	if((ioctl(fd_jpeg, JPEG_S_PARAM, &jpeg_param)) < 0)
	{
		fprintf(stderr,"    set jpeg param failed:%d\n",errno);
		ret = -1;
		goto out;
	}
				
	/* Trigger JPEG engine */
	if((ioctl(fd_jpeg, JPEG_TRIGGER, NULL)) < 0)
	{
		fprintf(stderr,"    trigger jpeg failed:%d\n",errno);
		ret = -1;
			goto out;
	}

	/* Get JPEG Encode information */
	len = read(fd_jpeg, jpeginfo, jpeginfo_size);

	if(len<0) {
		fprintf(stderr, "    No data can get\n");
		if(jpeginfo->state == JPEG_MEM_SHORTAGE)
			printf("Memory Stortage\n");	
		ret = -1;
		goto out;
	}
	sprintf( filename, "./Capture%03d.jpg",  capture_count);	

	ret = Write2File(filename,(void *) jpeg_param.vaddr_dst, jpeginfo->image_size[0]);
	if(ret < 0)
	{
		printf("    write to file, errno=%d\n", errno);
	}
	else
	{
		printf("    Capture & Output file %s (%dBytes)\n", filename,jpeginfo->image_size[0]);		
		capture_count++;
	}

out:
	free(jpeginfo);
	return 0;

}




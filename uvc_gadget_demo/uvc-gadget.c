/*
 * UVC gadget test application
 *
 * Copyright (C) 2010 Ideas on board SPRL <laurent.pinchart@ideasonboard.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,d
 */

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include "w55fa92_vpe.h"
#include <dirent.h>
#include <asm/types.h>          /* for videodev2.h */
#include "jpegcodec.h"
#include "videodev.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "ch9.h"
#include "video.h"
#include "uvc-gadget_demo.h"
#include "uvc.h"
#include "V4L.h"
#include "jpeg_image.c"
#include <unistd.h>

struct uvc_device *dev;
char vpe_device[] = "/dev/vpe";
char device_jpeg[] = "/dev/video0";
int s_i32JpegFd;
int vpe_fd;
int videoin_select = 1;
int user_memory_mode = 0;
int skip_frames = 0;
int g_u32Width, g_u32Height;
static jpeg_param_t s_sJpegParam;
static jpeg_info_t *s_pJpegInfo = NULL;	
unsigned char *pJpegBuffer = NULL; 
char *mjpeg_image[2] = {NULL, NULL};
unsigned int jpeg_buffer_paddress = 0, jpeg_buffer_vaddress = 0;
unsigned int jpeg_buffer_size;
int uvc_buffer_num = UVC_BUFFER_NUM;

/* ---------------------------------------------------------------------------
 * Request processing
 */

struct uvc_frame_info
{
	unsigned int width;
	unsigned int height;
	unsigned int intervals[8];
};

struct uvc_format_info
{
	unsigned int fcc;
	const struct uvc_frame_info *frames;
};

static const struct uvc_frame_info uvc_frames_yuyv[] = {
	{  640, 480, { 666666, 10000000, 50000000, 0 }, },
	{ 1280, 720, { 50000000, 0 }, },
	{ 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames_mjpeg[] = {
	{  640, 480, { 666666, 10000000, 50000000, 0 }, },
	{ 1280, 720, { 50000000, 0 }, },
	{ 0, 0, { 0, }, },
};

static const struct uvc_format_info uvc_formats[] = {
	{ V4L2_PIX_FMT_YUYV, uvc_frames_yuyv },
	{ V4L2_PIX_FMT_MJPEG, uvc_frames_mjpeg },
};

struct uvc_device
{
	int fd;

	struct uvc_streaming_control probe;
	struct uvc_streaming_control commit;

	int control;

	unsigned int fcc;
	unsigned int width;
	unsigned int height;

	void **mem;
	unsigned int nbufs;
	unsigned int bufsize;

	uint8_t color;
	unsigned int imgsize[3];

	void *imgdata[3];
};


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
int FormatConversion(char* pSrcBuf, char* pDstBuf, int Tarwidth, int Tarheight)
{
	unsigned int volatile vworkbuf, tmpsize;
	unsigned int value, buf=0, ret =0;
	vpe_transform_t vpe_setting;
	unsigned int ptr_y, ptr_u, ptr_v;
	ptr_y = (unsigned int)pSrcBuf;
	ptr_u = ptr_y+Tarwidth*Tarheight;		/* Planar YUV422 */ 
	ptr_v = ptr_u+Tarwidth*Tarheight/2;

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

	value = 0;

	ioctl(vpe_fd, VPE_SET_MMU_MODE, &value);

	unsigned int test=0;
	{	
		vpe_setting.src_addrPacY = ptr_y;				/* Planar YUV422 */
		vpe_setting.src_addrU = ptr_u;	
		vpe_setting.src_addrV = ptr_v;				
		vpe_setting.src_format = VPE_SRC_PLANAR_YUV422;
		vpe_setting.src_width = Tarwidth;
		vpe_setting.src_height = Tarheight;

		vpe_setting.src_leftoffset = 0;
		vpe_setting.src_rightoffset = 0;
		
		vpe_setting.dest_addrPac = (unsigned int)pDstBuf;
	
		vpe_setting.dest_format = VPE_DST_PACKET_YUV422;
		vpe_setting.dest_width = Tarwidth;
		vpe_setting.dest_height = Tarheight;
		vpe_setting.dest_leftoffset = 0;
		vpe_setting.dest_rightoffset = 0;
		vpe_setting.algorithm = VPE_SCALE_DDA;
		vpe_setting.rotation = VPE_OP_NORMAL;
		
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
	}
	
	return ret;	
}	

static struct uvc_device *uvc_open(const char *devname)
{
	struct uvc_device *dev;
	struct v4l2_capability cap;
	int ret;
	int fd;

	fd = open(devname, O_RDWR | O_NONBLOCK);
	if (fd == -1) {
		printf("v4l2 open failed: %s (%d)\n", strerror(errno), errno);
		return NULL;
	}

	printf("open succeeded, file descriptor = %d\n", fd);

	ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
	if (ret < 0) {
		printf("unable to query device: %s (%d)\n", strerror(errno),
			errno);
		close(fd);
		return NULL;
        }

	printf("device is %s on bus %s\n", cap.card, cap.bus_info);

	dev = malloc(sizeof *dev);

	if (dev == NULL) {
		close(fd);
		return NULL;
	}

	memset(dev, 0, sizeof *dev);
	dev->fd = fd;

	return dev;
}

static void uvc_close(struct uvc_device *dev)
{
	close(dev->fd);
	free(dev->imgdata[0]);
	free(dev->imgdata[1]);
	free(dev->mem);
	free(dev);
}


int JpegEncFromVIN(uint32_t u32PlanarYBufPhyAddr, uint8_t **ppu8JpegEncBuf, uint32_t *pu32JpegEncLen, uint32_t offset)
{
	uint32_t u32Width, u32Height;	
	int32_t i32Len;
	uint32_t u32PlanarUBufPhyAddr, u32PlanarVBufPhyAddr;

	memset((void*)&s_sJpegParam, 0, sizeof(s_sJpegParam));

	u32Width = g_u32Width;				
	u32Height = g_u32Height;	

	u32PlanarUBufPhyAddr = 	u32PlanarYBufPhyAddr + u32Width*u32Height;

#ifdef _PLANAR_YUV420_
	u32PlanarVBufPhyAddr = 	u32PlanarfUBufPhyAddr + u32Width*u32Height/4;		
#else
	u32PlanarVBufPhyAddr = 	u32PlanarUBufPhyAddr + u32Width*u32Height/2;	
#endif	

	if((ioctl(s_i32JpegFd, JPEG_SET_ENC_USER_YADDRESS, u32PlanarYBufPhyAddr)) < 0){
		ERR_PRINT("Set jpeg Y address failed:%d\n",errno);
		return -1;
	}
	if((ioctl(s_i32JpegFd, JPEG_SET_ENC_USER_UADDRESS, u32PlanarUBufPhyAddr)) < 0){
		ERR_PRINT("Set jpeg U address failed:%d\n",errno);
		return -1;
	}
	if((ioctl(s_i32JpegFd, JPEG_SET_ENC_USER_VADDRESS, u32PlanarVBufPhyAddr)) < 0){
		ERR_PRINT("Set jpeg V address failed:%d\n",errno);
		return -1;
	}
	
	s_sJpegParam.encode = 1;					/* Encode */
	s_sJpegParam.src_bufsize = offset;				/* Src buffer (Raw Data) */
	s_sJpegParam.scale = 0; 
	s_sJpegParam.encode_source_format = DRVJPEG_ENC_SRC_PLANAR;	/* Source is Planar Format */	
	s_sJpegParam.encode_image_format = DRVJPEG_ENC_PRIMARY_YUV422; 

	s_sJpegParam.buffersize = 0;					/* only for continuous shot */
	s_sJpegParam.buffercount = 1;		

	s_sJpegParam.encode_width = u32Width;
	s_sJpegParam.encode_height = u32Height;

	/* Set encode source stride (Must calling after IOCTL - JPEG_S_PARAM) */
	ioctl(s_i32JpegFd, JPEG_SET_ENC_STRIDE, u32Width);

	/* Set operation property to JPEG engine */
	if((ioctl(s_i32JpegFd, JPEG_S_PARAM, &s_sJpegParam)) < 0){
		ERR_PRINT("Set jpeg parame failed:%d\n",errno);
		return -1;
	}
	
	/* Trigger JPEG engine */
	if((ioctl(s_i32JpegFd, JPEG_TRIGGER, NULL)) < 0){
		ERR_PRINT("Trigger jpeg failed:%d\n",errno);
		return -1;
	}
	i32Len = read(s_i32JpegFd, s_pJpegInfo, sizeof(jpeg_info_t) + sizeof (__u32));
	
	if(i32Len < 0){
		ERR_PRINT("No data can get\n");
		if(s_pJpegInfo->state == JPEG_MEM_SHORTAGE)
			ERR_PRINT("Memory Stortage\n");	
		return -1;
	}

	*ppu8JpegEncBuf = pJpegBuffer + offset;
	*pu32JpegEncLen = s_pJpegInfo->image_size[0];
	return 0;
}

/* ---------------------------------------------------------------------------
 * Video streaming
 */
static void uvc_video_fill_buffer(struct uvc_device *dev, struct v4l2_buffer *buf)
{
	unsigned int datasize;
	static int index = 0;
	uint64_t u64TS;
	uint32_t offset = 0;	
	uint8_t *pu8PicPtr;
	uint32_t u32PicPhyAdr;
	uint32_t u32JpegEncLen;
	uint8_t *pu8JpegBuff;

	if(videoin_select)
		while(skip_frames)
		{
			ReadV4LPicture(&pu8PicPtr, &u64TS, &u32PicPhyAdr);
			TriggerV4LNextFrame();
			skip_frames--;
		}

	/* Fill the buffer with video data. */
	switch (dev->fcc) {
	case V4L2_PIX_FMT_YUYV:
		if(videoin_select)
		{
			if(ReadV4LPicture(&pu8PicPtr, &u64TS, &u32PicPhyAdr) == ERR_V4L_SUCCESS)
			{	
				TriggerV4LNextFrame();
				datasize = 2 * dev->width * dev->height;

				/* Foramt Conversion from Video IN Buffer (Planar) to JPEG Buffer (Packet) */
				if(user_memory_mode)
					FormatConversion((char *)u32PicPhyAdr, (char *)(jpeg_buffer_paddress + buf->m.offset), dev->width, dev->height);
				else
					FormatConversion((char *)u32PicPhyAdr, (char *)jpeg_buffer_paddress, dev->width, dev->height);
	
				/* Copy image data from JPEG Buffer to UVC Buffer */
				if(!user_memory_mode)			
					memcpy(dev->mem[buf->index], (void *)jpeg_buffer_vaddress, datasize);	
	
				dev->color++;
				buf->bytesused = datasize;
			}
		}
		else
		{
			datasize = dev->width * 2 * dev->height;
			memset(dev->mem[buf->index], dev->color & 0xFF , datasize);	
			dev->color++;
			buf->bytesused = datasize;
		}
		break;

	case V4L2_PIX_FMT_MJPEG:
		if(videoin_select)
		{
			if(ReadV4LPicture(&pu8PicPtr, &u64TS, &u32PicPhyAdr) == ERR_V4L_SUCCESS)
			{		
				TriggerV4LNextFrame();
				if(user_memory_mode)
					offset = buf->m.offset;

				if(JpegEncFromVIN(u32PicPhyAdr, &pu8JpegBuff, &u32JpegEncLen, offset) < 0)
					break;

				/* Copy image data from JPEG Buffer to UVC Buffer */
				if(!user_memory_mode)
					memcpy(dev->mem[buf->index], pu8JpegBuff, u32JpegEncLen);
				buf->bytesused = u32JpegEncLen;	
			}
		}
		else
		{
			if(mjpeg_image[0] == NULL && mjpeg_image[1] == NULL)
			{
				if(dev->color > 60)
				{	
					dev->color = 0;
					index ^= 0x01;
				}
			}	
			else
			{
				if(dev->color > 30)
				{	
					dev->color = 0;
					index ^= 0x01;
				}
				if(mjpeg_image[index] == NULL)
					index ^= 0x01;
			}		

			memcpy(dev->mem[buf->index], dev->imgdata[index], dev->imgsize[index]);
			buf->bytesused = dev->imgsize[index];	
			dev->color++;
		}
		break;
	}
}

static int uvc_video_process(struct uvc_device *dev)
{
	struct v4l2_buffer buf;
	int ret;
	memset(&buf, 0, sizeof buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buf.memory = V4L2_MEMORY_MMAP;

	if ((ret = ioctl(dev->fd, VIDIOC_DQBUF, &buf)) < 0) {
		if(!user_memory_mode)
		printf("Unable to dequeue buffer: %s (%d).\n", strerror(errno),
			errno);
		return ret;
	}

	uvc_video_fill_buffer(dev, &buf);

	if ((ret = ioctl(dev->fd, VIDIOC_QBUF, &buf)) < 0) {
		printf("Unable to requeue buffer: %s (%d).\n", strerror(errno),
			errno);
		return ret;
	}
	return 0;
}

static int uvc_video_reqbufs(struct uvc_device *dev, int nbufs)
{
	struct v4l2_requestbuffers rb;
	struct v4l2_buffer buf;
	unsigned int i;
	int ret;

	if(!user_memory_mode)
		for (i = 0; i < dev->nbufs; ++i)
			munmap(dev->mem[i], dev->bufsize);
	free(dev->mem);
	dev->mem = 0;
	dev->nbufs = 0;

	memset(&rb, 0, sizeof rb);
	rb.count = nbufs;
	rb.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	rb.memory = V4L2_MEMORY_MMAP;

	ret = ioctl(dev->fd, VIDIOC_REQBUFS, &rb);
	if (ret < 0) {
		printf("Unable to allocate buffers: %s (%d).\n",
			strerror(errno), errno);
		return ret;
	}

	printf("%u buffers allocated.\n", rb.count);

	/* Map the buffers. */
	dev->mem = malloc(rb.count * sizeof dev->mem[0]);

	for (i = 0; i < rb.count; ++i) {
		memset(&buf, 0, sizeof buf);
		buf.index = i;
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(dev->fd, VIDIOC_QUERYBUF, &buf);
		if (ret < 0) {
			printf("Unable to query buffer %u: %s (%d).\n", i,
				strerror(errno), errno);
			return -1;
		}
		printf("length: %u offset: %u\n", buf.length, buf.m.offset);
		if(user_memory_mode)
			dev->mem[i] = (void *)jpeg_buffer_vaddress + buf.length * i;
		else	
			dev->mem[i] = mmap(0, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, dev->fd, buf.m.offset);

		if (dev->mem[i] == MAP_FAILED) {
			printf("Unable to map buffer %u: %s (%d)\n", i,
				strerror(errno), errno);
			return -1;
		}
		printf("Buffer %u mapped at address %p.\n", i, dev->mem[i]);
	}

	dev->bufsize = buf.length;
	dev->nbufs = rb.count;
	return 0;
}

static int uvc_video_stream(struct uvc_device *dev, int enable)
{
	struct v4l2_buffer buf;
	unsigned int i;
	int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	int ret;

	if (!enable) {
		printf("Stopping video stream.\n");
		ioctl(dev->fd, VIDIOC_STREAMOFF, &type);
		return 0;
	}

//	printf("Starting video stream.\n");

	for (i = 0; i < dev->nbufs; ++i) {
		memset(&buf, 0, sizeof buf);

		buf.index = i;
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.memory = V4L2_MEMORY_MMAP;
	
		uvc_video_fill_buffer(dev, &buf);

//		printf("Queueing buffer %u.\n", i);
		if ((ret = ioctl(dev->fd, VIDIOC_QBUF, &buf)) < 0) {
			printf("Unable to queue buffer: %s (%d).\n",
				strerror(errno), errno);
			break;
		}
	}

	ioctl(dev->fd, VIDIOC_STREAMON, &type);
	return ret;
}

static int uvc_video_set_format(struct uvc_device *dev)
{
	struct v4l2_format fmt;
	int ret;

//	printf("Setting format to 0x%08x %ux%u\n",
//		dev->fcc, dev->width, dev->height);

	if(dev->height > g_u32Height || g_u32Width > dev->width)
		skip_frames = SKIP_FRAME_NUM;

	g_u32Height = dev->height;
	g_u32Width = dev->width;

	memset(&fmt, 0, sizeof fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	fmt.fmt.pix.width = dev->width;
	fmt.fmt.pix.height = dev->height;
	fmt.fmt.pix.pixelformat = dev->fcc;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	if(videoin_select)
	{
		ioctl(dev->fd, VIDIOC_S_USER_MEMORY_POOL_NUMBER, &uvc_buffer_num);

		if (dev->fcc == V4L2_PIX_FMT_MJPEG)
			fmt.fmt.pix.sizeimage = 600 * 1024;

		StopV4LCapture();		
#ifdef _PLANAR_YUV420_
		SetV4LEncode(0, g_u32Width, g_u32Height, VIDEO_PALETTE_YUV420P);
#else
		SetV4LEncode(0, g_u32Width, g_u32Height, VIDEO_PALETTE_YUV422P);
#endif
		SetV4LViewWindow(g_u32Width, g_u32Height, g_u32Width, g_u32Height);

		StartV4LCapture();
	}
	else
	{
		if (dev->fcc == V4L2_PIX_FMT_MJPEG)
			fmt.fmt.pix.sizeimage = dev->imgsize[0] * 1.5;
	}
	

	if ((ret = ioctl(dev->fd, VIDIOC_S_FMT, &fmt)) < 0)
		printf("Unable to set format: %s (%d).\n",
			strerror(errno), errno);

	return ret;
}

static int uvc_video_init(struct uvc_device *dev __attribute__((__unused__)))
{
	return 0;
}


static void uvc_fill_streaming_control(struct uvc_device *dev, struct uvc_streaming_control *ctrl, int iframe, int iformat)
{
	const struct uvc_format_info *format;
	const struct uvc_frame_info *frame;
	unsigned int nframes;
	int size;

	if (iformat < 0)
		iformat = ARRAY_SIZE(uvc_formats) + iformat;
	if (iformat < 0 || iformat >= (int)ARRAY_SIZE(uvc_formats))
		return;
	format = &uvc_formats[iformat];

	nframes = 0;
	while (format->frames[nframes].width != 0)
		++nframes;

	if (iframe < 0)
		iframe = nframes + iframe;
	if (iframe < 0 || iframe >= (int)nframes)
		return;
	frame = &format->frames[iframe];

	memset(ctrl, 0, sizeof *ctrl);

	ctrl->bmHint = 1;
	ctrl->bFormatIndex = iformat + 1;
	ctrl->bFrameIndex = iframe + 1;
	ctrl->dwFrameInterval = frame->intervals[0];
	switch (format->fcc) {
	case V4L2_PIX_FMT_YUYV:
		ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2;
		break;
	case V4L2_PIX_FMT_MJPEG:
		if(videoin_select)
			ctrl->dwMaxVideoFrameSize = 600 *1024;
		else
			ctrl->dwMaxVideoFrameSize = dev->imgsize[0];
		break;
	}
	ctrl->dwMaxPayloadTransferSize = 512;	/* TODO this should be filled by the driver. */
	ctrl->bmFramingInfo = 3;
	ctrl->bPreferedVersion = 1;
	ctrl->bMaxVersion = 1;
}

static void uvc_events_process_standard(struct uvc_device *dev, struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp)
{
//	printf("standard request\n");
	(void)dev;
	(void)ctrl;
	(void)resp;
}

static void uvc_events_process_control(struct uvc_device *dev, uint8_t req, uint8_t cs, struct uvc_request_data *resp)
{
//	printf("control request (req %02x cs %02x)\n", req, cs);
	(void)dev;
	(void)resp;
}

static void uvc_events_process_streaming(struct uvc_device *dev, uint8_t req, uint8_t cs, struct uvc_request_data *resp)
{
	struct uvc_streaming_control *ctrl;

//	printf("streaming request (req %02x cs %02x)\n", req, cs);

	if (cs != UVC_VS_PROBE_CONTROL && cs != UVC_VS_COMMIT_CONTROL)
		return;

	ctrl = (struct uvc_streaming_control *)&resp->data;
	resp->length = sizeof *ctrl;

	switch (req) {
	case UVC_SET_CUR:
		dev->control = cs;
		resp->length = 34;
		break;

	case UVC_GET_CUR:
		if (cs == UVC_VS_PROBE_CONTROL)
			memcpy(ctrl, &dev->probe, sizeof *ctrl);
		else
			memcpy(ctrl, &dev->commit, sizeof *ctrl);
		break;

	case UVC_GET_MIN:
	case UVC_GET_MAX:
	case UVC_GET_DEF:
		uvc_fill_streaming_control(dev, ctrl, req == UVC_GET_MAX ? -1 : 0,
					   req == UVC_GET_MAX ? -1 : 0);
		break;

	case UVC_GET_RES:
		memset(ctrl, 0, sizeof *ctrl);
		break;

	case UVC_GET_LEN:
		resp->data[0] = 0x00;
		resp->data[1] = 0x22;
		resp->length = 2;
		break;

	case UVC_GET_INFO:
		resp->data[0] = 0x03;
		resp->length = 1;
		break;
	}
}

static void uvc_events_process_class(struct uvc_device *dev, struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp)
{
	if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)
		return;

	switch (ctrl->wIndex & 0xff) {
	case UVC_INTF_CONTROL:
		uvc_events_process_control(dev, ctrl->bRequest, ctrl->wValue >> 8, resp);
		break;

	case UVC_INTF_STREAMING:
		uvc_events_process_streaming(dev, ctrl->bRequest, ctrl->wValue >> 8, resp);
		break;

	default:
		break;
	}
}

static void uvc_events_process_setup(struct uvc_device *dev, struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp)
{
	dev->control = 0;

//	printf("bRequestType %02x bRequest %02x wValue %04x wIndex %04x "
//		"wLength %04x\n", ctrl->bRequestType, ctrl->bRequest,
//		ctrl->wValue, ctrl->wIndex, ctrl->wLength);

	switch (ctrl->bRequestType & USB_TYPE_MASK) {
	case USB_TYPE_STANDARD:
		uvc_events_process_standard(dev, ctrl, resp);
		break;

	case USB_TYPE_CLASS:
		uvc_events_process_class(dev, ctrl, resp);
		break;

	default:
		break;
	}
}

static void uvc_events_process_data(struct uvc_device *dev, struct uvc_request_data *data)
{
	struct uvc_streaming_control *target;
	struct uvc_streaming_control *ctrl;
	const struct uvc_format_info *format;
	const struct uvc_frame_info *frame;
	const unsigned int *interval;
	unsigned int iformat, iframe;
	unsigned int nframes;

	switch (dev->control) {
	case UVC_VS_PROBE_CONTROL:
//		printf("setting probe control, length = %d\n", data->length);
		target = &dev->probe;
		break;

	
	case UVC_VS_COMMIT_CONTROL:
//		printf("setting commit control, length = %d\n", data->length);
		target = &dev->commit;
		break;

	default:
		printf("setting unknown control, length = %d\n", data->length);
		return;
	}

	ctrl = (struct uvc_streaming_control *)&data->data;
	iformat = clamp((unsigned int)ctrl->bFormatIndex, 1U,
			(unsigned int)ARRAY_SIZE(uvc_formats));
	format = &uvc_formats[iformat-1];

	nframes = 0;
	while (format->frames[nframes].width != 0)
		++nframes;

	iframe = clamp((unsigned int)ctrl->bFrameIndex, 1U, nframes);
	frame = &format->frames[iframe-1];
	interval = frame->intervals;

	while (interval[0] < ctrl->dwFrameInterval && interval[1])
		++interval;

	target->bFormatIndex = iformat;
	target->bFrameIndex = iframe;
	switch (format->fcc) {
	case V4L2_PIX_FMT_YUYV:
		target->dwMaxVideoFrameSize = frame->width * frame->height * 2;
		break;
	case V4L2_PIX_FMT_MJPEG:
		if(videoin_select)
			target->dwMaxVideoFrameSize = 600 *1024;
		else
			target->dwMaxVideoFrameSize = dev->imgsize[0];
		break;
	}
	target->dwFrameInterval = *interval;

	if (dev->control == UVC_VS_COMMIT_CONTROL) {
		dev->fcc = format->fcc;
		dev->width = frame->width;
		dev->height = frame->height;

		uvc_video_set_format(dev);
	}
}

static void uvc_events_process(struct uvc_device *dev)
{
	struct v4l2_event v4l2_event;
	struct uvc_event *uvc_event = (void *)&v4l2_event.u.data;
	struct uvc_request_data resp;
	int ret;

	ret = ioctl(dev->fd, VIDIOC_DQEVENT, &v4l2_event);
	if (ret < 0) {
		printf("VIDIOC_DQEVENT failed: %s (%d)\n", strerror(errno),
			errno);
		return;
	}

	memset(&resp, 0, sizeof resp);
	resp.length = -EL2HLT;

	switch (v4l2_event.type) {
	case UVC_EVENT_CONNECT:
	case UVC_EVENT_DISCONNECT:
		return;

	case UVC_EVENT_SETUP:
		uvc_events_process_setup(dev, &uvc_event->req, &resp);
		break;

	case UVC_EVENT_DATA:
		uvc_events_process_data(dev, &uvc_event->data);
		return;

	case UVC_EVENT_STREAMON:
		uvc_video_reqbufs(dev, uvc_buffer_num);
		uvc_video_stream(dev, 1);
		return;

	case UVC_EVENT_STREAMOFF:
		uvc_video_stream(dev, 0);
		uvc_video_reqbufs(dev, 0);
		return;
	}

	ioctl(dev->fd, UVCIOC_SEND_RESPONSE, &resp);
	
	if (ret < 0) {
		printf("UVCIOC_S_EVENT failed: %s (%d)\n", strerror(errno),
			errno);
		return;
	}
}

static void uvc_events_init(struct uvc_device *dev)
{
	struct v4l2_event_subscription sub;

	uvc_fill_streaming_control(dev, &dev->probe, 0, 0);
	uvc_fill_streaming_control(dev, &dev->commit, 0, 0);

	memset(&sub, 0, sizeof sub);
	sub.type = UVC_EVENT_SETUP;
	ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
	sub.type = UVC_EVENT_DATA;
	ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
	sub.type = UVC_EVENT_STREAMON;
	ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
	sub.type = UVC_EVENT_STREAMOFF;
	ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
}

/* ---------------------------------------------------------------------------
 * main
 */
static void image_load(struct uvc_device *dev, int load, const char *img)
{
	int fd = -1;
	static int i = 0;
	
	if(load == 0)
	{	
		dev->imgdata[0] = jpeg_image0;
		dev->imgdata[1] = jpeg_image1;
		dev->imgsize[0] = sizeof(jpeg_image0);
		dev->imgsize[1] = sizeof(jpeg_image1);
		goto sort;
	}

	if (img == NULL)
		return;
		
	fd = open(img, O_RDONLY);
	if (fd == -1) {
		printf("Unable to open MJPEG image '%s'\n", img);
		return;
	}

	dev->imgsize[i] = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	dev->imgdata[i] = malloc(dev->imgsize[i]);
	if (dev->imgdata[i] == NULL) {
		printf("Unable to allocate memory for MJPEG image %d\n",i);
		dev->imgsize[i] = 0;
		return;
	}

	read(fd, dev->imgdata[i], dev->imgsize[i]);
	printf("JPEG Image(%s) : %8d Bytes\n",i,img,dev->imgsize[i]);
	close(fd);
	i++;	
	if(i > 1)
	{
sort:
		if(dev->imgsize[1] > dev->imgsize[0])
		{
			dev->imgsize[2] = dev->imgsize[0];
			dev->imgsize[0] = dev->imgsize[1];
			dev->imgsize[1] = dev->imgsize[2];

			dev->imgdata[2] = dev->imgdata[0];
			dev->imgdata[0] = dev->imgdata[1];
			dev->imgdata[1] = dev->imgdata[2];
		}
	}
}

static void usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s [options]\n", argv0);
	fprintf(stderr, "Available options are\n");
	fprintf(stderr, " -b number	UVC Buffer Number\n");
	fprintf(stderr, " -d device	Video device - Gadget\n");
	fprintf(stderr, " -f 		Using fix pattern (option u will be ignored)\n");
	fprintf(stderr, " -h		Print this help screen and exit\n");
	fprintf(stderr, " -i image	MJPEG image 0 file\n");
	fprintf(stderr, " -j image	MJPEG image 1 file\n");
	fprintf(stderr, " -v device	Video device - Sensor\n");
	fprintf(stderr, " -u 	        Let V4l2 use the buffer assigned by user application (for g_webcam_n329 module only)\n");
}

int main(int argc, char *argv[])
{
	char *device = "/dev/video2";
	char *videoin_device = "/dev/video0";
    	DIR *dir;
	int i;
	fd_set fds;
	int ret, opt;
	int size;
	unsigned int user_memorypool_enable = 1; 
	uint32_t u32PreviewImgWidth=0, u32PreviewImgHeight=0;
	E_IMAGE_RESOL eEncodeImgResol = eIMAGE_VGA;


	while ((opt = getopt(argc, argv, "fvb:d:hi:j:u")) != -1) {
		switch (opt) {
		case 'f':
			videoin_select = 0;
			break;	
		case 'u':
			user_memory_mode = 1;
			break;
		case 'v':
			videoin_device = optarg;
			break;	
		case 'b':
			uvc_buffer_num = atoi(optarg);
			break;
		case 'd':
			device = optarg;
			break;

		case 'h':
			usage(argv[0]);
			return 0;

		case 'i':
			mjpeg_image[0] = optarg;
			break;

		case 'j':
			mjpeg_image[1] = optarg;
			break;
		default:
			fprintf(stderr, "Invalid option '-%c'\n", opt);
			usage(argv[0]);
			return 1;
		}
	}


	if(videoin_select == 1)
	{
		/* Video IN Init */
		if(InitV4LDevice(eEncodeImgResol) == ERR_V4L_SUCCESS)
		{
			printf("Init V4L device pass\n");
			g_u32Width = 640;
			g_u32Height = 480;
			u32PreviewImgWidth = 640;
			u32PreviewImgHeight = 480;
#ifdef _PLANAR_YUV420_
			//Planar YUV420
			SetV4LEncode(0, g_u32Width, g_u32Height, VIDEO_PALETTE_YUV420P);
#else
			//Planar YUV422
			SetV4LEncode(0, g_u32Width, g_u32Height, VIDEO_PALETTE_YUV422P);
#endif
			SetV4LViewWindow(u32PreviewImgWidth, u32PreviewImgHeight, g_u32Width, g_u32Height);
			StartV4LCapture();
		}
		else
		{
			printf("Init V4L device Fail! - Use built-in pattern\n");
			videoin_select = 0;
			user_memory_mode = 0;
		}

		/* JPEG  Init */
		printf("Open jpegcodec device\n");

		/* Try to open folder "/sys/class/video4linux/video1/",
		   if the folder exists, jpegcodec is "/dev/video1", otherwises jpegcodec is "/dev/video0" */
		dir = opendir("/sys/class/video4linux/video1/");
		if(dir)
		{
			closedir(dir);		
			device_jpeg[10]++;
		}	

		for(i=0;i<5;i++)
		{
			s_i32JpegFd = open(device_jpeg, O_RDWR);
			if (s_i32JpegFd < 0)
				continue;	
			break;
		}
		if (s_i32JpegFd < 0)
		{
			printf(" JPEG Engine is busy! - Use built-in pattern\n");
			videoin_select = 0;
			user_memory_mode = 0;
			goto set_jpeg_pattern;
		}
        	printf("  *jpegcodec is %s\n",device_jpeg);	

		s_pJpegInfo = malloc(sizeof(jpeg_info_t) + sizeof(__u32));

		if(s_pJpegInfo == NULL){
			ERR_PRINT("Allocate JPEG info failed! - Use built-in pattern\n");
			close(s_i32JpegFd);
			videoin_select = 0;
			user_memory_mode = 0;
			goto set_jpeg_pattern;
		}

	        /* allocate memory for JPEG engine */
		ioctl(s_i32JpegFd, JPEG_GET_JPEG_BUFFER, (__u32)&jpeg_buffer_size);

		pJpegBuffer = mmap(NULL, jpeg_buffer_size, PROT_READ|PROT_WRITE, MAP_SHARED, s_i32JpegFd, 0);

		if(pJpegBuffer == MAP_FAILED)
		{
			printf(" JPEG Map Failed - Use built-in jpeg pattern\n");
			videoin_select = 0;
			user_memory_mode = 0;
			goto set_jpeg_pattern;
		}
		else
		{
			ioctl(s_i32JpegFd, JPEG_GET_JPEG_BUFFER_PADDR, (__u32)&jpeg_buffer_paddress);
			
			jpeg_buffer_vaddress = (unsigned int) pJpegBuffer;

			printf("  *Jpeg engine buffer: %dKB\n",jpeg_buffer_size / 1024);
			printf("     -Physical address:    0x%08X\n", jpeg_buffer_paddress);
			printf("     -User Space address:  0x%08X\n", jpeg_buffer_vaddress);	
  		}

		/* VPE Init */
		if ( InitVPE() ){				
			exit(-1);
		}	
	}
	else
		user_memory_mode = 0;

	dev = uvc_open(device);

	if (dev == NULL)
		return 1;

	if(videoin_select && user_memory_mode)
	{
		if(jpeg_buffer_size >= (uvc_buffer_num * MAX_IMAGE_SIZE))
		{
			ioctl(dev->fd, VIDIOC_S_USER_MEMORY_POOL_ENABLE, &user_memorypool_enable);
		
			ioctl(dev->fd, VIDIOC_S_MEMORY_POOL_PADDR, &jpeg_buffer_paddress);

			ioctl(dev->fd, VIDIOC_S_USER_MEMORY_POOL_NUMBER, &uvc_buffer_num);

			size = MAX_IMAGE_SIZE;
			ioctl(dev->fd, VIDIOC_S_USER_MEMORY_POOL_OFFSET, &size);

			printf("  *Assign JPEG buffer to UVC buffer\n");
			printf("     -Physical address:    0x%08X\n", jpeg_buffer_paddress);
			printf("     -User Space address:  0x%08X\n", jpeg_buffer_vaddress);
			printf("  *Each UVC buffer size is %d Bytes\n", size);
		}
		else
		{
			printf("Can't use user memory mode (JPEG Buffer Size %dKB < Required Size %dKB) -> Disable User memory mode\n", jpeg_buffer_size /1024,(uvc_buffer_num * MAX_IMAGE_SIZE) / 1024);
			user_memory_mode = 0;
		}
	}

	if(!videoin_select)
	{
set_jpeg_pattern:
		if(mjpeg_image[0] == NULL && mjpeg_image[1] == NULL)
			image_load(dev, 0, 0);
		else
		{
			image_load(dev, 1, mjpeg_image[0]);
		 	image_load(dev, 1, mjpeg_image[1]);
		}
	}			

	uvc_events_init(dev);
	uvc_video_init(dev);

	FD_ZERO(&fds);
	FD_SET(dev->fd, &fds);

	while (1) {
		fd_set efds = fds;
		fd_set wfds = fds;

		ret = select(dev->fd + 1, NULL, &wfds, &efds, NULL);
		if (ret < 0)
			return ret;
		if (FD_ISSET(dev->fd, &efds))
			uvc_events_process(dev);
		if (FD_ISSET(dev->fd, &wfds))
			uvc_video_process(dev);	
	}

	uvc_close(dev);
out:	
	if(videoin_select)
	{
		if(pJpegBuffer)
			munmap(pJpegBuffer, jpeg_buffer_size);
		close(s_i32JpegFd);
	}	

	return 0;
}

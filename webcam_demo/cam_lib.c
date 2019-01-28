#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>              
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <malloc.h>
//#include <linux/delay.h>
#include "fb_lib.h"
#include "cam_lib.h"

#include "jpegcodec.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))

struct buffer *buffers = NULL;

unsigned int capture_fmt = V4L2_PIX_FMT_YUYV;

unsigned int rawdata_paddress = 0, rawdata_vaddress = 0, bitstream_paddress = 0, bitstream_vaddress = 0;

static int init_mmap(int fd)
{
	struct v4l2_requestbuffers req;
	unsigned int n_buffers = 0;

	CLEAR(req);

	/*
		      JPEG Engine Buffer Map
		[Buffer for Other Use		]
		[Buffer for Webcam Bitstream	]
		[v4l2 Buffer index 0		]
	*/
	bitstream_paddress = jpeg_buffer_paddress + BUFFER_SIZE_OTHER_USE;	//jpeg bit-stream physical address
	bitstream_vaddress = jpeg_buffer_vaddress + BUFFER_SIZE_OTHER_USE;	//v4l2 driver buffer logical address

	rawdata_paddress = bitstream_paddress + BUFFER_SIZE_WEBCAM_BITSTREAM;	//v4l2 driver buffer physical address
	rawdata_vaddress = bitstream_vaddress + BUFFER_SIZE_WEBCAM_BITSTREAM;	//v4l2 driver buffer logical address

	printf("  *JPEG Engine Buffer\n");
	printf("    -Address for Encode Bitstream   0x%08X\n",bitstream_vaddress);

	if(user_memorypool_enable)
	{	
		ioctl(fd, VIDIOC_S_USER_MEMORY_POOL_ENABLE, &user_memorypool_enable);
	
		ioctl(fd, VIDIOC_S_MEMORY_POOL_PADDR, &rawdata_paddress);

		printf("    -Address for V4L2 Driver Buffer 0x%08X\n",rawdata_vaddress);
	}
	else
	{
		ioctl(fd, VIDIOC_S_USER_MEMORY_POOL_ENABLE, &user_memorypool_enable);
		printf("    -Address for Raw Buffer         0x%08X\n",rawdata_vaddress);
	}
	req.count = BUFFER_COUNT;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == ioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			return -ERR_NO_SUPPORT;
		} else {
			return -ERR_UNKNOWN;
		}
	}

	buffers = calloc(BUFFER_COUNT, sizeof(*buffers));

	if (!buffers) {
		return -ERR_OUT_OF_MEM;
	}
	printf("  *V4L2 Buffers\n");	
	for (n_buffers = 0; n_buffers < BUFFER_COUNT; ++n_buffers)
	{
		struct v4l2_buffer buf;

		CLEAR(buf);
		if(user_memorypool_enable)
		{
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.index = n_buffers;

			if (-1 == ioctl(fd, VIDIOC_QUERYBUF, &buf))
				return -ERR_UNKNOWN;

			buffers[n_buffers].length = buf.length;
			buffers[n_buffers].start = (void *)rawdata_vaddress + buf.length * n_buffers;
		}
		else
		{
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = n_buffers;
	
			if (-1 == ioctl(fd, VIDIOC_QUERYBUF, &buf))
				return -ERR_UNKNOWN;

			buffers[n_buffers].length = buf.length;
			buffers[n_buffers].start = mmap(NULL /* start anywhere */ ,
						buf.length,
						PROT_READ | PROT_WRITE
						/* required */ ,
						MAP_SHARED /* recommended */ ,
						fd, buf.m.offset);

			if (MAP_FAILED == buffers[n_buffers].start)
				return -ERR_UNKNOWN;
		}
		printf("    -Index %d 0x%08X\n", n_buffers,(unsigned int)buffers[n_buffers].start);
	}

    return 0;
}

static void stop_capturing(int fd)
{
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ioctl(fd, VIDIOC_STREAMOFF, &type);
}

static int start_capturing(int fd)
{
	unsigned int i;
	enum v4l2_buf_type type;

	for (i = 0; i < BUFFER_COUNT; ++i) {
		struct v4l2_buffer buf;
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if (-1 == ioctl(fd, VIDIOC_QBUF, &buf))
			return -ERR_UNKNOWN;
	}
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == ioctl(fd, VIDIOC_STREAMON, &type))
		return -ERR_UNKNOWN;
    return 0;
}

int cam_start(int fd)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	unsigned int min;
	int ret;

	if (-1 == ioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			return -ERR_NO_DEV;
		} else {
			return -ERR_NO_VIDEO;
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		return -ERR_NO_VIDEO;
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		return -ERR_NO_SUPPORT;
	}

	/* Select video input, video standard and tune here. */
	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == ioctl(fd, VIDIOC_CROPCAP, &cropcap))
	{
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect;	/* reset to default */

		if (-1 == ioctl(fd, VIDIOC_S_CROP, &crop))
		{
			switch (errno)
			{
				case EINVAL:
					/* Cropping not supported. */
					break;
				default:
					/* Errors ignored. */
					break;
			}
		}
	}
	else 
	{
		/* Errors ignored. */
	}

	CLEAR(fmt);

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = image_width;
	fmt.fmt.pix.height = image_height;

	if (-1 == ioctl(fd, VIDIOC_G_FMT, &fmt))
		return -ERR_NO_FMT;

	printf("  *Width=%d Height=%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
	printf("  *fmt=%x\n", fmt.fmt.pix.pixelformat);

	/* Note VIDIOC_S_FMT may change width and height. */

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	ret = init_mmap(fd);
        if (ret < 0)
            return ret;

	return start_capturing(fd);
}

int GetKey(int32_t i32KpdFd, int32_t *pi32KeyCode)
{
	struct input_event sKpdData;

	read(i32KpdFd,&sKpdData,sizeof(sKpdData));
	if(sKpdData.type == 0){
		return -1;
	}
	if ((sKpdData.value != 0)){
		return -1;
	}
	*pi32KeyCode = sKpdData.code;

	return 0;
}

int cam_read(int fd, struct v4l2_buffer *buf)
{	
	unsigned int i;

	int32_t i32KeyCode;
	memset(buf,0,sizeof(struct v4l2_buffer));
	buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf->memory = V4L2_MEMORY_MMAP;
	buf->index = 0;

	if (-1 == ioctl(fd, VIDIOC_DQBUF, buf)) {
		return -ERR_UNKNOWN;
	}

	assert(buf->index < BUFFER_COUNT);
	usleep(10);
	FormatConvert((char*)buffers[buf->index].start, (char*) fb_addr, vinfo.xres, vinfo.yres);
	usleep(10);

	if(!(GetKey(i32KpdFd, &i32KeyCode) < 0))
	{
		switch(i32KeyCode)
		{
			case CAMERA:
			{
				jpegEnc(buf);
				break;	
			}
			default:
				break;
		}
	}
	
	if (-1 == ioctl(fd, VIDIOC_QBUF, buf))
		return -ERR_UNKNOWN;

	return 1;
}

void cam_stop(int fd)
{
	unsigned int i;

	stop_capturing(fd);

	if(user_memorypool_enable == 0)
		for (i = 0; i < BUFFER_COUNT; ++i)
			munmap(buffers[i].start, buffers[i].length);

	free(buffers);
}

void cam_close(int fd)
{
	close(fd);
}




#ifndef CAM_LIB_H
#define CAM_LIB_H

#include <linux/input.h>
#include <linux/fb.h>
#include "Misc.h"
#include "fb_lib.h"
#include <asm/types.h>          /* for videodev2.h */
#include "videodev2.h"

enum {
        ERR_NO_DEV = 1,
        ERR_NO_VIDEO,
        ERR_NO_SUPPORT,
        ERR_NO_FMT,
        ERR_OUT_OF_MEM,
        ERR_UNKNOWN
};

struct buffer {
	void *start;
	size_t length;
};

//Other process may use JPEG engine too. After closing JEPG engine, the buffer may not allow to be used by other process.
#define BUFFER_SIZE_OTHER_USE 		0*1024	
//Bitstream Buffer for Webcam JPEG encode
#define BUFFER_SIZE_WEBCAM_BITSTREAM	200*1024

extern int BUFFER_COUNT;
extern int vpe_fd;
extern int fd_jpeg;
extern int32_t i32KpdFd;
extern unsigned char *fb_addr;
extern struct buffer *buffers;
extern int image_width;
extern int image_height;
extern unsigned int jpeg_buffer_paddress,jpeg_buffer_vaddress;
extern unsigned int rawdata_paddress, rawdata_vaddress, bitstream_paddress, bitstream_vaddress;
extern int cam_start(int fd);
extern int cam_read(int fd, struct v4l2_buffer *buf);
extern void cam_stop(int fd);
extern void cam_close(int fd);
extern int jpegEnc(struct v4l2_buffer *buf);
extern unsigned int user_memorypool_enable;
extern int fb_fd;
extern struct fb_var_screeninfo vinfo;
extern struct fb_fix_screeninfo finfo;
#endif

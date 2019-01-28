/* ijg_jpeg_demo.c
 *
 *
 * Copyright (c)2008 Nuvoton technology corporation
 * http://www.nuvoton.com
 *
 * Audio playback demo application
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/soundcard.h>
#include <sys/poll.h>
#include <pthread.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <linux/fb.h>

#include <jpeglib.h>

#define IOCTL_FB_LOCK			_IOW('v', 64, unsigned int)
#define IOCTL_FB_UNLOCK			_IOW('v', 65, unsigned int)
#define IOCTL_WAIT_VSYNC		_IOW('v', 67, unsigned int)

typedef struct
{
	int	lcm_fd;
	unsigned char *pLCMBuffer;
	unsigned char *pLCMBuffer2;
	struct fb_var_screeninfo fb_vinfo;
	struct fb_fix_screeninfo fb_finfo;
	unsigned char u8bytes_per_pixel;
} S_DEMO_LCM;

S_DEMO_LCM g_sDemo_Lcm;

int InitLCM()
{
	printf("\n### Open LCM device.\n");

	g_sDemo_Lcm.lcm_fd = open( "/dev/fb0", O_RDWR );
	if ( g_sDemo_Lcm.lcm_fd <= 0 )
	{
		printf( "### Error: cannot open LCM device, returns %d!\n", g_sDemo_Lcm.lcm_fd );
		return 0;
	}

	if ( ioctl(g_sDemo_Lcm.lcm_fd, FBIOGET_VSCREENINFO, &(g_sDemo_Lcm.fb_vinfo)) )
	{
		printf( "ioctl FBIOGET_VSCREENINFO failed!\n" );
		close( g_sDemo_Lcm.lcm_fd );
		return 0;
	}
	g_sDemo_Lcm.u8bytes_per_pixel = g_sDemo_Lcm.fb_vinfo.bits_per_pixel / 8;

	if ( ioctl(g_sDemo_Lcm.lcm_fd, FBIOGET_FSCREENINFO, &(g_sDemo_Lcm.fb_finfo)) )
	{
		printf( "ioctl FBIOGET_FSCREENINFO failed!\n" );
		close( g_sDemo_Lcm.lcm_fd );
        return 0;
    }

	return 1;
}

int _DemoLCM_SetLCMBuffer()
{
	// Map the device to memory
	printf( "### Run mmap.\n" );

	g_sDemo_Lcm.pLCMBuffer = mmap( NULL, (g_sDemo_Lcm.fb_finfo.line_length * g_sDemo_Lcm.fb_vinfo.yres ), PROT_READ|PROT_WRITE, MAP_SHARED, g_sDemo_Lcm.lcm_fd, 0 );
	if ( (int)g_sDemo_Lcm.pLCMBuffer == -1 )
	{
		printf( "### Error: failed to map LCM device to memory!\n" );
		return 0;
	}
	else
		printf( "### LCM Buffer at:%p, width = %d, height = %d, line_length = %d.\n\n", g_sDemo_Lcm.pLCMBuffer, g_sDemo_Lcm.fb_vinfo.xres, g_sDemo_Lcm.fb_vinfo.yres, g_sDemo_Lcm.fb_finfo.line_length );

	return 1;
}

METHODDEF(void)
my_error_exit( j_common_ptr cinfo )
{
	printf( "### Oops! error occurred, here is the message from library. ###\n" );

	(*cinfo->err->output_message) (cinfo);
	jpeg_destroy(cinfo);

	printf( "### Here, you can exit program or not. ###\n" );

	exit(0);	// FIX ME: if you want to return to console while error occured.
}

void decompress( char * jpeg_filename, int enableScaleDown )
{
	printf( "2. Decompress %s\n", jpeg_filename );

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error( &jerr );
//	cinfo.err->trace_level = 1;			// FIXME: un-mark for printing H/W JPEG CODEC message
	jerr.error_exit = my_error_exit;
	jpeg_create_decompress( &cinfo );

//	cinfo.output_components = 4;	// FIX ME: if we need output ARGB8888

#if 1	// FIX ME: file as source
	FILE * infile;
	if ( (infile = fopen( jpeg_filename, "rb" )) == NULL )
	{
		fprintf( stderr, "can't open %s\n", jpeg_filename );
		exit( 1 );
	}

	jpeg_stdio_src( &cinfo, infile );
#else	// FIX ME: memory as source
	FILE *fpr;
	char * src1;

	fpr = fopen( jpeg_filename, "rb" );
	if ( fpr < 0 )
	{
		printf( "Open read file error \n" );
		return;
	}
	fseek( fpr, 0L, SEEK_END );
	int sz2 = ftell( fpr );
	fseek( fpr, 0L, SEEK_SET );
	src1 = malloc( sz2 );
	fread( src1, 1, sz2, fpr );
	fclose( fpr );

	jpeg_mem_src( &cinfo, src1, sz2 );
#endif

	jpeg_read_header( &cinfo, TRUE );

	int scale_num, scale_denom;
	scale_num = scale_denom = 1;
	if ( enableScaleDown )
	{
		unsigned short u16RealWidth, u16RealHeight, u16RealWidth2, u16RealHeight2;
		(void) jpeg_calc_output_dimensions(&cinfo);
		u16RealWidth2 = u16RealWidth = cinfo.output_width;
		u16RealHeight2 = u16RealHeight = cinfo.output_height;
		printf( "\tbefore scale width = %d, before scale height = %d\n", u16RealWidth, u16RealHeight );
		int scale_flag;
		float scale_ratio;
		scale_flag = 0;
		scale_num = 4;
		scale_denom = 8;
_FIT_:
		if ( (u16RealWidth2 > g_sDemo_Lcm.fb_vinfo.xres) || (u16RealHeight2 > g_sDemo_Lcm.fb_vinfo.yres) )
		{
			scale_flag = 1;
			scale_ratio = (float)scale_num / (float)scale_denom;
			u16RealWidth2 = u16RealWidth * scale_ratio;
			u16RealHeight2 = u16RealHeight * scale_ratio;
			printf( "\tafter scaled width = %d, after scaled height = %d\n", u16RealWidth2, u16RealHeight2 );
			scale_denom++;
			goto _FIT_;
		}
		if ( scale_flag )
		{
			scale_denom--;
		}
		else
		{
			scale_num = 1;
			scale_denom = 1;
		}
	}

	cinfo.dct_method = JDCT_IFAST;				// don't care, hw not support
//	cinfo.desired_number_of_colors = 256;		// don't care, hw not support
//	cinfo.quantize_colors = TRUE;				// don't care, hw not support
//	cinfo.dither_mode = JDITHER_FS;				// don't care, hw not support
//	cinfo.dither_mode = JDITHER_NONE;			// don't care, hw not support
//	cinfo.dither_mode = JDITHER_ORDERED;		// don't care, hw not support
//	cinfo.two_pass_quantize = FALSE;			// don't care, hw not support
	cinfo.scale_num = scale_num;				// hw just support decode scaling down
	cinfo.scale_denom = scale_denom;			// and hw needs denom great than num
//	cinfo.do_fancy_upsampling = FALSE;			// don't care, hw not support
//	cinfo.do_block_smoothing = TRUE;			// don't care, hw not support
//	cinfo.out_color_space = JCS_RGB;			// FIX ME: RGB565 packet or YCbCr422 packet

	jpeg_start_decompress( &cinfo );

	printf( "%d, %d, %d\n", cinfo.output_width, cinfo.output_height, cinfo.output_components );

	JSAMPARRAY buffer;
	int row_stride = cinfo.output_width * cinfo.output_components;
	buffer = (*cinfo.mem->alloc_sarray)( (j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1 );

	unsigned char * frame_buffer;
	unsigned char * _frame_buffer;
	frame_buffer = (unsigned char *)malloc( cinfo.output_width * cinfo.output_height * cinfo.output_components );
	_frame_buffer = frame_buffer;

	while ( cinfo.output_scanline < cinfo.output_height )
	{
		jpeg_read_scanlines( &cinfo, buffer, 1 );
		memcpy( _frame_buffer, buffer[0], row_stride );
		_frame_buffer += row_stride;
	}

	jpeg_finish_decompress( &cinfo );

	jpeg_destroy_decompress( &cinfo );

#if 1	// FIX ME: file as source
	fclose( infile );
#else	// FIX ME: memory as source
	free( src1 );
#endif

#if 1	// FIX ME: write decoded result to t.bin, it will take times in writing
	FILE * outfile;
	if ( (outfile = fopen( "t.bin", "wb" )) == NULL )
	{
		fprintf( stderr, "can't open %s\n", jpeg_filename );
		exit( 1 );
	}
	fwrite( frame_buffer, 1, (cinfo.output_width * cinfo.output_height * cinfo.output_components), outfile );
	fclose( outfile );
#endif

	int i, display_width, display_height, display_offset;
	ioctl( g_sDemo_Lcm.lcm_fd, IOCTL_WAIT_VSYNC );
	ioctl( g_sDemo_Lcm.lcm_fd, IOCTL_FB_LOCK );
	g_sDemo_Lcm.pLCMBuffer2 = g_sDemo_Lcm.pLCMBuffer;

#if 1	// FIX ME: before rendering, cleaning up the last off-line frame buffer, it will take times in cleaning
	memset( g_sDemo_Lcm.pLCMBuffer2, 0, (g_sDemo_Lcm.fb_finfo.line_length * g_sDemo_Lcm.fb_vinfo.yres) );
#endif

	// hw decode color format default is RGB565 total 16 bits per pixel
	if ( cinfo.output_components == 2 )
	{
		if ( g_sDemo_Lcm.fb_vinfo.bits_per_pixel == 32 )
		{
			unsigned short * pu16Temp;
			unsigned char u8R, u8G, u8B;
			pu16Temp = (unsigned short *)frame_buffer;
			_frame_buffer = (unsigned char *)malloc( cinfo.output_width * cinfo.output_height * g_sDemo_Lcm.u8bytes_per_pixel );
			for ( i = 0; i < cinfo.output_width * cinfo.output_height; ++i )
			{
				u8R = ((pu16Temp[i] & 0xF800) >> 11) << 3;
				u8G = ((pu16Temp[i] & 0x07E0) >> 5) << 2;
				u8B = (pu16Temp[i] & 0x001F) << 3;
				_frame_buffer[i*4] = u8B;
				_frame_buffer[i*4 + 1] = u8G;
				_frame_buffer[i*4 + 2] = u8R;
				_frame_buffer[i*4 + 3] = 0x00;
			}
			free( (unsigned char *)frame_buffer );
			frame_buffer = (unsigned char *)malloc( cinfo.output_width * cinfo.output_height * g_sDemo_Lcm.u8bytes_per_pixel );
			memcpy( frame_buffer, _frame_buffer, (cinfo.output_width * cinfo.output_height * g_sDemo_Lcm.u8bytes_per_pixel) );
			free( (unsigned char *)_frame_buffer );
		}
	}
	else if ( cinfo.output_components == 3 )
	{
		if ( g_sDemo_Lcm.fb_vinfo.bits_per_pixel == 16 )
		{
			for ( i = 0; i < (cinfo.output_width * cinfo.output_height); ++i )
			{
				unsigned short r = frame_buffer[i*3];
				unsigned short g = frame_buffer[i*3 + 1];
				unsigned short b = frame_buffer[i*3 + 2];
				unsigned short rgb = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
				frame_buffer[i*2] = rgb & 0x00FF;
				frame_buffer[i*2+1] = (rgb & 0xFF00) >> 8;
			}
		}
		else
		{
			_frame_buffer = (unsigned char *)malloc( cinfo.output_width * cinfo.output_height * g_sDemo_Lcm.u8bytes_per_pixel );
			for ( i = 0; i < (cinfo.output_width * cinfo.output_height); ++i )
			{
				unsigned short r = frame_buffer[i*3];
				unsigned short g = frame_buffer[i*3 + 1];
				unsigned short b = frame_buffer[i*3 + 2];
				_frame_buffer[i*4] = b;
				_frame_buffer[i*4+1] = g;
				_frame_buffer[i*4+2] = r;
				_frame_buffer[i*4+3] = 0x00;
			}
			free( (unsigned char *)frame_buffer );
			frame_buffer = (unsigned char *)malloc( cinfo.output_width * cinfo.output_height * g_sDemo_Lcm.u8bytes_per_pixel );
			memcpy( frame_buffer, _frame_buffer, (cinfo.output_width * cinfo.output_height * g_sDemo_Lcm.u8bytes_per_pixel) );
			free( (unsigned char *)_frame_buffer );
		}

	}
	else if ( cinfo.output_components == 4 )
	{
		if ( g_sDemo_Lcm.fb_vinfo.bits_per_pixel == 16 )
		{
			for ( i = 0; i < (cinfo.output_width * cinfo.output_height); ++i )
			{
				unsigned short b = frame_buffer[i*4];
				unsigned short g = frame_buffer[i*4 + 1];
				unsigned short r = frame_buffer[i*4 + 2];
				unsigned short a = frame_buffer[i*4 + 3];
				unsigned short rgb = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
				frame_buffer[i*2] = rgb & 0x00FF;
				frame_buffer[i*2+1] = (rgb & 0xFF00) >> 8;
			}
		}
	}
	else if ( cinfo.output_components == 1 )
	{
		_frame_buffer = (unsigned char *)malloc( cinfo.output_width * cinfo.output_height );
		memcpy( _frame_buffer, frame_buffer, (cinfo.output_width * cinfo.output_height) );
		free( (unsigned char *)frame_buffer );
		frame_buffer = (unsigned char *)malloc( cinfo.output_width * cinfo.output_height * g_sDemo_Lcm.u8bytes_per_pixel );
		for ( i = 0; i < (cinfo.output_width * cinfo.output_height); ++i )
		{
			unsigned short y = _frame_buffer[i];
			if ( g_sDemo_Lcm.fb_vinfo.bits_per_pixel == 16 )
			{
				unsigned short rgb = ((y >> 3) << 11) | ((y >> 2) << 5) | (y >> 3);
				frame_buffer[i*2] = rgb & 0x00FF;
				frame_buffer[i*2+1] = (rgb & 0xFF00) >> 8;
			}
			else
				frame_buffer[i*4] = frame_buffer[i*4+1] = frame_buffer[i*4+2] = frame_buffer[i*4+3] = y;
		}
		free( (unsigned char *)_frame_buffer );
	}
	_frame_buffer = frame_buffer;

	if ( cinfo.output_width > g_sDemo_Lcm.fb_vinfo.xres )
	{
		display_offset = (cinfo.output_width - g_sDemo_Lcm.fb_vinfo.xres) * g_sDemo_Lcm.u8bytes_per_pixel;
		display_width = g_sDemo_Lcm.fb_finfo.line_length;
	}
	else
	{
		display_offset = 0;
		display_width = cinfo.output_width * g_sDemo_Lcm.u8bytes_per_pixel;
	}

	if ( cinfo.output_height > g_sDemo_Lcm.fb_vinfo.yres )
		display_height = g_sDemo_Lcm.fb_vinfo.yres;
	else
		display_height = cinfo.output_height;

	/* FIX ME: for non-scaling down case */
	if ( (cinfo.output_width == g_sDemo_Lcm.fb_vinfo.xres) && (cinfo.output_height == g_sDemo_Lcm.fb_vinfo.yres) )
	{
		memcpy( g_sDemo_Lcm.pLCMBuffer2, _frame_buffer, (g_sDemo_Lcm.fb_vinfo.xres * g_sDemo_Lcm.fb_vinfo.yres * g_sDemo_Lcm.u8bytes_per_pixel) );
	}
	else
	{
		/* FIX ME: for other render case, it will take times in rendering */
		for ( i = 0; i < display_height; ++i )
		{
			memcpy( g_sDemo_Lcm.pLCMBuffer2, _frame_buffer, display_width );
			g_sDemo_Lcm.pLCMBuffer2 += (g_sDemo_Lcm.fb_vinfo.xres * g_sDemo_Lcm.u8bytes_per_pixel);
			_frame_buffer += (display_width + display_offset);
		}
	}
	ioctl( g_sDemo_Lcm.lcm_fd, IOCTL_FB_UNLOCK );

	free( (unsigned char *)frame_buffer );
}

void compress( char * jpeg_filename, int quality, char * raw_filename, int image_width, int image_height )
{
	int i;

#if 1
	unsigned char * image_buffer;
	image_buffer = (unsigned char *)malloc( image_width * image_height * 2 );	// hw is 1 pixel 2 bytes
#else
	unsigned char * image_buffer[image_height];
	for ( i = 0; i < image_height; ++i )
		image_buffer[i] = (unsigned char *)malloc( image_width * 2 );			// hw is 1 pixel 2 bytes
#endif

	FILE *fd;
	fd = fopen( raw_filename, "rb" );
	if ( fd == NULL )
	{
		printf( "open %s error!\n", raw_filename );
		exit( 1 );
	}

#if 1
	fread( image_buffer, 1, (image_width * image_height * 2), fd );		// hw is 1 pixel 2 bytes
#else
	for ( i = 0; i < image_height; ++i )
		fread( image_buffer[i], 1, (image_width * 2), fd );				// hw is 1 pixel 2 bytes
#endif

	fclose( fd );

	printf( "\n**** HW JPEGCodec Test Program ****\n" );
	printf( "1. Compress %s to %s, quality = %d\n", raw_filename, jpeg_filename, quality );

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error( &jerr );
//	cinfo.err->trace_level = 1;			// FIXME: un-mark for printing H/W JPEG CODEC message
	jerr.error_exit = my_error_exit;
	jpeg_create_compress( &cinfo );

	FILE * outfile;
	if ( (outfile = fopen( jpeg_filename, "wb" )) == NULL )
	{
		fprintf( stderr, "can't open %s\n", jpeg_filename );
		exit( 1 );
	}
#if 1	// FIX ME: file as destination
	jpeg_stdio_dest( &cinfo, outfile );
#else	// FIXE ME: memory as destination
	unsigned char * outbuffer;
	unsigned long outsize;
	outsize = 200 * 1024;
	outbuffer = malloc( outsize );
	printf( "$$$ %d, 0x%x $$$\n", outsize, outbuffer );
	jpeg_mem_dest( &cinfo, &outbuffer, &outsize );
#endif

	cinfo.image_width = image_width;
	cinfo.image_height = image_height;
	cinfo.input_components = 3;			// hw don't care, because hw just support 2 bytes per pixel
	cinfo.in_color_space = JCS_YCbCr;	// hw just support input YCbYCr

	jpeg_set_defaults( &cinfo );

	jpeg_set_quality( &cinfo, quality, TRUE );
//	jpeg_set_colorspace( &cinfo, JCS_GRAYSCALE );	// hw not support
//	cinfo.comp_info->h_samp_factor = 2;
//	cinfo.comp_info->v_samp_factor = 1;
	cinfo.dct_method = JDCT_IFAST;					// hw not support
//	cinfo.scale_num = 8;							// hw just support encode scaling up
//	cinfo.scale_denom = 4;							// and hw needs denom less than num
//	cinfo.smoothing_factor = 100;					// hw not support
//	cinfo.do_fancy_downsampling = TRUE;				// hw not support

	jpeg_start_compress( &cinfo, TRUE );

#if 1
	JSAMPROW row_pointer[1];
	int row_stride = image_width * 2;	// hw is 1 pixel 2 bytes

	while ( cinfo.next_scanline < cinfo.image_height )
	{
		row_pointer[0] = &image_buffer[cinfo.next_scanline * row_stride];
		jpeg_write_scanlines( &cinfo, row_pointer, 1 );
	}
#else
	jpeg_write_scanlines( &cinfo, image_buffer, cinfo.image_height );
#endif

	jpeg_finish_compress( &cinfo );

#if 0	// FIX ME: memory as destination
	fwrite( outbuffer, 1, outsize, outfile );
#endif

	fclose( outfile );

#if 1
	free( (unsigned char *)image_buffer );
#else
	for ( i = 0; i < image_height; ++i )
		free( (unsigned char *)image_buffer[i] );
#endif

#if 0	// FIX ME: memory as destination
	free( outbuffer );
#endif

	jpeg_destroy_compress( &cinfo );
}

int main( int argc, char *argv[] )
{
	if ( !InitLCM() )
		exit( 1 );

	if ( !_DemoLCM_SetLCMBuffer() )
		exit( 1 );

	switch ( argc )
	{
	case 3:
		// argv[1]: input JPEG filename, like "my_jpeg.jpg" or others
		decompress( argv[1], atoi( argv[2] ) );
		break;
	case 6:
		// argv[1]: output JPEG filename, like "my_jpeg.jpg" or others
		// argv[2]: compress quality, like "75", 0~100, worst to best
		// argv[3]: input raw filename, like "for_hw_yuv422_480x272.bin" or others
		// argv[4]: input raw width, like "480" or others
		// argv[5]: input raw height, like "272" or others
		compress( argv[1], atoi( argv[2] ), argv[3], atoi( argv[4] ), atoi( argv[5] ) );
		decompress( argv[1], 0 );
		break;
	default:
		printf( "\nNote: There are two functions \"Encode then Decode\" and \"Decode Only\":\n" );
		printf( "\n1. Encode then Decode:\n" );
		printf( "Usage:\t%s arg1 arg2 arg3 arg4 arg5\n", argv[0] );
		printf( "Options:\n" );
		printf( "argv1:\toutput JPEG filename, like \"my_jpeg.jpg\" or others\n" );
		printf( "argv2:\tcompress quality, like \"75\", 0 (worst) ~ 100 (best)\n" );
		printf( "argv3:\tinput raw filename, like \"for_hw_yuv422_480x272.bin\" or others\n" );
		printf( "argv4:\tinput raw width, like \"480\" or others\n" );
		printf( "argv5:\tinput raw height, like \"272\" or others\n\n\n" );
		printf( "2. Decode Only:\n" );
		printf( "Usage:\t%s arg1 arg2\n", argv[0] );
		printf( "Option:\n" );
		printf( "argv1:\tinput JPEG filename, like \"my_jpeg.jpg\" or others\n" );
		printf( "argv2:\tenable scale down or not, like \"1\": enable, \"0\": disable\n\n" );
		break;
	}

	return 0;
}

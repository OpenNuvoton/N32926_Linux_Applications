/* Misc.h
 *
 *
 * Copyright (c)2008 Nuvoton technology corporation
 * http://www.nuvoton.com
 *
 * Misc function
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef __MISC_H__
#define __MISC_H__

#define DEBUG_PRINT(x, ...) fprintf(stdout, x, ##__VA_ARGS__)
//#define DEBUG_PRINT(x, ...)

#define ERR_PRINT(x, ...) fprintf(stderr, x, ##__VA_ARGS__)

#ifndef BOOL
#define BOOL int32_t
#define FALSE 0
#define TRUE 1
#endif

#define RETRY_NUM 4


#define clamp(val, min, max) ({                 \
        typeof(val) __val = (val);              \
        typeof(min) __min = (min);              \
        typeof(max) __max = (max);              \
        (void) (&__val == &__min);              \
        (void) (&__val == &__max);              \
        __val = __val < __min ? __min: __val;   \
        __val > __max ? __max: __val; })

#define ARRAY_SIZE(a)	((sizeof(a) / sizeof(a[0])))

#define VIDIOC_S_MEMORY_POOL_PADDR		_IOW('V', 92, int)
#define VIDIOC_S_USER_MEMORY_POOL_ENABLE	_IOW('V', 93, int)
#define VIDIOC_S_USER_MEMORY_POOL_NUMBER	_IOW('V', 94, int)
#define VIDIOC_S_USER_MEMORY_POOL_OFFSET	_IOW('V', 95, int)

#define UVC_BUFFER_NUM 2
#define SKIP_FRAME_NUM 10

#define MAX_IMAGE_SIZE (1280 * 720 * 2)

//#define __MMU_MODE__

typedef enum
{
	eIMAGE_QVGA,
	eIMAGE_WQVGA,
	eIMAGE_VGA,
	eIMAGE_SVGA,
	eIMAGE_HD720
}E_IMAGE_RESOL;

#endif


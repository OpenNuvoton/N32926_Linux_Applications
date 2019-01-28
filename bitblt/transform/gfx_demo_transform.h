/****************************************************************
 *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved.
 *
 ****************************************************************/
 
#ifndef __GFX_DEMO_TRANSFORM_H__
#define __GFX_DEMO_TRANSFORM_H__

#ifdef	__cplusplus
extern "C"
{
#endif

#include <linux/fb.h>
#include "gfx.h"

void gfx_demo_scale(float ox, float oy, S_DRVBLT_RECT *bbox);
void gfx_demo_rotate(float ox, float oy, S_DRVBLT_RECT *bbox);
void gfx_demo_fadein(float ox, float oy, S_DRVBLT_RECT *bbox);
void gfx_demo_fadeout(float ox, float oy, S_DRVBLT_RECT *bbox);
void gfx_demo_flip(float ox, float oy, S_DRVBLT_RECT *bbox);
void gfx_demo_multilayer(S_DRVBLT_RECT *bbox);

#ifdef	__cplusplus
}
#endif

#endif	// __GFX_DEMO_TRANSFORM_H__


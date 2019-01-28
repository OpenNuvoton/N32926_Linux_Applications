/****************************************************************
 *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved.
 *
 ****************************************************************/
 
#ifndef __GFX_DEMO_SPRITE_H__
#define __GFX_DEMO_SPRITE_H__

#ifdef	__cplusplus
extern "C"
{
#endif

#include <linux/fb.h>
#include "gfx.h"

struct GfxDemoCtx {
    void *                      pat_bg_va;
    void *                      pat_sp_va;
};

void gfx_demo_setup(void);
void gfx_demo_shutdown(void);
void gfx_demo_sprite(float ox, float oy, int colkey_en);

#ifdef	__cplusplus
}
#endif

#endif	// __GFX_DEMO_SPRITE_H__


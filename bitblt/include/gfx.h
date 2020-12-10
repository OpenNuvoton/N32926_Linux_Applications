/****************************************************************
 *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved.
 *
 ****************************************************************/
 
#ifndef __GFX_H__
#define __GFX_H__

#ifdef	__cplusplus
extern "C"
{
#endif

#include <linux/fb.h>
#include "w55fa92_blt.h"

struct GfxCtx {
    int                         fbfd;
    struct fb_var_screeninfo    var;
    struct fb_fix_screeninfo    fix;
    void *                      fb_va;
    
    int                             bltfd;
    int                             mmu;
    S_DRVBLT_BLIT_OP	            blitop;
    S_DRVBLT_BLIT_TRANSFORMATION	tfm;
    S_DRVBLT_FILL_OP				fillop;
    int                             srcfmt_premulalpha;
    
    /* Valid if mmu off*/
    int                         devmemfd;
    unsigned long               srcbuf_size;
    unsigned long               srcbuf_pa;
    void *                      srcbuf_va;
    unsigned long               srcbuf_width;
    unsigned long               srcbuf_height;
    unsigned long               srcbuf_stride;
    
    struct timeval              tv_lastdraw;
};

const struct GfxCtx *gfx_ctx(void);
void gfx_setup(int mmu);
void gfx_shutdown(void);
void gfx_begindraw(void);
void gfx_enddraw(void);
void gfx_prepsrcpat(void *pat, unsigned long w, unsigned long h, unsigned long s, E_DRVBLT_BMPIXEL_FORMAT pixfmt, int alpha);
void gfx_clrsrcpat(void);
void gfx_clear(S_DRVBLT_ARGB8 *color, S_DRVBLT_RECT *bbox);
void gfx_identity(float ox, float oy, S_DRVBLT_RECT *bbox);
void gfx_scale(float ox, float oy, float sx, float sy, S_DRVBLT_RECT *bbox);
void gfx_rotate(float ox, float oy, float deg, S_DRVBLT_RECT *bbox);
void gfx_alpha(float ox, float oy, float alpha, S_DRVBLT_RECT *bbox);
void gfx_horizflip(float ox, float oy, S_DRVBLT_RECT *bbox);
void gfx_vertflip(float ox, float oy, S_DRVBLT_RECT *bbox);
void gfx_flush(void);
void gfx_waitintvl(uint32_t ms);
void gfx_set_tilemode(int tilemode);
void gfx_set_smooth(int smooth);
void gfx_set_colkey(int en, uint16_t colkey);
void gfx_rect_clamp(const S_DRVBLT_RECT *rect_src, S_DRVBLT_RECT *rect_dst);

void rect_intersect(const S_DRVBLT_RECT *rect_src1, const S_DRVBLT_RECT *rect_src2, S_DRVBLT_RECT *rect_dst);
void rect_union(const S_DRVBLT_RECT *rect_src1, const S_DRVBLT_RECT *rect_src2, S_DRVBLT_RECT *rect_dst);

#ifdef	__cplusplus
}
#endif

#endif	// __GFX_H__


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <errno.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include "gfx_demo_transform.h"

#define SRCPAT_WIDTH       80
#define SRCPAT_HEIGHT      60
#define SRCPAT_STRIDE       (SRCPAT_WIDTH * 4)
uint8_t srcpat[] = {
#include "pat_argb8888_80x60.txt"
};

extern int myapp_isterm(void);

void gfx_demo_scale(float ox, float oy, S_DRVBLT_RECT *bbox)
{
     if (myapp_isterm()) {
        return;
    }
    
    struct scale_2d_ {
        float sx;
        float sy;
    };
    struct scale_2d_ scale_arr[] = {
        {0.5f, 1.0f}, {1.0f, 1.0f}, {2.0f, 1.0f},
        {1.0f, 0.5f}, {1.0f, 1.0f}, {1.0f, 2.0f},
        {0.5f, 0.5f}, {1.0f, 1.0f}, {2.0f, 2.0f},
    };
    struct scale_2d_ *scale_ind = scale_arr;
    struct scale_2d_ *scale_end = scale_arr + sizeof (scale_arr) / sizeof (scale_arr[0]);
    S_DRVBLT_ARGB8 bgcolor = {255, 255, 255, 0};
    S_DRVBLT_RECT bbox_fs = {0, gfx_ctx()->var.xres, 0, gfx_ctx()->var.yres};
        
    /* Clear full screen */
    gfx_begindraw();        // Lock frame buffer.
    gfx_clear(&bgcolor, &bbox_fs);
    gfx_flush();
    gfx_enddraw();          // Unlock frame buffer.
        
    gfx_prepsrcpat(srcpat, SRCPAT_WIDTH, SRCPAT_HEIGHT, SRCPAT_STRIDE, eDRVBLT_SRC_ARGB8888, 0);
    for (; scale_ind < scale_end && ! myapp_isterm(); scale_ind ++) {
        /* Draw rotated result within a bounding box. */
        gfx_begindraw();            // Lock frame buffer.
        gfx_clear(&bgcolor, bbox);  // Clear update region.
        gfx_flush();
        gfx_scale(ox, oy, scale_ind->sx, scale_ind->sy, bbox);
        gfx_flush();
        gfx_enddraw();              // Unlock frame buffer.
        gfx_waitintvl(500);         // Wait for at most 500 ms before next draw.
    }
}

void gfx_demo_rotate(float ox, float oy, S_DRVBLT_RECT *bbox)
{
     if (myapp_isterm()) {
        return;
    }
    
    float deg = 0.0f;
    S_DRVBLT_ARGB8 bgcolor = {255, 255, 255, 0};
    S_DRVBLT_RECT bbox_fs = {0, gfx_ctx()->var.xres, 0, gfx_ctx()->var.yres};
        
    /* Clear full screen */
    gfx_begindraw();        // Lock frame buffer.
    gfx_clear(&bgcolor, &bbox_fs);
    gfx_flush();
    gfx_enddraw();          // Unlock frame buffer.
        
    gfx_prepsrcpat(srcpat, SRCPAT_WIDTH, SRCPAT_HEIGHT, SRCPAT_STRIDE, eDRVBLT_SRC_ARGB8888, 0);
    while (! myapp_isterm()) {
        /* Draw rotated result within a bounding box. */
        gfx_begindraw();            // Lock frame buffer.
        gfx_clear(&bgcolor, bbox);  // Clear update region.
        gfx_flush();
        gfx_rotate(ox, oy, deg, bbox);
        gfx_flush();
        gfx_enddraw();              // Unlock frame buffer.
        gfx_waitintvl(500);         // Wait for at most 500 ms before next draw.

        deg += 20.0f;
        if (deg > 360.0f) {
            break;
        }
    }
}

void gfx_demo_fadein(float ox, float oy, S_DRVBLT_RECT *bbox)
{
     if (myapp_isterm()) {
        return;
    }
    
    float alpha = 0.0f;
    S_DRVBLT_ARGB8 bgcolor = {255, 255, 255, 0};
    S_DRVBLT_RECT bbox_fs = {0, gfx_ctx()->var.xres, 0, gfx_ctx()->var.yres};
    
    /* Clear full screen */
    gfx_begindraw();        // Lock frame buffer.
    gfx_clear(&bgcolor, &bbox_fs);
    gfx_flush();
    gfx_enddraw();          // Unlock frame buffer.
        
    gfx_prepsrcpat(srcpat, SRCPAT_WIDTH, SRCPAT_HEIGHT, SRCPAT_STRIDE, eDRVBLT_SRC_ARGB8888, 0);
    while (! myapp_isterm()) {
        /* Draw rotated result within a bounding box.*/
        gfx_begindraw();            // Lock frame buffer.
        gfx_clear(&bgcolor, bbox);  // Clear update region.
        gfx_flush();
        gfx_alpha(ox, oy, alpha, bbox);
        gfx_flush();
        gfx_enddraw();              // Unlock frame buffer.
        gfx_waitintvl(500);         // Wait for at most 500 ms before next draw.
            
        alpha += 0.1f;
        if (alpha > 1.1f) { // Rounding error.
            break;
        }
    }
}

void gfx_demo_fadeout(float ox, float oy, S_DRVBLT_RECT *bbox)
{
    if (myapp_isterm()) {
        return;
    }
    
    float alpha = 1.0f;
    S_DRVBLT_ARGB8 bgcolor = {255, 255, 255, 0};
    S_DRVBLT_RECT bbox_fs = {0, gfx_ctx()->var.xres, 0, gfx_ctx()->var.yres};
    
    /* Clear full screen */    
    gfx_begindraw();        // Lock frame buffer.
    gfx_clear(&bgcolor, &bbox_fs);
    gfx_flush();
    gfx_enddraw();          // Unlock frame buffer.
        
    gfx_prepsrcpat(srcpat, SRCPAT_WIDTH, SRCPAT_HEIGHT, SRCPAT_STRIDE, eDRVBLT_SRC_ARGB8888, 0);
    while (! myapp_isterm()) {
        /* Draw rotated result within a bounding box.*/
        gfx_begindraw();            // Lock frame buffer.
        gfx_clear(&bgcolor, bbox);  // Clear update region.
        gfx_flush();
        gfx_alpha(ox, oy, alpha, bbox);
        gfx_flush();
        gfx_enddraw();              // Unlock frame buffer.
        gfx_waitintvl(500);         // Wait for at most 500 ms before next draw.
            
        alpha -= 0.1f;
        if (alpha < -0.1f) {    // Rounding error.
            break;
        }
    }
}

void gfx_demo_flip(float ox, float oy, S_DRVBLT_RECT *bbox)
{
    if (myapp_isterm()) {
        return;
    }
    
    S_DRVBLT_ARGB8 bgcolor = {255, 255, 255, 0};
    S_DRVBLT_RECT bbox_fs = {0, gfx_ctx()->var.xres, 0, gfx_ctx()->var.yres};
    unsigned cd = 3;
        
    /* Clear full screen */
    gfx_begindraw();        // Lock frame buffer.
    gfx_clear(&bgcolor, &bbox_fs);
    gfx_flush();
    gfx_enddraw();          // Unlock frame buffer.
        
    gfx_prepsrcpat(srcpat, SRCPAT_WIDTH, SRCPAT_HEIGHT, SRCPAT_STRIDE, eDRVBLT_SRC_ARGB8888, 0);
    while (! myapp_isterm() && cd) {
        /* Draw no-transformed result within a bounding box. */
        gfx_begindraw();            // Lock frame buffer.
        gfx_clear(&bgcolor, bbox);  // Clear update region.
        gfx_flush();
        gfx_identity(ox, oy, bbox);
        gfx_flush();
        gfx_enddraw();              // Unlock frame buffer.
        gfx_waitintvl(500);         // Wait for at most 500 ms before next draw.
            
        /* Draw horizontal flip result within a bounding box.*/
        gfx_begindraw();            // Lock frame buffer.
        gfx_clear(&bgcolor, bbox);  // Clear update region.
        gfx_flush();        
        gfx_horizflip(ox, oy, bbox);
        gfx_flush();
        gfx_enddraw();              // Unlock frame buffer.
        gfx_waitintvl(500);         // Wait for at most 500 ms before next draw.
        
        /* Draw no-transformed result within a bounding box. */
        gfx_begindraw();            // Lock frame buffer.
        gfx_clear(&bgcolor, bbox);  // Clear update region.
        gfx_flush();
        gfx_identity(ox, oy, bbox);
        gfx_flush();
        gfx_enddraw();              // Unlock frame buffer.
        gfx_waitintvl(500);         // Wait for at most 500 ms before next draw.
            
        /* Draw vertical flip result within a bounding box.*/
        gfx_begindraw();            // Lock frame buffer.
        gfx_clear(&bgcolor, bbox);  // Clear update region.
        gfx_flush();        
        gfx_vertflip(ox, oy, bbox);
        gfx_flush();
        gfx_enddraw();              // Unlock frame buffer.
        gfx_waitintvl(500);         // Wait for at most 500 ms before next draw.
        
        cd --;
    }
}

void gfx_demo_multilayer(S_DRVBLT_RECT *bbox)
{
    if (myapp_isterm()) {
        return;
    }
    
    S_DRVBLT_ARGB8 bgcolor = {255, 255, 255, 0};
    S_DRVBLT_RECT bbox_fs = {0, gfx_ctx()->var.xres, 0, gfx_ctx()->var.yres};
    unsigned cd = 3;
        
    /* Clear full screen */
    gfx_begindraw();        // Lock frame buffer.
    gfx_clear(&bgcolor, &bbox_fs);
    gfx_flush();
    gfx_enddraw();          // Unlock frame buffer.
    
    gfx_begindraw();        // Lock frame buffer.
    
    /* Draw 1st layer */
    {
        /* Specify source pattern */
        void *pat = srcpat;
        short pat_width = SRCPAT_WIDTH;
        short pat_height = SRCPAT_HEIGHT;
        short pat_stride = SRCPAT_STRIDE;
        /* Specify where to draw in destination CS. */
        short ox = SRCPAT_WIDTH;
        short oy = SRCPAT_HEIGHT;
        S_DRVBLT_RECT pat_bbox = {ox, ox + pat_width, oy, oy + pat_height};
        S_DRVBLT_RECT pat_bbox_clamp;
        
        rect_intersect(&pat_bbox, bbox, &pat_bbox_clamp);                                   // Make draw range limited by passed-in bbox, which cannot exceed dims of FB.
        gfx_prepsrcpat(pat, pat_width, pat_height, pat_stride, eDRVBLT_SRC_ARGB8888, 0);    // Update source pattern.
        gfx_identity(ox, oy, &pat_bbox_clamp);                                              // Draw on specified position with draw range limited by bounding box.
        gfx_flush();
    }
    
    /* Draw 2nd layer */
    {
        /* Specify source pattern */
        void *pat = srcpat;
        short pat_width = SRCPAT_WIDTH;
        short pat_height = SRCPAT_HEIGHT;
        short pat_stride = SRCPAT_STRIDE;
        /* Specify where to draw in destination CS. */
        short ox = SRCPAT_WIDTH * 3 / 2;
        short oy = SRCPAT_HEIGHT * 3 / 2;
        S_DRVBLT_RECT pat_bbox = {ox, ox + pat_width, oy, oy + pat_height};
        S_DRVBLT_RECT pat_bbox_clamp;
        
        rect_intersect(&pat_bbox, bbox, &pat_bbox_clamp);                                   // Make draw range limited by passed-in bbox, which cannot exceed dims of FB.
        gfx_prepsrcpat(pat, pat_width, pat_height, pat_stride, eDRVBLT_SRC_ARGB8888, 0);    // Update source pattern.
        gfx_identity(ox, oy, &pat_bbox_clamp);                                              // Draw on specified position with draw range limited by bounding box.
        gfx_flush();
    }
    
    /* Draw 3rd layer */
    {
        /* Specify source pattern */
        void *pat = srcpat;
        short pat_width = SRCPAT_WIDTH;
        short pat_height = SRCPAT_HEIGHT;
        short pat_stride = SRCPAT_STRIDE;
        /* Specify where to draw in destination CS. */
        short ox = SRCPAT_WIDTH * 2;
        short oy = SRCPAT_HEIGHT * 2;
        S_DRVBLT_RECT pat_bbox = {ox, ox + pat_width, oy, oy + pat_height};
        S_DRVBLT_RECT pat_bbox_clamp;
        
        rect_intersect(&pat_bbox, bbox, &pat_bbox_clamp);                                   // Make draw range limited by passed-in bbox, which cannot exceed dims of FB.
        gfx_prepsrcpat(pat, pat_width, pat_height, pat_stride, eDRVBLT_SRC_ARGB8888, 0);    // Update source pattern.
        gfx_identity(ox, oy, &pat_bbox_clamp);                                              // Draw on specified position with draw range limited by bounding box.
        gfx_flush();
    }
   
    /* End of draw multi-layer */
    gfx_enddraw();              // Unlock frame buffer.
    gfx_waitintvl(300);         // Wait for at most 500 ms before next draw.
        
}

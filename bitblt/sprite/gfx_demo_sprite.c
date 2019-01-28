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

#include "gfx_demo_sprite.h"

#define PAT_BG_FN       "BG.bin"
#define PAT_BG_DPP      16
#define PAT_BG_WIDTH    480
#define PAT_BG_HEIGHT   272
#define PAT_BG_STRIDE   (PAT_BG_WIDTH * PAT_BG_DPP / 8)
#define PAT_BG_SIZE     (PAT_BG_STRIDE * PAT_BG_HEIGHT)

#define PAT_SP_FN       "SP.bin"
#define PAT_SP_DPP      16
#define PAT_SP_WIDTH    736
#define PAT_SP_HEIGHT   160
#define PAT_SP_STRIDE   (PAT_SP_WIDTH * PAT_SP_DPP / 8)
#define PAT_SP_SIZE     (PAT_SP_STRIDE * PAT_SP_HEIGHT)

#define COLORKEY        0xF81F

struct GfxDemoCtx gfxdemoctx;

static void _loadpattern(void);
static void _unloadpattern(void);

const struct GfxDemoCtx *gfx_demo_ctx(void)
{
    return &gfxdemoctx;
}

extern int myapp_isterm(void);

void gfx_demo_setup(void)
{
    gfxdemoctx.pat_bg_va = NULL;
    gfxdemoctx.pat_sp_va = NULL;
    _loadpattern();
}

void gfx_demo_shutdown(void)
{
    _unloadpattern();
}

void _loadpattern(void)
{
    int pat_bg_fd = -1;
    int pat_sp_fd = -1;
    
    _unloadpattern();
    
    pat_bg_fd = open(PAT_BG_FN, O_RDONLY);
    if (pat_bg_fd < 0) {
        printf("Open %s failed: %s\n", PAT_BG_FN, strerror(errno));
        exit(EXIT_FAILURE);
    }
    gfxdemoctx.pat_bg_va = mmap(NULL, PAT_BG_SIZE, PROT_READ, MAP_SHARED, pat_bg_fd, 0);
    if (gfxdemoctx.pat_bg_va == MAP_FAILED) {
        printf("mmap() %s failed: %s\n", PAT_BG_FN, strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    pat_sp_fd = open(PAT_SP_FN, O_RDONLY);
    if (pat_sp_fd < 0) {
        printf("Open %s failed: %s\n", PAT_SP_FN, strerror(errno));
        exit(EXIT_FAILURE);
    }
    gfxdemoctx.pat_sp_va = mmap(NULL, PAT_SP_SIZE, PROT_READ, MAP_SHARED, pat_sp_fd, 0);
    if (gfxdemoctx.pat_sp_va == MAP_FAILED) {
        printf("mmap() %s failed: %s\n", PAT_SP_FN, strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if (pat_bg_fd >= 0) {
        close(pat_bg_fd);
        pat_bg_fd = -1;
    }
    if (pat_sp_fd >= 0) {
        close(pat_sp_fd);
        pat_sp_fd = -1;
    }    
}

void _unloadpattern(void)
{
    if (gfxdemoctx.pat_bg_va != NULL && gfxdemoctx.pat_bg_va != MAP_FAILED) {
        munmap(gfxdemoctx.pat_bg_va, PAT_BG_SIZE);
        gfxdemoctx.pat_bg_va = NULL;
    }
    
    if (gfxdemoctx.pat_sp_va != NULL && gfxdemoctx.pat_sp_va != MAP_FAILED) {
        munmap(gfxdemoctx.pat_sp_va, PAT_SP_SIZE);
        gfxdemoctx.pat_sp_va = NULL;
    }
}

void gfx_demo_sprite(float ox, float oy, int colkey_en)
{
    if (myapp_isterm()) {
        return;
    }
    
    S_DRVBLT_ARGB8 bgcolor = {255, 255, 255, 0};
    S_DRVBLT_RECT bbox_fs = {0, gfx_ctx()->var.xres, 0, gfx_ctx()->var.yres};
    unsigned cd = 5;
    
    // Define where to draw BG.
    struct cs_2d {
        float x;
        float y;
    };
    struct cs_2d cs_bg_arr[] = {
        {50.0f, 50.0f},
        {-200.0f, 50.0f},
        {-200.0f, -100.0f},
        {50.0f, -100.0f}
    };
    struct cs_2d *cs_bg_ind = cs_bg_arr;
    struct cs_2d *cs_bg_end = cs_bg_arr + sizeof (cs_bg_arr) / sizeof (cs_bg_arr[0]);
    
    // The source pattern consists of 7 sprites. Define positions/bounding boxes of these sprites here.
    S_DRVBLT_RECT bbox_sprite_arr[] = {
        {ox, ox + 96, oy, oy + 160},
        {ox, ox + 96, oy, oy + 160},
        {ox, ox + 112, oy, oy + 160},
        {ox, ox + 112, oy, oy + 160},
        {ox, ox + 112, oy, oy + 160},
        {ox, ox + 112, oy, oy + 160},
        {ox, ox + 96, oy, oy + 160}
    };
    S_DRVBLT_RECT *bbox_sprite_ind = bbox_sprite_arr;
    S_DRVBLT_RECT *bbox_sprite_end = bbox_sprite_arr + sizeof (bbox_sprite_arr) / sizeof (bbox_sprite_arr[0]);
    float ox_sprite_ind = bbox_sprite_arr[0].i16Xmin;
    
    while (! myapp_isterm() && cd) {
        gfx_begindraw();            // Lock frame buffer.
            
        gfx_clear(&bgcolor, &bbox_fs);  // Clear full screen.
        gfx_flush();
        
        // Draw BG.
        gfx_prepsrcpat(gfx_demo_ctx()->pat_bg_va, PAT_BG_WIDTH, PAT_BG_HEIGHT, PAT_BG_STRIDE, eDRVBLT_SRC_RGB565, 0);
        gfx_set_tilemode(0);
        gfx_identity(cs_bg_ind->x, cs_bg_ind->y, &bbox_fs);
        gfx_flush();
            
        // Draw sprite.
        gfx_prepsrcpat(gfx_demo_ctx()->pat_sp_va, PAT_SP_WIDTH, PAT_SP_HEIGHT, PAT_SP_STRIDE, eDRVBLT_SRC_RGB565, 0);
        gfx_set_colkey(colkey_en, COLORKEY);
        gfx_set_tilemode(0);
        gfx_identity(ox_sprite_ind, oy, bbox_sprite_ind);
        gfx_flush();
            
        gfx_enddraw();              // Unlock frame buffer.
            
        cs_bg_ind ++;               // Move BG.
        if (cs_bg_ind == cs_bg_end) {
            cs_bg_ind = cs_bg_arr;
            
        }
        
        ox_sprite_ind -= (bbox_sprite_ind->i16Xmax - bbox_sprite_ind->i16Xmin);
        bbox_sprite_ind ++;         // Next sprite.
        if (bbox_sprite_ind == bbox_sprite_end) {
            bbox_sprite_ind = bbox_sprite_arr;
            ox_sprite_ind = bbox_sprite_arr[0].i16Xmin;    
            cd --;
        }
        
        gfx_waitintvl(200);         // Wait for at most 200 ms before next draw.
    }
}

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

#include "gfx.h"

#define IOCTL_LCD_ENABLE_INT        _IO('v', 28)
#define IOCTL_LCD_DISABLE_INT       _IO('v', 29)
#define IOCTL_LCD_GET_DMA_BASE          _IOR('v', 32, unsigned int *)
#define IOCTL_LCD_SET_DMA_BASE          _IOR('v', 40, unsigned int *)
#define IOCTL_LCD_GET_VPOST_BASE        _IOR('v', 41, unsigned int *)
#define IOCTL_LCD_SET_VPOST_BASE        _IOR('v', 42, unsigned int *)
// Support separate lock/unlock of frame buffer and OSD buffer.
#define IOCTL_OSD_LOCK              _IOW('v', 62, unsigned int)
#define IOCTL_OSD_UNLOCK            _IOW('v', 63, unsigned int)
#define IOCTL_FB_LOCK               _IOW('v', 64, unsigned int)
#define IOCTL_FB_UNLOCK             _IOW('v', 65, unsigned int)
#define IOCTL_FB_LOCK_RESET         _IOW('v', 66, unsigned int)
#define IOCTL_WAIT_VSYNC            _IOW('v', 67, unsigned int) 

#define DEVMEM_GET_STATUS   _IOR('m', 4, unsigned int)
#define DEVMEM_SET_STATUS   _IOW('m', 5, unsigned int)
#define DEVMEM_GET_PHYSADDR _IOR('m', 6, unsigned int)
#define DEVMEM_GET_VIRADDR  _IOR('m', 7, unsigned int)

static struct GfxCtx gfxctx;

const struct GfxCtx *gfx_ctx(void)
{
    return &gfxctx;
}

void gfx_setup(int mmu)
{   
    gfxctx.fbfd = -1;
    gfxctx.fb_va = MAP_FAILED;
    gfxctx.bltfd = -1;
    gfxctx.mmu = mmu;
    gfxctx.devmemfd = -1;
    gfxctx.srcbuf_size = 0;
    gfxctx.srcbuf_pa = -1;
    gfxctx.srcbuf_va = MAP_FAILED;
    
    gfxctx.fbfd = open("/dev/fb0", O_RDWR);
    if (gfxctx.fbfd == -1) {
        printf("Open /dev/fb0 failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (ioctl(gfxctx.fbfd, FBIOGET_VSCREENINFO, &gfxctx.var) == -1) {
        printf("ioctl FBIOGET_VSCREENINFO failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("xres=%d, yres=%d, bits_per_pixel=%d\n", gfxctx.var.xres, gfxctx.var.yres, gfxctx.var.bits_per_pixel);

    if (ioctl(gfxctx.fbfd, FBIOGET_FSCREENINFO, &gfxctx.fix) == -1) {
        printf("ioctl FBIOGET_FSCREENINFO failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("smem_start=0x%08x, smem_len=0x%08x, line_length=%d\n", gfxctx.fix.smem_start, gfxctx.fix.smem_len, gfxctx.fix.line_length);
    
    gfxctx.fb_va = mmap(NULL, gfxctx.fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, gfxctx.fbfd, 0);
    if (gfxctx.fb_va == MAP_FAILED) {
        printf("mmap /dev/fb0 failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("frame buffer: va=0x%08x\n", gfxctx.fb_va);
        
    gfxctx.bltfd = open("/dev/blt", O_RDWR);
    if (gfxctx.bltfd == -1) {
        printf("Open /dev/blt failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if (ioctl(gfxctx.bltfd, BLT_IOCTMMU, gfxctx.mmu) == -1) {
        printf("Configure BLT device to MMU mode failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    gfx_set_tilemode(0);
    gfx_set_smooth(0);

    gettimeofday(&gfxctx.tv_lastdraw, NULL);
}

void gfx_shutdown(void)
{
    gfx_clrsrcpat();
    
    close(gfxctx.bltfd);
    gfxctx.bltfd = -1;
    
    if (gfxctx.fb_va != NULL && gfxctx.fb_va != MAP_FAILED) {
        munmap(gfxctx.fb_va, gfxctx.fix.smem_len);
    }
    gfxctx.fb_va = NULL;
    close(gfxctx.fbfd);
    gfxctx.fbfd = -1;
}

void gfx_begindraw(void)
{
    // If separate lock/unlock is not supported, invoke global lock/unlock.
    if (ioctl(gfxctx.fbfd, IOCTL_FB_LOCK)) {
        ioctl(gfxctx.fbfd, IOCTL_LCD_DISABLE_INT);
    }
}

void gfx_enddraw(void)
{
    // If separate lock/unlock is not supported, invoke global lock/unlock.
    if (ioctl(gfxctx.fbfd, IOCTL_FB_UNLOCK)) {
        ioctl(gfxctx.fbfd, IOCTL_LCD_ENABLE_INT);
    }
}

/**
  * @brief              Prepare source pattern.
  * @param[in]  pat     Source pattern.
  * @param[in]  s       Width of source pattern.
  * @param[in]  s       Height of source pattern.
  * @param[in]  s       Stride of source pattern.
  * @param[in]  pixfmt  Pixel format of source pattern.
  * @param[in]  alpha   Transparent pixel format or not.
  */
void gfx_prepsrcpat(void *pat, unsigned long w, unsigned long h, unsigned long s, E_DRVBLT_BMPIXEL_FORMAT pixfmt, int alpha)
{
    gfx_clrsrcpat();
    
    gfxctx.srcbuf_width = w;
    gfxctx.srcbuf_height = h;
    gfxctx.srcbuf_stride = s;
    
    if (gfxctx.mmu) {
        gfxctx.srcbuf_size = gfxctx.srcbuf_stride * gfxctx.srcbuf_height;
        /* Need to be cache line aligned because this buffer can access by both CPU and BLT */
        int err = posix_memalign(&gfxctx.srcbuf_va, 32, gfxctx.srcbuf_size);
        if (err) {
            printf("posix_memalign failed: %s\n", strerror(err));
            exit(EXIT_FAILURE);
        }
        //printf("srcbuf: size=0x%08x, va=0x%08x\n", gfxctx.srcbuf_size, gfxctx.srcbuf_va);
    }
    else {
        gfxctx.devmemfd = open("/dev/devmem", O_RDWR);
        if (gfxctx.devmemfd == -1) {
            printf("Open /dev/devmem failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        gfxctx.srcbuf_size = gfxctx.srcbuf_stride * gfxctx.srcbuf_height;
        gfxctx.srcbuf_va = mmap(NULL, gfxctx.srcbuf_size, PROT_READ | PROT_WRITE, MAP_SHARED, gfxctx.devmemfd, 0);
        if (gfxctx.srcbuf_va == MAP_FAILED) {
            printf("mmap /dev/mem failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (ioctl(gfxctx.devmemfd, DEVMEM_GET_PHYSADDR, &gfxctx.srcbuf_pa) == -1) {
            printf("ioctl DEVMEM_GET_PHYSADDR failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        //printf("srcbuf: size=0x%08x, pa=0x%08x, va=0x%08x\n", gfxctx.srcbuf_size, gfxctx.srcbuf_pa, gfxctx.srcbuf_va);
    }
    
    /* Prepare source pattern. */
    memcpy(gfxctx.srcbuf_va, pat, gfxctx.srcbuf_size);
    
    gfxctx.tfm.srcFormat = pixfmt;
    gfxctx.tfm.flags = alpha ? eDRVBLT_HASTRANSPARENCY : 0;
}

/**
  * @brief              Clear source pattern.
  */
void gfx_clrsrcpat(void)
{
    if (gfxctx.mmu) {
        if (gfxctx.srcbuf_va != NULL && gfxctx.srcbuf_va != MAP_FAILED) {
            free(gfxctx.srcbuf_va);
        }
        gfxctx.srcbuf_va = NULL;
        gfxctx.srcbuf_size = 0;
    }
    else {
        gfxctx.srcbuf_pa = -1;
        if (gfxctx.srcbuf_va != NULL && gfxctx.srcbuf_va != MAP_FAILED) {
            munmap(gfxctx.srcbuf_va, gfxctx.srcbuf_size);
        }
        gfxctx.srcbuf_va = NULL;
        gfxctx.srcbuf_size = 0;
        close(gfxctx.devmemfd);
        gfxctx.devmemfd = -1;
    }
    
    gfxctx.srcbuf_width = 0;
    gfxctx.srcbuf_height = 0;
    gfxctx.srcbuf_stride = 0;
}

/**
  * @brief              Fill bounding box with solid color.
  * @param[in]  color   Fill color.
  * @param[in]  bbox    Bounding box in FB CS.
  */
void gfx_clear(S_DRVBLT_ARGB8 *color, S_DRVBLT_RECT *bbox)
{
    S_DRVBLT_RECT bbox_clamp;
    gfx_rect_clamp(bbox, &bbox_clamp);
    
    gfxctx.fillop.color.u8Blue = color->u8Blue;
    gfxctx.fillop.color.u8Green = color->u8Green;
    gfxctx.fillop.color.u8Red = color->u8Red;
    gfxctx.fillop.color.u8Alpha = color->u8Alpha;
    gfxctx.fillop.blend = color->u8Alpha ? 1 : 0;
    gfxctx.fillop.u32FrameBufAddr = (gfxctx.mmu ? (unsigned long) gfxctx.fb_va : gfxctx.fix.smem_start) + gfxctx.fix.line_length * bbox_clamp.i16Ymin + (gfxctx.var.bits_per_pixel / 8) * bbox_clamp.i16Xmin;
    gfxctx.fillop.rowBytes = gfxctx.fix.line_length;
    gfxctx.fillop.format = gfxctx.var.bits_per_pixel == 16 ? eDRVBLT_DEST_RGB565 : eDRVBLT_DEST_ARGB8888;
    gfxctx.fillop.rect.i16Xmin = bbox_clamp.i16Xmin;//0;
    gfxctx.fillop.rect.i16Xmax = bbox_clamp.i16Xmax;//gfxctx.var.xres;
    gfxctx.fillop.rect.i16Ymin = bbox_clamp.i16Ymin;//0;
    gfxctx.fillop.rect.i16Ymax = bbox_clamp.i16Ymax;//gfxctx.var.yres;
    
    if ((ioctl(gfxctx.bltfd, BLT_SET_FILL, &gfxctx.fillop)) == -1) {
        printf("ioctl BLT_SET_FILL failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((ioctl(gfxctx.bltfd, BLT_TRIGGER, NULL)) == -1) {
        printf("ioctl BLT_TRIGGER failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void gfx_identity(float ox, float oy, S_DRVBLT_RECT *bbox)
{   
    gfx_scale(ox, oy, 1.0f, 1.0f, bbox);
}

/**
  * @brief              Scale source pattern along x/y directions, enclosed within bounding box.
  * @param[in]  ox,oy   coordinates of center in FB CS.
  * @param[in]  ox,oy   Scale factor along x/y directions.
  * @param[in]  bbox    Bounding box in FB CS.
  * 
  * Ma * Mt * Ms * Src = Dst, where
  * Ma: matrix for amendment to mapping point error (line shift error)
  * Mt: translation matrix
  * Ms: scale matrix
  *
  *      / 1 0 -0.5 \       / 1 0 tx \       / sx 0  0 \
  * Ma = | 0 0 -0.5 |  Mt = | 0 1 ty |  Ms = | 0  sy 0 |
  *      \ 0 0 1    /       \ 0 0 1  /       \ 0  0  1 /
  *
  * Src = Inv(Ms) * Inv(Mt) * Inv(Ma) * DST
  *
  *           / 1/sx 0    0 \            / 1 0 -tx \            / 1 0 0.5 \
  * Inv(Ms) = | 0    1/sy 0 |  Inv(Mt) = | 0 1 -ty |  Inv(Ma) = | 0 1 0.5 |
  *           \ 0    0    1 /            \ 0 0 1   /            \ 0 0 1   /
  *
  *                               / 1/sx 0    -tx/sx+0.5*(a+c) \   / a c src.xoffset \
  * Inv(Ms) * Inv(Mt) * Inv(Ma) = | 0    1/sy -ty/sy+0.5*(b+d) | = | b d src.yoffset |
  *                               \ 0    0    1                /   \ 0 0 0           /
  *
  */
void gfx_scale(float ox, float oy, float sx, float sy, S_DRVBLT_RECT *bbox)
{   
    S_DRVBLT_RECT bbox_clamp;
    gfx_rect_clamp(bbox, &bbox_clamp);
    
    /* */
    ox -= (float) bbox_clamp.i16Xmin;
    oy -= (float) bbox_clamp.i16Ymin;
    
    gfxctx.blitop.dest.u32FrameBufAddr = (gfxctx.mmu ? (unsigned long) gfxctx.fb_va : gfxctx.fix.smem_start) + gfxctx.fix.line_length * bbox_clamp.i16Ymin + (gfxctx.var.bits_per_pixel / 8) * bbox_clamp.i16Xmin;
    gfxctx.blitop.dest.i32XOffset = 0;
    gfxctx.blitop.dest.i32YOffset = 0;
    gfxctx.blitop.dest.i32Stride = gfxctx.fix.line_length;
    gfxctx.blitop.dest.i16Width = bbox_clamp.i16Xmax - bbox_clamp.i16Xmin;//gfxctx.var.xres;
    gfxctx.blitop.dest.i16Height = bbox_clamp.i16Ymax - bbox_clamp.i16Ymin;//gfxctx.var.yres;

    gfxctx.blitop.src.pSARGB8 = NULL;
    gfxctx.blitop.src.u32SrcImageAddr = gfxctx.mmu ? (unsigned long) gfxctx.srcbuf_va : gfxctx.srcbuf_pa;
    gfxctx.blitop.src.i32Stride = gfxctx.srcbuf_stride;
    gfxctx.blitop.src.i32XOffset = -ox / sx * 0x10000;
    gfxctx.blitop.src.i32YOffset = -oy / sy * 0x10000;
    gfxctx.blitop.src.i16Width = gfxctx.srcbuf_width;
    gfxctx.blitop.src.i16Height = gfxctx.srcbuf_height;
        
    gfxctx.tfm.colorMultiplier.i16Blue  = 0x0100;
    gfxctx.tfm.colorMultiplier.i16Green = 0x0100;
    gfxctx.tfm.colorMultiplier.i16Red   = 0x0100;
    gfxctx.tfm.colorMultiplier.i16Alpha = 0x0100;
    gfxctx.tfm.colorOffset.i16Blue  = 0;
    gfxctx.tfm.colorOffset.i16Green = 0;
    gfxctx.tfm.colorOffset.i16Red   = 0;
    gfxctx.tfm.colorOffset.i16Alpha = 0;
    gfxctx.tfm.matrix.a = (uint32_t) ((1 / sx) * 0x10000);
    gfxctx.tfm.matrix.b = 0x00000000;
    gfxctx.tfm.matrix.c = 0x00000000;
    gfxctx.tfm.matrix.d = (uint32_t) ((1 / sy) * 0x10000);
    //gfxctx.tfm.srcFormat = eDRVBLT_SRC_ARGB8888;
    gfxctx.tfm.destFormat = gfxctx.var.bits_per_pixel == 16 ? eDRVBLT_DEST_RGB565 : eDRVBLT_DEST_ARGB8888;
    //gfxctx.tfm.flags = 0;
    //gfxctx.tfm.fillStyle = eDRVBLT_CLIP;//(eDRVBLT_NOTSMOOTH | eDRVBLT_CLIP);
    gfxctx.tfm.userData = NULL;

    // Apply amendment to mapping point error (line shift error). Can be omitted if line shift error not encountered.
    {
        gfxctx.blitop.src.i32XOffset += (gfxctx.tfm.matrix.a + gfxctx.tfm.matrix.c) / 2;
        gfxctx.blitop.src.i32YOffset += (gfxctx.tfm.matrix.b + gfxctx.tfm.matrix.d) / 2;
    }
    
    gfxctx.blitop.transformation = &gfxctx.tfm;
    
    if ((ioctl(gfxctx.bltfd, BLT_SET_BLIT, &gfxctx.blitop)) == -1) {
        printf("ioctl BLT_SET_BLIT failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((ioctl(gfxctx.bltfd, BLT_TRIGGER, NULL)) == -1) {
        printf("ioctl BLT_TRIGGER failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

#define PI_OVER_180 0.01745329252f

/**
  * @brief              Rotate source pattern with top-left point as center, enclosed within bounding box.
  * @param[in]  ox,oy   coordinates of center in FB CS.
  * @param[in]  deg     Angle in degree
  * @param[in]  bbox    Bounding box in FB CS.
  * 
  * Ma * Mt * Mr * Src = Dst, where
  * Ma: matrix for amendment to mapping point error (line shift error)
  * Mt: translation matrix
  * Mr: rotation matrix
  *
  *      / 1 0 -0.5 \       / 1 0 tx \       / cos£c -sin£c 0 \
  * Ma = | 0 0 -0.5 |  Mt = | 0 1 ty |  Mr = | sin£c con£c  0 |
  *      \ 0 0 1    /       \ 0 0 1  /       \ 0    0     1 /
  *
  * Src = Inv(Mr) * Inv(Mt) * Inv(Ma) * DST
  *
  *           / cos£c  sin£c 0 \            / 1 0 -tx \            / 1 0 0.5 \
  * Inv(Mr) = | -sin£c con£c 0 |  Inv(Mt) = | 0 1 -ty |  Inv(Ma) = | 0 1 0.5 |
  *           \ 0     0    1 /            \ 0 0 1   /            \ 0 0 1   /
  *
  *                               / con£c  sin£c -con£c*tx-sin£c*ty+0.5*(a+c) \   / a c src.xoffset \
  * Inv(Mr) * Inv(Mt) * Inv(Ma) = | -sin£c con£c sin£c*tx-con£c*ty+0.5*(b+d)  | = | b d src.yoffset |
  *                               \ 0     0    1                          /   \ 0 0 0           /
  *
  */
void gfx_rotate(float ox, float oy, float deg, S_DRVBLT_RECT *bbox)
{
    S_DRVBLT_RECT bbox_clamp;
    gfx_rect_clamp(bbox, &bbox_clamp);
    
    /* Center of origin is relative to actual drawing rectangle. */
    ox -= (float) bbox_clamp.i16Xmin;
    oy -= (float) bbox_clamp.i16Ymin;
    
    gfxctx.blitop.dest.u32FrameBufAddr = (gfxctx.mmu ? (unsigned long) gfxctx.fb_va : gfxctx.fix.smem_start) + gfxctx.fix.line_length * bbox_clamp.i16Ymin + (gfxctx.var.bits_per_pixel / 8) * bbox_clamp.i16Xmin;
    gfxctx.blitop.dest.i32XOffset = 0;
    gfxctx.blitop.dest.i32YOffset = 0;
    gfxctx.blitop.dest.i32Stride = gfxctx.fix.line_length;
    gfxctx.blitop.dest.i16Width = bbox_clamp.i16Xmax - bbox_clamp.i16Xmin;//gfxctx.var.xres;
    gfxctx.blitop.dest.i16Height = bbox_clamp.i16Ymax - bbox_clamp.i16Ymin;//gfxctx.var.yres;

    gfxctx.blitop.src.pSARGB8 = NULL;
    gfxctx.blitop.src.u32SrcImageAddr = gfxctx.mmu ? (unsigned long) gfxctx.srcbuf_va : gfxctx.srcbuf_pa;
    gfxctx.blitop.src.i32Stride = gfxctx.srcbuf_stride;
    gfxctx.blitop.src.i32XOffset = -(cos(PI_OVER_180 * deg) * ox + sin(PI_OVER_180 * deg) * oy) * 0x10000;
    gfxctx.blitop.src.i32YOffset = (sin(PI_OVER_180 * deg) * ox - cos(PI_OVER_180 * deg) * oy) * 0x10000;
    gfxctx.blitop.src.i16Width = gfxctx.srcbuf_width;
    gfxctx.blitop.src.i16Height = gfxctx.srcbuf_height;
        
    gfxctx.tfm.colorMultiplier.i16Blue  = 0x0100;
    gfxctx.tfm.colorMultiplier.i16Green = 0x0100;
    gfxctx.tfm.colorMultiplier.i16Red   = 0x0100;
    gfxctx.tfm.colorMultiplier.i16Alpha = 0x0100;
    gfxctx.tfm.colorOffset.i16Blue  = 0;
    gfxctx.tfm.colorOffset.i16Green = 0;
    gfxctx.tfm.colorOffset.i16Red   = 0;
    gfxctx.tfm.colorOffset.i16Alpha = 0;
    gfxctx.tfm.matrix.a = cos(PI_OVER_180 * deg) * 0x10000;//0x00010000;
    gfxctx.tfm.matrix.b = -sin(PI_OVER_180 * deg) * 0x10000;//0x00000000;
    gfxctx.tfm.matrix.c = sin(PI_OVER_180 * deg) * 0x10000;//0x00000000;
    gfxctx.tfm.matrix.d = cos(PI_OVER_180 * deg) * 0x10000;//0x00010000;
    //gfxctx.tfm.srcFormat = eDRVBLT_SRC_ARGB8888;
    gfxctx.tfm.destFormat = gfxctx.var.bits_per_pixel == 16 ? eDRVBLT_DEST_RGB565 : eDRVBLT_DEST_ARGB8888;
    //gfxctx.tfm.flags = 0;
    //gfxctx.tfm.fillStyle = eDRVBLT_CLIP;//(eDRVBLT_NOTSMOOTH | eDRVBLT_CLIP);
    gfxctx.tfm.userData = NULL;

    // Apply amendment to mapping point error (line shift error). Can be omitted if line shift error not encountered.
    {
        gfxctx.blitop.src.i32XOffset += (gfxctx.tfm.matrix.a + gfxctx.tfm.matrix.c) / 2;
        gfxctx.blitop.src.i32YOffset += (gfxctx.tfm.matrix.b + gfxctx.tfm.matrix.d) / 2;
    }
    
    gfxctx.blitop.transformation = &gfxctx.tfm;
    
    if ((ioctl(gfxctx.bltfd, BLT_SET_BLIT, &gfxctx.blitop)) == -1) {
        printf("ioctl BLT_SET_BLIT failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((ioctl(gfxctx.bltfd, BLT_TRIGGER, NULL)) == -1) {
        printf("ioctl BLT_TRIGGER failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void gfx_alpha(float ox, float oy, float alpha, S_DRVBLT_RECT *bbox)
{   
    S_DRVBLT_RECT bbox_clamp;
    gfx_rect_clamp(bbox, &bbox_clamp);
    
    /* */
    ox -= (float) bbox_clamp.i16Xmin;
    oy -= (float) bbox_clamp.i16Ymin;
    
    gfxctx.blitop.dest.u32FrameBufAddr = (gfxctx.mmu ? (unsigned long) gfxctx.fb_va : gfxctx.fix.smem_start) + gfxctx.fix.line_length * bbox_clamp.i16Ymin + (gfxctx.var.bits_per_pixel / 8) * bbox_clamp.i16Xmin;
    gfxctx.blitop.dest.i32XOffset = 0;
    gfxctx.blitop.dest.i32YOffset = 0;
    gfxctx.blitop.dest.i32Stride = gfxctx.fix.line_length;
    gfxctx.blitop.dest.i16Width = bbox_clamp.i16Xmax - bbox_clamp.i16Xmin;//gfxctx.var.xres;
    gfxctx.blitop.dest.i16Height = bbox_clamp.i16Ymax - bbox_clamp.i16Ymin;//gfxctx.var.yres;

    gfxctx.blitop.src.pSARGB8 = NULL;
    gfxctx.blitop.src.u32SrcImageAddr = gfxctx.mmu ? (unsigned long) gfxctx.srcbuf_va : gfxctx.srcbuf_pa;
    gfxctx.blitop.src.i32Stride = gfxctx.srcbuf_stride;
    gfxctx.blitop.src.i32XOffset = -ox * 0x10000;
    gfxctx.blitop.src.i32YOffset = -oy * 0x10000;
    gfxctx.blitop.src.i16Width = gfxctx.srcbuf_width;
    gfxctx.blitop.src.i16Height = gfxctx.srcbuf_height;
        
    gfxctx.tfm.colorMultiplier.i16Blue  = 0x0100;
    gfxctx.tfm.colorMultiplier.i16Green = 0x0100;
    gfxctx.tfm.colorMultiplier.i16Red   = 0x0100;
    gfxctx.tfm.colorMultiplier.i16Alpha = (int16_t) (alpha * 256);
    gfxctx.tfm.colorOffset.i16Blue  = 0;
    gfxctx.tfm.colorOffset.i16Green = 0;
    gfxctx.tfm.colorOffset.i16Red   = 0;
    gfxctx.tfm.colorOffset.i16Alpha = 0;
    gfxctx.tfm.matrix.a = 0x00010000;
    gfxctx.tfm.matrix.b = 0x00000000;
    gfxctx.tfm.matrix.c = 0x00000000;
    gfxctx.tfm.matrix.d = 0x00010000;
    //gfxctx.tfm.srcFormat = eDRVBLT_SRC_ARGB8888;
    gfxctx.tfm.destFormat = gfxctx.var.bits_per_pixel == 16 ? eDRVBLT_DEST_RGB565 : eDRVBLT_DEST_ARGB8888;
    gfxctx.tfm.flags |= eDRVBLT_HASCOLORTRANSFORM | eDRVBLT_HASALPHAONLY;
    //gfxctx.tfm.fillStyle = eDRVBLT_CLIP;//(eDRVBLT_NOTSMOOTH | eDRVBLT_CLIP);
    gfxctx.tfm.userData = NULL;

    // Apply amendment to mapping point error (line shift error). Can be omitted if line shift error not encountered.
    {
        gfxctx.blitop.src.i32XOffset += (gfxctx.tfm.matrix.a + gfxctx.tfm.matrix.c) / 2;
        gfxctx.blitop.src.i32YOffset += (gfxctx.tfm.matrix.b + gfxctx.tfm.matrix.d) / 2;
    }
    
    gfxctx.blitop.transformation = &gfxctx.tfm;
    
    if ((ioctl(gfxctx.bltfd, BLT_SET_BLIT, &gfxctx.blitop)) == -1) {
        printf("ioctl BLT_SET_BLIT failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((ioctl(gfxctx.bltfd, BLT_TRIGGER, NULL)) == -1) {
        printf("ioctl BLT_TRIGGER failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

/**
  * @brief              Flip source pattern horizontally, enclosed within bounding box.
  * @param[in]  ox,oy   coordinates of center in FB CS.
  * @param[in]  bbox    Bounding box in FB CS.
  * 
  * Ma * Mt * Mh * Src = Dst, where
  * Ma: matrix for amendment to mapping point error (line shift error)
  * Mt: translation matrix
  * Mh: horiz-flip matrix
  *
  *      / 1 0 -0.5 \       / 1 0 tx \       / -1 0 sw \
  * Ma = | 0 0 -0.5 |  Mt = | 0 1 ty |  Mh = | 0  1 0  |
  *      \ 0 0 1    /       \ 0 0 1  /       \ 0  0 1  /
  *
  * Src = Inv(Mh) * Inv(Mt) * Inv(Ma) * DST
  *
  *           / -1 0 sw  \            / 1 0 -tx \            / 1 0 0.5 \
  * Inv(Mh) = | 0  1 0   |  Inv(Mt) = | 0 1 -ty |  Inv(Ma) = | 0 1 0.5 |
  *           \ 0  0 1   /            \ 0 0 1   /            \ 0 0 1   /
  *
  *                               / -1 0 tx+sw+0.5*(a+c) \   / a c src.xoffset \
  * Inv(Mh) * Inv(Mt) * Inv(Ma) = | 0  1 -ty+0.5*(b+d)   | = | b d src.yoffset |
  *                               \ 0  0    1            /   \ 0 0 0           /
  *
  */
void gfx_horizflip(float ox, float oy, S_DRVBLT_RECT *bbox)
{
    S_DRVBLT_RECT bbox_clamp;
    gfx_rect_clamp(bbox, &bbox_clamp);
    
    /* Center of origin is relative to actual drawing rectangle. */
    ox -= (float) bbox_clamp.i16Xmin;
    oy -= (float) bbox_clamp.i16Ymin;
    
    gfxctx.blitop.dest.u32FrameBufAddr = (gfxctx.mmu ? (unsigned long) gfxctx.fb_va : gfxctx.fix.smem_start) + gfxctx.fix.line_length * bbox_clamp.i16Ymin + (gfxctx.var.bits_per_pixel / 8) * bbox_clamp.i16Xmin;
    gfxctx.blitop.dest.i32XOffset = 0;
    gfxctx.blitop.dest.i32YOffset = 0;
    gfxctx.blitop.dest.i32Stride = gfxctx.fix.line_length;
    gfxctx.blitop.dest.i16Width = bbox_clamp.i16Xmax - bbox_clamp.i16Xmin;//gfxctx.var.xres;
    gfxctx.blitop.dest.i16Height = bbox_clamp.i16Ymax - bbox_clamp.i16Ymin;//gfxctx.var.yres;

    gfxctx.blitop.src.pSARGB8 = NULL;
    gfxctx.blitop.src.u32SrcImageAddr = gfxctx.mmu ? (unsigned long) gfxctx.srcbuf_va : gfxctx.srcbuf_pa;
    gfxctx.blitop.src.i32Stride = gfxctx.srcbuf_stride;
    gfxctx.blitop.src.i32XOffset = (ox + gfxctx.srcbuf_width) * 0x10000;
    gfxctx.blitop.src.i32YOffset = -oy * 0x10000;
    gfxctx.blitop.src.i16Width = gfxctx.srcbuf_width;
    gfxctx.blitop.src.i16Height = gfxctx.srcbuf_height;
        
    gfxctx.tfm.colorMultiplier.i16Blue  = 0x0100;
    gfxctx.tfm.colorMultiplier.i16Green = 0x0100;
    gfxctx.tfm.colorMultiplier.i16Red   = 0x0100;
    gfxctx.tfm.colorMultiplier.i16Alpha = 0x0100;
    gfxctx.tfm.colorOffset.i16Blue  = 0;
    gfxctx.tfm.colorOffset.i16Green = 0;
    gfxctx.tfm.colorOffset.i16Red   = 0;
    gfxctx.tfm.colorOffset.i16Alpha = 0;
    gfxctx.tfm.matrix.a = -1 * 0x10000;
    gfxctx.tfm.matrix.b = 0;
    gfxctx.tfm.matrix.c = 0;
    gfxctx.tfm.matrix.d = 0x00010000;
    //gfxctx.tfm.srcFormat = eDRVBLT_SRC_ARGB8888;
    gfxctx.tfm.destFormat = gfxctx.var.bits_per_pixel == 16 ? eDRVBLT_DEST_RGB565 : eDRVBLT_DEST_ARGB8888;
    //gfxctx.tfm.flags = 0;
    //gfxctx.tfm.fillStyle = eDRVBLT_CLIP;//(eDRVBLT_NOTSMOOTH | eDRVBLT_CLIP);
    gfxctx.tfm.userData = NULL;

    // Apply amendment to mapping point error (line shift error). Can be omitted if line shift error not encountered.
    {
        gfxctx.blitop.src.i32XOffset += (gfxctx.tfm.matrix.a + gfxctx.tfm.matrix.c) / 2;
        gfxctx.blitop.src.i32YOffset += (gfxctx.tfm.matrix.b + gfxctx.tfm.matrix.d) / 2;
    }
    
    gfxctx.blitop.transformation = &gfxctx.tfm;
    
    if ((ioctl(gfxctx.bltfd, BLT_SET_BLIT, &gfxctx.blitop)) == -1) {
        printf("ioctl BLT_SET_BLIT failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((ioctl(gfxctx.bltfd, BLT_TRIGGER, NULL)) == -1) {
        printf("ioctl BLT_TRIGGER failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

/**
  * @brief              Flip source pattern vertically, enclosed within bounding box.
  * @param[in]  ox,oy   coordinates of center in FB CS.
  * @param[in]  bbox    Bounding box in FB CS.
  * 
  * Ma * Mt * Mv * Src = Dst, where
  * Ma: matrix for amendment to mapping point error (line shift error)
  * Mt: translation matrix
  * Mv: vert-flip matrix
  *
  *      / 1 0 -0.5 \       / 1 0 tx \       / 1 0  0  \
  * Ma = | 0 0 -0.5 |  Mt = | 0 1 ty |  Mv = | 0 -1 sh |
  *      \ 0 0 1    /       \ 0 0 1  /       \ 0 0  1  /
  *
  * Src = Inv(Mv) * Inv(Mt) * Inv(Ma) * DST
  *
  *           / 1 0  0  \            / 1 0 -tx \            / 1 0 0.5 \
  * Inv(Mh) = | 0 -1 sh |  Inv(Mt) = | 0 1 -ty |  Inv(Ma) = | 0 1 0.5 |
  *           \ 0 0  1  /            \ 0 0 1   /            \ 0 0 1   /
  *
  *                               / 1 0  -tx+0.5*(a+c)   \   / a c src.xoffset \
  * Inv(Mh) * Inv(Mt) * Inv(Ma) = | 0 -1 ty+sh+0.5*(b+d) | = | b d src.yoffset |
  *                               \ 0 0  1               /   \ 0 0 0           /
  *
  */
void gfx_vertflip(float ox, float oy, S_DRVBLT_RECT *bbox)
{
    S_DRVBLT_RECT bbox_clamp;
    gfx_rect_clamp(bbox, &bbox_clamp);
    
    /* Center of origin is relative to actual drawing rectangle. */
    ox -= (float) bbox_clamp.i16Xmin;
    oy -= (float) bbox_clamp.i16Ymin;
    
    gfxctx.blitop.dest.u32FrameBufAddr = (gfxctx.mmu ? (unsigned long) gfxctx.fb_va : gfxctx.fix.smem_start) + gfxctx.fix.line_length * bbox_clamp.i16Ymin + (gfxctx.var.bits_per_pixel / 8) * bbox_clamp.i16Xmin;
    gfxctx.blitop.dest.i32XOffset = 0;
    gfxctx.blitop.dest.i32YOffset = 0;
    gfxctx.blitop.dest.i32Stride = gfxctx.fix.line_length;
    gfxctx.blitop.dest.i16Width = bbox_clamp.i16Xmax - bbox_clamp.i16Xmin;//gfxctx.var.xres;
    gfxctx.blitop.dest.i16Height = bbox_clamp.i16Ymax - bbox_clamp.i16Ymin;//gfxctx.var.yres;

    gfxctx.blitop.src.pSARGB8 = NULL;
    gfxctx.blitop.src.u32SrcImageAddr = gfxctx.mmu ? (unsigned long) gfxctx.srcbuf_va : gfxctx.srcbuf_pa;
    gfxctx.blitop.src.i32Stride = gfxctx.srcbuf_stride;
    gfxctx.blitop.src.i32XOffset = -ox * 0x10000;
    gfxctx.blitop.src.i32YOffset = (oy + gfxctx.srcbuf_height) * 0x10000;
    gfxctx.blitop.src.i16Width = gfxctx.srcbuf_width;
    gfxctx.blitop.src.i16Height = gfxctx.srcbuf_height;
        
    gfxctx.tfm.colorMultiplier.i16Blue  = 0x0100;
    gfxctx.tfm.colorMultiplier.i16Green = 0x0100;
    gfxctx.tfm.colorMultiplier.i16Red   = 0x0100;
    gfxctx.tfm.colorMultiplier.i16Alpha = 0x0100;
    gfxctx.tfm.colorOffset.i16Blue  = 0;
    gfxctx.tfm.colorOffset.i16Green = 0;
    gfxctx.tfm.colorOffset.i16Red   = 0;
    gfxctx.tfm.colorOffset.i16Alpha = 0;
    gfxctx.tfm.matrix.a = 1 * 0x10000;
    gfxctx.tfm.matrix.b = 0;
    gfxctx.tfm.matrix.c = 0;
    gfxctx.tfm.matrix.d = -1 * 0x10000;
    //gfxctx.tfm.srcFormat = eDRVBLT_SRC_ARGB8888;
    gfxctx.tfm.destFormat = gfxctx.var.bits_per_pixel == 16 ? eDRVBLT_DEST_RGB565 : eDRVBLT_DEST_ARGB8888;
    //gfxctx.tfm.flags = 0;
    //gfxctx.tfm.fillStyle = eDRVBLT_CLIP;//(eDRVBLT_NOTSMOOTH | eDRVBLT_CLIP);
    gfxctx.tfm.userData = NULL;

    // Apply amendment to mapping point error (line shift error). Can be omitted if line shift error not encountered.
    {
        gfxctx.blitop.src.i32XOffset += (gfxctx.tfm.matrix.a + gfxctx.tfm.matrix.c) / 2;
        gfxctx.blitop.src.i32YOffset += (gfxctx.tfm.matrix.b + gfxctx.tfm.matrix.d) / 2;
    }
    
    gfxctx.blitop.transformation = &gfxctx.tfm;
    
    if ((ioctl(gfxctx.bltfd, BLT_SET_BLIT, &gfxctx.blitop)) == -1) {
        printf("ioctl BLT_SET_BLIT failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((ioctl(gfxctx.bltfd, BLT_TRIGGER, NULL)) == -1) {
        printf("ioctl BLT_TRIGGER failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void gfx_flush(void)
{
    while (ioctl(gfxctx.bltfd, BLT_FLUSH) == -1) {
        if (errno == EINTR) {
            continue;
        }
            
        printf("ioctl BLT_FLUSH failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void gfx_waitintvl(uint32_t ms)
{        
    struct timeval      tv_cur;
    
    while (1) {
        gettimeofday(&tv_cur, NULL);
        int et = (tv_cur.tv_sec - gfxctx.tv_lastdraw.tv_sec) * 1000 + (tv_cur.tv_usec - gfxctx.tv_lastdraw.tv_usec) / 1000;   // Elapsed time in ms.
        int rmn = ms - et;
        if (rmn > 0) {
            usleep(rmn * 1000);
            continue;
        }
            
        gfxctx.tv_lastdraw = tv_cur;
        break;
    }
}

/**
  * @brief                  Configure tiling mode on drawing source pattern.
  * @param[in]  tilemode    Tiling mode. Decide how to draw when mapping is out of source pattern. Valid values include:
  *                         - 0 No draw
  *                         - 1 Draw with source pattern repeated
  *                         - 2 Draw edge of source pattern
  */
void gfx_set_tilemode(int tilemode)
{
    switch (tilemode) {
    case 0:
        gfxctx.tfm.fillStyle |= eDRVBLT_CLIP;
        break;
        
    case 1:
        gfxctx.tfm.fillStyle &= ~eDRVBLT_CLIP;
        gfxctx.tfm.fillStyle &= ~eDRVBLT_REPEAT_EDGE;
        break;
        
    case 2:
        gfxctx.tfm.fillStyle &= ~eDRVBLT_CLIP;
        gfxctx.tfm.fillStyle |= eDRVBLT_REPEAT_EDGE;
        break;
    }
}

void gfx_set_smooth(int smooth)
{
    if (smooth) {
        gfxctx.tfm.fillStyle &= ~eDRVBLT_NOTSMOOTH;
    }
    else {
        gfxctx.tfm.fillStyle |= eDRVBLT_NOTSMOOTH;
    }
}

/**
  * @brief                  Enable/disable color key when source pattern format is RGB565.
  * @param[in]  en          Enable/disable.
  * @param[in]  colkey      Color key value when color key is enabled.
  */
void gfx_set_colkey(int en, uint16_t colkey)
{
    int retval;
    
    if (en) {
        retval = ioctl(gfxctx.bltfd, BLT_ENABLE_RGB565_COLORCTL, NULL);
        if (retval == -1) {
            printf("ioctl(BLT_ENABLE_RGB565_COLORCTL) failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        retval = ioctl(gfxctx.bltfd, BLT_IOCTRGB565COLORKEY, colkey);
        if (retval == -1) {
            printf("ioctl(BLT_IOCTRGB565COLORKEY) failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        gfxctx.tfm.flags |= eDRVBLT_HASTRANSPARENCY;
    }
    else {
        retval = ioctl(gfxctx.bltfd, BLT_DISABLE_RGB565_COLORCTL, NULL);
        if (retval == -1) {
            printf("ioctl(BLT_DISABLE_RGB565_COLORCTL) failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        gfxctx.tfm.flags &= ~eDRVBLT_HASTRANSPARENCY;
    }
}

/**
  * @brief                  Clamp with valid destination rectangle.
  * @param[in]  rect_src    Rectangle to be clamped.
  * @param[out]  rect_dst   Rectangle after clamped.
  */
void gfx_rect_clamp(const S_DRVBLT_RECT *rect_src, S_DRVBLT_RECT *rect_dst)
{
    S_DRVBLT_RECT bbox_fs = {0, gfx_ctx()->var.xres, 0, gfx_ctx()->var.yres};
    rect_intersect(rect_src, &bbox_fs, rect_dst);
}

void rect_intersect(const S_DRVBLT_RECT *rect_src1, const S_DRVBLT_RECT *rect_src2, S_DRVBLT_RECT *rect_dst)
{
    rect_dst->i16Xmin = (rect_src1->i16Xmin >= rect_src2->i16Xmin) ? rect_src1->i16Xmin : rect_src2->i16Xmin;
    rect_dst->i16Xmax = (rect_src1->i16Xmax <= rect_src2->i16Xmax) ? rect_src1->i16Xmax : rect_src2->i16Xmax;
    rect_dst->i16Ymin = (rect_src1->i16Ymin >= rect_src2->i16Ymin) ? rect_src1->i16Ymin : rect_src2->i16Ymin;
    rect_dst->i16Ymax = (rect_src1->i16Ymax <= rect_src2->i16Ymax) ? rect_src1->i16Ymax : rect_src2->i16Ymax;
}

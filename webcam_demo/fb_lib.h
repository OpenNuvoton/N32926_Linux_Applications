#ifndef FB_LIB_H
#define FB_LIB_H

enum FBx{
		FB_0 = 0,
		FB_1
	};

extern int fb_open(int fbx);
extern int fb_render(int fd, unsigned char *img, unsigned char *map,
	      int xoffset, int yoffset, 
              int width, int height, int depth);
extern void fb_close(int fd);

int FormatConvert(char* pSrcBuf, char* pDstBuf, int Tarwidth, int Tarheight);
extern void myExitHandler (int sig);
int InitVPE(char* pSrcBuf, char* pDstBuf, int Tarwidth, int Tarheight);
/* VPE device */
extern int vpe_fd;
extern int pw;
extern int ph;
extern int fb_fd;
#endif

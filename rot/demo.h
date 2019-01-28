#define __LCM_320x240__


#ifdef __LCM_320x240__
#define src_w_step 		3		//src pattern 240*320
#define src_h_step 		4
#define dst_w_step 		4		//dst pattern 320*240
#define dst_h_step 		3
#define OPT_SRC_WIDTH	240
#define OPT_SRC_HEIGHT	320
#define OPT_LCM_WIDTH	320
#define OPT_LCM_HEIGHT 	240
#endif 

#define DBG_PRINTF(...)		
#define ERR_PRINTF	printf


int32_t OpenROTDevice(uint32_t *pu32RotPhyAddr);			/* Rotation Physical buffer address */
void CloseROTDevice(void);


int OpenFBDevice(uint32_t *pu32FBPhyAddr);
void CloseFBDevice(void);

void ShowResult(uint32_t buf);

extern int32_t i32DevROT;
extern uint8_t* pu8MapROTBuf;
extern int32_t i32DevFB;
extern struct fb_var_screeninfo gsVar;
extern uint32_t gu32FBPhysicalAddr;

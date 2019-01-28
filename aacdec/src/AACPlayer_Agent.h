/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp.  All rights reserved. *
 *                                                              *
 ****************************************************************/


#ifndef	__AACPLAYER_AGENT_H__
#define __AACPLAYER_AGENT_H__

#include <pthread.h>
#include "AACDecoder.h"

#ifdef __cplusplus
extern "C" {
#endif


// ==================================================
// Constant definition
// ==================================================
#define AACPLAYER_VOLUME_DEFAULT	80   //60
#define AACPLAYER_VOLUME_MAX		100
#define AACPLAYER_VOLUME_MIN		0

//#define AACPLAYER_PSUEDO_AUDIO		// Defined if no real audio out, e.g. profiling


// ==================================================
// Macro definition
// ==================================================
//#define AACPLAYER_DEBUG_MSG(_fmt, ...)	fprintf(stderr, "[DEBUG] %s: " _fmt "\n", __FUNCTION__, ##__VA_ARGS__)
#define AACPLAYER_DEBUG_MSG(_fmt, ...)


// ==================================================
// Type declaration
// ==================================================
typedef enum
{
	eAACPLAYER_STATE_STOPPED	= 0,
	eAACPLAYER_STATE_PLAYING	= 1,
	eAACPLAYER_STATE_PAUSED		= 2,
	eAACPLAYER_STATE_TOPAUSE	= 3,
	eAACPLAYER_STATE_TORESUME	= 4,
	eAACPLAYER_STATE_TOSTOP		= 5,
} E_AACPLAYER_STATE;


// ==================================================
// Global variable declaration
// ==================================================
extern volatile E_AACPLAYER_STATE	g_eAACPlayer_State;
extern volatile unsigned int		g_u32AACPlayer_PlaySampleCnt;	// Per channel

extern pthread_mutex_t				g_sPlayerStateMutex;
extern S_AACDEC_FRAME_INFO			g_sAACDec_FrameInfo;
extern unsigned int					g_u32AACPlayer_SampleRate,
									g_u32AACPlayer_Volume;


// ==================================================
// Function declaration
// ==================================================

bool							// [out]true/false: Successful/Failed
AACPlayer_PlayFile(
    char* 	pczFileName			// [in] File name
);

bool							// [out]true/false: Successful/Failed
AACPlayer_ChangeVolume(
	int		i32NewVolume
);

bool							// [out]true/false: Successful/Failed
AACPlayer_Pause(void);

bool							// [out]true/false: Successful/Failed
AACPlayer_Resume(void);

bool							// [out]true/false: Successful/Failed
AACPlayer_Seek(
	int i32DeltaIn10ms			// [in] Time position delta in 10ms
								//			> 0: Forward seek
								//			< 0: Backward seek
);

void AACPlayer_Stop(void);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __AACPLAYER_AGENT_H__

/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp.  All rights reserved. *
 *                                                              *
 ****************************************************************/


#ifndef	__AACRECORDER_AGENT_H__
#define __AACRECORDER_AGENT_H__

#include <pthread.h>
#include "AACEncoder.h"

#ifdef __cplusplus
extern "C" {
#endif


// ==================================================
// Constant definition
// ==================================================
#if 0 // 44.1K
#define AACRECORDER_BIT_RATE	96000   //64000	// 64K bps
#define AACRECORDER_CHANNEL_NUM	2       //1		// Mono
#define AACRECORDER_QUALITY		90		// 1 ~ 999
#define AACRECORDER_SAMPLE_RATE	44100   //16000	// 16K Hz
#endif

#if 1
#define AACRECORDER_BIT_RATE	64000	// 64K bps
#define AACRECORDER_CHANNEL_NUM	1		// Mono
#define AACRECORDER_QUALITY		90		// 1 ~ 999
#define AACRECORDER_SAMPLE_RATE	16000	// 16K Hz
#endif

//#define AACRECORDER_PSUEDO_AUDIO		// Defined if no real audio in, e.g. profiling


// ==================================================
// Macro definition
// ==================================================
//#define AACRECORDER_DEBUG_MSG(_fmt, ...)	fprintf(stderr, "[DEBUG] %s: " _fmt "\n", __FUNCTION__, ##__VA_ARGS__)
#define AACRECORDER_DEBUG_MSG(_fmt, ...)


// ==================================================
// Type declaration
// ==================================================
typedef enum
{
	eAACRECORDER_STATE_STOPPED		= 0,
	eAACRECORDER_STATE_RECORDING	= 1,
	eAACRECORDER_STATE_PAUSED		= 2,
	eAACRECORDER_STATE_TOPAUSE		= 3,
	eAACRECORDER_STATE_TORESUME		= 4,
	eAACRECORDER_STATE_TOSTOP		= 5,
} E_AACRECORDER_STATE;


// ==================================================
// Global variable declaration
// ==================================================
extern E_AACRECORDER_STATE	g_eAACRecorder_State;
extern unsigned int		g_u32AACRecorder_EncFrameCnt,
									g_u32AACRecorder_RecFrameCnt;

extern pthread_mutex_t				g_sRecorderStateMutex;


// ==================================================
// Function declaration
// ==================================================

bool							// [out]true/false: Successful/Failed
AACRecorder_RecordFile(
    char* 	pczFileName			// [in] File name
);

bool							// [out]true/false: Successful/Failed
AACRecorder_Pause(void);

bool							// [out]true/false: Successful/Failed
AACRecorder_Resume(void);

void AACRecorder_Stop(void);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __AACRECORDER_AGENT_H__

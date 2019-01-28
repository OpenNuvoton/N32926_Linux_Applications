/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved. 	*
 *                                                              *
 ****************************************************************/

#ifndef  __AACENCODER_H__
#define __AACENCODER_H__

#ifdef __cplusplus
extern "C" {
#endif


// ==================================================
// Constant definition
// ==================================================
#define AACENC_VERSION		0x0100	// v1.00

#define AACENC_FRAMESIZE	1024	// Samples per channel in frame


// ==================================================
// Type declaration
// ==================================================

// boolean type
#ifndef __cplusplus
	#if defined(bool) && (false != 0 || true != !false)
		#warning to redefine bool: false(0), true(!0)
	#endif

	#undef bool
	#undef false
	#undef true
	#define bool	char
	#define false	0
	#define true	(!false)
#endif


// Error code
typedef enum {
	eAACENC_ERROR_NONE			= 0x0000,	// No error

	// Un-recoverable errors
	eAACENC_ERROR_BUFLEN		= 0x0001,	// Input buffer too small (or EOF)
	eAACENC_ERROR_BUFPTR		= 0x0002,	// Invalid (null) buffer pointer
	eAACENC_ERROR_NOMEM			= 0x0031,	// Not enough memory
	
	// Recoverable errors
	eAACENC_ERROR_BADBITRATE	= 0x0103,	// Forbidden bitrate value
	eAACENC_ERROR_BADSAMPLERATE	= 0x0104,	// Reserved sample frequency value
} E_AACENC_ERROR;


typedef struct{
	bool			m_bUseAdts;
	bool			m_bUseMidSide;
	bool			m_bUseTns;
	unsigned int	m_u32Quality;			// 1 ~ 999
	unsigned int	m_u32BitRate;			// bps
	unsigned int	m_u32ChannelNum;
	unsigned int	m_u32SampleRate;		// Hz
} S_AACENC;


// ==================================================
// Function declaration
// ==================================================

E_AACENC_ERROR
AACEnc_Initialize(
	S_AACENC *psEncoder,			// [in]
	bool		bUseHWEnc		// [in] Use H/W encoder if existed
);

E_AACENC_ERROR
AACEnc_EncodeFrame(
	short	*pi16PCMBuf,		// [in] PCM buffer
	char	*pi8FrameBuf,		// [in] AAC frame buffer
	int		i32FrameBufSize,	// [in] Bytes of AAC frame buffer size
	int		*pi32FrameSize		// [out]Bytes of encoded AAC frame
);

void AACEnc_Finalize(void);

unsigned int
AACEnc_GetVersion(void);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __AACENCODER_H__

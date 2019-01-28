/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved. 	*
 *                                                              *
 ****************************************************************/

#ifndef  __AACDECODER_H__
#define __AACDECODER_H__

#ifdef __cplusplus
extern "C" {
#endif


// ==================================================
// Constant definition
// ==================================================
#define AACDEC_VERSION	0x0112		// v1.12: Only support LC profile


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
	eAACDEC_ERROR_NONE			= 0,

	// Unrecoveralbe errors
	eAACDEC_ERROR_NULLFILE		= -1,	// Null file pointer
	eAACDEC_ERROR_NOMEM			= -2,	// No memory
	eAACDEC_ERROR_LIBINIT		= -3,	// Fail to initialize library
	eAACDEC_ERROR_CONFINIT		= -4,	// Fail to initialize configuration
	eAACDEC_ERROR_FLOWBREAK		= -5,	// Flow control is broken
	eAACDEC_ERROR_OUTRANGE		= -6,	// Out of range
	eAACDEC_ERROR_NOAAC			= -7,	// No AAC data
	eAACDEC_ERROR_READFILE		= -8,	// Fail to read file
	eAACDEC_ERROR_FLOWSTOP		= -9,	// Flow control is stopped
	eAACDEC_ERROR_NULLPTR		= -10,	// Null pointer assignment

	// Recoveralbe errors
	eAACDEC_ERROR_NOGAIN		= 1,	// Gain control not yet implemented
	eAACDEC_ERROR_SHORTPLUS		= 2,	// Pulse coding not allowed in short blocks
	eAACDEC_ERROR_HUFFMAN		= 3,	// Invalid huffman codebook
	eAACDEC_ERROR_SCALEFACTOR	= 4,	// Negative scalefactor found, should be impossible
	eAACDEC_ERROR_SYNC			= 5,	// Unable to find ADTS syncword
	eAACDEC_ERROR_CCP			= 6,	// Channel coupling not yet implemented
	eAACDEC_ERROR_ECHCFG		= 7,	// Channel configuration not allowed in error resilient frame
	eAACDEC_ERROR_BITERROR		= 8,	// Bit error in error resilient scalefactor decoding
	eAACDEC_ERROR_HUFFMANSF		= 9,	// Error decoding huffman scalefactor = bitstream error,
	eAACDEC_ERROR_HUFFMANCODE	= 10,	// Error decoding huffman codeword = bitstream error,
	eAACDEC_ERROR_HUFFMANCB		= 11,	// Non existent huffman codebook number found
	eAACDEC_ERROR_CHANNELS		= 12,	// Invalid number of channels
	eAACDEC_ERROR_ELEMENTS		= 13,	// Maximum number of bitstream elements exceeded
	eAACDEC_ERROR_INPUTBUF		= 14,	// Input data buffer too small
	eAACDEC_ERROR_RANGEOUT		= 15,	// Input data buffer too small
	eAACDEC_ERROR_SFBS			= 16,	// Maximum number of scalefactor bands exceeded
	eAACDEC_ERROR_QVALUE		= 17,	// Quantised value out of range
	eAACDEC_ERROR_LAGRAGEOUT	= 18,	// LTP lag out of range
	eAACDEC_ERROR_SBR			= 19,	// Invalid SBR parameter decoded
	eAACDEC_ERROR_SBRNOINIT		= 20,	// SBR called without being initialised
	eAACDEC_ERROR_CHCFG			= 21,	// Unexpected channel configuration change
	eAACDEC_ERROR_ELMCFG		= 22,	// Error in program_config_element
	eAACDEC_ERROR_FISTSBR		= 23,	// First SBR frame is not the same as first AAC frame
	eAACDEC_ERROR_ELMSBR		= 24,	// Unexpected fill element with SBR data
	eAACDEC_ERROR_ELMNOSBR		= 25,	// Not all elements were provided with SBR data
} E_AACDEC_ERROR;


typedef enum
{
	eAACDEC_MP4_ALBUM		= 0,
	eAACDEC_MP4_ARTIST		= 1,
	eAACDEC_MP4_COMMENT		= 2,
	eAACDEC_MP4_COMPILATION	= 3,
	eAACDEC_MP4_DATE		= 4,
	eAACDEC_MP4_DISC		= 5,
	eAACDEC_MP4_GENRE		= 6,
	eAACDEC_MP4_TEMPO		= 7,
	eAACDEC_MP4_TITLE		= 8,
	eAACDEC_MP4_TOOL		= 9,
	eAACDEC_MP4_TRACK		= 10,
	eAACDEC_MP4_WRITER		= 11,
	eAACDEC_MP4_TAGNUM					// Number of MP4 tag field
} E_AACDEC_MP4TAG;


typedef enum
{
	eAACDEC_FLOW_CONTINUE	= 0x0000,	// Continue normally
  	eAACDEC_FLOW_STOP		= 0x0010,	// Stop decoding normally
  	eAACDEC_FLOW_BREAK		= 0x0011,	// Stop decoding and signal an error
  	eAACDEC_FLOW_IGNORE		= 0x0020	// Ignore the current frame
} E_AACDEC_FLOW;


// Kenny,2010/12/9: Support customized prescan frames
typedef enum
{
	eAACDEC_PRESCAN_NONE	=  0,		// Not prescan
	eAACDEC_PRESCAN_ALL		= -1,		// Prescan all frames in only one pass
	eAACDEC_PRESCAN_DEFAULT	= -2,		// Prescan default number of frames in each pass
} E_AACDEC_PRESCAN;
// -- END --


typedef struct
{
	E_AACDEC_ERROR	m_eError;				// Error code

	unsigned int	m_u32Samplerate;		// Sample rate in AAC
	unsigned int	m_u32Channels;			// Number of channel in AAC
	unsigned int	m_u32Samples;			// Number of sample per channel in sample buffer

	unsigned int	m_u32Sbr;				// SBR: 0: off, 1: on; normal, 2: on; downsampled
	unsigned int	m_u32Vbr;				// Variable bitrate
	unsigned int	m_u32Object_type;		// MPEG-4 ObjectType
	unsigned int	m_u32Header_type;		// AAC header type; MP4 will be signalled as RAW also    
	bool			m_bMP4;					// MP4(M4A) or AAC

// Kenny,2010/12/9: Support customized prescan frames
//	bool			m_bPrescan_frames;
	int				m_i32Prescan_frames;	// Number of prescan frames: 0: no; -1: all; >0: partial; others: default
// -- END --
	unsigned int	m_u32Avg_bitrate;		// in bps (Only available if m_bPrescan_frames is true)
	unsigned int	m_u32Total_duration;	// in 10ms (Only available if m_bPrescan_frames is true)
	unsigned int	m_u32Total_frames;		// Number of frame in AAC (Only available if m_bPrescan_frames is true)

	unsigned int	m_u32Cur_bitrate;		// in bps (unimplemented)
	unsigned int	m_u32Cur_frame;			// Current frame number
	unsigned int	m_u32Cur_position;		// Time position in 10ms if m_u32Total_duration is available
											//		else file offset in bytes
	char			*m_pcSample_buffer;		// Decoded sample buffer
} S_AACDEC_FRAME_INFO;


// file callback structure
typedef struct
{
	unsigned int	(*m_pfnRead)(
						void				*pFile,
						void				*pBuffer,
						unsigned int		u32Length
					);
	unsigned int	(*m_pfnSeek)(
						void				*pFile,
						unsigned long long	u64Position
					);
	E_AACDEC_FLOW	(*m_pfnFirst_frame_callback)(
						void				*pUser_data,
						char				*pchPcmbuf,
						unsigned int		u32Pcmbuf_length,
						S_AACDEC_FRAME_INFO	*psFrame_info);
	E_AACDEC_FLOW	(*m_pfnFrame_callback)(
						void				*pUser_data,
						char				*pchPcmbuf,
						unsigned int		u32Pcmbuf_length,
						S_AACDEC_FRAME_INFO	*psFrame_info);
	void			(*m_pfnInfo_callback)(
						void				*pUser_data,
						S_AACDEC_FRAME_INFO	*psFrame_info
					);
	void			(*m_pfnTerminal_callback)(
						void				*pUser_data
					);

	void			*m_pFile;
	unsigned int	m_u32File_size;
} S_AACDEC_FILE_CALLBACK;


// ==================================================
// Function declaration
// ==================================================

bool											// [out]true / false: Success / Failed
AACDec_Initialize(
	S_AACDEC_FILE_CALLBACK	*psAACFileCallback,	// [in]
	S_AACDEC_FRAME_INFO		*psFrameInfo,		// [out]
// Kenny,2010/12/9: Support frame prescan in multi-pass
	bool					*pbIsPrescanEnd		// [out]Should call this API again until *pbIsPrescanEnd is true
// -- END --
);

bool							// [out]true / false: Conitnue / Stop decode
AACDec_DecodeFrame(void);

void AACDec_Finalize(void);

void AACDec_SetPosition(
	unsigned int u32Position	// [in] Time position in 10ms if total duration is available
								//		else file offset in bytes
);

// ASCII code version
char*
AACDec_GetMP4TAG_A(
	E_AACDEC_MP4TAG eTagIdx
);

// Unicode version
bool							// [out]true: Have this info, false: No this info
AACDec_GetMP4TAG(
	E_AACDEC_MP4TAG eTagIdx,
	unsigned short*	pwzBuffer,
	unsigned int	u32BufferLen
);

unsigned int
AACDec_GetVersion(void);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __AACDECODER_H__

/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp.  All rights reserved. *
 *                                                              *
 ****************************************************************/


#ifndef	__AACDECODER_CALLBACK_H__
#define __AACDECODER_CALLBACK_H__

#include "AACDecoder.h"

#ifdef __cplusplus
extern "C" {
#endif


unsigned int
AACDecoder_Read_Callback(
	void			*pFile,
	void			*pBuffer,
	unsigned int	u32Length
);

unsigned int
AACDecoder_Seek_Callback(
	void				*pFile, 
	unsigned long long	u64Position
);

E_AACDEC_FLOW
AACDecoder_FirstFrame_Callback(
	void				*pUser_data,
	char				*pcPcmbuf,
	unsigned int		u32Pcmbuf_length,
	S_AACDEC_FRAME_INFO	*psFrame_info
);

E_AACDEC_FLOW
AACDecoder_Frame_Callback(
	void				*pUser_data,
	char				*pcPcmbuf,
	unsigned int		u32Pcmbuf_length,
	S_AACDEC_FRAME_INFO	*psFrame_info
);

void AACDecoder_Terminal_Callback(
	void	*pUser_data
);

void AACDecoder_Info_Callback(
	void				*pUser_data,
	S_AACDEC_FRAME_INFO	*psFrame_info
);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __AACDECODER_CALLBACK_H__

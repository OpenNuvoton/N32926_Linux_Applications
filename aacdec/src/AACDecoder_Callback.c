/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp.  All rights reserved. *
 *                                                              *
 ****************************************************************/

#include <stdio.h>
#include "AACDecoder_Callback.h"
#include "AACPlayer_Agent.h"


unsigned int
AACDecoder_Read_Callback(
	void			*pFile,
	void			*pBuffer,
	unsigned int	u32Length
) {
	return fread(pBuffer, 1, u32Length, (FILE*)pFile);
} // AACDecoder_Read_Callback()


unsigned int
AACDecoder_Seek_Callback(
	void				*pFile,
	unsigned long long	u64Position
) {
	const unsigned long	u32CurPosition = ftell((FILE*)pFile);

	if (u64Position < u32CurPosition) {
		AACPLAYER_DEBUG_MSG("Backward seek to %lld from %ld bytes offset", u64Position, u32CurPosition);
	}

	return fseek((FILE*)pFile, u64Position, SEEK_SET);
} // AACDecoder_Seek_Callback()


E_AACDEC_FLOW
AACDecoder_FirstFrame_Callback(
	void				*pUser_data,
	char				*pcPcmbuf,
	unsigned int		u32Pcmbuf_length,
	S_AACDEC_FRAME_INFO	*psFrame_info
) {
	return eAACDEC_FLOW_CONTINUE;
} // AACDecoder_FirstFrame_Callback()


E_AACDEC_FLOW
AACDecoder_Frame_Callback(
	void				*pUser_data,
	char				*pcPcmbuf,
	unsigned int		u32Pcmbuf_length,
	S_AACDEC_FRAME_INFO	*psFrame_info
) {
	return eAACDEC_FLOW_CONTINUE;
} // AACDecoder_Frame_Callback()


void AACDecoder_Terminal_Callback(
	void	*pUser_data
) {
} // AACDecoder_Terminal_Callback()


void AACDecoder_Info_Callback(
	void				*pUser_data,
	S_AACDEC_FRAME_INFO	*psFrame_info
) {
} // AACDecoder_Info_Callback()

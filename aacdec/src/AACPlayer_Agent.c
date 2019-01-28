/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp.  All rights reserved. *
 *                                                              *
 ****************************************************************/


#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "AACPlayer_Agent.h"
#include "AACDecoder_Callback.h"


// ==================================================
// Constant definition
// ==================================================
#define DECFRAME_BUFNUM	256		// Number of decoded frame buffer


// ==================================================
// Private function declaration
// ==================================================
void s_DecodeThread(void);
void s_PlaybackThread(void);

#ifndef AACPLAYER_PSUEDO_AUDIO
void s_CloseAudioPlayDevice(void);

bool						// [out]true/false: Successful/Failed
s_OpenAudioPlayDevice(void);
#endif


// ==================================================
// Public global variable definition
// ==================================================
volatile E_AACPLAYER_STATE	g_eAACPlayer_State = eAACPLAYER_STATE_STOPPED;
volatile unsigned int		g_u32AACPlayer_PlaySampleCnt = 0;

pthread_mutex_t		g_sPlayerStateMutex;
S_AACDEC_FRAME_INFO	g_sAACDec_FrameInfo;
unsigned int		g_u32AACPlayer_ChannelNum = 0,
					g_u32AACPlayer_SampleRate = 0,
					g_u32AACPlayer_Volume = AACPLAYER_VOLUME_DEFAULT;


// ==================================================
// Private global variable definition
// ==================================================
S_AACDEC_FILE_CALLBACK	s_sAACDec_Callback;
FILE						*s_psAACFile = NULL;
int						s_i32DevDSP_BlockSize = 0;
#ifndef AACPLAYER_PSUEDO_AUDIO
int						s_i32DevDSP = -1;
#endif

volatile bool	s_bDecodeThread_Alive = false,
						s_bPlaybackThread_Alive = false;

// Decoded frame buffer
volatile char	*s_apcDecFrame_Buf[DECFRAME_BUFNUM];		// Array of pointer to decoded frame buffer
volatile int		s_ai32DecFrame_BufSize[DECFRAME_BUFNUM],	// Array of byte size of decoded frame buffer
						s_ai32DecFrame_PCMSize[DECFRAME_BUFNUM],	// Array of byte size of PCM data in decoded frame buffer
						s_i32DecFrame_ReadIdx = 0,
						s_i32DecFrame_WriteIdx = 0,
						s_i32DecFrame_FullNum = 0;					// Number of full decoded frame buffer

//FILE *g_file_process = NULL;
// ==================================================
// Plublic function implementation
// ==================================================
bool						// [out]true/false: Successful/Failed
AACPlayer_PlayFile
(
	char* 	pczFileName		// [in] File name
) {
	pthread_mutex_lock(&g_sPlayerStateMutex);
	AACPLAYER_DEBUG_MSG("ENTER: file %s, state %d", pczFileName, g_eAACPlayer_State);

	if (!pczFileName || g_eAACPlayer_State != eAACPLAYER_STATE_STOPPED) {
		pthread_mutex_unlock(&g_sPlayerStateMutex);
		return false;
	} // if

	// Immediately change state to avoid reentry
	g_eAACPlayer_State = eAACPLAYER_STATE_PLAYING;
	AACPLAYER_DEBUG_MSG("State %d (playing)", g_eAACPlayer_State);
	pthread_mutex_unlock(&g_sPlayerStateMutex);

	bool	bResult = true;

	// Open file
	s_psAACFile = fopen(pczFileName, "rb");

	if (s_psAACFile == NULL) {
		perror(pczFileName);
		bResult = false;
		goto EndOfPlayFile;
	} // if

	// Initialize decode frame buffers before decode thread started
	s_i32DecFrame_ReadIdx = s_i32DecFrame_WriteIdx = s_i32DecFrame_FullNum = 0;
	memset(s_apcDecFrame_Buf, 0, sizeof(s_apcDecFrame_Buf));
	memset((void*)s_ai32DecFrame_BufSize, 0, sizeof(s_ai32DecFrame_BufSize));

	// Create decode thread
	pthread_t 	sDecodeThread;

	// Make sure precharge even child thread starts slower than parent thread
	s_bDecodeThread_Alive = true;

	if (pthread_create(&sDecodeThread, NULL, (void*)s_DecodeThread, NULL) != 0) {
		s_bDecodeThread_Alive = false;
		perror("pthread_create(DecodeThread) failed");
		bResult = false;
		goto EndOfPlayFile;
	} // if

	// Precharge decoded frame buffers
	// (NOT detect child thread handle becuase it is slowly updated by parent thread)
	AACPLAYER_DEBUG_MSG("To wait precharge buffers, d-thread %d", s_bDecodeThread_Alive);

	do {
		// Force task switch to avoid busy polling
		struct timespec	sSleepTime = { 0, 0 };
		nanosleep(&sSleepTime, NULL);
	} while (s_bDecodeThread_Alive && s_i32DecFrame_FullNum < DECFRAME_BUFNUM);

	// Precharge done or decoding failed
	if (s_i32DecFrame_FullNum > 0) {
		// Create playback thread if has decoded data
		pthread_mutex_lock(&g_sPlayerStateMutex);
		AACPLAYER_DEBUG_MSG("Precharged %d buffers: d-thread %d, state %d", s_i32DecFrame_FullNum, s_bDecodeThread_Alive, g_eAACPlayer_State);
		pthread_mutex_unlock(&g_sPlayerStateMutex);

		pthread_t	sPlaybackThread;
		s_bPlaybackThread_Alive = false;

		while (pthread_create(&sPlaybackThread, NULL, (void*)s_PlaybackThread, NULL) != 0) {
			perror("pthread_create(PlaybackThread) failed");
			bResult = false;
		} // if
	} // if
	else {
		// No decoded data
		pthread_mutex_lock(&g_sPlayerStateMutex);
		AACPLAYER_DEBUG_MSG("No buffer full, d-thread %d, state %d, to stop player", s_bDecodeThread_Alive, g_eAACPlayer_State);
		pthread_mutex_unlock(&g_sPlayerStateMutex);
		bResult = false;
	} // else

EndOfPlayFile:
	if (bResult == false)
		AACPlayer_Stop();

	pthread_mutex_lock(&g_sPlayerStateMutex);
	AACPLAYER_DEBUG_MSG("LEAVE: state %d", g_eAACPlayer_State);
	pthread_mutex_unlock(&g_sPlayerStateMutex);
	return bResult;
} // AACPlayer_PlayFile()


bool							// [out]true/false: Successful/Failed
AACPlayer_ChangeVolume(
	int		i32NewVolume
) {
	if (i32NewVolume < AACPLAYER_VOLUME_MIN)
		i32NewVolume = AACPLAYER_VOLUME_MIN;

	if (i32NewVolume > AACPLAYER_VOLUME_MAX)
		i32NewVolume = AACPLAYER_VOLUME_MAX;

	if (i32NewVolume == g_u32AACPlayer_Volume)
		return true;

#ifndef AACPLAYER_PSUEDO_AUDIO
	const int	i32DevMixer = open("/dev/mixer", O_RDWR);

	if (i32DevMixer < 0) {
		perror("open(/dev/mixer) failed");
		return false;
	} // if

	const int	i32Volume = (i32NewVolume << 8) | i32NewVolume;	// H/L: Right/Left

	if (ioctl(i32DevMixer, MIXER_WRITE(SOUND_MIXER_PCM), &i32Volume) < 0) {
		perror("ioctl(/dev/mixer, MIXER_WRITE(SOUND_MIXER_PCM)) failed");
		close(i32DevMixer);
		return false;
	} // if

	close(i32DevMixer);
#endif

	g_u32AACPlayer_Volume = i32NewVolume;
	return true;
} // AACPlayer_ChangeVolume()


bool							// [out]true/false: Successful/Failed
AACPlayer_Pause(void) {
	pthread_mutex_lock(&g_sPlayerStateMutex);

	if (g_eAACPlayer_State != eAACPLAYER_STATE_PLAYING) {
		pthread_mutex_unlock(&g_sPlayerStateMutex);
		return false;
	} // if

	// Immediately change state to avoid reentry
	g_eAACPlayer_State = eAACPLAYER_STATE_TOPAUSE;
	AACPLAYER_DEBUG_MSG("State %d (to-pause)", g_eAACPlayer_State);
	pthread_mutex_unlock(&g_sPlayerStateMutex);

	// Wait for playback thread suspended
	while (1) {
		// Force task switch to avoid busy polling
		struct timespec	sSleepTime = { 0, 0 };
		nanosleep(&sSleepTime, NULL);
		pthread_mutex_lock(&g_sPlayerStateMutex);

		if (g_eAACPlayer_State == eAACPLAYER_STATE_PAUSED) {
			pthread_mutex_unlock(&g_sPlayerStateMutex);
			break;
		} // if

		pthread_mutex_unlock(&g_sPlayerStateMutex);
	} // while 1

	return true;
} // AACPlayer_Pause()


bool							// [out]true/false: Successful/Failed
AACPlayer_Resume(void) {
	pthread_mutex_lock(&g_sPlayerStateMutex);

	if (g_eAACPlayer_State != eAACPLAYER_STATE_PAUSED) {
		pthread_mutex_unlock(&g_sPlayerStateMutex);
		return false;
	} // if

	// Immediately change state to avoid reentry
	g_eAACPlayer_State = eAACPLAYER_STATE_TORESUME;
	AACPLAYER_DEBUG_MSG("State %d (to-resume)", g_eAACPlayer_State);
	pthread_mutex_unlock(&g_sPlayerStateMutex);

	// Wait for playback thread resumed
	while (1) {
		// Force task switch to avoid busy polling
		struct timespec	sSleepTime = { 0, 0 };
		nanosleep(&sSleepTime, NULL);

		if (g_eAACPlayer_State == eAACPLAYER_STATE_PLAYING) {
			pthread_mutex_unlock(&g_sPlayerStateMutex);
			break;
		} // if

		pthread_mutex_unlock(&g_sPlayerStateMutex);
	} // while 1

	return true;
} // AACPlayer_Resume()


bool							// [out]true/false: Successful/Failed
AACPlayer_Seek(
	int i32DeltaIn10ms			// [in] Time position delta in 10ms
								//			> 0: Forward seek
								//			< 0: Backward seek
) {
	pthread_mutex_lock(&g_sPlayerStateMutex);

	if (g_eAACPlayer_State != eAACPLAYER_STATE_PLAYING) {
		pthread_mutex_unlock(&g_sPlayerStateMutex);
		return false;
	} // if

	pthread_mutex_unlock(&g_sPlayerStateMutex);

	if (i32DeltaIn10ms == 0)
		return true;

	unsigned int	u32MaxPosition, u32NewPosition;

	if (g_sAACDec_FrameInfo.m_u32Total_duration)
		u32MaxPosition = g_sAACDec_FrameInfo.m_u32Total_duration;
	else {
		// Position is file offset if no total duration info
		u32MaxPosition = s_sAACDec_Callback.m_u32File_size;
		i32DeltaIn10ms = g_sAACDec_FrameInfo.m_u32Samplerate / 400 * i32DeltaIn10ms;
	} // else

	u32NewPosition = g_sAACDec_FrameInfo.m_u32Cur_position + i32DeltaIn10ms;

	if (i32DeltaIn10ms > 0 && u32NewPosition >= u32MaxPosition) {
		AACPlayer_Stop();
		return true;
	} // if

	if (i32DeltaIn10ms < 0 && g_sAACDec_FrameInfo.m_u32Cur_position < -i32DeltaIn10ms)
		u32NewPosition = 0;

	AACDec_SetPosition(u32NewPosition);
	return true;
} // AACPlayer_Seek()


void AACPlayer_Stop(void) {
	// Stop decode and playback threads anyway
	pthread_mutex_lock(&g_sPlayerStateMutex);
	AACPLAYER_DEBUG_MSG("To wait d-thread %d and p-thread %d ended, state %d", s_bDecodeThread_Alive, s_bPlaybackThread_Alive, g_eAACPlayer_State);

	if (s_bDecodeThread_Alive || s_bPlaybackThread_Alive) {
		g_eAACPlayer_State = eAACPLAYER_STATE_TOSTOP;
		AACPLAYER_DEBUG_MSG("State %d (to-stop)", g_eAACPlayer_State);
		pthread_mutex_unlock(&g_sPlayerStateMutex);

		// Wait for decode and playback threads terminated
		do {
			// Force task switch to avoid busy polling
			struct timespec	sSleepTime = { 0, 0 };
			nanosleep(&sSleepTime, NULL);
		} while (s_bDecodeThread_Alive || s_bPlaybackThread_Alive);
	} // if

	// Free decoded frame buffers
	AACPLAYER_DEBUG_MSG("To free buffers, state %d", g_eAACPlayer_State);
	pthread_mutex_unlock(&g_sPlayerStateMutex);

	int	i32Index;
	memset((void*)s_ai32DecFrame_BufSize, 0, sizeof(s_ai32DecFrame_BufSize));

	for (i32Index = DECFRAME_BUFNUM - 1; i32Index >= 0; i32Index--)
		if (s_apcDecFrame_Buf[i32Index]) {
			free((void*)(s_apcDecFrame_Buf[i32Index]));
			s_apcDecFrame_Buf[i32Index] = NULL;
		} // if

	// Close file
	if (s_psAACFile) {
		fclose(s_psAACFile);
		s_psAACFile = NULL;
	} // if

	pthread_mutex_lock(&g_sPlayerStateMutex);
	g_eAACPlayer_State = eAACPLAYER_STATE_STOPPED;
	AACPLAYER_DEBUG_MSG("State %d (stopped)", g_eAACPlayer_State);
	pthread_mutex_unlock(&g_sPlayerStateMutex);
} // AACPlayer_Stop()


// ==================================================
// Private Function implementation
// ==================================================

#ifndef AACPLAYER_PSUEDO_AUDIO
void s_CloseAudioPlayDevice(void) {
	if (s_i32DevDSP >= 0) {
		close(s_i32DevDSP);
		s_i32DevDSP = -1;
	} // if
/*
        if ( g_file_process != NULL )
        {
	fclose(g_file_process);
    printf("Close TEst file\n");
    g_file_process = NULL;
        }
*/
} // s_CloseAudioPlayDevice()


bool					// [out]true/false: Successful/Failed
s_OpenAudioPlayDevice(void) {
	s_i32DevDSP_BlockSize = 0;
	s_i32DevDSP = open("/dev/dsp", O_RDWR);

	if (s_i32DevDSP < 0) {
		s_i32DevDSP = open("/dev/dsp2", O_RDWR);

		if (s_i32DevDSP < 0) {
			fprintf(stderr, "Failed to open any dsp device\n");
			return false;
		} // if
	} // if

	const int	i32DevMixer = open("/dev/mixer", O_RDWR);

	if (i32DevMixer < 0) {
		perror("open(/dev/mixer) failed");
		s_CloseAudioPlayDevice();
		return false;
	} // if

	const int	i32OSS_Format = AFMT_S16_LE,
			i32Volume = (g_u32AACPlayer_Volume << 8) | g_u32AACPlayer_Volume;	// H/L: Right/Left
//        int 	data = 0x5050;
//	i32Volume = 0x6060;
//printf("\n Volume setting = %d\n", i32Volume);
	ioctl(s_i32DevDSP, SNDCTL_DSP_SETFMT, &i32OSS_Format);
	ioctl(i32DevMixer, MIXER_WRITE(SOUND_MIXER_PCM), &i32Volume);
//	ioctl(i32DevMixer, MIXER_WRITE(SOUND_MIXER_PCM), &data);
	ioctl(s_i32DevDSP, SNDCTL_DSP_SPEED, &g_u32AACPlayer_SampleRate);
	ioctl(s_i32DevDSP, SNDCTL_DSP_CHANNELS, &g_u32AACPlayer_ChannelNum);
	ioctl(s_i32DevDSP, SNDCTL_DSP_GETBLKSIZE, &s_i32DevDSP_BlockSize);

	close(i32DevMixer);

	return true;
} // s_OpenAudioPlayDevice()
#endif // !AACPLAYER_PSUEDO_AUDIO


void s_DecodeThread(void) {
	s_bDecodeThread_Alive = true;
	pthread_detach(pthread_self());
	pthread_mutex_lock(&g_sPlayerStateMutex);
	AACPLAYER_DEBUG_MSG("ENTER: d-thread %d, state %d", s_bDecodeThread_Alive, g_eAACPlayer_State);
/*
if ( g_file_process == NULL )
{
g_file_process = fopen("processD.txt", "w");
if ( g_file_process == NULL )
printf("Open Test file to be failed\n");
else
printf("Open Test file OK\n");
}
*/

	// Terminate decode thread if immediately stop after play
	if (g_eAACPlayer_State != eAACPLAYER_STATE_PLAYING) {
		pthread_mutex_unlock(&g_sPlayerStateMutex);
		s_bDecodeThread_Alive = false;
		return;
	} // if
	pthread_mutex_unlock(&g_sPlayerStateMutex);

	struct stat	sFileStat;

	if (fstat(fileno(s_psAACFile), &sFileStat) < 0) {
		perror("fstat() failed");
		goto EndOfDecodeThread;
	} // if

	// Prepare s_sAACDec_Callback
	s_sAACDec_Callback.m_pfnRead = AACDecoder_Read_Callback;
	s_sAACDec_Callback.m_pfnSeek = AACDecoder_Seek_Callback;
	s_sAACDec_Callback.m_pfnInfo_callback = AACDecoder_Info_Callback;
	s_sAACDec_Callback.m_pfnFirst_frame_callback = AACDecoder_FirstFrame_Callback;
	s_sAACDec_Callback.m_pfnFrame_callback = AACDecoder_Frame_Callback;
	s_sAACDec_Callback.m_pfnTerminal_callback = AACDecoder_Terminal_Callback;
	s_sAACDec_Callback.m_pFile = (void*)s_psAACFile;
	s_sAACDec_Callback.m_u32File_size = sFileStat.st_size;

	// Pre-scan default frames in multi-pass to get bitrate, duration, and frames
	// g_sAACDec_FrameInfo.m_i32Prescan_frames (Number of prescan frames):
	//		eAACDEC_PRESCAN_NONE	=  0		/* Not prescan */
	//		eAACDEC_PRESCAN_ALL		= -1		/* Prescan all frames in only one pass */
	//		eAACDEC_PRESCAN_DEFAULT	= -2		/* Prescan default number of frames in each pass */
	//		other values > 0					/* Prescan customized frames in each pass */
	g_sAACDec_FrameInfo.m_i32Prescan_frames = eAACDEC_PRESCAN_DEFAULT;

	// Initialize AAC Decoder in multi-pass
	bool	bIsParseEnd = false;
	AACPLAYER_DEBUG_MSG("To init AACDec, d-thread %d", s_bDecodeThread_Alive);

	do {
		if (false == AACDec_Initialize(&s_sAACDec_Callback, &g_sAACDec_FrameInfo, &bIsParseEnd)) {
			fprintf(stderr, "AAC Player: Fail to initialize AAC Decoder: Error code 0x%08x\n", g_sAACDec_FrameInfo.m_eError);

			// Still require to finalize AAC Decoder
			AACDec_Finalize();
			goto EndOfDecodeThread;
		} // if
	} while (bIsParseEnd == false);

	AACPLAYER_DEBUG_MSG("Init AACDec done, d-thread %d", s_bDecodeThread_Alive);

	// Backup info because g_sAACDec_FrameInfo will be cleared before DecodeFrame
	g_u32AACPlayer_ChannelNum = g_sAACDec_FrameInfo.m_u32Channels;
	g_u32AACPlayer_SampleRate = g_sAACDec_FrameInfo.m_u32Samplerate;

	bool	bContinueDecode = true;

	while (bContinueDecode) {
		// Check command
		pthread_mutex_lock(&g_sPlayerStateMutex);

		if (g_eAACPlayer_State == eAACPLAYER_STATE_TOSTOP) {
			pthread_mutex_unlock(&g_sPlayerStateMutex);
			break;
		} // if

		pthread_mutex_unlock(&g_sPlayerStateMutex);

		// Fill decoded frame buffer even if paused
//fprintf(g_file_process, "Write Before writing --> %d, %d\n", s_i32DecFrame_FullNum, s_i32DecFrame_WriteIdx);
		if (s_i32DecFrame_FullNum < DECFRAME_BUFNUM) {
			bContinueDecode = AACDec_DecodeFrame();

			if (g_sAACDec_FrameInfo.m_eError == eAACDEC_ERROR_NONE) {
				if (g_sAACDec_FrameInfo.m_u32Samples <= 0)
					continue;

				const int	i32DecodedSize = 2 * g_sAACDec_FrameInfo.m_u32Samples * g_sAACDec_FrameInfo.m_u32Channels;

				if (s_ai32DecFrame_BufSize[s_i32DecFrame_WriteIdx] < i32DecodedSize) {
					AACPLAYER_DEBUG_MSG("To allocate buffer %d: %d bytes", s_i32DecFrame_WriteIdx, i32DecodedSize);
					s_apcDecFrame_Buf[s_i32DecFrame_WriteIdx] = realloc((void*)(s_apcDecFrame_Buf[s_i32DecFrame_WriteIdx]), i32DecodedSize);

					if (s_apcDecFrame_Buf[s_i32DecFrame_WriteIdx] == NULL) {
						perror("realloc() failed");
						break;
					} // if

					s_ai32DecFrame_BufSize[s_i32DecFrame_WriteIdx] = i32DecodedSize;
				} // if

				memcpy((void*)(s_apcDecFrame_Buf[s_i32DecFrame_WriteIdx]), g_sAACDec_FrameInfo.m_pcSample_buffer, i32DecodedSize);
				s_ai32DecFrame_PCMSize[s_i32DecFrame_WriteIdx] = i32DecodedSize;
				s_i32DecFrame_FullNum++;
				AACPLAYER_DEBUG_MSG("Decoded %d bytes into buffer %d (%d buffers full)", i32DecodedSize, s_i32DecFrame_WriteIdx, s_i32DecFrame_FullNum);
//				s_i32DecFrame_WriteIdx = (s_i32DecFrame_WriteIdx + 1) % DECFRAME_BUFNUM;
                s_i32DecFrame_WriteIdx++;
                if ( s_i32DecFrame_WriteIdx == DECFRAME_BUFNUM )
					s_i32DecFrame_WriteIdx = 0;

//fprintf(g_file_process, "Write Cur Full --> %d, %d\n", s_i32DecFrame_FullNum, s_i32DecFrame_WriteIdx);
			} // if g_sAACDec_FrameInfo.m_eError == eAACDEC_ERROR_NONE
			else {
				fprintf(stderr, "AAC Player: Fail to decode file: Error code 0x%08x\n", g_sAACDec_FrameInfo.m_eError);
				break;
			} // else
		} // if s_i32DecFrame_FullNum < DECFRAME_BUFNUM
		else {
			// Sleep if decoded frame buffers full
			struct timespec	sSleepTime = { 0, 0 };
			nanosleep(&sSleepTime, NULL);
		} // else
	} // while bContinueDecode

	// Finalize AAC Decoder
	AACDec_Finalize();

EndOfDecodeThread:
	s_bDecodeThread_Alive = false;
	pthread_mutex_lock(&g_sPlayerStateMutex);
	AACPLAYER_DEBUG_MSG("LEAVE: d-thread %d, state %d", s_bDecodeThread_Alive, g_eAACPlayer_State);
	pthread_mutex_unlock(&g_sPlayerStateMutex);
} // s_DecodeThread()


void s_PlaybackThread(void) {
	s_bPlaybackThread_Alive = true;
	pthread_detach(pthread_self());
	pthread_mutex_lock(&g_sPlayerStateMutex);
	AACPLAYER_DEBUG_MSG("ENTER: p-thread %d, state %d", s_bPlaybackThread_Alive, g_eAACPlayer_State);
	pthread_mutex_unlock(&g_sPlayerStateMutex);

#ifndef AACPLAYER_PSUEDO_AUDIO
	// Initialize audio playback device
	s_OpenAudioPlayDevice();
#else
	s_i32DevDSP_BlockSize = 0x2000;		// Assumed 8192 bytes
#endif

	if (s_i32DevDSP_BlockSize > 0) {
		AACPLAYER_DEBUG_MSG("Playback device block %d bytes", s_i32DevDSP_BlockSize);
		g_u32AACPlayer_PlaySampleCnt = 0;

		while (1) {
			// Check command
			pthread_mutex_lock(&g_sPlayerStateMutex);

			if (g_eAACPlayer_State == eAACPLAYER_STATE_TOSTOP) {
				pthread_mutex_unlock(&g_sPlayerStateMutex);

				// Stop playback after decoding ended
				break;
			} // if

			if (g_eAACPlayer_State == eAACPLAYER_STATE_TOPAUSE) {
#ifndef AACPLAYER_PSUEDO_AUDIO
				// Close audio playback device because SNDCTL_DSP_SETTRIGGER results in pause
#if 1
				s_CloseAudioPlayDevice();
#else
				int	i32Trigger = 0;
				ioctl(s_i32DevDSP, SNDCTL_DSP_SETTRIGGER, &i32Trigger);
#endif
#endif // !AACPLAYER_PSUEDO_AUDIO
				g_eAACPlayer_State = eAACPLAYER_STATE_PAUSED;
				AACPLAYER_DEBUG_MSG("State %d (paused)", g_eAACPlayer_State);
				pthread_mutex_unlock(&g_sPlayerStateMutex);
				continue;
			} // if

			if (g_eAACPlayer_State == eAACPLAYER_STATE_PAUSED) {
				pthread_mutex_unlock(&g_sPlayerStateMutex);

				// Force task switch to avoid busy polling
				struct timespec	sSleepTime = { 0, 0 };
				nanosleep(&sSleepTime, NULL);
				continue;
			} // if

			if (g_eAACPlayer_State == eAACPLAYER_STATE_TORESUME) {
#ifndef AACPLAYER_PSUEDO_AUDIO
				// Reopen audio playback device because SNDCTL_DSP_SETTRIGGER results in pause
#if 1
				s_OpenAudioPlayDevice();
#else
				int	i32Trigger = PCM_ENABLE_OUTPUT;
				ioctl(s_i32DevDSP, SNDCTL_DSP_SETTRIGGER, &i32Trigger);
#endif
#endif // !AACPLAYER_PSUEDO_AUDIO
				g_eAACPlayer_State = eAACPLAYER_STATE_PLAYING;
				AACPLAYER_DEBUG_MSG("State %d (playing)", g_eAACPlayer_State);

				// Immediately fill internal buffer of audio playback device when resume
			} // if

			pthread_mutex_unlock(&g_sPlayerStateMutex);

			// Fill internal buffer of audio playback device
			if (s_i32DecFrame_FullNum > 0) {
				const char	*pcDecodedPCM = (char*)(s_apcDecFrame_Buf[s_i32DecFrame_ReadIdx]);
//fprintf(g_file_process, "Encoder curent --> %d, %d\n", s_i32DecFrame_FullNum, s_i32DecFrame_ReadIdx);
				while (s_ai32DecFrame_PCMSize[s_i32DecFrame_ReadIdx] > 0) {
#ifndef AACPLAYER_PSUEDO_AUDIO
					// Check fill bytes
					audio_buf_info	sDevDSP_Info;
					ioctl(s_i32DevDSP, SNDCTL_DSP_GETOSPACE, &sDevDSP_Info);

					if (sDevDSP_Info.bytes <= 0) {
						// Force task switch to avoid busy polling
						struct timespec	sSleepTime = { 0, 0 };
						nanosleep(&sSleepTime, NULL);
						continue;
					} // if

					int	i32FillBytes = s_ai32DecFrame_PCMSize[s_i32DecFrame_ReadIdx];

					if (i32FillBytes > sDevDSP_Info.bytes)
						i32FillBytes = sDevDSP_Info.bytes;

					if (i32FillBytes > s_i32DevDSP_BlockSize)
						i32FillBytes = s_i32DevDSP_BlockSize;

					// Suspend until internal buffer of audio playback device could be written
					fd_set	sWriteFDSet;
					FD_ZERO(&sWriteFDSet);
					FD_SET(s_i32DevDSP, &sWriteFDSet);
					select(s_i32DevDSP + 1, NULL, &sWriteFDSet, NULL, NULL);

					// Fill PCM data
					if (write(s_i32DevDSP, pcDecodedPCM, i32FillBytes) != i32FillBytes) {
						perror("write(/dev/dsp) failed");
						goto EndOfPlayback;		// Auto stop player
					} // if

#else // !AACPLAYER_PSUEDO_AUDIO
					int	i32FillBytes = s_ai32DecFrame_PCMSize[s_i32DecFrame_ReadIdx];

					if (i32FillBytes > s_i32DevDSP_BlockSize)
						i32FillBytes = s_i32DevDSP_BlockSize;

#endif // AACPLAYER_PSUEDO_AUDIO

					g_u32AACPlayer_PlaySampleCnt += i32FillBytes / 2 / g_u32AACPlayer_ChannelNum;
					pcDecodedPCM += i32FillBytes;
					s_ai32DecFrame_PCMSize[s_i32DecFrame_ReadIdx] -= i32FillBytes;
					AACPLAYER_DEBUG_MSG("Read %d bytes from buffer %d (%d buffers full)", i32FillBytes, s_i32DecFrame_ReadIdx, s_i32DecFrame_FullNum);
				} // while s_ai32DecFrame_PCMSize[s_i32DecFrame_ReadIdx] > 0

				s_i32DecFrame_FullNum--;
				AACPLAYER_DEBUG_MSG("Buffer %d empty (%d buffers full)", s_i32DecFrame_ReadIdx, s_i32DecFrame_FullNum);
//				s_i32DecFrame_ReadIdx = (s_i32DecFrame_ReadIdx + 1) % DECFRAME_BUFNUM;
				s_i32DecFrame_ReadIdx++;
				if ( s_i32DecFrame_ReadIdx == DECFRAME_BUFNUM )
					s_i32DecFrame_ReadIdx = 0;
//fprintf(g_file_process, "Encoder final --> %d, %d\n", s_i32DecFrame_FullNum, s_i32DecFrame_ReadIdx);
			} // if s_i32DecFrame_FullNum > 0
			else if (s_bDecodeThread_Alive) {
				// Sleep if decoded frame buffers empty and still in decoding
				struct timespec	sSleepTime = { 0, 0 };
				nanosleep(&sSleepTime, NULL);
			} // if
			else {
				// Decoding ended
				pthread_mutex_lock(&g_sPlayerStateMutex);
				AACPLAYER_DEBUG_MSG("d-thread %d ended, state %d", s_bDecodeThread_Alive, g_eAACPlayer_State);
				pthread_mutex_unlock(&g_sPlayerStateMutex);
				break;
			} // else
		} // while 1

#ifndef AACPLAYER_PSUEDO_AUDIO
EndOfPlayback:
		// Finalize audio playback device
		s_CloseAudioPlayDevice();
#endif
	} // if s_i32DevDSP_BlockSize > 0

	// Auto stop player
	s_bPlaybackThread_Alive = false;
	AACPLAYER_DEBUG_MSG("To stop player, p-thread %d", s_bPlaybackThread_Alive);
	AACPlayer_Stop();
	pthread_mutex_lock(&g_sPlayerStateMutex);
	AACPLAYER_DEBUG_MSG("LEAVE: p-thread %d, state %d", s_bPlaybackThread_Alive, g_eAACPlayer_State);
	pthread_mutex_unlock(&g_sPlayerStateMutex);
} // s_PlaybackThread()

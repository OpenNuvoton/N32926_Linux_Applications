/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp.  All rights reserved. *
 *                                                              *
 ****************************************************************/


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/stat.h>

#include "AACRecorder_Agent.h"


// ==================================================
// Constant definition
// ==================================================
#define PCMFRAME_BUFNUM		8   //64*2						// Number of PCM frame buffer
#define PCMFRAME_BUFSIZE	(2 * AACENC_FRAMESIZE)	// Bytes of 1-ch PCM frame buffer
#define ENCFRAME_BUFSIZE	(PCMFRAME_BUFSIZE / 2)	// Bytes of encoded frame buffer


// ==================================================
// Private function declaration
// ==================================================
extern int CheckKey(int	i32DevKeypad);
void s_EncodeThread(int	i32DevKeypad);


#ifndef AACRECORDER_PSUEDO_AUDIO
void s_CloseAudioRecDevice(void);

bool						// [out]true/false: Successful/Failed
s_OpenAudioRecDevice(
	int	i32SampleRate,
	int	i32ChannelNum
);
#endif


// ==================================================
// Public global variable definition
// ==================================================
E_AACRECORDER_STATE	g_eAACRecorder_State = eAACRECORDER_STATE_STOPPED;
unsigned int			g_u32AACRecorder_EncFrameCnt = 0,
						g_u32AACRecorder_RecFrameCnt = 0;

//pthread_mutex_t					g_sRecorderStateMutex;


// ==================================================
// Private global variable definition
// ==================================================
FILE				*s_psAACFile = NULL;
int				s_i32DevDSP_BlockSize = 0;
#ifndef AACRECORDER_PSUEDO_AUDIO
int				s_i32DevDSP = -1;
#endif

volatile bool	s_bEncodeThread_Alive = false;

// PCM frame buffer
//static volatile char
volatile char	*s_apcPCMFrame_Buf[PCMFRAME_BUFNUM];		// Array of pointer to PCM frame buffer
//static volatile int
volatile int		s_ai32PCMFrame_PCMSize[PCMFRAME_BUFNUM];	// Array of byte size of PCM data in PCM frame buffer
int		s_i32PCMFrame_FullNum = 0;					// Number of full PCM frame buffer

int		s_i32PCMFrame_ReadIdx = 0;
int		s_i32PCMFrame_WriteIdx = 0;
int s_i32no;
//FILE *g_file_process;
//FILE *g_file_data;
extern char g_strInputFile[512];
// ==================================================
// Plublic function implementation
// ==================================================
bool						// [out]true/false: Successful/Failed
AACRecorder_RecordFile
(
    char* 	pczFileName		// [in] File name
)
{
//	pthread_mutex_lock(&g_sRecorderStateMutex);
	AACRECORDER_DEBUG_MSG("ENTER: file %s, state %d", pczFileName, g_eAACRecorder_State);
	if (!pczFileName || g_eAACRecorder_State != eAACRECORDER_STATE_STOPPED) {
//		pthread_mutex_unlock(&g_sRecorderStateMutex);
		return false;
	} // if

	// Immediately change state to avoid reentry
	g_eAACRecorder_State = eAACRECORDER_STATE_RECORDING;
	AACRECORDER_DEBUG_MSG("State %d (recording)", g_eAACRecorder_State);
//	pthread_mutex_unlock(&g_sRecorderStateMutex);

	bool	bResult = true;

	// Open file
	s_psAACFile = fopen(pczFileName, "wb");
	if (s_psAACFile == NULL) {
		perror(pczFileName);
		bResult = false;
		goto EndOfRecordFile;
	} // if
	
	// Initialize PCM frame buffers before record thread started
	int	i32Index;

	AACRECORDER_DEBUG_MSG("To allocate PCM buffers: %d bytes * %d", PCMFRAME_BUFSIZE, PCMFRAME_BUFNUM);
	s_i32PCMFrame_ReadIdx = s_i32PCMFrame_WriteIdx = s_i32PCMFrame_FullNum = 0;
	memset((void*)s_ai32PCMFrame_PCMSize, 0, sizeof(s_ai32PCMFrame_PCMSize));
	for (i32Index = 0; i32Index < PCMFRAME_BUFNUM; i32Index++) {
		s_apcPCMFrame_Buf[i32Index] = malloc(PCMFRAME_BUFSIZE);
		if (s_apcPCMFrame_Buf[i32Index] == NULL) {
			perror("malloc()");
			bResult = false;
			goto EndOfRecordFile;
		} // if
	} // for

EndOfRecordFile:
	if (bResult == false)
		AACRecorder_Stop();

//	pthread_mutex_lock(&g_sRecorderStateMutex);
	AACRECORDER_DEBUG_MSG("LEAVE: state %d", g_eAACRecorder_State);
//	pthread_mutex_unlock(&g_sRecorderStateMutex);

	return bResult;
} // AACRecorder_RecordFile()


bool							// [out]true/false: Successful/Failed
AACRecorder_Pause(void) {
//	pthread_mutex_lock(&g_sRecorderStateMutex);
	if (g_eAACRecorder_State != eAACRECORDER_STATE_RECORDING) {
//		pthread_mutex_unlock(&g_sRecorderStateMutex);
		return false;
	} // if

	// Immediately change state to avoid reentry
	g_eAACRecorder_State = eAACRECORDER_STATE_TOPAUSE;
	AACRECORDER_DEBUG_MSG("State %d (to-pause)", g_eAACRecorder_State);
//	pthread_mutex_unlock(&g_sRecorderStateMutex);

	// Wait for record thread suspended
	while (1) {
//		pthread_mutex_lock(&g_sRecorderStateMutex);
		if (g_eAACRecorder_State == eAACRECORDER_STATE_PAUSED) {
//			pthread_mutex_unlock(&g_sRecorderStateMutex);
			break;
		} // if
//		pthread_mutex_unlock(&g_sRecorderStateMutex);
	} // while 1

	return true;
} // AACRecorder_Pause()


bool							// [out]true/false: Successful/Failed
AACRecorder_Resume(void) {
//	pthread_mutex_lock(&g_sRecorderStateMutex);
	if (g_eAACRecorder_State != eAACRECORDER_STATE_PAUSED) {
//		pthread_mutex_unlock(&g_sRecorderStateMutex);
		return false;
	} // if

	// Immediately change state to avoid reentry
	g_eAACRecorder_State = eAACRECORDER_STATE_TORESUME;
	AACRECORDER_DEBUG_MSG("State %d (to-resume)", g_eAACRecorder_State);
//	pthread_mutex_unlock(&g_sRecorderStateMutex);

	// Wait for record thread resumed
	while (1) {
//		pthread_mutex_lock(&g_sRecorderStateMutex);
		if (g_eAACRecorder_State == eAACRECORDER_STATE_RECORDING) {
//			pthread_mutex_unlock(&g_sRecorderStateMutex);
			break;
		} // if
//		pthread_mutex_unlock(&g_sRecorderStateMutex);
	} // while 1

	return true;
} // AACRecorder_Resume()


void AACRecorder_Stop(void) {
	// Stop record and encode threads anyway
//	pthread_mutex_lock(&g_sRecorderStateMutex);
	AACRECORDER_DEBUG_MSG("To wait e-thread %d ended, state %d", s_bEncodeThread_Alive, g_eAACRecorder_State);
	if ( s_bEncodeThread_Alive) {
		g_eAACRecorder_State = eAACRECORDER_STATE_TOSTOP;
		AACRECORDER_DEBUG_MSG("State %d (to-stop)", g_eAACRecorder_State);
//		pthread_mutex_unlock(&g_sRecorderStateMutex);

	} // if

	// Free PCM frame buffers
	AACRECORDER_DEBUG_MSG("To free PCM buffers, state %d", g_eAACRecorder_State);
//	pthread_mutex_unlock(&g_sRecorderStateMutex);

	int	i32Index;

	memset((void*)s_ai32PCMFrame_PCMSize, 0, sizeof(s_ai32PCMFrame_PCMSize));
	for (i32Index = PCMFRAME_BUFNUM - 1; i32Index >= 0; i32Index--)
		if (s_apcPCMFrame_Buf[i32Index]) {
			free((void*)(s_apcPCMFrame_Buf[i32Index]));
			s_apcPCMFrame_Buf[i32Index] = NULL;
		} // if

	// Close file
	if (s_psAACFile) {
		fclose(s_psAACFile);
		s_psAACFile = NULL;
		sync();
	} // if

//	pthread_mutex_lock(&g_sRecorderStateMutex);
	g_eAACRecorder_State = eAACRECORDER_STATE_STOPPED;
	AACRECORDER_DEBUG_MSG("State %d (stopped)", g_eAACRecorder_State);
//	pthread_mutex_unlock(&g_sRecorderStateMutex);
} // AACRecorder_Stop()


// ==================================================
// Private Function implementation
// ==================================================

#ifndef AACRECORDER_PSUEDO_AUDIO
void s_CloseAudioRecDevice(void) {
	if (s_i32DevDSP >= 0) {
		close(s_i32DevDSP);
		s_i32DevDSP = -1;
	} // if
} // s_CloseAudioRecDevice()


bool					// [out]true/false: Successful/Failed
s_OpenAudioRecDevice(
	int	i32SampleRate,
	int	i32ChannelNum
) {
	s_i32DevDSP_BlockSize = 0;
#if 0
	s_i32DevDSP = open("/dev/dsp1", O_RDWR);
	if (s_i32DevDSP < 0) {
		perror("/dev/dsp1");
		return false;
	} // if

	ioctl(s_i32DevDSP, SNDCTL_DSP_SETFMT, AFMT_S16_LE);
	fcntl(s_i32DevDSP, F_SETFL, O_NONBLOCK);

	if (ioctl(s_i32DevDSP, SNDCTL_DSP_SPEED, &i32SampleRate) < 0) {
		printf("AAC Recorder: Unsupported sample rate: %d\n", i32SampleRate);
		s_CloseAudioRecDevice();
		return false;
	} // if

	ioctl(s_i32DevDSP, SNDCTL_DSP_CHANNELS, &i32ChannelNum);
	ioctl(s_i32DevDSP, SNDCTL_DSP_GETBLKSIZE, &s_i32DevDSP_BlockSize);
#endif
	struct stat statbuf;
	int len;
    if (stat(g_strInputFile, &statbuf) == -1) {
       /* check the value of errno */
	}
    len = statbuf.st_size;

//	s_i32DevDSP = open("/tmp/test.pcm", O_RDONLY);
	s_i32DevDSP = open(g_strInputFile, O_RDONLY);
	if (s_i32DevDSP < 0) {
		printf("Can not open %s\n", g_strInputFile);
		return false;
	} // if
	s_i32DevDSP_BlockSize = 0x800;		// Assumed 8192 bytes
//        s_i32no = (16000*2*20/s_i32DevDSP_BlockSize)+1;
	s_i32no = (len/s_i32DevDSP_BlockSize)+1;
	int pos = lseek(s_i32DevDSP, 0x2C, SEEK_SET);
	if ( pos == -1 )
	{
		printf("seek position error\n");
		return false;
	}
	return true;
} // s_OpenAudioRecDevice()
#endif // !AACRECORDER_PSUEDO_AUDIO


void s_EncodeThread(int	i32DevKeypad) {
	char	*pcPCMBuf;
	char *  pcEncFrameBuf=NULL; 
	int		i32ReadBytes;
	int  number=0;

        //int no1=0;
	s_bEncodeThread_Alive = true;

//	pthread_mutex_lock(&g_sRecorderStateMutex);
	AACRECORDER_DEBUG_MSG("ENTER: e-thread %d, state %d", s_bEncodeThread_Alive, g_eAACRecorder_State);
//	pthread_mutex_unlock(&g_sRecorderStateMutex);

	// Prepare encoded frame buffer
	AACRECORDER_DEBUG_MSG("To allocate 1 encoded buffer %d bytes", ENCFRAME_BUFSIZE);

	// Initialize audio record device

	s_OpenAudioRecDevice(AACRECORDER_SAMPLE_RATE, AACRECORDER_CHANNEL_NUM);

//	if (s_i32DevDSP_BlockSize > 0) {
		pcPCMBuf = (char *)malloc(s_i32DevDSP_BlockSize);
		if (pcPCMBuf == NULL) {
			perror("malloc()");
			goto EndOfEncode;
		} // if
//	}

	pcEncFrameBuf = (char *)malloc(ENCFRAME_BUFSIZE);

	if (pcEncFrameBuf == NULL) {
		perror("malloc()");
		goto EndOfEncode;		// Auto stop recorder
	} // if

	// Initialize AAC Encoder
	S_AACENC sEncoder;

	sEncoder.m_u32SampleRate = AACRECORDER_SAMPLE_RATE;
	sEncoder.m_u32ChannelNum = AACRECORDER_CHANNEL_NUM;
	sEncoder.m_u32BitRate = AACRECORDER_BIT_RATE * sEncoder.m_u32ChannelNum;
	sEncoder.m_u32Quality = AACRECORDER_QUALITY;
	sEncoder.m_bUseAdts = true;
	sEncoder.m_bUseMidSide = false;
	sEncoder.m_bUseTns = false;

	AACRECORDER_DEBUG_MSG("To init AACEnc, e-thread %d", s_bEncodeThread_Alive);
	E_AACENC_ERROR	eAACEnc_Error = AACEnc_Initialize(&sEncoder, true);

	if (eAACEnc_Error != eAACENC_ERROR_NONE) {
		printf("AAC Recorder: Fail to initialize AAC Encoder: Error code 0x%08x\n", eAACEnc_Error);
		goto EndOfEncode;		// Auto stop recorder
	} // if

	printf("\n");
	AACRECORDER_DEBUG_MSG("Init AACEnc done, e-thread %d", s_bEncodeThread_Alive);

	g_u32AACRecorder_EncFrameCnt = 0;

	while (1) {
		// Check command
//		pthread_mutex_lock(&g_sRecorderStateMutex);
		if (g_eAACRecorder_State == eAACRECORDER_STATE_TOSTOP) {
//			pthread_mutex_unlock(&g_sRecorderStateMutex);

			// Stop encode after recording ended
			if (s_i32PCMFrame_FullNum == 0)
			break;
		} // if
//		pthread_mutex_unlock(&g_sRecorderStateMutex);

		// Encode PCM frame buffer even if paused
// read PCM data
		i32ReadBytes = read(s_i32DevDSP, pcPCMBuf, s_i32DevDSP_BlockSize);
		number ++;
		if ( ( number > s_i32no) || (i32ReadBytes <= 0 ))
		{
		   i32ReadBytes = 0; 
           s_bEncodeThread_Alive = false;
		   g_eAACRecorder_State = eAACRECORDER_STATE_TOSTOP;
		}
		if (i32ReadBytes > 0) {
			AACRECORDER_DEBUG_MSG("Read %d bytes from record device", i32ReadBytes);
			if (s_i32PCMFrame_FullNum < PCMFRAME_BUFNUM-1) {
				char	*pcPCMDst = (char*)(s_apcPCMFrame_Buf[s_i32PCMFrame_WriteIdx]) + s_ai32PCMFrame_PCMSize[s_i32PCMFrame_WriteIdx],
				*pcPCMSrc = pcPCMBuf;
				int	i32FillBytes = PCMFRAME_BUFSIZE - s_ai32PCMFrame_PCMSize[s_i32PCMFrame_WriteIdx];
				if (i32FillBytes > i32ReadBytes)
				i32FillBytes = i32ReadBytes;
				memcpy(pcPCMDst, pcPCMSrc, i32FillBytes);
				pcPCMDst += i32FillBytes;
				pcPCMSrc += i32FillBytes;

				s_ai32PCMFrame_PCMSize[s_i32PCMFrame_WriteIdx] += i32FillBytes;
				if (s_ai32PCMFrame_PCMSize[s_i32PCMFrame_WriteIdx] >= PCMFRAME_BUFSIZE) {
					g_u32AACRecorder_RecFrameCnt++;
					s_i32PCMFrame_WriteIdx++;
					if ( s_i32PCMFrame_WriteIdx == PCMFRAME_BUFNUM )
						s_i32PCMFrame_WriteIdx = 0;

					pcPCMDst = (char*)s_apcPCMFrame_Buf[s_i32PCMFrame_WriteIdx];
					s_i32PCMFrame_FullNum++;
				} // if
				i32ReadBytes -= i32FillBytes;
			}
		}

		if (s_i32PCMFrame_FullNum > 0) {
			int	i32EncFrameSize;
			eAACEnc_Error = AACEnc_EncodeFrame((short*)s_apcPCMFrame_Buf[s_i32PCMFrame_ReadIdx], pcEncFrameBuf, ENCFRAME_BUFSIZE, &i32EncFrameSize);
			if (eAACEnc_Error != eAACENC_ERROR_NONE) {
				printf("AAC Recorder: Fail to encode file: Error code 0x%08x\n", eAACEnc_Error);
				goto EndOfEncode;		// Auto stop recorder
			} // if

			if (fwrite(pcEncFrameBuf, 1, i32EncFrameSize, s_psAACFile) != i32EncFrameSize) {
				perror("fwrite()");
				goto EndOfEncode;		// Auto stop recorder
			} // if
                    //    no1++;
 		    //	printf("Writing file --> %d, %d\n", no1, i32EncFrameSize);
			g_u32AACRecorder_EncFrameCnt++;
			s_ai32PCMFrame_PCMSize[s_i32PCMFrame_ReadIdx] = 0;
			s_i32PCMFrame_ReadIdx++;
			if ( s_i32PCMFrame_ReadIdx == PCMFRAME_BUFNUM )
    			s_i32PCMFrame_ReadIdx = 0;
			if (s_i32PCMFrame_ReadIdx == 0)
				sync();
			s_i32PCMFrame_FullNum--;
		} // if s_i32PCMFrame_FullNum > 0
		else {
 		    g_eAACRecorder_State = eAACRECORDER_STATE_TOSTOP;
//			pthread_mutex_lock(&g_sRecorderStateMutex);
			AACRECORDER_DEBUG_MSG("state %d", g_eAACRecorder_State);
//			pthread_mutex_unlock(&g_sRecorderStateMutex);
			break;
		} // else
				// Update current time if no key pressed
		static unsigned int	u32LastCurSecond = 0;
		const unsigned int	u32CurSecond = AACENC_FRAMESIZE * g_u32AACRecorder_RecFrameCnt / AACRECORDER_SAMPLE_RATE;
		if ( number % 16 == 15 )
		{
		if (u32CurSecond != u32LastCurSecond) {
			printf("\rRecord time: %d m %2d s", u32CurSecond / 60, u32CurSecond % 60);
			fflush(stdout);
			u32LastCurSecond = u32CurSecond;
		}// if
		}
		if ( CheckKey(i32DevKeypad) == 1 )
		{
			printf("press key to stop\n");
			break;
		}
	} // while 1
    	printf("\n");
EndOfEncode:
	// Finalize AAC Encoder
	s_CloseAudioRecDevice();
	AACEnc_Finalize();

	if ( pcPCMBuf ) {
		AACRECORDER_DEBUG_MSG("To free PCM buffer");
		free(pcPCMBuf);
	}

	if (pcEncFrameBuf) {
		AACRECORDER_DEBUG_MSG("To free encoded buffer");
		free(pcEncFrameBuf);
	} // if

	// Auto stop recorder
	s_bEncodeThread_Alive = false;
	AACRECORDER_DEBUG_MSG("To stop recorder, e-thread %d", s_bEncodeThread_Alive);
	AACRecorder_Stop();

//	pthread_mutex_lock(&g_sRecorderStateMutex);
	AACRECORDER_DEBUG_MSG("LEAVE: e-thread %d, state %d", s_bEncodeThread_Alive, g_eAACRecorder_State);
//	pthread_mutex_unlock(&g_sRecorderStateMutex);

} // s_EncodeThread()

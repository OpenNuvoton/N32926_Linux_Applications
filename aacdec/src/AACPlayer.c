/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp.  All rights reserved. *
 *                                                              *
 ****************************************************************/

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "AACPlayer_Agent.h"


// ==================================================
// Constant definition
// ==================================================
#define LONGPRESS_DOWNCNT	10		// Key is long-pressed if down count >= 10
#define SEEK_SECOND			20		// 20 second per forward/backward seek

//#define AACPLAYER_PSUEDO_KEYPAD	// Defined if no real keypad, e.g. profiling


// ==================================================
// Type declaration
// ==================================================

// Keypad definition of N32905 dev board
typedef enum {
	eKEYCODE_DOWN		= 15,
	eKEYCODE_ENTER		= 13,
	eKEYCODE_HOME		= 19,
	eKEYCODE_LEFT		= 1,
	eKEYCODE_RIGHT		= 2,
	eKEYCODE_UP			= 14
} E_TEST_KEYCODE;

#if 0
// Keypad definition of W55FA93 demo board
typedef enum {
	eKEYCODE_DOWN		= 15,
	eKEYCODE_HOME		= 19,
	eKEYCODE_RIGHT		= 13,
	eKEYCODE_UP			= 14,
	eKEYCODE_VOLUP		= -1,		// FIXME
	eKEYCODE_VOLDOWN	= -2,		// FIXME
} E_TEST_KEYCODE;
#endif

typedef enum {
	eKEYVALUE_UP		= 0,
	eKEYVALUE_DOWN		= 1,
	eKEYVALUE_PRESSED	= 2,
} E_KEYVALUE;


// ==================================================
// Private function declaration
// ==================================================

static int						// [out]1/0: Recognized/Unrecognized file name
s_FilterAACFile(
	const struct dirent	*psFileInfo
);

static void s_PrintAACInfo(void);

#ifndef AACPLAYER_PSUEDO_KEYPAD
static int						// Device keypad handle
s_OpenKeypadDevice(void);

static bool						// [out]true: Some key down, false: No key down
s_PollingKeypad(
	int	i32DevKeypad,
	int	*pi32KeyCode,
	int	*pi32KeyValue
);
#endif


// ==================================================
// Plublic function implementation
// ==================================================

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("Usage: %s <File or Directory>\n", argv[0]);
		return -1;
	} // if

#ifndef AACPLAYER_PSUEDO_KEYPAD
	// Initialize keypad device
	int	i32DevKeypad = s_OpenKeypadDevice();

	if (i32DevKeypad < 0)
		return -2;
#endif

	// Scan files in directory
	char			szFileName[1024];
	struct dirent	**ppFileNameList = NULL;
	int				i32DirNameLen = 0;
	int				i32FileNumInDir;

	strcpy(szFileName, argv[1]);
	i32FileNumInDir = scandir(szFileName, &ppFileNameList, s_FilterAACFile, alphasort);

	if (i32FileNumInDir == 0) {
		printf("%s: No recognized file", szFileName);
		goto EndOfPlayer;
	} // if

	if (i32FileNumInDir > 0) {
		i32DirNameLen = strlen(szFileName);

		if (szFileName[i32DirNameLen - 1] != '/') {
			szFileName[i32DirNameLen++] = '/';
			szFileName[i32DirNameLen] = 0;
		} // if
	} // if

	const unsigned int	u32Ver = AACDec_GetVersion();
	printf("AAC Decoder library %d.%02x Copyright (c) Nuvoton Technology Corp.\n"
		   "	UP key             : Play previous file\n"
		   "	DOWN key           : Play next file\n"
		   "	RIGHT key          : Pause / Resume\n"
		   "	UP key long-press  : Backward seek\n"
		   "	DOWN key long-press: Forward seek\n"
		   "	HOME key           : Exit\n",
		   u32Ver >> 8, u32Ver & 0xFF);

	// Play each recognized file in directory
	int	i32PlayFileIdx = 0;

	while (1) {
		if (i32FileNumInDir > 0)
			// Make full file name
			strcpy(&szFileName[i32DirNameLen], ppFileNameList[i32PlayFileIdx]->d_name);

		printf("\n%s: ", szFileName);
		fflush(stdout);

		if (AACPlayer_PlayFile(szFileName)) {
#ifndef AACPLAYER_PSUEDO_KEYPAD
			unsigned int	u32KeyDownCount = 0;
#endif // !AACPLAYER_PSUEDO_KEYPAD
			unsigned int	u32LastCurSecond = 0;
			pthread_mutex_lock(&g_sPlayerStateMutex);

			while (g_eAACPlayer_State != eAACPLAYER_STATE_STOPPED) {
				pthread_mutex_unlock(&g_sPlayerStateMutex);
#ifndef AACPLAYER_PSUEDO_KEYPAD
				// Check keypad
				int	i32KeyCode, i32KeyValue;

				if (s_PollingKeypad(i32DevKeypad, &i32KeyCode, &i32KeyValue)) {
					if (i32KeyValue == eKEYVALUE_UP) {
#if 0
						extern int	i32IsDumpKeyPressed;
#endif
						switch (i32KeyCode) {
						case eKEYCODE_UP:			// Play previous file
#if 1
							if (u32KeyDownCount < LONGPRESS_DOWNCNT) {
								AACPLAYER_DEBUG_MSG("UP key: To stop player");
								AACPlayer_Stop();
								printf(" [Previous]");
								i32PlayFileIdx += i32FileNumInDir - 2;
							} // if
							else {
								AACPlayer_Seek(SEEK_SECOND * -100);
								printf(" [Backward seek]\n");
							} // else
#else
							i32IsDumpKeyPressed = 1;
#endif
							break;

						case eKEYCODE_DOWN:			// Play next file
							if (u32KeyDownCount < LONGPRESS_DOWNCNT) {
								AACPLAYER_DEBUG_MSG("DOWN key: To stop player");
								AACPlayer_Stop();
								printf(" [Next]");
							} // if
							else {
								AACPlayer_Seek(SEEK_SECOND * 100);
								printf(" [Forward seek]\n");
							} // else
							break;

						case eKEYCODE_RIGHT:		// Pause / Resume
							pthread_mutex_lock(&g_sPlayerStateMutex);

							if (g_eAACPlayer_State == eAACPLAYER_STATE_PLAYING) {
								pthread_mutex_unlock(&g_sPlayerStateMutex);
								AACPlayer_Pause();
								printf(" [Paused]\n");
							} // if
							else {
								pthread_mutex_unlock(&g_sPlayerStateMutex);
								AACPlayer_Resume();
							} // else
							break;
#if 0
						case eKEYCODE_VOLUP:		// Volume up
							AACPlayer_ChangeVolume(g_u32AACPlayer_Volume + 20);
							break;

						case eKEYCODE_VOLDOWN:		// Volume down
							AACPlayer_ChangeVolume(g_u32AACPlayer_Volume - 20);
							break;
#endif
						case eKEYCODE_HOME:			// Exit
							AACPLAYER_DEBUG_MSG("HOME key: To stop player");
							AACPlayer_Stop();
							printf(" [Exit]\n");
							goto EndOfPlayer;
						} // switch

						u32KeyDownCount = 0;
					} // if i32KeyValue == eKEYVALUE_UP
					else
						u32KeyDownCount++;
				} // if some key down/up
				else
#endif // !AACPLAYER_PSUEDO_KEYPAD
				{ 
					// Update current time if no key pressed
					// g_sAACDec_FrameInfo will be cleared if decoding failed
					//const unsigned int	u32CurSecond = g_u32AACPlayer_PlaySampleCnt / g_u32AACPlayer_SampleRate;
					const unsigned int	u32CurSecond = g_sAACDec_FrameInfo.m_u32Cur_position / 100;

					if (u32CurSecond != u32LastCurSecond) {
						if (u32LastCurSecond == 0)
							s_PrintAACInfo();

						printf("\rCurrent time: %d m %2d s", u32CurSecond / 60, u32CurSecond % 60);
						fflush(stdout);
						u32LastCurSecond = u32CurSecond;
					} // if
					else {
						// Force task switch to avoid busy polling
						struct timespec	sSleepTime = { 0, 0 };
						nanosleep(&sSleepTime, NULL);
					} // else
				} // else no key down/up
			} // while g_eAACPlayer_State != eAACPLAYER_STATE_STOPPED

			pthread_mutex_unlock(&g_sPlayerStateMutex);
		} // if AACPlayer_PlayFile(szFileName)

		printf("\n");

		if (i32FileNumInDir <= 0)
			break;		// Single file playback is ended

		// Play next recognized file in directory
		i32PlayFileIdx = (i32PlayFileIdx + 1) % i32FileNumInDir;
	} // while 1

EndOfPlayer:
	// Free file name list allocated by scandir()
	if (ppFileNameList) {
		int	i32Index;

		for (i32Index = i32FileNumInDir - 1; i32Index >= 0; i32Index--)
			free(ppFileNameList[i32Index]);

		free(ppFileNameList);
	} // if

#ifndef AACPLAYER_PSUEDO_KEYPAD
	close(i32DevKeypad);
#endif // !AACPLAYER_PSUEDO_KEYPAD
	AACPLAYER_DEBUG_MSG("LEAVE");
	return 0;
} // main()


// ==================================================
// Private Function implementation
// ==================================================

static int						// [out]1/0: Recognized/Unrecognized file name
s_FilterAACFile(
	const struct dirent	*psFileInfo
) {
	// Check regular or unknown file type (some F/S cannot get type)
	if (psFileInfo->d_type != DT_REG && psFileInfo->d_type != DT_UNKNOWN)
		return 0;

	// Check file extension
	char	*pszExt = strrchr(psFileInfo->d_name, '.');

	if (pszExt == NULL)
		return 0;

	return 	strcasecmp(pszExt, ".aac") == 0
			||	strcasecmp(pszExt, ".mp4") == 0
			||	strcasecmp(pszExt, ".m4a") == 0
			||	strcasecmp(pszExt, ".m4b") == 0;
} // s_FilterAACFile()


#ifndef AACPLAYER_PSUEDO_KEYPAD

#define DEVKEYPAD_RETRY_NUM 	4

static int						// Device keypad handle
s_OpenKeypadDevice(void) {
	char	szDevPath[] = "/dev/input/event0";
	char	szDevName[256];
	int		i32DevKeypad = -1;
	int		i32Index;

	for (i32Index = 0; i32Index < DEVKEYPAD_RETRY_NUM; i32Index++) {
		if ((i32DevKeypad = open(szDevPath, O_RDONLY)) < 0) {
			perror(szDevPath);
			break;
		} // if

		memset(szDevName, 0x0, sizeof(szDevName));

		if (ioctl(i32DevKeypad, EVIOCGNAME(sizeof(szDevName)), szDevName) < 0) {
			perror(szDevPath);
			break;
		} // if

		if (strstr(szDevName, "Keypad") != NULL)
			return i32DevKeypad;

		close(i32DevKeypad);
		szDevPath[16]++;
	} // for

	printf("Failed to find any keypad device\n");
	return -1;
} // s_OpenKeypadDevice()


static bool						// [out]true: Some key down/up, false: No key down/up
s_PollingKeypad(
	int	i32DevKeypad,
	int	*pi32KeyCode,
	int	*pi32KeyValue
) {
	struct timeval	sTimeout = { 0, 0 };	// 0s 0us
	fd_set			sReadFDSet;

	FD_ZERO(&sReadFDSet);
	FD_SET(i32DevKeypad, &sReadFDSet);
	select(i32DevKeypad + 1, &sReadFDSet, NULL, NULL, &sTimeout);

	if (FD_ISSET(i32DevKeypad, &sReadFDSet)) {
		struct input_event	sEvent;

		if (read(i32DevKeypad, &sEvent, sizeof(sEvent)) == sizeof(sEvent)
				&&	sEvent.type) {
			*pi32KeyCode = sEvent.code;
			*pi32KeyValue = sEvent.value;
			return true;
		} // if
	} // if

	return false;
} // s_PollingKeypad()

#endif // !AACPLAYER_PSUEDO_KEYPAD


static void s_wprint(
	unsigned short* pwszString
) {
	for (; *pwszString; pwszString++) {
		const char	*pszString = (char*)pwszString;

		if (pszString[1])
			printf("%c%c", pszString[0], pszString[1]);
		else
			printf("%c", pszString[0]);
	} // for
} // s_wprint()


static void s_PrintAACInfo(void) {
	printf("%d Hz, %d channels, avg %d bps, %d m %.3f s, %d frames\n",
		   g_sAACDec_FrameInfo.m_u32Samplerate, g_sAACDec_FrameInfo.m_u32Channels, g_sAACDec_FrameInfo.m_u32Avg_bitrate,
		   g_sAACDec_FrameInfo.m_u32Total_duration / 6000, g_sAACDec_FrameInfo.m_u32Total_duration % 6000 / 100.0,
		   g_sAACDec_FrameInfo.m_u32Total_frames);

	unsigned int	u32TagIdx;

	for (u32TagIdx = 0; u32TagIdx < eAACDEC_MP4_TAGNUM; u32TagIdx++) {
		const char		*apszName[eAACDEC_MP4_TAGNUM] = {
			"Album", "Artist", "Comment", "Compilation", "Date",
			"Disc", "Genre", "Tempo", "Title", "Tool", "Track",
			"Writer",
		};
		unsigned short	au16UnicodeStr[256];

		if (AACDec_GetMP4TAG(u32TagIdx, au16UnicodeStr, 256)) {
			printf("\t%s: \t\t", apszName[u32TagIdx]);
			s_wprint(au16UnicodeStr);
			printf("\n");
		} // if
	} // for
} // s_PrintAACInfo()

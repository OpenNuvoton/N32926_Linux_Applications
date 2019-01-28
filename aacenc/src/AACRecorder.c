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
#include <linux/input.h>

#include "AACRecorder_Agent.h"


// ==================================================
// Constant definition
// ==================================================
//#define AACRECORDER_PSUEDO_KEYPAD		// Defined if no real keypad, e.g. profiling


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
} E_KEYCODE;

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

extern void s_EncodeThread(int	i32DevKeypad );

int CheckKey(int i32DevKeypad);

#ifndef AACRECORDER_PSUEDO_KEYPAD
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
char g_strInputFile[512];
int main(int argc, char **argv)
{
	if (argc != 3) {
		printf("Usage: %s <AAC File> <PCM File>\n", argv[0]);
		return -1;
	} // if


 

#ifndef AACRECORDER_PSUEDO_KEYPAD
	// Initialize keypad device
	int	i32DevKeypad = s_OpenKeypadDevice();

	if (i32DevKeypad < 0)
		return -2;
#endif

	const unsigned int	u32Ver = AACEnc_GetVersion();

	printf(	"AAC Encoder library %d.%02x Copyright (c) Nuvoton Technology Corp.\n"
			"	RIGHT key: Pause / Resume\n"
			"	HOME key : Exit\n",
			u32Ver >> 8, u32Ver & 0xFF);

         strcpy(g_strInputFile, argv[2]);
	if (AACRecorder_RecordFile(argv[1])) {
		printf(	"Record to %s:\n"
				"	Channel number:	%d\n"
				"	Sample rate:	%d Hz\n"
				"	Bit rate:	%d bps\n"
				"	Quality:	%d (1 ~ 999)\n",
			argv[1], AACRECORDER_CHANNEL_NUM, AACRECORDER_SAMPLE_RATE, 				AACRECORDER_BIT_RATE, AACRECORDER_QUALITY);
#if 0
		pthread_mutex_lock(&g_sRecorderStateMutex);
		while (g_eAACRecorder_State != eAACRECORDER_STATE_STOPPED) {
			pthread_mutex_unlock(&g_sRecorderStateMutex);
			
			// Check keypad
			int	i32KeyCode, i32KeyValue;

			if (s_PollingKeypad(i32DevKeypad, &i32KeyCode, &i32KeyValue)) {
				if (i32KeyValue == eKEYVALUE_UP)
					switch(i32KeyCode) {
					case eKEYCODE_RIGHT:		// Pause / Resume
						if (g_eAACRecorder_State == eAACRECORDER_STATE_RECORDING) {
							AACRecorder_Pause();
							printf(" [Paused]\n");
						} // if
						else
							AACRecorder_Resume();
						break;

					case eKEYCODE_HOME:			// Exit
						AACRECORDER_DEBUG_MSG("HOME key: To stop recorder");
						AACRecorder_Stop();
						printf(" [Exit]\n");
						goto EndOfRecorder;
					} // switch
			} // if some key down/up
			
		} // while g_eAACRecorder_State != eAACRECORDER_STATE_STOPPED
		pthread_mutex_unlock(&g_sRecorderStateMutex);
#endif
		s_EncodeThread(i32DevKeypad);        
	} // if AACRecorder_RecordFile(argv[1])

#ifndef AACRECORDER_PSUEDO_KEYPAD
//EndOfRecorder:
	close(i32DevKeypad);
#endif

	AACRECORDER_DEBUG_MSG("LEAVE");
	return 0;
} // main()

// return 1 stop
// return 0, go on
int CheckKey(int i32DevKeypad)
{
			// Check keypad
	int	i32KeyCode, i32KeyValue;
	if (s_PollingKeypad(i32DevKeypad, &i32KeyCode, &i32KeyValue)) {
		if (i32KeyValue == eKEYVALUE_UP) {
			switch(i32KeyCode) {
				case eKEYCODE_RIGHT:		// Pause / Resume
					if (g_eAACRecorder_State == eAACRECORDER_STATE_RECORDING) {
						AACRecorder_Pause();
						printf(" [Paused]\n");
					} // if
					else
						AACRecorder_Resume();
					break;
				case eKEYCODE_HOME:			// Exit
					AACRECORDER_DEBUG_MSG("HOME key: To stop recorder");
					AACRecorder_Stop();
					printf(" [Exit]\n");
				return 1; // 
			} // switch
		} // if some key down/up
	} // while g_eAACRecorder_State != eAACRE
	return 0;
}
// ==================================================
// Private Function implementation
// ==================================================

#ifndef AACRECORDER_PSUEDO_KEYPAD

#define DEVKEYPAD_RETRY_NUM 	4

static int						// Device keypad handle
s_OpenKeypadDevice(void)
{
	char	szDevPath[] = "/dev/input/event0",
			szDevName[256];
	int		i32DevKeypad = -1,
			i32Index;

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
)
{
	struct timeval	sTimeout = { 0, 0 };	// 0s 0us
	fd_set			sReadFDSet;

	FD_ZERO(&sReadFDSet);
	FD_SET(i32DevKeypad, &sReadFDSet);
	if (select(i32DevKeypad + 1, &sReadFDSet, NULL, NULL, &sTimeout) < 0)
		perror("select()");
	else if (FD_ISSET(i32DevKeypad, &sReadFDSet)) {
		struct input_event	sEvent;

		read(i32DevKeypad, &sEvent, sizeof(sEvent));
		if (sEvent.type) {
			*pi32KeyCode = sEvent.code;
			*pi32KeyValue = sEvent.value;
			return true;
		} // if
	} // if

	return false;
} // s_PollingKeypad()

#endif // !AACRECORDER_PSUEDO_KEYPAD

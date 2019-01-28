#include <sys/time.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "powerManager.h"

#define ENABLE_DO_POWEROFF		// comment this if co-work with desktopManager
//#define ENABLE_POWER_MODE		// comment this if do not want to enter to power saving modes
//#define USING_ADCDEV

#define TS_DEV		"/dev/input/event0"
#define KPD_DEV		"/dev/input/event1"
#define FB_DEV		"/dev/fb0"
#define SYSMGR1_DEV	"/dev/sysmgr1"
#define PWRMGR_DEV	"/sys/devices/platform/w55fa92-clk/clock"
#define ADC_DEV		"/sys/devices/platform/w55fa92-adc/adc"

#define DISPLAY_OFF_TIMEOUT	30*1000
#define IDLE_TIMEOUT		30*1000

int fd_ts, fd_kpd, fd_sysmgr;
unsigned int power_state, old_power_state;
unsigned long cur_time, prev_time;

unsigned long GetTime()
{
	timeval		tv;
	unsigned long	t;

	gettimeofday(&tv , NULL);
	t = tv.tv_sec * 1000;
	t += tv.tv_usec / 1000;

	return t;
}

bool CheckInputEvent(void)
{
	fd_set	rfd;
	struct	timeval to;
	struct	input_event	kpd_data;
	int	max_fd, ret;
	bool	event;
	
	event = false;
	to.tv_sec = 0;
	to.tv_usec = 0;
	FD_ZERO(&rfd);
	FD_SET(fd_ts, &rfd);
	FD_SET(fd_kpd, &rfd);
	max_fd = (fd_ts > fd_kpd) ? fd_ts : fd_kpd;

	ret = select(max_fd+1, &rfd, NULL, NULL, &to);
	if (ret < 0) {
		perror("select() failed");
	}
	else if (ret > 0) {
		// check keypad input event
		if (FD_ISSET(fd_kpd, &rfd)) {
			read(fd_kpd, &kpd_data, sizeof(kpd_data));
			if (kpd_data.type > 0) {
				//printf("kpd: %d\n", kpd_data.value);
				event = true;
			}
		}
	}

	return event;
}

bool ProcessPowerState(unsigned int state)
{
	int fd_fb, fd_pwrmgr;
#ifdef USING_ADCDEV
	int fd_adc;
	unsigned char ch;
#endif

	if (state == power_state) {
		//printf("The power state is not changed. Do nothing.\n");
		return true;
	}

	fd_pwrmgr = -1;
	old_power_state = power_state;
	power_state = state;

	if (state >= POWER_STATE_IDLE) {
		fd_pwrmgr = open(PWRMGR_DEV, O_WRONLY);
		if (fd_pwrmgr < 0) {
			printf("open device %s failed, errno = %d\n", PWRMGR_DEV, errno);
			return false;
		}
#ifdef USING_ADCDEV
		fd_adc = open(ADC_DEV, O_WRONLY);
		if (fd_adc < 0) {
			printf("open device %s failed, errno = %d\n", ADC_DEV, errno);
			close(fd_pwrmgr);
			return false;
		}
#endif
	}

	if (state == POWER_STATE_NORMAL) {
		printf("enter POWER_STATE_NORMAL\n");
		if (old_power_state == POWER_STATE_DISPLAY_OFF) {
			fd_fb = open(FB_DEV, O_RDWR);
			if (fd_fb < 0) {
				printf("open device %s failed, errno = %d\n", FB_DEV, errno);
				return false;
			}
			ioctl(fd_fb, VIDEO_DISPLAY_ON);
			close(fd_fb);
		}
	}
	else if (state == POWER_STATE_DISPLAY_OFF) {
		printf("enter POWER_STATE_DISPLAY_OFF\n");
		fd_fb = open(FB_DEV, O_RDWR);
		if (fd_fb < 0) {
			printf("open device %s failed, errno = %d\n", FB_DEV, errno);
			return false;
		}
		ioctl(fd_fb, VIDEO_DISPLAY_OFF);
		close(fd_fb);
	}
	else if (state == POWER_STATE_IDLE) {
		printf("enter POWER_STATE_IDLE\n");
#ifdef USING_ADCDEV
		if (fd_adc > 0) {
			ch = '0';
			write(fd_adc, &ch, 1);
			usleep(3000);
		}
#endif
		write(fd_pwrmgr, "id", sizeof("id"));
#ifdef USING_ADCDEV
		if (fd_adc > 0) {
			ch = '1';
			write(fd_adc, &ch, 1);
		}
#endif
		printf("enter POWER_STATE_NORMAL\n");
		old_power_state = power_state;
		power_state = POWER_STATE_NORMAL;
	}
	else if (state == POWER_STATE_MEMORY_IDLE) {
		printf("enter POWER_STATE_MEMORY_IDLE\n");
#ifdef USING_ADCDEV
		if (fd_adc > 0) {
			ch = '0';
			write(fd_adc, &ch, 1);
			usleep(3000);
		}
#endif
		write(fd_pwrmgr, "mi", sizeof("mi"));
#ifdef USING_ADCDEV
		if (fd_adc > 0) {
			ch = '1';
			write(fd_adc, &ch, 1);
		}
#endif
		printf("enter POWER_STATE_NORMAL\n");
		old_power_state = power_state;
		power_state = POWER_STATE_NORMAL;
	}
	else if (state == POWER_STATE_POWER_DOWN) {
		printf("enter POWER_STATE_POWER_DOWN\n");
		write(fd_pwrmgr, "pd", sizeof("pd"));
		printf("enter POWER_STATE_NORMAL\n");
		old_power_state = power_state;
		power_state = POWER_STATE_NORMAL;
	}
	else if ((state == POWER_STATE_RTC_POWER_OFF) || (state == POWER_STATE_POWER_OFF)) {
		printf("enter POWER_STATE_POWER_OFF\n");
		// this command should be done in desktopManager
#ifdef ENABLE_DO_POWEROFF
		system("poweroff");
#endif
		while (1)
			sleep(1);
	}
	else {
		printf("power state 0x%08x is unknown!\n", state);
	}
	
	if (state >= POWER_STATE_IDLE) {
		if (fd_pwrmgr > 0)
			close(fd_pwrmgr);
#ifdef USING_ADCDEV
		if (fd_adc > 0)
			close(fd_adc);
#endif
	}

	// reset timeout counter
	prev_time = cur_time;

	return true;
}

static void ProcessSysMgrStatus(unsigned int status)
{
	if (status == SYSMGR_STATUS_SD_INSERT) {
		printf("PM: SD_INSERT\n");
		ProcessPowerState(POWER_STATE_NORMAL);
		prev_time = cur_time;
	}
	else if (status == SYSMGR_STATUS_SD_REMOVE) {
		printf("PM: SD_REMOVE\n");
		ProcessPowerState(POWER_STATE_NORMAL);
		prev_time = cur_time;
	}
	if (status == SYSMGR_STATUS_USBD_PLUG) {
		printf("PM: USBD_PLUG\n");
		ProcessPowerState(POWER_STATE_NORMAL);
		prev_time = cur_time;
	}
	else if (status == SYSMGR_STATUS_USBD_UNPLUG) {
		printf("PM: USBD_UPLUG\n");
		ProcessPowerState(POWER_STATE_NORMAL);
		prev_time = cur_time;
	}
#if 0
	if (status == SYSMGR_STATUS_AUDIO_OPEN) {
		printf("PM: AUDIO_OPEN\n");
	}
	else if (status == SYSMGR_STATUS_AUDIO_CLOSE) {
		printf("PM: AUDIO_CLOSE\n");
	}
#endif
	if (status == SYSMGR_STATUS_NORMAL) {
		printf("PM: NORMAL\n");
		ProcessPowerState(POWER_STATE_NORMAL);
		prev_time = cur_time;
	}
	else if (status == SYSMGR_STATUS_DISPLAY_OFF) {
		printf("PM: DISPLAY_OFF\n");
		ProcessPowerState(POWER_STATE_DISPLAY_OFF);
	}
#if 0
	else if (status == SYSMGR_STATUS_IDLE) {
		printf("PM: IDLE\n");
		ProcessPowerState(POWER_STATE_IDLE);
	}
	else if (status == SYSMGR_STATUS_MEMORY_IDLE) {
		printf("PM: MEMORY_IDLE\n");
		ProcessPowerState(POWER_STATE_MEMORY_IDLE);
	}
#endif
	else if (status == SYSMGR_STATUS_POWER_DOWN) {
		printf("PM: POWER_DOWN\n");
		ProcessPowerState(POWER_STATE_POWER_DOWN);
	}
	else if (status == SYSMGR_STATUS_RTC_POWER_OFF) {
		printf("PM: RTC_POWER_OFF\n");
		ProcessPowerState(POWER_STATE_RTC_POWER_OFF);
	}
	else if (status == SYSMGR_STATUS_POWER_OFF) {
		printf("PM: POWER_OFF\n");
		ProcessPowerState(POWER_STATE_POWER_OFF);
	}
}

static void PowerOff_Check()
{
	static int power_off_count = 0;
	unsigned int state = 0;

	if (power_off_count < 20) {
		if (ioctl(fd_sysmgr, SYSMGR_IOC_GET_POWER_KEY, &state) < 0) {
			fprintf(stderr,"SYSMGR_IOC_GET_POWER_KEY ioctl failed:%d\n", errno);
		}
		if (state == 0) {
			printf("power_key = %d\n", state);
			power_off_count++;
		} else {
			power_off_count = 0;
		}
	}
	else if (power_off_count == 20) {
		printf("system is going to power off\n");
		state = SYSMGR_CMD_RTC_POWER_OFF;
		if (ioctl(fd_sysmgr, SYSMGR_IOC_SET_POWER_STATE, &state) < 0) {
			fprintf(stderr,"SYSMGR_IOC_SET_POWER_STATE ioctl failed:%d\n", errno);
		}
		power_off_count++;
		prev_time = cur_time;
	}

	return ;
}

static void ProcessPowerManagement()
{
	unsigned int usbd_state = 0;
	unsigned int state = 0;

	PowerOff_Check();

#ifndef ENABLE_POWER_MODE
	return ;
#endif

	// skip ProcessPowerManagement while USB cable is detected
	if (ioctl(fd_sysmgr, SYSMGR_IOC_GET_USBD_STATE, &usbd_state) < 0) {
		fprintf(stderr,"SYSMGR_IOC_GET_USBD_STATE ioctl failed:%d\n", errno);
	}
	if (usbd_state > 0) {
		//printf("usbd_state = %d\n", usbd_state);
		return;
	}

	cur_time = GetTime();

	if (power_state == POWER_STATE_NORMAL) {
		if (cur_time - prev_time > DISPLAY_OFF_TIMEOUT) {
			printf("DISPLAY_OFF_TIMEOUT\n");
			// call SYSMGR_IOC_SET_POWER_STATE to disable touch screen
			//ProcessPowerState(POWER_STATE_DISPLAY_OFF);
			state = SYSMGR_CMD_DISPLAY_OFF;
			if (ioctl(fd_sysmgr, SYSMGR_IOC_SET_POWER_STATE, &state) < 0) {
				fprintf(stderr,"SYSMGR_IOC_SET_POWER_STATE ioctl failed:%d\n", errno);
			}
		}
		else {
			if (CheckInputEvent()) {
				prev_time = cur_time;
			}
		}
	}
	else if (power_state == POWER_STATE_DISPLAY_OFF) {
		if (cur_time - prev_time > IDLE_TIMEOUT) {
			printf("SUSPEND_TIMEOUT\n");
			//ProcessPowerState(POWER_STATE_IDLE);
			//ProcessPowerState(POWER_STATE_MEMORY_IDLE);
			ProcessPowerState(POWER_STATE_POWER_DOWN);
		}
		else {
			if (CheckInputEvent()) {
				// call SYSMGR_IOC_SET_POWER_STATE to enable touch screen
				//ProcessPowerState(POWER_STATE_NORMAL);
				state = SYSMGR_CMD_NORMAL;
				if (ioctl(fd_sysmgr, SYSMGR_IOC_SET_POWER_STATE, &state) < 0) {
					fprintf(stderr,"SYSMGR_IOC_SET_POWER_STATE ioctl failed:%d\n", errno);
				}
				prev_time = cur_time;
			}
		}
	}

	return ;
}

bool PM_Init(void)
{
	fd_ts = open(TS_DEV, O_RDONLY);
	if (fd_ts < 0) {
		printf("open device %s failed, errno = %d\n", TS_DEV, errno);
		return false;
	}
	fd_kpd = open(KPD_DEV, O_RDONLY);
	if (fd_kpd < 0) {
		printf("open device %s failed, errno = %d\n", KPD_DEV, errno);
		return false;
	}
	fd_sysmgr = open(SYSMGR1_DEV, O_RDWR);
	if (fd_sysmgr < 0) {
		printf("open device %s failed, errno = %d\n", SYSMGR1_DEV, errno);
		return false;
	}

	power_state = POWER_STATE_NORMAL;
	old_power_state = POWER_STATE_NORMAL;
	cur_time = GetTime();
	prev_time = cur_time;
	
	return true;
} 

void PM_Service(void)
{
	fd_set		rfd;
	struct		timeval to;
	unsigned int	status;
	int		ret;
	
	status = 0x0;
	to.tv_sec = 0;
	to.tv_usec = 0;
	FD_ZERO(&rfd);
	FD_SET(fd_sysmgr, &rfd);
	ret = select(fd_sysmgr+1, &rfd, NULL, NULL, &to);
	if (ret < 0) {
		perror("select() failed");
	}
	else if (ret > 0) {
		if (FD_ISSET(fd_sysmgr, &rfd)) {
			read(fd_sysmgr, &status, 4);
			//printf("system status is %d \n", status);
			ProcessSysMgrStatus(status);
		}
	}
	ProcessPowerManagement();
}

int main(void)
{
	printf("####################\n");
	printf("Copyright (c) 2009 - 2012 Nuvoton All rights reserved, Power Manager (%s - %s)\n", __DATE__, __TIME__);
	printf("####################\n");

	if (!PM_Init()) {
		printf("Power Manager initial failed!\n");
		return -1;
	}

	while(1) {
		PM_Service();
		usleep(50000);
	}

	return 0;
}


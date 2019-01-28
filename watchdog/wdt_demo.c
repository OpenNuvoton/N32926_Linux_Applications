#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/watchdog.h>
#define WDIOC_GETTIMELEFT        _IOR(WATCHDOG_IOCTL_BASE, 10, int)

int main(void)
{

	int i = 0;
	int interval,bootstatus;
	int new_interval = 7, timeval;
	
	int fd = open("/dev/watchdog", O_RDWR);

	printf("\nWatchdog Demo code\n");
	if (fd == -1) {
		perror("watchdog");
		exit(EXIT_FAILURE);
	}
	else
	{
		if(ioctl(fd, WDIOC_GETBOOTSTATUS, &bootstatus) ==0)
			printf("  Last boot is caused by %s.\n", (bootstatus!=0)? "Watchdog":"Power-On-Reset");

		if(ioctl(fd, WDIOC_GETTIMEOUT, &interval) ==0)
			printf("  Current watchdog interval is %d\n",interval);

		if(ioctl(fd, WDIOC_SETTIMEOUT, &new_interval) ==0)
			printf("  Set Current watchdog interval to %d\n",new_interval);
			
		if(ioctl(fd, WDIOC_GETTIMEOUT, &interval) ==0)
			printf("  Current watchdog interval is %d\n",interval);

		printf("Watchdog Test\n");
		printf("  Current interval is %d seconds.\n",interval);		
		printf("  Keep alive every 5 seconds (2 times).\n");

		while (1){
			if(i++ < 2)
			{
				printf("      *Keep alive %d\n", i);
				ioctl(fd, WDIOC_KEEPALIVE);
				if(ioctl(fd, WDIOC_GETTIMELEFT, &timeval) ==0)
					printf("        Time to WDT reset %dms\n",timeval *10);
			}
			else
			{
				printf("        Sleep 2 seconds\n");
				sleep(2);
				break;		
			}
			printf("        Sleep 1 second\n");
			sleep(1);
			if(ioctl(fd, WDIOC_GETTIMELEFT, &timeval) ==0)
				printf("        Time to WDT reset %dms\n",timeval *10);		
	
			printf("        Sleep 2 seconds\n");
			sleep(2);
			if(ioctl(fd, WDIOC_GETTIMELEFT, &timeval) ==0)
				printf("        Time to WDT reset %dms\n",timeval *10);		
		}
		printf("        Sleep 1 second\n");
		sleep(1);
		while(1)
		{
			if(ioctl(fd, WDIOC_GETTIMELEFT, &timeval) ==0)
				printf("        Time to WDT reset %dms\n",timeval *10);
			if(timeval > 10)
				usleep(100000);
			else
				usleep(10000);			
		}
		while(1);
		
	}		

	return 0;
}


/****************************************************************************
 *                                                                          *
 * Copyright (c) 2010 Nuvoton Technology Corp. All rights reserved.         *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 *
 * FILENAME
 *     kpd.c
 *
 * DESCRIPTION
 *     This file is a kpd demo program
 *
 **************************************************************************/

#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/ioctl.h>
#include<pthread.h>
#include<fcntl.h>
#include<linux/input.h>
#include<string.h>
#include<strings.h>

#define test_bit(bit, array)    (array[bit/8] & (1<<(bit%8)))

#define RETRY_NUM 4
char device[] = "/dev/input/event0";

static int fd= -1;
static int init_device(void)
{
  char devname[256];
  static char keypad[] = "Keypad";
  struct input_id device_info;
  unsigned int yalv, i, result;
  unsigned char evtype_bitmask[EV_MAX/8 + 1];

  result = 0;
  for (i = 0; i < RETRY_NUM; i++) {
    printf("trying to open %s ...\n", device);
    if ((fd=open(device,O_RDONLY)) < 0) {		
      break;
    }
    memset(devname, 0x0, sizeof(devname));
    if (ioctl(fd, EVIOCGNAME(sizeof(devname)), devname) < 0) {
      printf("%s evdev ioctl error!\n", device);
    } else {    
      if (strstr(devname, keypad) != NULL) {
        printf("Keypad device is %s\n", device);
        result = 1;
        break;
      }
    }	
    close(fd);
    device[16]++;
  }
  if (result == 0) {
    printf("can not find any Keypad device!\n");
    return -1;
  }

  ioctl(fd, EVIOCGBIT(0, sizeof(evtype_bitmask)), evtype_bitmask);
    
  printf("Supported event types:\n");
  for (yalv = 0; yalv < EV_MAX; yalv++) {
    if (test_bit(yalv, evtype_bitmask)) {
      /* this means that the bit is set in the event types list */
      printf("  Event type 0x%02x ", yalv);
      switch ( yalv)
	{
	case EV_SYN :
	  printf(" EV_SYN\n");
	  break;
	case EV_REL :
	  printf(" EV_REL\n");
	  break;
	case EV_KEY :
	    printf(" EV_KEY\n");
	    break;
	default:
	  printf(" Other event type: 0x%04hx\n", yalv);
	}
      
    }        
  }
  return 0;
}

int main(void)
{
  struct input_event data;

  if(init_device()<0) {
    printf("init error\n");
    return -1;
  }
  for(;;){

    read(fd,&data,sizeof(data));
    if(data.type == 0)
        continue;
        printf("%d.%06d ", data.time.tv_sec, data.time.tv_usec);
        printf("%d %d %d %s\n", data.type, data.code, data.value, (data.value == 0) ? "up" : "down");

  }
}

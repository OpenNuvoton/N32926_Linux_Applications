
/* hid_gadget_test */
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "Pattern.h"

#define BUF_LEN 	512
#define PACKET_SIZE	512
int fd = 0;

#define HID_CMD_SIGNATURE   0x43444948

/* HID Transfer Commands */
#define HID_CMD_NONE     0x00
#define HID_CMD_ERASE    0x71
#define HID_CMD_READ     0xD2
#define HID_CMD_WRITE    0xC3

#define PAGE_SIZE	256
#define PATTERN_SIZE	PAGE_SIZE*1000

typedef struct{
	unsigned char	u8Cmd;
	unsigned char	u8Size;
	unsigned int	u32Arg1;
	unsigned int	u32Arg2;
	unsigned int	u32Signature;
	unsigned int	u32Checksum;
}__attribute__ ((packed)) CMD_T;

char data[PATTERN_SIZE];
char command[64];

CMD_T  gCmd;    
CMD_T  *pgCmd; 

int HID_CmdEraseSectors(CMD_T *pCmd)
{
	unsigned int u32StartSector;
	unsigned int u32Sectors;
	unsigned int i;
    
	u32StartSector = pCmd->u32Arg1;
	u32Sectors = pCmd->u32Arg2;

	printf("Erase command - Sector: %d Sector Cnt: %d\n", u32StartSector, u32Sectors);

	/* To note the command has been done */
	pCmd->u8Cmd = HID_CMD_NONE;
	
	return 0;
}
int HID_CmdReadPages(CMD_T *pCmd)
{
	unsigned int u32StartPage;
	unsigned int u32Pages;
	unsigned int u32Address;
	unsigned int u32Count = 0;
	unsigned int offset = 0;
	unsigned int status = 0;
	unsigned int u32Len = 0;
	u32StartPage = pCmd->u32Arg1;
	u32Pages     = pCmd->u32Arg2;

	if(u32Pages)
	{
		u32Len = u32Pages * PAGE_SIZE;
		printf("HID_CmdReadPages %d\n",u32Len);
		offset = 0;
		while(u32Len)
		{
			//printf("Send %d Total %d\n",PACKET_SIZE, u32Count);
			status = write(fd, PatternRead+offset, PACKET_SIZE);
			if ( status != PACKET_SIZE) {
				printf("Send %d Wrong - Status %d\n",PACKET_SIZE, status);
				return 5;
			}
			offset += PACKET_SIZE;
			if(offset >= sizeof(PatternRead))
				offset = 0;
			u32Len -= PACKET_SIZE;
			u32Count += PACKET_SIZE;
		}
	}
	return 0;
}

int HID_CmdWritePages(CMD_T *pCmd)
{
	unsigned int u32StartPage;
	unsigned int u32Pages;
	unsigned int u32Address;
	unsigned int u32Count = 0;
	unsigned int offset = 0;
	unsigned int status = 0;
	unsigned int u32Len = 0;
 	unsigned int i = 0;   

	u32StartPage = pCmd->u32Arg1;
	u32Pages     = pCmd->u32Arg2;

	if(u32Pages)
	{
		u32Len = u32Pages * PAGE_SIZE;
		printf("HID_CmdWritePages %d\n",u32Len);
		offset = 0;
		while(u32Len)
		{
			//printf("Receive %d Total %d\n",PACKET_SIZE, u32Count);
			status = read(fd, data+offset, PACKET_SIZE);
			if ( status != PACKET_SIZE) {
				printf("Receive %d Wrong - Status %d\n",PACKET_SIZE, status);
				return 5;
			}
			offset += PACKET_SIZE;
			if(offset >= sizeof(PatternWrite))
				offset = 0;
			u32Len -= PACKET_SIZE;
			u32Count += PACKET_SIZE;
		}
	}
	printf("  Compare Data - Size %d\n",u32Pages * PAGE_SIZE);
	for(i = 0;i<u32Pages * PAGE_SIZE;i++)
	{
		if(PatternWrite[i] != data[i])
		{
			printf("[%d] Pattern[%02X] != Readback[%02X]",i, PatternWrite[i],data[i]);
			printf("    Compare Fail!!\n");
			return;
		}
	}
	printf("    Compare Pass!!\n");

	return 0;
}

unsigned int CalCheckSum(unsigned char *buf, unsigned int size)
{
	unsigned int sum;
	int i;

	i = 0;
	sum = 0;
	while(size--)
	{
		sum+=buf[i++];
	}

	return sum;
}

int ProcessCommand(unsigned char *pu8Buffer,  unsigned int u32BufferLen)
{
	unsigned int u32sum;
	pgCmd = (CMD_T  *)( (unsigned int)pu8Buffer);
   	 /* Check size */
    	if((pgCmd->u8Size > sizeof(gCmd)) || (pgCmd->u8Size > u32BufferLen))    
     		return -1;
        
   	 /* Check signature */
    	if(pgCmd->u32Signature != HID_CMD_SIGNATURE)
    		return -1;
        
	/* Calculate checksum & check it*/        
	u32sum = CalCheckSum((unsigned char *)pgCmd, pgCmd->u8Size);
	if(u32sum != pgCmd->u32Checksum)
        	return -1;

	/* It's Command and Copy command from Buffer to Command Parameter */        
    	memcpy((unsigned char *)&gCmd, (unsigned char  *)pgCmd, sizeof(gCmd));
    
   	/* Check Command */
	switch(gCmd.u8Cmd)
	{
		case HID_CMD_ERASE:
		{
			HID_CmdEraseSectors(&gCmd);
			break;
		}		
		case HID_CMD_READ:
		{
			HID_CmdReadPages(&gCmd);
			break;
		}		
		case HID_CMD_WRITE:
		{
			HID_CmdWritePages(&gCmd);
			break;		
		}		
		default:
			return -1;
	}	
	
	return 0;
}

int main(int argc, const char *argv[])
{
	const char *filename = NULL;
	int status;
	char buf[BUF_LEN];
	int cmd_len;
	fd_set rfds;
	int retval, i;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s devname\n",
			argv[0]);
		return 1;
	}
		
	filename = argv[1];

	if ((fd = open(filename, O_RDWR, 0666)) == -1) {
		perror(filename);
		return 3;
	}


	printf("Press any keyboard to start\n");
	while (42) {

		FD_ZERO(&rfds);
		FD_SET(STDIN_FILENO, &rfds);
		FD_SET(fd, &rfds);

		retval = select(fd + 1, &rfds, NULL, NULL, NULL);
		if (retval == -1 && errno == EINTR)
			continue;
		if (retval < 0) {
			perror("select()");
			return 4;
		}

		if (FD_ISSET(fd, &rfds)) {
			cmd_len = read(fd, buf, BUF_LEN - 1);
			printf("recv report:");
			for (i = 0; i < cmd_len; i++)
				printf(" %02x", buf[i]);
			printf("\n");
		}
		if (FD_ISSET(STDIN_FILENO, &rfds)) {
			cmd_len = read(STDIN_FILENO, buf, BUF_LEN - 1);

			if (cmd_len == 0)
				break;

			while(1)
			{
				status = read(fd, command, PACKET_SIZE);

				if ( status != PACKET_SIZE) {
					perror(filename);
					printf("Receive Command %d Wrong - Status %d\n",PACKET_SIZE, status);
					return 5;
				}
				ProcessCommand(command,PACKET_SIZE);	
			}
		}
	}

	close(fd);
	return 0;
}

#include <stdio.h>		/* Standard input/output definitions */
#include <string.h>		/* String function definitions */
#include <unistd.h>		/* UNIX standard function definitions */
#include <fcntl.h>		/* File control definitions */
#include <errno.h>		/* Error number definitions */
#include <asm/termios.h>	/* POSIX terminal control definitions */

#define PATTERN_SIZE	1024
#define NO_FLOW_CONTROL 0
#define HW_FLOW_CONTROL 1
#define SW_FLOW_CONTROL 2

int open_port(void)
{
	int fd, fset;

	//fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
	fd = open("/dev/ttyS0", O_RDWR);

	fset = fcntl(fd, F_SETFL, 0);
	if (fset < 0)
		printf("fcntl failed!\n");
	else
		printf("fcntl = %d\n", fset);

	if (isatty(STDIN_FILENO) == 0)
		printf("standard input is not a terminal device\n");
	else
		printf("isatty success!\n");

	printf("fd = %d\n", fd);

	return fd;
}

static int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop, int nControl)
{
	struct termios newtio, oldtio;
	if (tcgetattr(fd, &oldtio) != 0) { 
		perror("SetupSerial 1");
		return -1;
	}
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag |= CLOCAL | CREAD; 
	newtio.c_cflag &= ~CSIZE; 

	switch (nBits) {
	case 7:
		newtio.c_cflag |= CS7;
		break;
	case 8:
		newtio.c_cflag |= CS8;
		break;
	}

	switch (nEvent) {
	case 'O':
		newtio.c_cflag |= PARENB;
		newtio.c_cflag |= PARODD;
		newtio.c_iflag |= (INPCK | ISTRIP);
		break;
	case 'E': 
		newtio.c_iflag |= (INPCK | ISTRIP);
		newtio.c_cflag |= PARENB;
		newtio.c_cflag &= ~PARODD;
		break;
	case 'N': 
		newtio.c_cflag &= ~PARENB;
		break;
	}

	switch (nSpeed) {
	case 2400:
		cfsetispeed(&newtio, B2400);
		cfsetospeed(&newtio, B2400);
		break;
	case 4800:
		cfsetispeed(&newtio, B4800);
		cfsetospeed(&newtio, B4800);
		break;
	case 9600:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
		break;
	case 19200:
		cfsetispeed(&newtio, B19200);
		cfsetospeed(&newtio, B19200);
		break;
	case 57600:
		cfsetispeed(&newtio, B57600);
		cfsetospeed(&newtio, B57600);
		break;
	case 115200:
		cfsetispeed(&newtio, B115200);
		cfsetospeed(&newtio, B115200);
		break;
	case 230400:
		cfsetispeed(&newtio, B230400);
		cfsetospeed(&newtio, B230400);
		break;
	case 460800:
		cfsetispeed(&newtio, B460800);
		cfsetospeed(&newtio, B460800);
		break;
	case 921600:
		cfsetispeed(&newtio, B921600);
		cfsetospeed(&newtio, B921600);
		break;
	case 1000000:
		cfsetispeed(&newtio, B1000000);
		cfsetospeed(&newtio, B1000000);
		break;
	case 1500000:
		cfsetispeed(&newtio, B1500000);
		cfsetospeed(&newtio, B1500000);
		break;
	case 2000000:
		cfsetispeed(&newtio, B2000000);
		cfsetospeed(&newtio, B2000000);
		break;
	case 3000000:
		cfsetispeed(&newtio, B3000000);
		cfsetospeed(&newtio, B3000000);
		break;
	case 4000000:
		cfsetispeed(&newtio, B4000000);
		cfsetospeed(&newtio, B4000000);
		break;
	default:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
		break;
	}
	if (nStop == 1)
		newtio.c_cflag &= ~CSTOPB;
	else if (nStop == 2)
		newtio.c_cflag |= CSTOPB;

	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 0;
	//newtio.c_cc[VTIME] = 1;
	//newtio.c_cc[VMIN] = 0;

	//newtio.c_lflag &= ~( ECHO | ECHOE | ISIG);
	//newtio.c_lflag |=ICANON;
	//newtio.c_oflag &= ~OPOST;
	//newtio.c_iflag |= (IGNPAR | ICRNL);

	if (nControl == NO_FLOW_CONTROL) {
		//newtio.c_cflag &= !CRTSCTS;
		//newtio.c_iflag &= !(IXON | IXOFF | IXANY);
	} else if (nControl == HW_FLOW_CONTROL)
		newtio.c_cflag |= CRTSCTS;
	else if (nControl == SW_FLOW_CONTROL)
		//newtio.c_iflag |= (IXON | IXOFF | IXANY);
		newtio.c_iflag |= (IXON | IXOFF);
	else {
		printf("wrong flow control setting !!\n");
		return -1;
	}
	printf("c_cflag=%o, c_iflag=%o\n", newtio.c_cflag, newtio.c_iflag);

	tcflush(fd,TCIFLUSH);
	
	if (tcsetattr(fd, TCSANOW, &newtio) != 0) {
		perror("com set error");
		return -1;
	}
	printf("set done!\n");

	return 0;
}

int write_port(int fd, unsigned char *buf, size_t len)
{
	int num;

	num = write(fd, buf, len);
	if (num < 0)
		printf("write failed! (%s)\n", strerror(errno));
	else

	return num;
}

int read_port(int fd, unsigned char *buf, size_t len, struct timeval *tout)
{
	fd_set inputs;
	int num, ret;
	
	num = 0 ;	

	FD_ZERO(&inputs);
	FD_SET(fd, &inputs);

	ret = select(fd+1, &inputs, (fd_set *)NULL, (fd_set *)NULL, tout);
	//printf("select = %d\n", ret);
	if (ret < 0) {
		perror("select error!!");
		return ret;
	}
	if (ret > 0) {
		if (FD_ISSET(fd, &inputs)) {
			num = read(fd, buf, len);			
		}
	}
	
	return num;
}

int main(int argc, char* argv[])
{
	struct timeval tout;
	unsigned char send_buf[PATTERN_SIZE];
	unsigned char recv_buf[PATTERN_SIZE];
	int fd, baud, loop, error, i, send_num, recv_num, offset;

	fd = open_port();
	if (fd < 0) {
		perror("open_port error!\n");
		return -1;
	}

	if (argc == 1)
		baud = 115200;
	else
		baud = atoi(argv[1]);

	if (set_opt(fd, baud, 8, 'N', 1, NO_FLOW_CONTROL) < 0) {
		perror("set_opt error!\n");
		return -1;
	}

	loop = 1;
	error = 0;	
	srand((unsigned) time(NULL));
	while (1) {
		printf("Start %d loop test\n", loop++);
		for (i = 0; i < PATTERN_SIZE; i++)
			send_buf[i] = rand() & 0xFF;

		offset = 0;
		while (offset < PATTERN_SIZE) {
			send_num = write_port(fd, send_buf+offset, PATTERN_SIZE-offset);
			if (send_num > 0) {
				offset += send_num;
				printf("Write %d bytes, total = %d\n\n", send_num, offset);
			}
		}
	
		offset = 0;
		while (offset < PATTERN_SIZE) {
			tout.tv_sec = 1;
			tout.tv_usec = 100*1000;
			recv_num = read_port(fd, recv_buf+offset, PATTERN_SIZE-offset, &tout);
			if (recv_num < 0)
				printf("read failed! (%s)\n", strerror(errno));
			else if (recv_num > 0) {
				offset += recv_num;
				printf("Read %d bytes, total = %d\n", recv_num, offset);
			}
		}
	
		for (i = 0; i < PATTERN_SIZE; i++) {
			//printf("\trecv_buf[%d] = 0x%02X\n", i, recv_buf[i]);
			if (send_buf[i] != recv_buf[i]) {
				printf("%dth data mismatch! send=0x%02X, recv=0x%02X\n", i, send_buf[i], recv_buf[i]);
				error = 1;
			}
		}
		if (error)
			return -1;
	}

	return 0;
}


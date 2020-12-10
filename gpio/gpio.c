/*
 * GPIO Driver Test/Example Program
 *
 * Compile with:
 *  gcc -s -Wall -Wstrict-prototypes gpio.c -o gpiotest
 *
 *
 * Note :
 *   PORT NAME[PIN] = GPIO [id]
 *
 *   PORTA[ 0]      = gpio[ 0x00]
 *   PORTA[ 1]      = gpio[ 0x01]
 *                  :
 *   PORTA[31]      = gpio[ 0x1F]
 *   PORTB[ 0]      = gpio[ 0x20]
 *                  :
 *   PORTB[31]      = gpio[ 0x3F]
 *                  :
 *                  :
 *                  :
 *   PORTE[ 0]      = gpio[ 0x80]
 *                  :
 *                  :
 *   PORTE[31]      = gpio[ 0x9F]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

/****************************************************************
* Constants
****************************************************************/

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define POLL_TIMEOUT (3 * 1000) /* 3 seconds */
#define MAX_BUF 64

/****************************************************************
 * gpio_export
 ****************************************************************/
int gpio_export(unsigned int gpio)
{
    int fd, len;
    char buf[MAX_BUF];

    fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/export");
        return fd;
    }

    len = snprintf(buf, sizeof(buf), "%d", gpio);
    write(fd, buf, len);
    close(fd);

    return 0;
}

/****************************************************************
 * gpio_unexport
 ****************************************************************/
int gpio_unexport(unsigned int gpio)
{
    int fd, len;
    char buf[MAX_BUF];

    fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/export");
        return fd;
    }

    len = snprintf(buf, sizeof(buf), "%d", gpio);
    write(fd, buf, len);
    close(fd);
    return 0;
}

/****************************************************************
 * gpio_set_dir
 ****************************************************************/
int gpio_set_dir(unsigned int gpio, unsigned int out_flag)
{
    int fd, len;
    char buf[MAX_BUF];

    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);

    fd = open(buf, O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/direction");
        return fd;
    }

    if (out_flag)
        write(fd, "out", 4);
    else
        write(fd, "in", 3);

    close(fd);
    return 0;
}

/****************************************************************
 * gpio_set_value
 ****************************************************************/
int gpio_set_value(unsigned int gpio, unsigned int value)
{
    int fd, len;
    char buf[MAX_BUF];

    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

    fd = open(buf, O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/set-value");
        return fd;
    }

    if (value)
        write(fd, "1", 2);
    else
        write(fd, "0", 2);

    close(fd);
    return 0;
}

/****************************************************************
 * gpio_get_value
 ****************************************************************/
int gpio_get_value(unsigned int gpio, unsigned int *value)
{
    int fd, len;
    char buf[MAX_BUF];
    char ch;

    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

    fd = open(buf, O_RDONLY);
    if (fd < 0)
    {
        perror("gpio/get-value");
        return fd;
    }

    read(fd, &ch, 1);

    if (ch != '0')
    {
        *value = 1;
    }
    else
    {
        *value = 0;
    }

    close(fd);
    return 0;
}


/****************************************************************
 * gpio_set_edge
 ****************************************************************/

int gpio_set_edge(unsigned int gpio, char *edge)
{
    int fd, len;
    char buf[MAX_BUF];

    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);

    fd = open(buf, O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/set-edge");
        return fd;
    }

    write(fd, edge, strlen(edge) + 1);
    close(fd);
    return 0;
}

/****************************************************************
 * gpio_fd_open
 ****************************************************************/

int gpio_fd_open(unsigned int gpio)
{
    int fd, len;
    char buf[MAX_BUF];

    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

    fd = open(buf, O_RDONLY | O_NONBLOCK );
    if (fd < 0)
    {
        perror("gpio/fd_open");
    }
    return fd;
}

/****************************************************************
 * gpio_fd_close
 ****************************************************************/

int gpio_fd_close(int fd)
{
    return close(fd);
}

/****************************************************************
 * Main
 ****************************************************************/
int main(void)
{
    struct pollfd fdset;
    int gpio_fd, rc, i;
    char *buf[MAX_BUF];
    unsigned int gpio = 32;
    int len;
    unsigned int toggle = 0, value;

    printf("\n**** GPIO Test Program ****\n");
    printf("1. Output \n");
    printf("2. Input \n");
    printf("3. Interrupt \n");
    printf("Select:");

    scanf("%d", &i);

    if(i == 1)
    {
        gpio_export(gpio);
        gpio_set_dir(gpio, 1);

        while (1)
        {
            toggle = !toggle;
            if(toggle)
            {
                gpio_set_value(gpio, 1);
                printf("\nGPIO %d output high\n", gpio);
            }
            else
            {
                gpio_set_value(gpio, 0);
                printf("\nGPIO %d output low\n", gpio);
            }

            usleep(500000);
        }
    }
    else if(i == 2)
    {
        gpio_export(gpio);
        gpio_set_dir(gpio, 0);

        while (1)
        {
            gpio_get_value(gpio, &value);
            printf("\nGPIO %d pin status %d\n", gpio, value);

            usleep(500000);
        }
    }
    else if(i == 3)
    {
        gpio_export(gpio);
        gpio_set_dir(gpio, 0);
        gpio_set_edge(gpio, "rising");
        gpio_fd = gpio_fd_open(gpio);

        read(gpio_fd, buf, MAX_BUF);

        while (1)
        {
            memset(&fdset, 0, sizeof(fdset));

            fdset.fd = gpio_fd;
            fdset.events = POLLPRI;

            rc = lseek(gpio_fd,0,SEEK_SET);
            if( rc == -1 )
            {
                printf("\nlseek() failed!\n");
                return -1;
            }

            rc = poll(&fdset, 1, POLL_TIMEOUT);

            if (rc < 0)
            {
                printf("\npoll() failed!\n");
                return -1;
            }

            if (rc == 0)
            {
                printf(".");
            }

            if (fdset.revents & POLLPRI)
            {
                len = read(fdset.fd, buf, MAX_BUF);
                printf("\npoll() GPIO %d interrupt occurred\n", gpio);
            }
            fflush(stdout);

        }

        gpio_fd_close(gpio_fd);
    }

    return 0;
}

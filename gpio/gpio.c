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
 *   PORTH[ 0]      = gpio[ 0xC0]
 *                  :
 *                  :
 *   PORTH[31]      = gpio[ 0xDF]
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

FILE *fp;

int main(void)
{
	char buffer[10];
	int value, i;
	int toggle = 0;

	printf("\n**** GPIO Test Program ****\n");
	printf("1. Output \n");
	printf("2. Input \n");
	printf("Select:");

	scanf("%d", &i);
	
	if(i == 1)
	{
		// equivalent shell command "echo 32 > export" to export the port 
		if ((fp = fopen("/sys/class/gpio/export", "w")) == NULL) {
			printf("Cannot open export file.\n");
			exit(1);
		}
		fprintf(fp, "%d", 32);
		fclose(fp);

		// equivalent shell command "echo out > direction" to set the port as an input  
		if ((fp = fopen("/sys/class/gpio/gpio32/direction", "rb+")) == NULL) {
			printf("Cannot open direction file.\n");
			exit(1);
		}
		fprintf(fp, "out");
		fclose(fp);
					
		while (1) {
			if ((fp = fopen("/sys/class/gpio/gpio32/value", "rb+")) == NULL) {
				printf("Cannot open value file.\n");
				exit(1);
			} else {
				toggle = !toggle;
				if(toggle)
					strcpy(buffer,"1");
				else
					strcpy(buffer,"0");
			
				fwrite(buffer, sizeof(char), sizeof(buffer) - 1, fp);			
				printf("ste out value: %c\n", buffer[0]);
				fclose(fp);
			}
			usleep(500000);
		}
	}
	else if(i == 2)
	{
		// equivalent shell command "echo 32 > export" to export the port 
		if ((fp = fopen("/sys/class/gpio/export", "w")) == NULL) {
			printf("Cannot open export file.\n");
			exit(1);
		}
		fprintf(fp, "%d", 32);
		fclose(fp);

		// equivalent shell command "echo in > direction" to set the port as an input  
		if ((fp = fopen("/sys/class/gpio/gpio32/direction", "rb+")) == NULL) {
			printf("Cannot open direction file.\n");
			exit(1);
		}
		fprintf(fp, "in");
		fclose(fp);

		// equivalent shell command "cat value"    		
		while (1) {
			if ((fp = fopen("/sys/class/gpio/gpio32/value", "rb")) == NULL) {
				printf("Cannot open value file.\n");
				exit(1);
			} else {
				fread(buffer, sizeof(char), sizeof(buffer) - 1, fp);
				value = atoi(buffer);
				printf("value: %d\n", value);
				fclose(fp);
			}
			usleep(500000);
		}
	}

	return 0;
}

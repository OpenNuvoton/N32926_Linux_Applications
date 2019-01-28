/* audio.c
 *
 *
 * Copyright (c)2008 Nuvoton technology corporation
 * http://www.nuvoton.com
 *
 * Audio playback demo application
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/soundcard.h>
#include <sys/poll.h>
#include <pthread.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

char *pcmfiles[] = {
		"./8k.pcm",
		"./11.025k.pcm",
		"./16k.pcm",
		"./22.05k.pcm",
		"./24k.pcm",
		"./32k.pcm",
		"./44.1k.pcm",
		"./48k.pcm"};
		
const int samplerate[] = {
		8000,
		11025,
		16000,
		22050,
		24000,
		32000,
		44100,
		48000};
		
int p_dsp, p_mixer;
int r_dsp, r_mixer;
static int rec_stop = 0;

static volatile int rec_volume, play_volume;
int close_audio_play_device()
{
 
  close(p_dsp);
  close(p_mixer);
	
  return 0;	
}

int open_audio_play_device()
{
printf("*****\n");
	p_dsp = open("/dev/dsp", O_RDWR);
	if( p_dsp < 0 ){
		printf("Open dsp error\n");
		return -1;
	}
printf("====\n");	
	p_mixer = open("/dev/mixer", O_RDWR);
	if( p_mixer < 0 ){
		printf("Open mixer error\n");
		return -1;
	}
}

int close_audio_rec_device()
{
 
  close(r_dsp);
  close(r_mixer);
	
  return 0;	
}

int open_audio_rec_device()
{
	r_dsp = open("/dev/dsp1", O_RDWR);
	if( r_dsp < 0 ){
		printf("Open dsp error\n");
		return -1;
	}
//	printf("Open dsp OK\n");	
	r_mixer = open("/dev/mixer1", O_RDWR);

	if( r_mixer < 0 ){
		printf("Open mixer error\n");
		return -1;
	}
//	printf("Open mixer OK\n");		
}

int record_to_pcm(char *file, int samplerate, int loop)
{
	int i, status = 0, frag;
	int channels = 2, volume = 0x6464;
//	FILE *fd;
	int fd;
	char *buffer;
	struct timeval time;
	struct timezone timezone;
	int nowsec;
	int vol;

	// record from external Codec (for example NAU8822)	
	r_dsp = open("/dev/dsp", O_RDONLY, 0);
	if( r_dsp < 0 ){
		printf("Open dsp error\n");
		return -1;
	}
	r_mixer = open("/dev/mixer", O_RDWR);	

	if( r_mixer < 0 ){
		printf("Open mixer error\n");
		return -1;
	}
	
	if ((fd = open((char*)file, O_WRONLY | O_CREAT | O_TRUNC, 0)) == -1) 	
	{	
		printf("open %s error!\n", file);
		return 0;
	}
		
	printf("open %s OK!\n", file);		
	
	int format;
	format = AFMT_S16_LE;
	if (ioctl(r_dsp, SNDCTL_DSP_SETFMT, &format) == -1) 
	{
		perror("SNDCTL_DSP_SETFMT");
		return;	
	}

	if (ioctl(r_dsp, SNDCTL_DSP_CHANNELS, & channels) == -1) 
	{
		perror("SNDCTL_DSP_CHANNELS");
		return;
	}
		
	int speed = samplerate;  
	if (ioctl(r_dsp, SNDCTL_DSP_SPEED, &speed) == -1) 
	{
		perror("SNDCTL_DSP_SPEED");
		return;
	} 
	else 
		printf("Actual Speed : %d \n",speed);

	ioctl(r_mixer, MIXER_WRITE(SOUND_MIXER_MIC), &rec_volume);	
	printf("set mic volume = 0x%x   !!!\n", rec_volume);	

	ioctl(r_mixer, MIXER_WRITE(SOUND_MIXER_MIC), &rec_volume);	
	
	//set the recording source (MIC)
    int recsrc = SOUND_MASK_MIC;
  
    if (ioctl(r_mixer, SOUND_MIXER_WRITE_RECSRC,&recsrc) == -1) 
    {
    	printf("CD");
		return;
	}
	
//	printf("volume=%d\n", volume);	
	printf("volume=%d\n", rec_volume);	
	printf("samplerate=%d\n", samplerate);	
	printf("channel=%d\n", channels);				
//	printf("frag=%d\n", frag);		
//	buffer = (char *)malloc(frag);
	
	printf("begin recording!\n");					
	
    	signed short applicbuf[2048];         
//		int totalbyte= speed * channels * 2 * 5 * 3;
		int totalbyte= speed * channels * 2 * loop * 2;	// 16-bit width, two channels
		int totalword = totalbyte/2;
		int total = 0, count;

        while (total != totalword)
        {
			if(totalword - total >= 2048)
				count = 2048;
			else 
				count = totalword - total;
			
       		read(r_dsp, applicbuf, count);
            write(fd, applicbuf, count);
			total += count;   		
		}

	printf("record exit!!\n");
    close(r_dsp);
	close(r_mixer);
	close(fd);
	return 0;
}

int record_to_pcm_single(char *file, int samplerate, int loop)
{
	int i, status = 0, frag;
	int channel = 1, volume = 0x6464;
	FILE *fd;
	char *buffer;
	struct timeval time;
	struct timezone timezone;
	int nowsec;
	
	open_audio_rec_device();
	fd = fopen(file, "w+");
	if(fd == NULL)
	{	
		printf("open %s error!\n", file);
		return 0;
	}
		
	printf("open %s OK!\n", file);		
	ioctl(r_mixer, MIXER_WRITE(SOUND_MIXER_MIC), &rec_volume);	/* set MIC max volume. Only support from external  DAC*/	
	status = ioctl(r_dsp, SNDCTL_DSP_SPEED, &samplerate);
	if(status<0)
	{
		fclose(fd);
		printf("Sample rate not support\n");
		return -1;
	}	
	ioctl(r_dsp, SNDCTL_DSP_CHANNELS, &channel);
	ioctl(r_dsp, SNDCTL_DSP_GETBLKSIZE, &frag);
	
	fcntl(r_dsp, F_SETFL, O_NONBLOCK);				/* Set to nonblock mode */

	/* fragments, 2^12 = 4096 bytes */
	frag = 0x2000c;	
	ioctl(r_dsp, SNDCTL_DSP_SETFRAGMENT, &frag);
	ioctl(r_dsp, SNDCTL_DSP_GETBLKSIZE, &frag);	

	loop *= 2;		/*  (one channel) * (16-bits per sdample)/frag) */
	loop *= samplerate;
	printf("pow(2, 12) = %d\n", pow(2,12));
	loop /= pow(2, 12);	/* frag =0x200C.  0xC=12 */		

	printf("samplerate=%d\n", samplerate);	
	printf("channel=%d\n", channel);				
	printf("frag=%d\n", frag);		
	
	buffer = (char *)malloc(frag);

	gettimeofday(&time, &timezone);
	nowsec = time.tv_sec;	
	printf("begin recording!\n");					
	for(i=0;i<loop;i++)
	{
		int size=0;
		do
		{
			size = read(r_dsp, buffer, frag);	/* recording */
		}while(size <= 0);					
		fwrite(buffer, 1, frag, fd);		
	}		

	printf("----\n");	
	printf("record exit!!\n");
	close_audio_rec_device();		
	fclose(fd);
	free(buffer);
	
	return 0;
}

int play_pcm(char *file, int samplerate)
{
	int data, oss_format, channels, sample_rate;	
	int i, j;
	char *buffer;
	
	//printf("<=== %s ===>\n", file);
	open_audio_play_device();
	
	FILE *fd;
	fd = fopen(file, "r+");
    if(fd == NULL)
    {
    	printf("open %s error!\n", file);
    	return 0;
    }
    
//	data = 0x5050;
	data = 0x5A5A;	
	oss_format=AFMT_S16_LE;/*standard 16bit little endian format, support this format only*/
	sample_rate = samplerate;
	channels = 2;
	ioctl(p_dsp, SNDCTL_DSP_SETFMT, &oss_format);
	ioctl(p_mixer, MIXER_WRITE(SOUND_MIXER_PCM), &data);
	ioctl(p_dsp, SNDCTL_DSP_SPEED, &sample_rate);
	ioctl(p_dsp, SNDCTL_DSP_CHANNELS, &channels);
			
	int frag;
	ioctl(p_dsp, SNDCTL_DSP_GETBLKSIZE, &frag);
	buffer = (char *)malloc(frag);
	printf("frag=%d, add[0x%x]\n", frag, buffer);	
	
	while(!feof(fd))	
	{		
		audio_buf_info info;			
		do{			
			ioctl(p_dsp , SNDCTL_DSP_GETOSPACE , &info);			
			usleep(100);
		}while(info.bytes < frag);
		
		fd_set writefds;
		struct timeval tv;
		tv.tv_sec       = 0;
		tv.tv_usec      = 0;
		FD_ZERO( &writefds );
		FD_SET( p_dsp , &writefds );
		tv.tv_sec       = 0;
		tv.tv_usec      = 0;
		
		select( p_dsp + 1 , NULL , &writefds , NULL, &tv );
		if( FD_ISSET( p_dsp, &writefds ))
		{	
			i = fread(buffer, 1, frag, fd);
//			printf("Read [%d] bytes from file\n", i);
			if( i > 0 ) j = write(p_dsp, buffer, i);   	
//			printf("Write [%d] bytes into dsp\n", j);
		}	
		usleep(100);
	}
		

	int bytes;
	ioctl(p_dsp,SNDCTL_DSP_GETODELAY,&bytes);
	int delay = bytes / (sample_rate * 2 * channels);	
	sleep(delay);
	
	printf("Stop Play\n");
	fclose(fd);
	free(buffer);
	close_audio_play_device();
}

int play_pcm_single(char *file, int samplerate)
{
	int data, oss_format, channels, sample_rate;	
	int i;
	char *buffer;
	
	//printf("<=== %s ===>\n", file);
	open_audio_play_device();
	
	FILE *fd;
	fd = fopen(file, "r+");
    if(fd == NULL)
    {
    	printf("open %s error!\n", file);
    	return 0;
    }
    
	data = 0x5050;
	oss_format=AFMT_S16_LE;/*standard 16bit little endian format, support this format only*/
	sample_rate = samplerate;
	channels = 1;
	ioctl(p_dsp, SNDCTL_DSP_SETFMT, &oss_format);
	ioctl(p_mixer, MIXER_WRITE(SOUND_MIXER_PCM), &data);
	ioctl(p_dsp, SNDCTL_DSP_SPEED, &sample_rate);
	ioctl(p_dsp, SNDCTL_DSP_CHANNELS, &channels);
			
	int frag;
	ioctl(p_dsp, SNDCTL_DSP_GETBLKSIZE, &frag);
	buffer = (char *)malloc(frag);
	printf("frag=%d\n", buffer);	
	
	fread(buffer, 1, frag, fd);
	while(!feof(fd))	
	{		
		audio_buf_info info;			
		do{			
			ioctl(p_dsp , SNDCTL_DSP_GETOSPACE , &info);			
			usleep(100);
		}while(info.bytes < frag);
		
		fd_set writefds;
		struct timeval tv;
		tv.tv_sec       = 0;
		tv.tv_usec      = 0;
		FD_ZERO( &writefds );
		FD_SET( p_dsp , &writefds );
		tv.tv_sec       = 0;
		tv.tv_usec      = 0;
		
		select( p_dsp + 1 , NULL , &writefds , NULL, &tv );
		if( FD_ISSET( p_dsp, &writefds ))
		{	

			write(p_dsp, buffer, frag);   
			fread(buffer, 1, frag, fd);
		}	
		usleep(100);
	}
	
	int bytes;
	ioctl(p_dsp,SNDCTL_DSP_GETODELAY,&bytes);
	int delay = bytes / (sample_rate * 2 * channels);	
	sleep(delay);
	
	printf("Stop Play\n");
	fclose(fd);
	free(buffer);
	close_audio_play_device();
}

void * p_rec(void * arg)
{
	printf("recording ...\n");
	record_to_pcm("./rec.pcm", 16000, -1);	
}

void * p_play(void * arg)
{
	printf("playing ...\n");
	play_pcm("./16k.pcm", 16000);	
	rec_stop = 1;
}

signed short applicbuf[2048];
int record_to_pcm_adc(char *file, int samplerate, int seconds)
{
	int audio_fd, music_fd, mixer_fd, count;
	int mode;
	char para;
	int format;
	int channels = 1;   
	int recsrc;
	int totalbyte= samplerate * channels * 2 * seconds;
	int total = 0;
	int speed;
	
	printf("Sample rate = %d\n", samplerate);
	printf("Second = %d\n", seconds);
	printf("totalbyte = %d\n", totalbyte);

	if ((audio_fd = open("/dev/dsp1",O_RDONLY,0)) == -1) 
	{
		perror("/dev/dsp1");
		return;
	}
	printf("open dsp1 successful\n");
	/*  Setting Sampling Parameters
		1. sample format
		2. number of channels
		3. sampling rate
	*/
	format = AFMT_S16_LE;
	if (ioctl(audio_fd,SNDCTL_DSP_SETFMT, &format) == -1) 
	{
		perror("SNDCTL_DSP_SETFMT");
		return;	
	}	
	if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &samplerate) == -1) 
	{
		perror("SNDCTL_DSP_SPEED");
		return;
	} 
	else 
		printf("Actual Speed : %d \n",speed);
	

	/* Volume control */
	if ((mixer_fd = open("/dev/mixer1", O_WRONLY)) == -1) 
    {
      	perror("open /dev/mixer1 error");
        return;
    }
	printf("open mixer successful\n");

	ioctl(mixer_fd , MIXER_WRITE(SOUND_MIXER_PCM), &rec_volume);	
	printf("set mic volume = 0x%x   !!!\n", rec_volume);	

/*
	//set the recording source (MIC)
    recsrc = SOUND_MASK_MIC;    
	if (ioctl(mixer_fd, SOUND_MIXER_WRITE_RECSRC,&recsrc) == -1) 
	{
    	printf("CD");
		return;
	}
*/

	//recording
	if ((music_fd = open("./rec.pcm", O_WRONLY | O_CREAT | O_TRUNC, 0)) == -1) 
	{
        perror("rec.pcm");
        return;
    }      		

    
	/* 	recording three minutes
		sample format :16bit
		channel num: 2 channels
		sample rate = speed                                                             
		data rate = speed * 2 * 2	bytes/sec
	*/

    while (total != totalbyte)
    {
		if(totalbyte - total >= 2048)
			count = 2048;
		else 
			count = totalbyte - total;
		
    		read(audio_fd, applicbuf, count);
        write(music_fd, applicbuf, count);
		total += count;   		
	}

	close(mixer_fd);
	close(audio_fd);
	close(music_fd);
}

int main()
{
	int i, loop, sr;
	char path[256];
	int buf;
	
	//signal (SIGTERM, mySignalHandler); /* for the TERM signal.. */
	//signal (SIGINT, mySignalHandler); /* for the CTRL+C signal.. */
	
	//while(1)
	{
		printf("\n**** Audio Test Program ****\n");
		printf("1. Play \n");
		printf("2. Record and then Play (two channels, by external Codec)\n");
		printf("3. Record and then Play (one channel, by FA92 ADC)\n");		
//		printf("3. Record & Play \n");
		printf("Select:");
		
		scanf("%d", &i);
	
		if(i<1 || i >3)
		  return 0;
		 
		if(i == 1)
		{ 
			printf("Playing ...\n");
			
			for(i=0;i<8;i++)
				play_pcm(pcmfiles[i], samplerate[i]);
		}
		else if(i == 2)
		{
			printf("\nRec Seconds:");
			scanf("%d", &loop);
			
			printf("Sample Rate(8000, 16000..):");
			scanf("%d", &sr);
					
			printf("Record volume (0 - 100) :");
			scanf("%d", &rec_volume);
			
			if (rec_volume > 100) rec_volume = 100;					

			buf = rec_volume;
			rec_volume <<= 8;
			rec_volume |= buf;								

			printf(" *** Recording ***\n");
//			printf("SampleRate=%d, Time=%d sec\n", sr, loop);
			printf("SampleRate=%d, Time=%d sec, Volume=%d\n", sr, loop, buf);
			
	//		loop *= 4;		// "loop" is the multiple of 8KB (by default); 4: (two channels) * (16-bits per sdample)/8)
	//		loop *= sr;
	//		loop /= 8000;			
			
			record_to_pcm("./rec.pcm", sr, loop);
	
			printf("\nDone, Now play it ...\n");
	
			getchar();
			play_pcm("./rec.pcm", sr);
		}
		else if(i==3)
		{
			printf("\nRec Seconds:");
			scanf("%d", &loop);
			
			printf("Sample Rate(8000, 11025, 12000 or 16000):");
			scanf("%d", &sr);
			
			printf("Record volume (0 - 100) :");
			scanf("%d", &rec_volume);

			if (rec_volume > 100) rec_volume = 100;					

			buf = rec_volume;
			rec_volume <<= 8;
			rec_volume |= buf;								

			printf(" *** Recording ***\n");
//			printf("SampleRate=%d, Time=%d sec\n", sr, loop);
			printf("SampleRate=%d, Time=%d sec, Volume=%d\n", sr, loop, buf);		

//			record_to_pcm_single("./rec.pcm", sr, loop);
			record_to_pcm_adc("./rec.pcm", sr, loop);
	
			printf("\nDone, Now play it ...\n");
	
			getchar();
			play_pcm_single("./rec.pcm", sr);
		}
		else
			printf("wrong input\n");
	}
	
	return 0;
}

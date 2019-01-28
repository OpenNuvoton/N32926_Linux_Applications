/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp.  All rights reserved. *
 *                                                              *
 ****************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <linux/msdos_fs.h>

#include "dsc.h"

#define AVUtil_Malloc malloc
#define AVUtil_Free free


#define DSC_INDEX_FILE ".dsc.idx"

typedef struct 
{
	uint32_t u32Mon;      //month;
	uint32_t u32MDay;     //day of month;
 	uint32_t u32SeqNo;    //Sequence number of day
}S_DSC_IDX_ELEM;


static char *s_pchFilePath = NULL;
static uint32_t s_u32FilePathLen = 0;
static S_DSC_IDX_ELEM s_sCurIdxElem;



ERRCODE 
GetDSCIndexElem(
	char *pchDSCFolderPath
)
{
	int32_t i32PathLen;
	int32_t i32IdxFileFD;
	ERRCODE err =  ERR_DSC_SUCCESS;

	i32PathLen = strlen(pchDSCFolderPath) + strlen(DSC_INDEX_FILE) + 10;

	if((s_pchFilePath == NULL) || (s_u32FilePathLen < i32PathLen)){
		if(s_pchFilePath != NULL)
			AVUtil_Free(s_pchFilePath);
		s_pchFilePath = (char *)AVUtil_Malloc(i32PathLen);
		if(s_pchFilePath == NULL)
			return ERR_DSC_MALLOC;
		s_u32FilePathLen = i32PathLen;
	}
	sprintf(s_pchFilePath,"%s/"DSC_INDEX_FILE, pchDSCFolderPath);		

	i32IdxFileFD = open(s_pchFilePath, O_RDWR);
	if(i32IdxFileFD < 0){		
		return ERR_DSC_OPEN_INDEX_FILE;		
	}
	
	if(read(i32IdxFileFD, (char *)&s_sCurIdxElem, sizeof(S_DSC_IDX_ELEM)) != sizeof(S_DSC_IDX_ELEM)){
		err = ERR_DSC_OPEN_INDEX_FILE;
	}
	close(i32IdxFileFD);
	return err;
}

ERRCODE 
UpdateDSCIndexFile(
	char *pchDSCFolderPath
)
{
	int32_t i32PathLen;
	int32_t i32IdxFileFD;
	ERRCODE err =  ERR_DSC_SUCCESS;
	uint32_t u32FileAttr;
	
	i32PathLen = strlen(pchDSCFolderPath) + strlen(DSC_INDEX_FILE) + 10;

	if((s_pchFilePath == NULL) || (s_u32FilePathLen < i32PathLen)){
		if(s_pchFilePath != NULL)
			AVUtil_Free(s_pchFilePath);
		s_pchFilePath = (char *)AVUtil_Malloc(i32PathLen);
		if(s_pchFilePath == NULL)
			return ERR_DSC_MALLOC;
		s_u32FilePathLen = i32PathLen;
	}
	sprintf(s_pchFilePath,"%s/"DSC_INDEX_FILE, pchDSCFolderPath);		

	i32IdxFileFD = open(s_pchFilePath, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	if(i32IdxFileFD < 0){		
		return ERR_DSC_OPEN_INDEX_FILE;		
	}

	if(write(i32IdxFileFD, (char *)&s_sCurIdxElem, sizeof(S_DSC_IDX_ELEM)) != sizeof(S_DSC_IDX_ELEM)){
		err = ERR_DSC_OPEN_INDEX_FILE;
	}
	u32FileAttr = ATTR_HIDDEN;
	ioctl(i32IdxFileFD, FAT_IOCTL_SET_ATTRIBUTES, &u32FileAttr);
	close(i32IdxFileFD);	
	return err;
}




ERRCODE 
DSC_NewCapFile(
	char *pchDSCFolderPath,
	int *i32NewFileFD
)
{
	ERRCODE err;
	time_t i32CurTime;
	struct tm *psCurTm;
	uint32_t u32SeqNo;
	int32_t i32PathLen;
	int32_t i32CapFileFD;
	struct stat sFileStat;

//	DEBUG_PRINT(stdout,"%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
	i32CurTime = time(NULL);
	psCurTm = localtime(&i32CurTime); 


	err = GetDSCIndexElem(pchDSCFolderPath);
	if(err != ERR_DSC_SUCCESS){
		if(err == ERR_DSC_OPEN_INDEX_FILE){
			s_sCurIdxElem.u32Mon = 	psCurTm->tm_mon + 1;
			s_sCurIdxElem.u32MDay = psCurTm->tm_mday;
			s_sCurIdxElem.u32SeqNo = 0;
		}
		else{
			return err;
		}
	}

	if((s_sCurIdxElem.u32Mon != psCurTm->tm_mon + 1) || (s_sCurIdxElem.u32MDay != psCurTm->tm_mday)){
		u32SeqNo = 0;
	}
	else{
		u32SeqNo = s_sCurIdxElem.u32SeqNo + 1;
	}

	i32PathLen = strlen(pchDSCFolderPath) + 8 + 3 + 10;    //file format 8.3
	if((s_pchFilePath == NULL) || (s_u32FilePathLen < i32PathLen)){
		if(s_pchFilePath != NULL)
			AVUtil_Free(s_pchFilePath);
		s_pchFilePath = (char *)AVUtil_Malloc(i32PathLen);
		if(s_pchFilePath == NULL)
			return ERR_DSC_MALLOC;
		s_u32FilePathLen = i32PathLen;
	}
	sprintf(s_pchFilePath,"%s/KRV%02d%02d%03d.jpg", pchDSCFolderPath, psCurTm->tm_mon + 1, psCurTm->tm_mday, u32SeqNo);		

	while(stat(s_pchFilePath, &sFileStat) == 0){
		u32SeqNo ++;
		if(u32SeqNo > 999)
			return ERR_DSC_OPEN_CAP_FILE;				
		sprintf(s_pchFilePath,"%s/KRV%02d%02d%03d.jpg", pchDSCFolderPath, psCurTm->tm_mon + 1, psCurTm->tm_mday, u32SeqNo);		 
	}

	i32CapFileFD = open(s_pchFilePath, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
//	DEBUG_PRINT(stdout,"%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
	if(i32CapFileFD < 0){		
		return ERR_DSC_OPEN_CAP_FILE;		
	}
	s_sCurIdxElem.u32Mon = 	psCurTm->tm_mon + 1;
	s_sCurIdxElem.u32MDay = psCurTm->tm_mday;
	s_sCurIdxElem.u32SeqNo = u32SeqNo;
	*i32NewFileFD = i32CapFileFD;
	return ERR_DSC_SUCCESS;
}

void DSC_CloseCapFile(
	char *pchDSCFolderPath,
	int i32CloseFileFD
)
{
//	DEBUG_PRINT(stdout,"%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
	fsync(i32CloseFileFD);
	close(i32CloseFileFD);
	UpdateDSCIndexFile(pchDSCFolderPath);
	if(s_pchFilePath){
		AVUtil_Free(s_pchFilePath);
		s_pchFilePath = NULL;
	}
}







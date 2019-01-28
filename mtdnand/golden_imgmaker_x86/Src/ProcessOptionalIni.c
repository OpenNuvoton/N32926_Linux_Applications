/*-----------------------------------------------------------------------------------*/
/* Nuvoton Technology Corporation confidential                                       */
/*                                                                                   */
/* Copyright (c) 2012 by Nuvoton Technology Corporation                              */
/* All rights reserved                                                               */
/*-----------------------------------------------------------------------------------*/
/*
 * FILENAME
 *      ProcessOptionalIni.c
 * DESCRIPTION
 *      Process INI file for TurboWriter.ini.
 * HISTORY
 *      2012/9/24   Create Ver 1.0.
 * REMARK
 *      None.
 *-----------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NandImageMaker.h"

#if 1
    #define dbgprintf printf
#else
    #define dbgprintf(...)
#endif

#define DEF_OPTIONAL_PATTERN "[N3292 USER_DEFINE]"

#define INI_BUF_SIZE    8192
char iniBuf[INI_BUF_SIZE];
int  buffer_current = 0, buffer_end = 0;    // position to buffer iniBuf

/*-----------------------------------------------------------------------------
 * To read one string line from file FileHandle and save it to Cmd.
 * Return:
 *      OK  : OK
 *      < 0 : FAIL or error code
 *---------------------------------------------------------------------------*/
int readLine (FILE *FileHandle, char *Cmd)
{
    int status, nReadLen, i_cmd;

    i_cmd = 0;

    while (1)
    {
        //--- parse INI file in buffer iniBuf[] that read in at previous fsReadFile().
        while(buffer_current < buffer_end)
        {
            if (iniBuf[buffer_current] == 0x0D)
            {
                // DOS   use 0x0D 0x0A as end of line;
                // Linux use 0x0A as end of line;
                // To support both DOS and Linux, we treat 0x0A as real end of line and ignore 0x0D.
                buffer_current++;
                continue;
            }
            else if (iniBuf[buffer_current] == 0x0A)   // Found end of line
            {
                Cmd[i_cmd] = 0;     // end of string
                buffer_current++;
                return OK;
            }
            else
            {
                Cmd[i_cmd] = iniBuf[buffer_current];
                buffer_current++;
                i_cmd++;
            }
        }

        //--- buffer iniBuf[] is empty here. Try to read more data from file to buffer iniBuf[].

        // no more data to read since previous fsReadFile() cannot read buffer full
        if ((buffer_end < INI_BUF_SIZE) && (buffer_end > 0))
        {
            if (i_cmd > 0)
            {
                // return last line of INI file that without end of line
                Cmd[i_cmd] = 0;     // end of string
                return OK;
            }
            else
            {
                Cmd[i_cmd] = 0;     // end of string to clear Cmd
                return FAIL;  // end of file
            }
        }
        else
        {
            //status = fsReadFile(FileHandle, (UINT8 *)iniBuf, INI_BUF_SIZE, &nReadLen);
            nReadLen = fread(iniBuf, 1, INI_BUF_SIZE, FileHandle);
            if (nReadLen == 0)
            {
                Cmd[i_cmd] = 0;     // end of string to clear Cmd
                return FAIL;    // error or end of file
            }
            buffer_current = 0;
            buffer_end = nReadLen;
        }
    }   // end of while (1)
}


UINT32 power(UINT32 x, UINT32 n)
{
    UINT32 i;
    UINT32 num = 1;

    for(i = 1; i <= n; i++)
        num *= x;
    return num;
}


UINT32 transfer_hex_string_to_int(unsigned char *str_name)
{
    char string[]="0123456789ABCDEF";
    UINT32 number[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    UINT32 i, j;
    UINT32 str_number = 0;

    for(i = 0; i < strlen(str_name); i++)
    {
        for(j = 0; j < strlen(string); j++)
        {
            if(toupper(str_name[i]) == string[j])
            {
                str_number += power(16, (strlen(str_name)-1-i))* number[j];
                break;
            }
        }
    }
    return str_number;
}


/*-----------------------------------------------------------------------------
 * To parse one pair of setting and store to ptr_Ini_Config.
 * The format of a pair should be <hex address> = <hex value>
 *---------------------------------------------------------------------------*/
void parsePair(IBR_BOOT_OPTIONAL_STRUCT_T *ptr_Ini_Config, char *Cmd)
{
    char delim[] = " =\t";
    char *token;
    token = strtok(Cmd, delim);
    if (token != NULL)
        ptr_Ini_Config->Pairs[ptr_Ini_Config->Counter].address = transfer_hex_string_to_int(token);
    token = strtok(NULL, delim);
    if (token != NULL)
        ptr_Ini_Config->Pairs[ptr_Ini_Config->Counter].value = transfer_hex_string_to_int(token);
}


/*-----------------------------------------------------------------------------
 * To parse Boot Code Optional Setting INI file and store configuration to global variable ptr_Ini_Config.
 * Return:
 *      OK / FAIL
 *---------------------------------------------------------------------------*/
int ProcessOptionalINI(char *fileName, IBR_BOOT_OPTIONAL_STRUCT_T *ptr_Ini_Config, int i32Platform)
{
    CHAR Cmd[8192];
    CHAR pcPlatform[32];
    FILE *FileHandle;
    int  status, i;

    //--- initial default value
    memset ( (void*)ptr_Ini_Config, 0xFF, sizeof(IBR_BOOT_OPTIONAL_STRUCT_T) );
    ptr_Ini_Config->OptionalMarker  = IBR_BOOT_CODE_OPTIONAL_MARKER;
    ptr_Ini_Config->Counter         = 0;

    //--- open INI file
    FileHandle = fopen(fileName, "r");
    if (FileHandle == NULL)
    {
        printf("WARNING: Optional INI file open fail: %s not found !!\n", fileName);
        return FAIL;
    }

    //--- parse INI file
    buffer_current = 0;
    buffer_end = 0;    // reset position to buffer iniBuf in readLine()
    do {
        status = readLine(FileHandle, Cmd);
        if (status < 0)     // read file error. Coulde be end of file.
            break;
NextMark2:
		//printf("%s \n", Cmd);
		sprintf(pcPlatform, "[N329%d USER_DEFINE]", i32Platform);
        if (strcmp(Cmd, pcPlatform) == 0)
        {
            do {
                status = readLine(FileHandle, Cmd);
                if (status < 0)
                    break;          // use default value since error code from NVTFAT. Coulde be end of file.
                else if (Cmd[0] == 0)
                    continue;       // skip empty line
                else if ((Cmd[0] == '/') && (Cmd[1] == '/'))
                    continue;       // skip comment line
                else if (Cmd[0] == '[')
                    goto NextMark2; // use default value since no assign value before next keyword
                else
                {
                    //dbgprintf("got pair #%d [%s]\n", ptr_Ini_Config->Counter, Cmd);
                    if (ptr_Ini_Config->Counter >= IBR_BOOT_CODE_OPTIONAL_MAX_NUMBER)
                    {
                    	printf("ERROR: The number of Boot Code Optional Setting pairs cannot > %d !\n", IBR_BOOT_CODE_OPTIONAL_MAX_NUMBER);
                    	printf("       Some pairs be ignored !\n");
                    	break;
                    }
                    parsePair(ptr_Ini_Config, Cmd);
                    ptr_Ini_Config->Counter++;
                }
            } while (1);
        }
    } while (status >= 0);  // keep parsing INI file
    
    
    //--- show final configuration
	dbgprintf("** Process %s file **\n", fileName);
	dbgprintf("  OptionalMarker = 0x%08X\n", ptr_Ini_Config->OptionalMarker);
	dbgprintf("  Counter        = %d\n",     ptr_Ini_Config->Counter);
    for (i = 0; i < ptr_Ini_Config->Counter; i++)
	    dbgprintf("  Pair %d: Address = 0x%08X, Value = 0x%08X\n", i, ptr_Ini_Config->Pairs[i].address, ptr_Ini_Config->Pairs[i].value);

	//dbgprintf("==============================\n");
    fclose(FileHandle);
    return OK;
}

/****************************************************************************
 *                                                                          *
 * Copyright (c) 2013 Nuvoton Technology Corp. All rights reserved.         *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 *
 * FILENAME
 *     mksysimg.c
 *
 * DESCRIPTION
 *     This file makes system image for IBR.
 *
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/param.h>
#include <unistd.h>


#include "nand_sa_writer.h"

#define uIBRAreaSize    4
#define MAX_FWInfo      16      // max element number for FWInfo

extern INI_INFO_T Ini_Writer;
extern IBR_BOOT_OPTIONAL_STRUCT_T optional_ini_file;

//--- define global variables

FW_UPDATE_INFO_T    FWInfo[MAX_FWInfo];
IBR_BOOT_STRUCT_T   NandMark;

unsigned char	*g_pBlockbuf=NULL;
UINT32	g_u32BlockSize = 64*2048;
UINT32	g_u32PageSize = 2048;
UINT32	g_u32OOBSize = 64;

char    *opOutSysFilePath = NULL;	// For output system.img

//===========================================================================================================
int dump_image(int FileInfoIdx)
{
    printf("FWIdex=[%d]\n", FileInfoIdx);
    printf("\t.imageNo: %d\n",   		FWInfo[FileInfoIdx].imageNo );
    printf("\t.imageFlag: %x\n",   		FWInfo[FileInfoIdx].imageFlag );
    printf("\t.startBlock: %d\n",   		FWInfo[FileInfoIdx].startBlock );
    printf("\t.endBlock: %d\n",   		FWInfo[FileInfoIdx].endBlock );
    printf("\t.executeAddr: 0x%08X\n",   	FWInfo[FileInfoIdx].executeAddr );
    printf("\t.fileLen: %d\n",   		FWInfo[FileInfoIdx].fileLen );
    printf("\t.imageName: %s\n",   		FWInfo[FileInfoIdx].imageName );
    return Successful;
}

//------------------------------------------------------------------
// Open a file and write it to NAND System Area as a system image.
//------------------------------------------------------------------
int write_fw_info(int FileInfoIdx, CHAR *filename, UINT16 imageFlag, UINT32 executeAddr)
{
    struct stat statBuff;

    if ( stat(filename, &statBuff) < 0 ) {
        printf("[%s] Wrong file stat\n", __FUNCTION__);
        return -1;
    }

    // nand information
    FWInfo[FileInfoIdx].imageNo     = FileInfoIdx;
    FWInfo[FileInfoIdx].imageFlag   = imageFlag;

    if ( imageFlag==ImageFlag_System )
    {
        FWInfo[FileInfoIdx].startBlock  = 0 ;
        FWInfo[FileInfoIdx].endBlock    = 3;
    } else {
        FWInfo[FileInfoIdx].startBlock  = FWInfo[FileInfoIdx-1].endBlock + 1 ;
        FWInfo[FileInfoIdx].endBlock    = FWInfo[FileInfoIdx].startBlock + (statBuff.st_size/g_u32BlockSize);
    }

    FWInfo[FileInfoIdx].executeAddr = executeAddr;
    FWInfo[FileInfoIdx].fileLen     = statBuff.st_size;
    
    memcpy(&FWInfo[FileInfoIdx].imageName[0], filename, 32);
    FWInfo[FileInfoIdx].imageName[31] = '\0';    

    dump_image(FileInfoIdx);

    return Successful;
}

int burn_images( int gFileInfoIdx )
{
    char szCmd[2048];
    int i, i32BlkCount=0;

    sprintf( szCmd, "./flash_erase /dev/mtd0 0 0");
    if ( (system( szCmd ) == -1) )
    {
        printf("Failed on erase System area\n");
        exit(1);
    }
    // System Area
    for (i=0; i<uIBRAreaSize; i++)    // duplicate Ini_Writer.ExecFileNum copy for ExecFile
    {
        // To Burn System image
        sprintf( szCmd, "./nandwrite -m -a /dev/mtd0 -s 0x%x %s", i*g_u32BlockSize,  opOutSysFilePath );
        if ( (system( szCmd ) == -1) )
        {
            printf("Failed on burning System image\n");
            exit(1);
        }

        // To dump System image
        sprintf( szCmd, "./nanddump -c -o /dev/mtd0 -s 0x%x -l 0x%x > /dev/null", i*g_u32BlockSize, g_u32PageSize );
        if ( (system( szCmd ) == -1) )
        {
            printf("Failed on nanddump System image\n");
            exit(1);
        }
    }

    i32BlkCount+=uIBRAreaSize;
    sprintf( szCmd, "./flash_erase /dev/mtd1 0 0" );
    if ( (system( szCmd ) == -1) )
    {
        printf("Failed on erase Execute area\n");
        exit(1);
    }
    // Execute area
    for (i=0; i<gFileInfoIdx; i++)    // duplicate Ini_Writer.ExecFileNum copy for ExecFile
    {
		switch(FWInfo[i].imageFlag)
		{
			case ImageFlag_Logo:
				// To Burn Logo image
				sprintf( szCmd, "./nandwrite -p -m -a /dev/mtd1 -s 0x%x %s", (FWInfo[i].startBlock-i32BlkCount)*g_u32BlockSize,  Ini_Writer.Logo );
				break;
			case ImageFlag_Exec:
				// To Burn Execute image
				sprintf( szCmd, "./nandwrite -p -m -a /dev/mtd1 -s 0x%x %s", (FWInfo[i].startBlock-i32BlkCount)*g_u32BlockSize,  Ini_Writer.ExecFile );
				break;
			default:
				break;
        }

		printf("%s\n", szCmd);
		if ( (system( szCmd ) == -1) )
		{
			printf("Failed on burning Execute image\n");
			exit(1);
		}
		// To dump Execute image
		sprintf( szCmd, "./nanddump -c -o /dev/mtd1 -s 0x%x -l 0x%x > /dev/null", (FWInfo[i].startBlock-uIBRAreaSize)*g_u32BlockSize, g_u32PageSize );
		if ( (system( szCmd ) == -1) )
		{
			printf("Failed on nanddump Execute image\n");
			exit(1);
		}        
    }


    return Successful;
}

void usage(void)
{
    fprintf(stderr, "Usage: mn_writer [<arguments> ...]\n\n"
            "Following options are available:\n"
            "       -q              Will not burn to NAND flash\n"
            "       -x              Out system image file path\n"
            "       -f              NANDWRITER.ini Input file path\n"
            "	-t              turbowriter.ini Input file path\n"
            "       -b              Block Size (Byte)\n"
            "       -p              Page Size (Byte)\n"
            "       -o              OOB Size (Byte)\n"
            "Example: ./mn_writer -x /tmp/system.img -t ./TurboWriter.ini -f ./NANDWRITER.ini -b 131072 -p 2048 -o 64\n\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    int status, nReadLen, i;
    int optional_ini_size;
    unsigned int     *p_uint;
    unsigned int system_area_size;
    int ch;
    int     gFileInfoIdx = -1;

    char    *ipMkSysImgFilePath = NULL;	// For mksysimg
    char    *ipTWFilePath = NULL;	// For Turbowriter


    FW_UPDATE_INFO_T *p_fw_update;

    FILE *fd_image;
    struct stat statBuff;

    int NoBurn=0;



    while ( (ch = getopt(argc, argv, "qf:t:b:p:o:x:")) != -1)
    {
        switch (ch) {

        case 'f':
            ipMkSysImgFilePath = optarg;
            break;

        case 't':
            ipTWFilePath = optarg;
            break;

        case 'x':
            opOutSysFilePath = optarg;
            break;

        case 'o':
            g_u32OOBSize = (UINT32)atoi(optarg);
            break;

        case 'b':
            g_u32BlockSize = (UINT32)atoi(optarg);
            break;

        case 'p':
            g_u32PageSize = (UINT32)atoi(optarg);
            break;

        case 'q':
            NoBurn=1;
            break;

        case '?':
        default:
            usage();
        }

    }

    if ( g_u32PageSize%512 != 0 || g_u32BlockSize%512 != 0 || g_u32OOBSize==0 )  {
        fprintf(stderr, "Wrong parameter! \n");
        usage();
    } else if ( !ipTWFilePath || !ipMkSysImgFilePath )  {
        fprintf(stderr, "Please enter INI files path. \n");
        usage();
    } else if ( stat(ipTWFilePath, &statBuff) < 0 ) {
        fprintf(stderr, "Wrong Turbowriter INI files path. (%s) \n", ipTWFilePath);
        exit(1);
    } else if ( stat(ipMkSysImgFilePath, &statBuff) < 0 ) {
        fprintf(stderr, "Wrong Image INI files path. (%s) \n", ipMkSysImgFilePath);
        exit(1);
    } else if ( ( g_pBlockbuf = (unsigned char*)malloc(g_u32BlockSize)) == NULL ) {
        fprintf(stderr, "Failed on allocation memory.\n" );
        exit(1);
    } else if ( (status = ProcessINI(ipMkSysImgFilePath)) < 0 ) {
        fprintf(stderr, "ERROR: Parsing INI file %s fail !)\n", ipMkSysImgFilePath);
        exit(1);
    } else if ( (fd_image = fopen(Ini_Writer.NandLoader, "r")) == NULL) {
        fprintf(stderr, "ERROR: Open NandLoader image file %s fail !\n", Ini_Writer.NandLoader);
        exit(1);
    }

    memset ( g_pBlockbuf, 0xFF, g_u32BlockSize );
    memset ( &FWInfo, 0xFF, sizeof(FWInfo) );

    printf("\n=====> mksysimg (v%d.%d) Begin <=====\n", MAJOR_VERSION_NUM, MINOR_VERSION_NUM);
    //---------------------------------------------------
    //--- First, MUST get NAND information
    //---------------------------------------------------
    printf("Show NAND chip information:\n");
    printf("    Erase size	= %d\n", g_u32BlockSize );
    printf("    Page per Block  = %d\n", g_u32BlockSize/g_u32PageSize );
    printf("    Page Size       = %d\n", g_u32PageSize );
    printf("    OOB Size        = %d\n", g_u32OOBSize );

    memset(&optional_ini_file, 0, sizeof(IBR_BOOT_OPTIONAL_STRUCT_T) );

    // Get the Boot Code Optional Setting from INI file (TurboWriter.ini) to optional_ini_file
    status = ProcessOptionalINI(ipTWFilePath);
    if (status < 0)
    {
        printf("ERROR: Parsing INI file %s fail !)\n", ipTWFilePath);
        exit(1);
    }

    if (optional_ini_file.Counter == 0)
        optional_ini_size = 0;
    else
    {
        optional_ini_size = 8 * (optional_ini_file.Counter + 1);
        if (optional_ini_file.Counter % 2 == 0)
            optional_ini_size += 8;     // for dummy pair to make sure 16 bytes alignment.
    }

    //---------------------------------------
    //--- copy NandLoader
    //---------------------------------------
    if (strlen(Ini_Writer.NandLoader) == 0)
        goto WriteLogo;

    printf("=====> Copy NandLoader <=====\n");
    stat(Ini_Writer.NandLoader, &statBuff);      // got file size

    memset ( &NandMark, 0, sizeof(NandMark) );
    // initial Boot Code Mark for NandLoader
    NandMark.BootCodeMarker = 0x57425AA5;

    if ( NoBurn )
        NandMark.ExeAddr = 0x01F00000;
    else
        NandMark.ExeAddr = 0x00900000;

    NandMark.ImageSize = statBuff.st_size;
    NandMark.Reserved = 0x00000000;

    // Copy NAND marker to buffer
    memcpy((void*)g_pBlockbuf,				 &NandMark ,sizeof(IBR_BOOT_STRUCT_T) );

    // Copy optional NAND marker to buffer
    memcpy((UINT8 *)(g_pBlockbuf+sizeof(IBR_BOOT_STRUCT_T)), (UINT8 *)&optional_ini_file, optional_ini_size);

    // Copy nandloader to buffer
    nReadLen = fread((UINT8 *)( g_pBlockbuf+sizeof(IBR_BOOT_STRUCT_T)+optional_ini_size),
                     1,
                     statBuff.st_size,
                     fd_image);
    fclose(fd_image);

    write_fw_info(++gFileInfoIdx, Ini_Writer.NandLoader, ImageFlag_System, NandMark.ExeAddr);

WriteLogo:
#if 1
    //---------------------------------------
    //--- copy Logo
    //---------------------------------------
    if (strlen(Ini_Writer.Logo) == 0)
        goto WriteExecFile;

    stat(Ini_Writer.Logo, &statBuff);      // got file size

    printf("=====> copy Logo info <=====\n");
    write_fw_info(++gFileInfoIdx, Ini_Writer.Logo, ImageFlag_Logo, 0x500000);
#endif

WriteExecFile:
    //---------------------------------------
    //--- copy ExecFile
    //---------------------------------------
    if (strlen(Ini_Writer.ExecFile) ==0)
        goto WriteSysteInfo;

    printf("=====> copy ExecFile info <=====\n");
    for (i=0; i<Ini_Writer.ExecFileNum; i++)    // duplicate Ini_Writer.ExecFileNum copy for ExecFile
        write_fw_info(++gFileInfoIdx, Ini_Writer.ExecFile, ImageFlag_Exec, Ini_Writer.ExecAddress);

WriteSysteInfo:
    //---------------------------------------
    //--- write System Information
    //---------------------------------------
    if (strlen(Ini_Writer.NandLoader)==0 && strlen(Ini_Writer.Logo)==0 && strlen(Ini_Writer.ExecFile)==0)
        goto _end_;

    p_uint = (unsigned int*)(g_pBlockbuf+(g_u32BlockSize-2*g_u32PageSize));
    // update image information
    *(p_uint+0) = 0x574255AA;
    *(p_uint+1) = ++gFileInfoIdx;
    *(p_uint+2) = 0;
    *(p_uint+3) = 0x57425963;

    printf("=====> update System Information <=====\n");
    memcpy( (void*)(p_uint+4), (void*)&FWInfo, sizeof(FWInfo) );
    fd_image = fopen(opOutSysFilePath, "w");
    fwrite(g_pBlockbuf, 1, g_u32BlockSize, fd_image );
    fsync ( fileno(fd_image) );
    fclose( fd_image );

__burn:
    printf("=====> Burn images<=====\n");
    burn_images ( gFileInfoIdx );
    printf("Done.\n");

_end_:
    free(g_pBlockbuf);
    printf("\n=====> Finish <=====\n");
    return Successful;
}


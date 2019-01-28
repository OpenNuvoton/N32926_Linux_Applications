/*-----------------------------------------------------------------------------------*/
/* Nuvoton Technology Corporation confidential                                       */
/*                                                                                   */
/* Copyright (c) 2012 by Nuvoton Technology Corporation                              */
/* All rights reserved                                                               */
/*-----------------------------------------------------------------------------------*/
/*
 * FILENAME
 *      main.c
 * DESCRIPTION
 *      Main program of GoldenImgMaker.
 *      GoldenImgMaker a golden NAND image that with valid format for Nuvoton FA92 platform.
 *      Please note that golden NAND image don't handle bad block issue on NAND flash.
 * HISTORY
 *      2014/09/05  Create Ver 1.0.
 * REMARK
 *      None.
 *-----------------------------------------------------------------------------------*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <libgen.h>
#include "goldenimgmaker.h"

//--- show debug messages or not ?
#define DBG_PRINTF  printf
//#define DBG_PRINTF(...)


const char * g_pcImgType[] =
{
    "IMG_DATA",
    "IMG_EXEC",
    "IMG_ROMFS",
    "IMG_SYS",
    "IMG_LOGO"
};

const char * g_pcChip[] =
{
    "N3290 Platform",
    "N3291 Platform",
    "N3292 Platform"
};


/*-----------------------------------------------------------------------------------*
 * Definition the global variables
 *-----------------------------------------------------------------------------------*/
unsigned char   gPage_buf[8192];    // buffer for a page of data
unsigned char   gRa_buf[512];       // buffer for redundancy area
unsigned long   gChecksum = 0;      // 32 bits checksum for all output data

/*-----------------------------------------------------------------------------
 * calculate 32 bits checksum for data
 * INPUT:
 *      data   : pointer to data.
 *      length : byte number of data.
 *      current_checksum : the current checksum for data that had calculated.
 * RETURN:
 *      final checksum value.
 *---------------------------------------------------------------------------*/
unsigned long calculate_checksum(unsigned char *data, int length, unsigned long current_checksum)
{
    int i;

    for (i = 0; i < length; i++)
        current_checksum = (current_checksum + data[i]) & 0xFFFFFFFF;
    return current_checksum;
}

/*-----------------------------------------------------------------------------
 * Create image for blank page that filled 0xFF.
 *---------------------------------------------------------------------------*/
int create_image_blank_page(
    GOLDEN_INFO_T* psgoldeninfo,
    IMG_INFO_T* psImgInfo,
    FILE *fdout,
    int page )
{
    int result;

    memset(gPage_buf, 0xFF, psgoldeninfo->m_i32PageSize );
    result = fwrite(gPage_buf, 1, psgoldeninfo->m_i32PageSize, fdout);
    gChecksum = calculate_checksum(gPage_buf, psgoldeninfo->m_i32PageSize, gChecksum);
    if (result == 0)
    {
        printf("ERROR: Fail to write page %d into file. Return code = 0x%x, (line %d)\n", page, result, __LINE__);
        return FAIL;
    }

	if ( psgoldeninfo->m_i32OOBSize == 0 )	return OK;

    memset(gRa_buf, 0xFF,  psgoldeninfo->m_i32OOBSize );
    result = fwrite(gRa_buf, 1, psgoldeninfo->m_i32OOBSize, fdout);
    gChecksum = calculate_checksum(gRa_buf, psgoldeninfo->m_i32OOBSize, gChecksum);
    if (result == 0)
    {
        printf("ERROR: Fail to write page %d into file. Return code = 0x%x, (line %d)\n", page, result, __LINE__);
        return FAIL;
    }

    return OK;
}


/*-----------------------------------------------------------------------------
 * Usage
 *---------------------------------------------------------------------------*/
void usage()
{
    fprintf(stderr, "[Usage] ./GoldenImgMaker\n"
            "	-v ....: version\n"
            "	-O ....: Output folder path\n"
            "	-p ....: page size\n"
            "	-o ....: OOB size\n"
            "	-s ....: page per block\n"
            "	-b ....: block number\n"
            "	-t ....: TurboWriter.ini path\n"
            "	-c ....: Chip. N3290:0, N3291:1, N3292:2\n"
            "	-P \"<ImgFilePath> [parameters]\"\n\n" );

    fprintf(stderr, "\t[parameters]\n"
            "\t\t-t ....: Image type. Data:0, Exec:1, ROMFS:2, Sys: 3, Logo:4\n"
            "\t\t-o ....: Redundent area size.(option)\n"
            "\t\t-S ....: MTD partition entry size.\n"
            "\t\t-O ....: MTD partition entry offset.\n"
            "\t\t-j ....: Won't determine parity code if all 0xff in a page.\n"
            "\t\t-b ....: BCH error bits(4, 8, 12, 15, 24).\n"
            "\t\t-e ....: Execution address.\n\n" );

    fprintf(stderr, "[Example] /GoldenImgMaker -o ./skyeye_golden -p 2048 -o 64 -s 64 -b 1024\n" \
            "	-P \"./NandLoader_240.bin -t 3 -S 0x80000 -O 0 -e 0x900000 -j 0 \" \n" \
            "	-P \"./conprog.bin -t 1 -O 0x80000 -S 0x1000000 -e 0x0 -j 0 \" \n" \
            "	-P \"./mtdnand_ubi_nand1.img -t 1 -O 0x1080000 -S 0x1000000 -e 0x0 -j 1\" \n\n" );
}

/*-----------------------------------------------------------------------------
 * Get all image information
 *---------------------------------------------------------------------------*/
int LoadImgOption ( GOLDEN_INFO_T* psGoldenInfo, int i32No, char* strCmdlineOption, char *strImgName )
{
    IMG_INFO_T* psImgInfo=&psGoldenInfo->m_sImgInfos[i32No];
    int img_argc=1, i;
    char *img_argv[DEF_MAX_ARGUMENTS];
    int ch;

    const char* short_options = "t:b:o:S:O:je:";
    const struct option long_options[] = {
        { "type",     	required_argument,   NULL,    't'     },
        { "errorbit",   required_argument,   NULL,    'b'     },
        { "oobsize",    required_argument,   NULL,    'o'     },
        { "size",  		required_argument,   NULL,    'S'     },
        { "offset",     required_argument,   NULL,    'O'     },
        { "allff",     	no_argument,  		 NULL,	  'j'     },
        { "execaddr",   required_argument,   NULL,    'e'     },
        {      0,     0,     0,     0}
    };

    img_argv[0]=strImgName;

    if(strCmdlineOption != NULL && strlen(strCmdlineOption) != 0)
    {
        char	*arg		= NULL,
                       *saveptr	= NULL,
                           *token		= NULL;

        arg = (char *)strdup(strCmdlineOption);

        if(strchr(arg, ' ') != NULL)
        {
            token=strtok_r(arg, " ", &saveptr);

            if(token != NULL)
            {
                img_argv[img_argc++] = strdup(token);

                while((token = strtok_r(NULL, " ", &saveptr)) != NULL)
                {
                    img_argv[img_argc++] = strdup(token);

                    if(img_argc >= DEF_MAX_ARGUMENTS)
                    {
                        DBG_PRINTF("Too many arguments to input plugin\n");
                        return FAIL;
                    }
                }
            }	// if(token != NULL)
        }	// if(strchr(arg, ' ') != NULL)

        free(arg);

    }	// if(strCmdlineOption != NULL && strlen(strCmdlineOption) != 0)

    // show all parameters for DBG purposes
    //	for(i = 0; i < img_argc; i++)
    //		DBG_PRINTF("img_argv[%d/%d]=%s\n", i, img_argc, img_argv[i]);

    // Parse parameters
    optind = 0;	// Reset the optind count
    while ( ( ch = getopt_long(img_argc, img_argv, short_options, long_options, NULL  ) ) != -1 )
    {
        switch (ch)
        {
        case 't':	//Type
            psImgInfo->m_sFwImgInfo.imageFlag = atoi(optarg);
            break;

        case 'b':	//BCH error bits
            psImgInfo->m_u32BCHErrorBits = atoi(optarg);
            break;

        case 'o':	//partial OOB size
            sscanf(optarg, "%x", &psImgInfo->m_u32PartitionOffset );
            break;

        case 'S':	//MTD partition entry size
            sscanf(optarg, "%x", &psImgInfo->m_u32PartitionSize );
            break;

        case 'O':	//MTD partition entry offset
            sscanf(optarg, "%x", &psImgInfo->m_u32PartitionOffset );
            break;

        case 'j':	//Won't determine parity code
            psImgInfo->m_i32SkipAllFFInPage=1;
            break;

        case 'e':	//Execution address
            sscanf(optarg, "%x", &psImgInfo->m_sFwImgInfo.executeAddr );
            break;

        default:
            usage();
            return FAIL;
        }
    }

    if ( psImgInfo->m_i32OOBSize == 0 )
        psImgInfo->m_i32OOBSize = psGoldenInfo->m_i32OOBSize;

    psImgInfo->m_sFwImgInfo.imageNo = i32No;

    strcpy(psImgInfo->m_psImgPath, strImgName);
    {
        struct stat sb;
        char *path2 = strdup(strImgName);
        char* token = basename(path2);
        if ( stat(strImgName, &sb) == -1) {
            perror("stat");
            return FAIL;
        }

        strcpy(psImgInfo->m_psImgBaseName, token);

        memset((void*)psImgInfo->m_sFwImgInfo.imageName, 0, sizeof(psImgInfo->m_sFwImgInfo.imageName) );
        if ( strlen(token) >= sizeof(psImgInfo->m_sFwImgInfo.imageName) )
            strncpy(psImgInfo->m_sFwImgInfo.imageName, token, sizeof(psImgInfo->m_sFwImgInfo.imageName)-1 );
        else
            strcpy(psImgInfo->m_sFwImgInfo.imageName, token );

        psImgInfo->m_sFwImgInfo.fileLen=(UINT32)sb.st_size;

        psImgInfo->m_sFwImgInfo.startBlock	= (UINT16)( psImgInfo->m_u32PartitionOffset/psGoldenInfo->m_i32BlockSize );
        psImgInfo->m_sFwImgInfo.endBlock 	= (UINT16)( (psImgInfo->m_u32PartitionOffset+psImgInfo->m_u32PartitionSize)/psGoldenInfo->m_i32BlockSize-1 );

        free(path2);

    }

    return OK;

}

int print_images_opts ( GOLDEN_INFO_T* psGoldenInfo )
{
    int i;

    printf("\n" );

    printf("Chip: %s\n", g_pcChip[psGoldenInfo->m_eChip] );

    printf("Output folder: %s\n", psGoldenInfo->m_psOutputPath );

    printf("Turbowriter.ini filepath: %s\n", psGoldenInfo->m_psTurboWriterPath );

    printf("All image number: %d\n", psGoldenInfo->m_i32ImgNumber );

    printf("Page size: %d\n", psGoldenInfo->m_i32PageSize );

    printf("OOB size: %d\n", psGoldenInfo->m_i32OOBSize );

    printf("Page per block: %d\n", psGoldenInfo->m_i32PagePerBlock );

    printf("Block number: %d\n", psGoldenInfo->m_i32BlockNumber );

    for(i = 0; i < psGoldenInfo->m_i32ImgNumber; i++)
    {
        IMG_INFO_T* psImgInfo = &psGoldenInfo->m_sImgInfos[i];
        printf("[%d]: %s\n", psImgInfo->m_sFwImgInfo.imageNo, psImgInfo->m_psImgPath );
        printf("\tBCH Error bits: %d\n", psImgInfo->m_u32BCHErrorBits );
        printf("\tOffset in MTD table: %d(0x%X)\n", psImgInfo->m_u32PartitionOffset, psImgInfo->m_u32PartitionOffset );
        printf("\tSize in MTD table: %d(0x%X)\n", psImgInfo->m_u32PartitionSize, psImgInfo->m_u32PartitionSize );
        printf("\tSkip all 0xff page: %s\n", psImgInfo->m_i32SkipAllFFInPage>0?"yes":"no" );

        printf("\tImage type: %s\n", g_pcImgType[psImgInfo->m_sFwImgInfo.imageFlag] );
        printf("\tImage name: %s\n", psImgInfo->m_sFwImgInfo.imageName );
        printf("\tImage file length: %d(0x%X)\n", psImgInfo->m_sFwImgInfo.fileLen, psImgInfo->m_sFwImgInfo.fileLen );
        printf("\tImage start block: %d(0x%X)\n", psImgInfo->m_sFwImgInfo.startBlock, psImgInfo->m_sFwImgInfo.startBlock );
        printf("\tImage end block: %d(0x%X)\n", psImgInfo->m_sFwImgInfo.endBlock, psImgInfo->m_sFwImgInfo.endBlock );

        printf("\n");

        printf("\tGolden image information:\n");
        printf("\tPath: %s\n",   psImgInfo->m_psGoldenImgName );
        printf("\tAt buffer address(Start_block*Page_per_block*(OOB_size+Page_size)): 0x%08X \n", psImgInfo->m_u32LocatedBufAddress );
        printf("\tBlock number(File_length/Page_per_block/(OOB_size+Page_size)): 0x%X(%d) \n", psImgInfo->m_u32GoldenImgBlockNum, psImgInfo->m_u32GoldenImgBlockNum );

        printf( "\n" );

    }	// for

}

int get_images_opts ( GOLDEN_INFO_T* psGoldenInfo, char ** golden_cmdline_opt )
{
    int i;
    size_t tmp = 0;
    char *strImgName;

    for(i = 0; i < psGoldenInfo->m_i32ImgNumber; i++)
    {
        tmp = (size_t)(strchr(golden_cmdline_opt[i], ' ') - golden_cmdline_opt[i]);

        if((strImgName = (tmp > 0) ? strndup(golden_cmdline_opt[i], tmp) : strdup(golden_cmdline_opt[i])) == NULL)
        {
            printf("Failed to located valid image name!\n");
            continue;
        }
        if ( LoadImgOption(psGoldenInfo, i, strchr(golden_cmdline_opt[i], ' '), strImgName ) != OK )
            return FAIL;

        free(strImgName);
        strImgName = NULL;
    }	// for

    return OK;
}



/*-----------------------------------------------------------------------------
 * calculate BCH parity code for a page of data
 * INPUT:
 *      fdout : file pointer for output file
 *      block / page : the block index and page index of raw data
 *      raw_data : pointer to a page of raw data
 *      bch_need_initial : TRUE to initial BCH if the BCH configuration is changed.
 * 		ra_size: the oob size.
 * OUTPUT:
 *      ra_data : pointer to the buffer for a page of BCH parity code
 *---------------------------------------------------------------------------*/
int calculate_BCH_parity(
    GOLDEN_INFO_T* psgoldeninfo,
    IMG_INFO_T* psImgInfo,
    FILE *fdout,
    int block, int page,
    unsigned char *raw_data,
    unsigned char *ra_data,
    int bch_need_initial)
{
    int field_index, field_parity_size, field_size, field_count;
    int protect_3B;
    int bch_error_bits, nvt_ra_size;
    int result;
    unsigned char bch_parity_buffer[512];
    unsigned char *parity_location_in_ra;

	if ( psImgInfo->m_i32OOBSize == 0 )
		return OK;

    memset(ra_data, 0xFF, psgoldeninfo->m_i32OOBSize);

    if ( block < 4 ) // first four blocks is system area
    {
        ra_data[0] = 0xFF;
        ra_data[1] = 0x5A;
        ra_data[2] = page & 0xFF;
        ra_data[3] = 0x00;
    }
    else
    {
        ra_data[0] = 0xFF;
        ra_data[1] = 0xFF;
        ra_data[2] = 0x00;
        ra_data[3] = 0x00;
    }
    bch_error_bits = psImgInfo->m_u32BCHErrorBits;
    //nvt_ra_size = g_i32ParityNum[psgoldeninfo->m_ePageSize][psImgInfo->m_eBCHAlgorithm];
    //nvt_ra_size = psgoldeninfo->m_i32OOBSize ;
    nvt_ra_size = psImgInfo->m_i32OOBSize;

    switch (bch_error_bits)
    {
    case  4:
        field_size = 512;
        break;
    case  8:
        field_size = 512;
        break;
    case 12:
        field_size = 512;
        break;
    case 15:
        field_size = 512;
        break;
    case 24:
        field_size = 1024;
        break;
    default:
        printf("ERROR: BCH T must be 4 or 8 or 12 or 15 or 24.\n\n");
        return FAIL;
    }

    field_count = psgoldeninfo->m_i32PageSize / field_size;
    for (field_index = 0; field_index < field_count; field_index++)
    {
        if (field_index == 0)
            protect_3B = TRUE;  // BCH protect 3 bytes only for field 0
        else
        {
            protect_3B = FALSE;
            bch_need_initial = FALSE;   // BCH engine only need to initial once. So, initial it only for field 0.
        }

        field_parity_size = calculate_BCH_parity_in_field(
                                raw_data + field_index * field_size, ra_data,
                                bch_error_bits, protect_3B, field_index, bch_parity_buffer, bch_need_initial);
        parity_location_in_ra = ra_data + nvt_ra_size - (field_parity_size * field_count) + (field_parity_size * field_index);
        memcpy(parity_location_in_ra, bch_parity_buffer, field_parity_size);
    }

#if 0
    //Wayne
    {
        int i=0;
        //printf("bch_error_bits=%d\n", bch_error_bits );
        printf("[block:%d, page:%d - OOB]=\n\t", block, page ,bch_error_bits );
        for ( i=0; i<psgoldeninfo->m_i32OOBSize; i++ )
        {
            printf("%02x ", ra_data[i] );
            if ( (i%16) == 15 )
                printf("\n\t");
        }
        printf("\n");
    }
#endif

    result = fwrite(ra_data, 1, psgoldeninfo->m_i32OOBSize, fdout);
    gChecksum = calculate_checksum(ra_data, psgoldeninfo->m_i32OOBSize, gChecksum);
    if (result == 0)
    {
        printf("ERROR: Fail to write block %d page %d into file, Return code = 0x%x (line %d)\n", block, page,  result, __LINE__);
        return FAIL;
    }

#if 0
    if (page == psgoldeninfo->m_i32PagePerBlock - 1)
        printf("	Image for block %d/%d, (%d/%d) done !!\n", block, psgoldeninfo->m_i32BlockNumber-1, (block-psImgInfo->m_sFwImgInfo.startBlock),(psImgInfo->m_sFwImgInfo.endBlock-psImgInfo->m_sFwImgInfo.startBlock));
#endif

    return OK;
}


/*-----------------------------------------------------------------------------
 * Create image of System image
 *      Copy System to image file block gBlock page 0.
 *      Also handle Boot Code Mark, Firmware information, and Reserved Area Size setting.
 *---------------------------------------------------------------------------*/
//#define __DEF_FOR_SPI__
int create_image_system(
    GOLDEN_INFO_T* psgoldeninfo,
    IMG_INFO_T* psImgInfo
) {
    FILE 	*fdout;
    char	tmp[512];
    char 	*filename = psImgInfo->m_psImgPath;
    int     page;
    int     result;
    FILE    *fdin;
    int block;
    struct stat     file_stat;
    unsigned char   *ptr_buf;
    IBR_BOOT_STRUCT_T   NandMark;
    int     optional_ini_size;
    
    //--- open output file for golder file
    sprintf(tmp, "%s/%s.golden", psgoldeninfo->m_psOutputPath, psImgInfo->m_psImgBaseName);
    fdout = fopen(tmp, "wb");
    if (fdout == NULL)
    {
        printf("ERROR: file open fail: %s not found !!\n", tmp);
        return FAIL;
    }

    strcpy(psImgInfo->m_psGoldenImgName, tmp);

    //--- open input file
    fdin = fopen(filename, "rb");
    if (fdin == NULL)
    {
        printf("ERROR: file open fail: %s not found !!\n", filename);
        goto LEB_SYS_FAIL;
    }


    //--- program NandLoader image
    page = 0;
    block = 0;

    psImgInfo->m_u32LocatedBufAddress = (block * psgoldeninfo->m_i32PagePerBlock + page) * ( psgoldeninfo->m_i32PageSize + psgoldeninfo->m_i32OOBSize  );
    DBG_PRINTF("\nCreate NandLoader at block %d page %d, buffer address at 0x%08X\n", block, page, psImgInfo->m_u32LocatedBufAddress);
    memset ( gPage_buf, 0xFF, psgoldeninfo->m_i32PageSize );
    ptr_buf = gPage_buf;

    // initial Boot Code Mark for NandLoader
    NandMark.BootCodeMarker = 0x57425AA5;       					// magic number
    NandMark.Reserved  = 0x00000000;
    NandMark.ExeAddr   = psImgInfo->m_sFwImgInfo.executeAddr;	// memory address to load and execute code
    NandMark.ImageSize = psImgInfo->m_sFwImgInfo.fileLen;		// image file size
    memcpy(gPage_buf, &NandMark, sizeof(IBR_BOOT_STRUCT_T));		// NandMark size MUST be sizeof(IBR_BOOT_STRUCT_T)
    ptr_buf += sizeof(IBR_BOOT_STRUCT_T);

    // initial Boot Code Optional Setting
    if ( psgoldeninfo->m_soptional_ini_file.Counter == 0 )
        optional_ini_size = 0;
    else
    {
        optional_ini_size = 8 * ( psgoldeninfo->m_soptional_ini_file.Counter + 1);
        if ( psgoldeninfo->m_soptional_ini_file.Counter % 2 == 0)
            optional_ini_size += 8;     // for dummy pair to make sure 16 bytes alignment.
        memcpy ( ptr_buf, &psgoldeninfo->m_soptional_ini_file, optional_ini_size );
        ptr_buf += optional_ini_size;
    }

#if  0
    {
        int h;
        IBR_BOOT_OPTIONAL_STRUCT_T *psIbrBootOptional = &psgoldeninfo->m_soptional_ini_file;
        printf("optional_ini_size=%d\n", optional_ini_size);
        printf("OptionalMarker=%X\n", psIbrBootOptional->OptionalMarker);
        printf("Counter=%X\n", psIbrBootOptional->Counter);
        for (h=0; h<psIbrBootOptional->Counter; h++)
            printf("Pairs[%d]: 0x%08X->0x%08X\n", h, psIbrBootOptional->Pairs[h].address, psIbrBootOptional->Pairs[h].value);
    }
#endif

    // create image for page 0 with Boot Code Mark
    result = fread(ptr_buf, 1, psgoldeninfo->m_i32PageSize-sizeof(IBR_BOOT_STRUCT_T)-optional_ini_size, fdin);
    if (result == 0)
    {
        printf("ERROR: Fail to read image from file %s. Return code = 0x%x (line %d)\n", filename, result, __LINE__);
        fclose(fdin);
        goto LEB_SYS_FAIL;
    }

    result = fwrite(gPage_buf, 1, psgoldeninfo->m_i32PageSize, fdout);
    gChecksum = calculate_checksum(gPage_buf, psgoldeninfo->m_i32PageSize, gChecksum);
    if (result == 0)
    {
        printf("ERROR: Fail to write block %d page %d into file %s. Return code = 0x%x (line %d)\n", block, page, tmp, result, __LINE__);
        fclose(fdin);
        goto LEB_SYS_FAIL;
    }
    
    if (calculate_BCH_parity(psgoldeninfo, psImgInfo, fdout, block, page, gPage_buf, gRa_buf, TRUE) == FAIL)
    {
		fprintf(stderr, "%s %d\n", __FUNCTION__, __LINE__);
        goto LEB_SYS_FAIL;
    }
    page++;

    // create image for page 1 ~ end of file
    while (!feof(fdin))
    {
        memset(gPage_buf, 0xFF, psgoldeninfo->m_i32PageSize);
        result = fread(gPage_buf, 1, psgoldeninfo->m_i32PageSize, fdin);
        if (result == 0)    // end of file
            break;

        result = fwrite(gPage_buf, 1, psgoldeninfo->m_i32PageSize, fdout);
        gChecksum = calculate_checksum(gPage_buf, psgoldeninfo->m_i32PageSize, gChecksum);
        if (result == 0)
        {
            printf("ERROR: Fail to write block %d page %d into file %s. Return code = 0x%x, (line %d)\n", block, page, tmp, result, __LINE__);
            fclose(fdin);
            goto LEB_SYS_FAIL;
        }
        if (calculate_BCH_parity(psgoldeninfo, psImgInfo, fdout, block, page, gPage_buf, gRa_buf, FALSE) == FAIL)
            goto LEB_SYS_FAIL;
        page++;

        // The file size of NandLoader MUST <= (ini_file.page_per_block - 2) page size
#ifdef __DEF_FOR_SPI__
        if (page >= (psgoldeninfo->m_i32PagePerBlock - 1))
#else        
        if (page >= (psgoldeninfo->m_i32PagePerBlock - 2))
#endif
        {
            printf("ERROR: NandLoader file size too large !\n");
            fclose(fdin);
            goto LEB_SYS_FAIL;
        }

    } // end of while (!feof())
    fclose(fdin);

    // pad 0xFF to unused pages till second last page
    DBG_PRINTF("\tPad 0xFF for NandLoader from block %d page %d, image file address 0x%x\n", block, page, (block * psgoldeninfo->m_i32PagePerBlock + page) * ( psgoldeninfo->m_i32PageSize + psgoldeninfo->m_i32OOBSize ));
#ifdef __DEF_FOR_SPI__
    for ( ; page < psgoldeninfo->m_i32PagePerBlock - 1; page++)
#else
    for ( ; page < psgoldeninfo->m_i32PagePerBlock - 2; page++)
#endif
    {
        if (create_image_blank_page(psgoldeninfo, psImgInfo, fdout, page) == FAIL)
            goto LEB_SYS_FAIL;
    }

    // set image information on second last page
    DBG_PRINTF("\tCreate image information at block %d page %d, image file address 0x%x\n", block, page, (block * psgoldeninfo->m_i32PagePerBlock + page) * ( psgoldeninfo->m_i32PageSize + psgoldeninfo->m_i32OOBSize ));
    memset(gPage_buf, 0xFF, psgoldeninfo->m_i32PageSize );
    ptr_buf = gPage_buf;
#ifdef __DEF_FOR_SPI__
    *(UINT32 *)(ptr_buf+0)  = 0xAA554257;
    *(UINT32 *)(ptr_buf+12) = 0x63594257;
#else
    *(UINT32 *)(ptr_buf+0)  = 0x574255AA;
    *(UINT32 *)(ptr_buf+12) = 0x57425963;
#endif
    *(UINT32 *)(ptr_buf+4)  = psgoldeninfo->m_i32ImgNumber ;

    // To fill all firmware information
    {
        int k=0;
        for(k=0; k<psgoldeninfo->m_i32ImgNumber; k++)
            memcpy((void*)(ptr_buf+16+k*sizeof(FW_UPDATE_INFO_T)), (void *)&psgoldeninfo->m_sImgInfos[k].m_sFwImgInfo , sizeof(FW_UPDATE_INFO_T) );
    }

    result = fwrite(gPage_buf, 1, psgoldeninfo->m_i32PageSize, fdout);
    gChecksum = calculate_checksum(gPage_buf, psgoldeninfo->m_i32PageSize, gChecksum);
    if (result == 0)
    {
        printf("ERROR: Fail to write block %d page %d into file %s. Return code = 0x%x, (line %d)\n", block, page, tmp, result, __LINE__);
        goto LEB_SYS_FAIL;
    }

    if (calculate_BCH_parity(psgoldeninfo, psImgInfo, fdout, block, page, gPage_buf, gRa_buf, FALSE) == FAIL)
        goto LEB_SYS_FAIL;

#ifndef __DEF_FOR_SPI__
    page++;

    // set reserve area size on last page
    DBG_PRINTF("\tCreate reserve area size at block %d page %d, image file address 0x%x\n", block, page, (block * psgoldeninfo->m_i32PagePerBlock + page) * ( psgoldeninfo->m_i32PageSize + psgoldeninfo->m_i32OOBSize ));
    memset(gPage_buf, 0xFF, psgoldeninfo->m_i32PageSize );
    ptr_buf = gPage_buf;
    *(UINT32 *)(ptr_buf+0)  = 0x574255AA;
    *(UINT32 *)(ptr_buf+4)  = 0; //ini_file.nvt_reserved_area * 1024 * 2;   // sector count for reserve area size
    *(UINT32 *)(ptr_buf+12) = 0x57425963;
    result = fwrite(gPage_buf, 1, psgoldeninfo->m_i32PageSize, fdout);
    gChecksum = calculate_checksum(gPage_buf, psgoldeninfo->m_i32PageSize, gChecksum);
    if (result == 0)
    {
        printf("ERROR: Fail to write block %d page %d into file %s. Return code = 0x%x, (line %d)\n", block, page, tmp, result, __LINE__);
        goto LEB_SYS_FAIL;
    }

    if (calculate_BCH_parity(psgoldeninfo, psImgInfo, fdout, block, page, gPage_buf, gRa_buf, FALSE) == FAIL)
        goto LEB_SYS_FAIL;
#endif

    fclose(fdout);

    return OK;

LEB_SYS_FAIL:
    fclose(fdout);
    return FAIL;
}

int ChkAllFFInPage (
    char * gPage_buf,
    int i32PageSize
) {
    int i=0;
    UINT32 * pu32Buf = (UINT32*)gPage_buf;
    for ( i=(i32PageSize/sizeof(UINT32))-1; i>=0; i-- )
        if ( pu32Buf[i] != 0xFFFFFFFF )
            break;

    if ( i < 0 )
        return TRUE;

    return FALSE;
}

/*-----------------------------------------------------------------------------
 * Create image for binary file.
 *      Directly copy binary file to image file block block page 0.
 * Return:
 *      FAIL : fail
 *      > 0  : the last block index that had programmed
 *---------------------------------------------------------------------------*/
int create_image_binary_file(
    GOLDEN_INFO_T* psgoldeninfo,
    IMG_INFO_T* psImgInfo
) {
    int block= psImgInfo->m_sFwImgInfo.startBlock;
    int bch_need_initial=TRUE;
    FILE *fdout;
    char 	*filename = psImgInfo->m_psImgPath;
    char	tmp[512];

    int     page;
    int     result;
    FILE    *fdin;
    int 	ret=0;
#ifdef DBG_SHOW_TIME
    int     t1, t2;
#endif

    //--- open output file for golder file
    sprintf(tmp, "%s/%s.golden", psgoldeninfo->m_psOutputPath, psImgInfo->m_psImgBaseName);
    fdout = fopen(tmp, "wb");
    if (fdout == NULL)
    {
        printf("ERROR: NandLoader file open fail: %s not found !!\n", tmp);
        return FAIL;
    }

    strcpy(psImgInfo->m_psGoldenImgName, tmp);

    //--- open input file
    fdin = fopen(filename, "rb");
    if (fdin == NULL)
    {
        printf("ERROR: Binary file open fail: %s not found !!\n", filename);
        goto LEB_BINARY_FAIL;
    }

    // create image for page 0 ~ end of file
    page = 0;
    psImgInfo->m_u32LocatedBufAddress = (block * psgoldeninfo->m_i32PagePerBlock + page) * ( psgoldeninfo->m_i32PageSize + psgoldeninfo->m_i32OOBSize );

    DBG_PRINTF("\nWrite %s to block %d page %d, memory address at 0x%08X\n", filename, block, page, psImgInfo->m_u32LocatedBufAddress );

#ifdef DBG_SHOW_TIME
    t1 = gettimeofday();
    t2 = t1;
#endif
    while (!feof(fdin))
    {
        memset(gPage_buf, 0xFF,  psgoldeninfo->m_i32PageSize );

        result = fread(gPage_buf, 1, psgoldeninfo->m_i32PageSize, fdin);
        if (result == 0)    // end of file
            break;

        result = fwrite(gPage_buf, 1, psgoldeninfo->m_i32PageSize, fdout);
        gChecksum = calculate_checksum(gPage_buf, psgoldeninfo->m_i32PageSize, gChecksum);
        if (result == 0)
        {
            printf("ERROR: Fail to write block %d page %d into file %s. Return code = 0x%x, (line %d)\n", block, page, tmp, result, __LINE__);
            fclose(fdin);
            goto LEB_BINARY_FAIL;
        }

if ( psImgInfo->m_i32OOBSize > 0 )
{
        if ( psImgInfo->m_i32SkipAllFFInPage ) // To check all 0xff in a page
        {
            if ( ChkAllFFInPage ( gPage_buf, psgoldeninfo->m_i32PageSize ) )
            {
                //printf("INFO: The page %d in block %d is all 0xFF.\n", page, block );
                memset( (void*)gRa_buf, 0xFF, psgoldeninfo->m_i32OOBSize );
                result = fwrite(gRa_buf, 1, psgoldeninfo->m_i32OOBSize, fdout);
                if (result == 0)
                {
                    printf("ERROR: Fail to write block %d page %d into file %s. Return code = 0x%x, (line %d)\n", block, page, tmp, result, __LINE__);
                    fclose(fdin);
                    goto LEB_BINARY_FAIL;
                } else
                    goto NEXT_PAGE;
            }
        }
}

        if (calculate_BCH_parity(psgoldeninfo, psImgInfo, fdout, block, page, gPage_buf, gRa_buf, bch_need_initial) == FAIL)	{
            fclose(fdin);
            goto LEB_BINARY_FAIL;
        }
        
        // BCH just need to initial one time
        if (bch_need_initial)
            bch_need_initial = FALSE;

NEXT_PAGE:
        page++;
        if (page >= psgoldeninfo->m_i32PagePerBlock)
        {
#ifdef DBG_SHOW_TIME
            t2 = gettimeofday();
            printf("Block %d done. Curret time is %u ms, spent time %d s %d ms\n", block, t2, (t2-t1)/1000, (t2-t1)%1000);
            t1 = t2;
#endif
            block++;
            page = 0;
        }
    } // end of while (!feof())
    fclose(fdin);

    if (page == 0)
        ret=block-1;
    else
    {
        // pad 0xFF to unused pages
        DBG_PRINTF("\tPad 0xFF for %s from block %d page %d, memory address at 0x%x\n", filename, block, page, (block * psgoldeninfo->m_i32PagePerBlock + page) * ( psgoldeninfo->m_i32PageSize + psgoldeninfo->m_i32OOBSize ));
        for ( ; page < psgoldeninfo->m_i32PagePerBlock; page++)
        {
            if (create_image_blank_page(psgoldeninfo, psImgInfo, fdout, page) == FAIL)
                goto LEB_BINARY_FAIL;
        }
        ret=block;
    }

    fclose(fdout);
    return ret;

LEB_BINARY_FAIL:
    fclose(fdout);
    return FAIL;
}

/*-----------------------------------------------------------------------------
 * Create image for blank block that filled 0xFF
 *---------------------------------------------------------------------------*/
int create_image_blank_block(
    GOLDEN_INFO_T* psgoldeninfo,
    IMG_INFO_T* psImgInfo,
    FILE *fdout,
    int block
) {
    int page;

    // create image for page 0 ~ end of file
    page = 0;
    DBG_PRINTF("\tPad 0xFF to block %d, image file address 0x%08X\n", block, (block * psgoldeninfo->m_i32PagePerBlock + page) * ( psgoldeninfo->m_i32PageSize + psgoldeninfo->m_i32OOBSize ));

    memset(gPage_buf, 0xFF, psgoldeninfo->m_i32PageSize);
    for (page = 0; page < psgoldeninfo->m_i32PagePerBlock; page++)
    {
        if (create_image_blank_page(psgoldeninfo, psImgInfo, fdout, page) == FAIL)
            return FAIL;
    }
    return OK;
}


/*-----------------------------------------------------------------------------
 * Main function
 *---------------------------------------------------------------------------*/
int main(int argc, char** argv)
{
    int i, ch;

    struct stat sb;

    char				*golden_cmdline_opt[DEF_MAX_IMAGES];

    GOLDEN_INFO_T		s_sGoldenInfo;

    memset((void*)&s_sGoldenInfo, 0, sizeof(GOLDEN_INFO_T) );

    s_sGoldenInfo.m_eChip = eCHIP_N3292;

    // show all parameters for DBG purposes
    for(i = 0; i < argc; i++)
        printf("argv[%d]=%s\n", i, argv[i]);

    // Parse parameters
    while ( ( ch = getopt(argc, argv, "hvO:t:c:p:o:s:b:P:" ) ) != -1 )
    {
        switch (ch)
        {
        case 'h':	//help
            usage();
            return OK;

        case 'v':	//version
            printf("GoldenImgMaker Version: %d.%d\n" \
                   "Compilation Date / Time.....: %s / %s\n", MAJOR_VERSION_NUM, MINOR_VERSION_NUM, __DATE__, __TIME__);
            return OK;

        case 'O':	//output path
            if ( strlen(optarg) >= sizeof(s_sGoldenInfo.m_psOutputPath) )
            {
                DBG_PRINTF("Output string size is over %d bytes", (int)sizeof(s_sGoldenInfo.m_psOutputPath) );
                return FAIL;
            }
            else
                strcpy(s_sGoldenInfo.m_psOutputPath, optarg);

            break;

        case 't':	//TurboWriter.ini
            if ( strlen(optarg) >= sizeof(s_sGoldenInfo.m_psTurboWriterPath) )
            {
                DBG_PRINTF("Turbowriter string size is over %d bytes", (int)sizeof(s_sGoldenInfo.m_psTurboWriterPath) );
                return FAIL;
            }
            else
                strcpy(s_sGoldenInfo.m_psTurboWriterPath, optarg);

            break;

        case 'c':	//chip, n3290, n3291, n3292
            s_sGoldenInfo.m_eChip=atoi(optarg);
            break;

        case 'p':	//page size
            s_sGoldenInfo.m_i32PageSize=atoi(optarg);
            break;

        case 'o':	//OOB size
            s_sGoldenInfo.m_i32OOBSize=atoi(optarg);
            break;

        case 's':	//page per block
            s_sGoldenInfo.m_i32PagePerBlock=atoi(optarg);
            break;

        case 'b':	//block number
            s_sGoldenInfo.m_i32BlockNumber=atoi(optarg);
            break;

        case 'P':	//image info
            golden_cmdline_opt[s_sGoldenInfo.m_i32ImgNumber++] = strdup(optarg);
            break;

        default:
            usage(0);
            return OK;
        }

    }	// while

    printf("GoldenImgMaker Version: %d.%d\n" \
           "Compilation Date / Time.....: %s / %s\n", MAJOR_VERSION_NUM, MINOR_VERSION_NUM, __DATE__, __TIME__);

    s_sGoldenInfo.m_i32BlockSize = s_sGoldenInfo.m_i32PageSize * s_sGoldenInfo.m_i32PagePerBlock;
    if ( s_sGoldenInfo.m_i32BlockSize < 1 )
    {
        printf("The blocksize is zero\n");
        return FAIL;
    } else if ( s_sGoldenInfo.m_psOutputPath[0]=='\0'  ) {
        printf("The path of output folder is invalid.\n");
        return FAIL;
    } else if ( s_sGoldenInfo.m_psTurboWriterPath[0]=='\0' ) {
        printf("The path of Turbowriter.ini is invalid.\n");
        return FAIL;
    } else if ( get_images_opts ( &s_sGoldenInfo, golden_cmdline_opt ) ) {
        return FAIL;
    } else if ( s_sGoldenInfo.m_eChip < 0 || s_sGoldenInfo.m_eChip >= eCHIP_CNT ) {
        printf("Unknow platform.\n");
        return FAIL;
    }

    printf("\n=====> Parsing %s as Boot Code Optional Setting INI file <=====\n", s_sGoldenInfo.m_psTurboWriterPath);

    //Todo: To check the ProcessOptionalINI
    if ( ProcessOptionalINI( s_sGoldenInfo.m_psTurboWriterPath, &s_sGoldenInfo.m_soptional_ini_file, s_sGoldenInfo.m_eChip ) != OK )
        return FAIL;

    // To mkdir
    if ( mkdir(s_sGoldenInfo.m_psOutputPath, 0755 ) == -1)
    {
        perror("mkdir");
        if ( errno != EEXIST )
            return FAIL;
    }

    //To produce all golden files
    for(i = 0; i < s_sGoldenInfo.m_i32ImgNumber; i++)
    {
        IMG_INFO_T* psImgInfo = &s_sGoldenInfo.m_sImgInfos[i];
        switch ( psImgInfo->m_sFwImgInfo.imageFlag )
        {
        case e_IMG_TYPE_SYS:
            create_image_system ( &s_sGoldenInfo, psImgInfo );
            break;
        case e_IMG_TYPE_EXEC:
        case e_IMG_TYPE_DATA:
        case e_IMG_TYPE_ROMFS:
        case e_IMG_TYPE_LOGO:
        default:
            create_image_binary_file ( &s_sGoldenInfo, psImgInfo );
            break;
        }

        if ( !stat(psImgInfo->m_psGoldenImgName, &sb) )
        {
			psImgInfo->m_u32GoldenImgBlockNum = (UINT32) ( sb.st_size / ( s_sGoldenInfo.m_i32PagePerBlock * ( s_sGoldenInfo.m_i32PageSize + s_sGoldenInfo.m_i32OOBSize ) ) ) ;
			if ( !psImgInfo->m_u32GoldenImgBlockNum && sb.st_size > 0 )
				psImgInfo->m_u32GoldenImgBlockNum = 1;
		}

    }	// for

    print_images_opts( &s_sGoldenInfo );
	export_elnec_csv( &s_sGoldenInfo );

    return 0;
}




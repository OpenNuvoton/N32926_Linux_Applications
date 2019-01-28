/*-----------------------------------------------------------------------------------*/
/* Nuvoton Technology Corporation confidential                                       */
/*                                                                                   */
/* Copyright (c) 2012 by Nuvoton Technology Corporation                              */
/* All rights reserved                                                               */
/*-----------------------------------------------------------------------------------*/
/*
 * FILENAME
 *      NandImageMaker.h
 * DESCRIPTION
 *      Header file for NandImageMaker.
 * HISTORY
 *      2012/06/19  Create Ver 1.0.
 * REMARK
 *      None.
 *-----------------------------------------------------------------------------------*/

#define MAJOR_VERSION_NUM   1
#define MINOR_VERSION_NUM   1

/*-----------------------------------------------------------------------------------*
 * Definition for self-defined Macro.
 *-----------------------------------------------------------------------------------*/
#define OK      0
#define FAIL    -1

#define TRUE    1
#define FALSE   0

#define Successful  0
#define Fail        -1

/*-----------------------------------------------------------------------------------*
 * Definition for self-defined Data Type.
 *-----------------------------------------------------------------------------------*/
typedef unsigned int    UINT32;     // 4 bytes in MS-Windows
typedef unsigned short  UINT16;     // 2 bytes in MS-Windows
typedef char            CHAR;

/*-----------------------------------------------------------------------------------*
 * Definition for self-defined Data Structure.
 *-----------------------------------------------------------------------------------*/
//----- F/W update information
typedef struct fw_update_info_t
{
    UINT16  imageNo;
    UINT16  imageFlag;
    UINT16  startBlock;
    UINT16  endBlock;
    UINT32  executeAddr;
    UINT32  fileLen;
    CHAR    imageName[32];
} FW_UPDATE_INFO_T;


//----- Boot image information
typedef struct IBR_boot_struct_t
{
    UINT32  BootCodeMarker;
    UINT32  ExeAddr;
    UINT32  ImageSize;
    UINT32  Reserved;
} IBR_BOOT_STRUCT_T;


//----- Boot Code Optional Setting
typedef struct IBR_boot_optional_pairs_struct_t
{
    UINT32  address;
    UINT32  value;
} IBR_BOOT_OPTIONAL_PAIRS_STRUCT_T;

#define IBR_BOOT_CODE_OPTIONAL_MARKER       0xAA55AA55
#define IBR_BOOT_CODE_OPTIONAL_MAX_NUMBER   63  // support maximum 63 pairs optional setting
typedef struct IBR_boot_optional_struct_t
{
    UINT32  OptionalMarker;
    UINT32  Counter;
    IBR_BOOT_OPTIONAL_PAIRS_STRUCT_T Pairs[IBR_BOOT_CODE_OPTIONAL_MAX_NUMBER];
} IBR_BOOT_OPTIONAL_STRUCT_T;


//----- INI file information
typedef struct INI_Info
{
    //--- NandImageMaker configuration
    int  bootable;          // TRUE for bootable NAND image; FALSE for non-bootable NAND such as external NAND cartridge
    char NandLoader[32];    // the file name of NandLoader image  for input
    char Logo[32];          // the file name of Logo image        for input
    char NVTLoader[32];     // the file name of NVTLoader image   for input
    char FAT[32];           // the file name of FAT image         for input
    char Output_image[32];  // the file name of NAND golden image for output

    //--- NAND flash spec
    char nand_flash[32];    // string for NAND flash description
    int  page_size;         // page size
    int  ra_size;           // redundancy area size
    int  page_per_block;    // page number per block
    int  block_number;      // block number

    //--- Nuvoton NAND driver spec
    int  nvt_IBR_block;     // the block number                that Nuvoton IBR really used
    int  nvt_IBR_ra_size;   // redundancy area size            that Nunoton IBR really used
    int  nvt_IBR_bch_err_bits;  // BCH correctable error bits  that Nuvoton IBR really used
    int  nvt_ra_size;       // redundancy area size            that Nuvoton NAND driver really used
    int  nvt_bch_err_bits;  // BCH correctable error bits that that Nuvoton NAND driver really used
    int  nvt_reserved_area; // the reserve area size (MBytes)  that Nuvoton NAND driver really used
} INI_INFO_T;

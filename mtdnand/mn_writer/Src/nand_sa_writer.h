/****************************************************************************
 *
 * Copyright (c) 2013 Nuvoton Technology Corp. All rights reserved.
 *
 ****************************************************************************/

/****************************************************************************
 *
 * FILENAME
 *     nand_sa_writer.h
 *
 *************************************************************************/
#ifndef __NAND_SA_WRITER_H_
#define __NAND_SA_WRITER_H_

#include <linux/types.h>
#include <linux/ioctl.h>


//--- define new data type
typedef unsigned int   u32;
typedef unsigned char  u8;
typedef unsigned char  UINT8;
typedef unsigned short UINT16;
typedef unsigned int   UINT32;
typedef unsigned char  CHAR;


//--- define function prototype
extern int ProcessINI(char *fileName);
extern int ProcessOptionalINI(char *fileName);

//--- define version number
#define MAJOR_VERSION_NUM   1
#define MINOR_VERSION_NUM   0


//-- define function return value
#define    Successful  0
#define    Fail        -1


//--- This struct definition is copied from NandDrv.h
typedef struct fmi_sm_info_t
{
    u32     uSectorPerFlash;
    u32     uBlockPerFlash;
    u32     uPagePerBlock;
    u32     uSectorPerBlock;
    u32     uLibStartBlock;     // the number of block that system area used
    u32     nPageSize;
    u32     uRegionProtect;     // the page number for Region Protect End Address
    u32     uIBRBlock;          // the number of block that IBR will read with different BCH rule
    char    bIsMulticycle;
    char    bIsMLCNand;
    char    bIsNandECC4;
    char    bIsNandECC8;
    char    bIsNandECC12;
    char    bIsNandECC15;
    char    bIsNandECC24;       // TRUE to use BCH T24
    char    bIsCheckECC;
    char    bIsRA224;           // TRUE to use 224 bytes in Redundancy Area
} FMI_SM_INFO_T;


//--- Definition for IOCTL. These definition are copied from NandDrv.h
#define NAND_IOC_MAGIC      'n'
#define NAND_IOC_NR         4

#define NAND_IOC_GET_CHIP_INFO      _IOR(NAND_IOC_MAGIC, 0, FMI_IOCTL_INFO_T *)
#define NAND_IOC_IO_BLOCK_ERASE     _IOW(NAND_IOC_MAGIC, 1, FMI_IOCTL_INFO_T *)
#define NAND_IOC_IO_PAGE_READ       _IOR(NAND_IOC_MAGIC, 2, FMI_IOCTL_INFO_T *)
#define NAND_IOC_IO_PAGE_WRITE      _IOW(NAND_IOC_MAGIC, 3, FMI_IOCTL_INFO_T *)

typedef struct fmi_ioctl_info_t     // the data structure for ioctl argument
{
    u32             uBlock; // the physical block number to access
    u32             uPage;  // the page number in block to access
    u8              uWrite_nandloader;  // TRUE to write "check marker" for NandLoader
    u8              *pData; // the pointer to data buffer
    FMI_SM_INFO_T   *pSM;   // the pointer to chip information
} FMI_IOCTL_INFO_T;


//--- N329xx F/W update information
#define ImageFlag_Exec      1
#define ImageFlag_System    3
#define ImageFlag_Logo      4

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


//--- N329xx IBR booting header
typedef struct IBR_boot_struct_t
{
    UINT32  BootCodeMarker;
    UINT32  ExeAddr;
    UINT32  ImageSize;
    UINT32  Reserved;
} IBR_BOOT_STRUCT_T;


//--- INI file format definition
typedef struct INI_Info {
    char    NandLoader[512];
    char    Logo[512];
    char    ExecFile[512];
    int     ExecFileNum;
    UINT32  ExecAddress;
} INI_INFO_T;


//--- Boot Code Optional Setting definition
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

#endif // __NAND_SA_WRITER_H_

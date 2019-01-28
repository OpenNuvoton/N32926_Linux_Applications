/*-----------------------------------------------------------------------------------*/
/* Nuvoton Technology Corporation confidential                                       */
/*                                                                                   */
/* Copyright (c) 2012 by Nuvoton Technology Corporation                              */
/* All rights reserved                                                               */
/*-----------------------------------------------------------------------------------*/
/*
 * FILENAME
 *      goldenimgmaker.h
 * DESCRIPTION
 *      Header file for GoldenImgMaker.
 * HISTORY
 *      2014/09/05  Create Ver 1.0.
 * REMARK
 *      None.
 *-----------------------------------------------------------------------------------*/

#include "NandImageMaker.h"

#define DEF_MAX_IMAGES		64
#define DEF_MAX_ARGUMENTS	32

//----- BCH algorithm
typedef enum {
	e_BCH_ALGO_4=4,
	e_BCH_ALGO_8=8,
	e_BCH_ALGO_12=12,
	e_BCH_ALGO_15=15,
	e_BCH_ALGO_24=24
} E_BCH_ALGO;

//----- Image type
typedef enum {
	e_IMG_TYPE_DATA,
	e_IMG_TYPE_EXEC,
	e_IMG_TYPE_ROMFS,
	e_IMG_TYPE_SYS,
	e_IMG_TYPE_LOGO
} E_IMG_TYPE;

typedef enum  {
        eBCH_T4,
        eBCH_T8,
        eBCH_T12,
        eBCH_T15,
        eBCH_T24,
        eBCH_CNT
} E_BCHALGORITHM;

typedef enum  {
        eCHIP_N3290,
        eCHIP_N3291,        
        eCHIP_N3292,
        eCHIP_CNT
} E_CHIP;

typedef enum {
        ePageSize_512,
        ePageSize_2048,
        ePageSize_4096,
        ePageSize_8192,
        ePageSize_CNT
} E_PAGESIZE;


//----- Image file information
typedef struct Img_Info {

	FW_UPDATE_INFO_T	m_sFwImgInfo;				// Base

	CHAR	 			m_psImgPath[256];			// the path of image
	
	CHAR	 			m_psImgBaseName[256];		// the base name of image
	
	UINT32 				m_u32BCHErrorBits;			// bch_error_bits 

	int					m_i32OOBSize;				// option

	E_BCHALGORITHM		m_eBCHAlgorithm;			// BCH algorithm enumeration

	UINT32 				m_u32PartitionOffset;		// the offset value of MTD partition
	
	UINT32		 		m_u32PartitionSize;			// the size of MTD partition		

	int		 			m_i32SkipAllFFInPage;		// To avoid to write data/sparearea if all 0xFF in a page

//Golden image information
	CHAR	 			m_psGoldenImgName[256];		// the base name of image
	UINT32 				m_u32LocatedBufAddress;		// The buffer address
	UINT32 				m_u32GoldenImgBlockNum;		// The buffer address
		
} IMG_INFO_T;




//----- Golden image file information
typedef struct Golden_Info {

	E_CHIP				m_eChip;

	CHAR	 			m_psOutputPath[256];			// the output path

	CHAR	 			m_psTurboWriterPath[256];		// the file path of turbowriter

	int					m_i32ImgNumber;					// Total image in golden info
	
	int					m_i32PageSize;					// How many data area size in page
	
	int					m_i32OOBSize;					// How many spare area size in page
	
	int					m_i32PagePerBlock;				// How many page number in a block
	
	int					m_i32BlockSize;					// Block size in bytes
	
	int					m_i32BlockNumber;				// How many block number in the flash
	
	E_PAGESIZE			m_ePageSize;					// Page size enumeration
	
	IMG_INFO_T			m_sImgInfos[DEF_MAX_IMAGES];

	IBR_BOOT_OPTIONAL_STRUCT_T  m_soptional_ini_file;  // store the optional setting that parsing from INI file.

} GOLDEN_INFO_T;

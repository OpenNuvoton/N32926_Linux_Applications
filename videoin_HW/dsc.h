/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp.  All rights reserved. *
 *                                                              *
 ****************************************************************/
#ifndef __DSC_H__
#define __DSC_H__

#include <stdint.h>

// Defined Error Code in here
#ifndef ERRCODE
#define ERRCODE int32_t
#endif

#define DSC_ERR					0x80800000							
#define ERR_DSC_SUCCESS				0
#define ERR_DSC_OPEN_INDEX_FILE			(DSC_ERR | 0x1)
#define ERR_DSC_MALLOC				(DSC_ERR | 0x2)
#define ERR_DSC_OPEN_CAP_FILE			(DSC_ERR | 0x3)


ERRCODE 
DSC_NewCapFile(
	char *pchDSCFolderPath,
	int *i32NewFileFD
);

void DSC_CloseCapFile(
	char *pchDSCFolderPath,
	int i32CloseFileFD
);



#endif


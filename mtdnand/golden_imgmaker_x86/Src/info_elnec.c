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


typedef struct  {
	int m_i32PartitionStartBlock;
	int m_i32PartitionEndBlock;
	int m_i32UsedBlockNum;
	union {
		unsigned int            m_u32InvalidBlockMgnValue;
		struct {
				// bits 11:0 Maximum allowed number of invalid blocks in partition
				// 0xFFF = feature disable (default)
				// any other value specifies the number of invalid blocks that can be accepted in partition.				
				unsigned int m_u32MaximumAllowedIBNumber :12;
				
				//  bits 15:12 – Invalid blocks management technique:
				//	0x0 = Treat all blocks
				//	0x1 or 0xF = Skip IB (default)
				//	0x2 = Skip IB with excess abandon
				//	0x3 = Check IB without access
				//	Note: It is possible to specify an equivalent of Check IB with Skip IB technique using Skip IB (0x1
				//	       or 0xF) technique and non 0xFFF value for Max. allowed number of invalid blocks in partition.
                unsigned int m_u32InvalidBlockMgnTech    :4;
				
				//  bits 22:16 – Reserved for future use, consider 0x7F value for future compatibility.
                unsigned int m_u32RESERVED   	:7;

				//  bit 23 – First block in partition must be good:
				//  0 = if first block in respective partition is invalid, device is considered bad and operation is aborted
				//  1 = feature disabled (default)
                unsigned int m_u32FirstBlockNeedGood:1 ;
                
                //  bits 31:24 – File system preparation:
				//  0xFF = feature disabled (default)
				//  0x00 = JFFS2 Clean Markers are written to unused blocks at respective partition end using MSB
				//	       byte ordering (big endian)
				//  0x01 = JFFS2 Clean Markers are written to unused blocks at respective partition end using LSB
				//         byte ordering (little endian)
				//  Using values other than specified here may cause partition table load error.
                unsigned int m_ui32FileSystemPreparation:8 ;
			
		} sIBM;
		
	} U_SpecialOption;
	
	#define COMMENT_STRING_LENGTH 	64
	char m_StrComment[COMMENT_STRING_LENGTH+1];
	
} S_ELNEC_ENTRY;

enum {
	eIbMgnTech_TreatAllBlocks,
	eIbMgnTech_SkipIB,
	eIbMgnTech_SkipIBWithoutAccess,
	eIbMgnTech_CkeckIBWithoutAccess,
	eIbMgnTech_SkipIBFF=0xf,
} E_IBMGNTECH;

enum {
	eFSPreparation_JFFS2MSB,
	eFSPreparation_JFFS2LSB,
	eFSPreparation_FeatureDisabled=0xff,
} E_FSPreparation;	
				
int export_elnec_csv ( GOLDEN_INFO_T* psGoldenInfo )
{
    int i;
	if( psGoldenInfo->m_i32ImgNumber > 0 )
	{
		char tmp[256];
		FILE * fdout;
		sprintf(tmp, "%s/elnec.csv", psGoldenInfo->m_psOutputPath );
		fdout = fopen(tmp, "wb");
		if ( fdout == NULL )
			return -1;

		S_ELNEC_ENTRY* psElnecEntry =  malloc ( sizeof(S_ELNEC_ENTRY)*psGoldenInfo->m_i32ImgNumber );
        printf("%s: Export CSV.\n", __FUNCTION__ );
		for(i = 0; i < psGoldenInfo->m_i32ImgNumber; i++)
		{
			IMG_INFO_T* psImgInfo = &psGoldenInfo->m_sImgInfos[i];
			memset( (void*)&psElnecEntry[i], 0, sizeof(S_ELNEC_ENTRY) );         
			psElnecEntry[i].m_i32PartitionStartBlock = psImgInfo->m_sFwImgInfo.startBlock;
			psElnecEntry[i].m_i32PartitionEndBlock = psImgInfo->m_sFwImgInfo.endBlock;
			psElnecEntry[i].m_i32UsedBlockNum = psImgInfo->m_u32GoldenImgBlockNum;
			strncpy( psElnecEntry[i].m_StrComment, psImgInfo->m_sFwImgInfo.imageName , COMMENT_STRING_LENGTH );
			
			psElnecEntry[i].U_SpecialOption.m_u32InvalidBlockMgnValue = 0xFFFFFFFF;
			
			if ( psImgInfo->m_sFwImgInfo.imageFlag == e_IMG_TYPE_SYS )
				psElnecEntry[i].U_SpecialOption.sIBM.m_u32MaximumAllowedIBNumber 	= 3;
			else 
				psElnecEntry[i].U_SpecialOption.sIBM.m_u32MaximumAllowedIBNumber 	= (psElnecEntry[i].m_i32PartitionEndBlock-psElnecEntry[i].m_i32PartitionStartBlock+1) * 0.05 ;
				
            psElnecEntry[i].U_SpecialOption.sIBM.m_u32InvalidBlockMgnTech 			= eIbMgnTech_SkipIBFF ;
            //psElnecEntry[i].U_SpecialOption.sIBM.m_u32RESERVED 					= 0xFF ;
            psElnecEntry[i].U_SpecialOption.sIBM.m_u32FirstBlockNeedGood 			= 0 ;
            psElnecEntry[i].U_SpecialOption.sIBM.m_ui32FileSystemPreparation		= eFSPreparation_FeatureDisabled ;

			fprintf(fdout, "%4d; %4d; %4d; 0x%x; %s\n", \
											psElnecEntry[i].m_i32PartitionStartBlock, \
											psElnecEntry[i].m_i32PartitionEndBlock, \
											psElnecEntry[i].m_i32UsedBlockNum, \
											psElnecEntry[i].U_SpecialOption.m_u32InvalidBlockMgnValue ,  \
											psElnecEntry[i].m_StrComment );
											
            printf("%4d; %4d; %4d; 0x%x; %s\n", \
											psElnecEntry[i].m_i32PartitionStartBlock, \
											psElnecEntry[i].m_i32PartitionEndBlock, \
											psElnecEntry[i].m_i32UsedBlockNum, \
											psElnecEntry[i].U_SpecialOption.m_u32InvalidBlockMgnValue ,  \
											psElnecEntry[i].m_StrComment );
											
		}	// for

		printf("\nTo decide maximum allowed invalid block number rule:\n" );
        printf("\t (1) The image type is SYS, the value is 3.\n" );
        printf("\t (2) others is total block number*0.05 .\n" );

		fclose(fdout);
		free(psElnecEntry);		
	} else
		printf("%s: no entry! \n", __FUNCTION__ );
		
	printf("\n");
	return 0;
}

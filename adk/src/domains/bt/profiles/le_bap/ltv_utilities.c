/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_bap
    \brief      Utility functions for extracting information from LTV (length, type, value) data

                Lengths, Types and Values here are defined in BAPS_Assigned_Numbers_v3
*/

#include "ltv_utilities.h"

#include <logging.h>

#define LTV_UTILITIES_LOG     DEBUG_LOG

#define LC3_CODEC_CONFIG_LTV_SIZE_AUDIO_CHANNEL_ALLOCATION                  0x04
#define LC3_CODEC_CONFIG_LTV_SIZE_OCTETS_PER_CODEC_FRAME                    0x02


#define LTV_LENGTH_OFFSET               0x00
#define LTV_TYPE_OFFSET                 0x01
#define LTV_VALUE_OFFSET                0x02

uint8 LtvUtilities_FindLc3EncoderVersionFromVsMetadata(uint8 * metadata, uint8 metadata_length)
{
    uint8 vs_ltv_value[VS_METADATA_LC3_VALUE_LENGTH] = {0};
    uint8 lc3_verion = 0x00;
    uint8 vs_ltv_offset;

    if(BapServerLtvUtilitiesFindLtvOffset(metadata, metadata_length, BAP_METADATA_LTV_TYPE_VENDOR_SPECIFIC_CONTEXTS, &vs_ltv_offset))
    {
        /* VS Metadata LTV present, check if LC3 VS LTV type is available in it */
        if (BapServerLtvUtilitiesFindLtvValue(&metadata[vs_ltv_offset + LTV_VALUE_OFFSET + VS_METADATA_LC3_COMPANY_ID_QUALCOMM_SIZE],
                                              metadata[vs_ltv_offset + LTV_LENGTH_OFFSET] - (LTV_TYPE_OFFSET + VS_METADATA_LC3_COMPANY_ID_QUALCOMM_SIZE),
                                              VS_METADATA_TYPE_LC3, vs_ltv_value, VS_METADATA_LC3_VALUE_LENGTH))
        {
            /* First octet is version */
            lc3_verion = vs_ltv_value[0];
        }
      
    }

    return lc3_verion;
}

uint8* LtvUtilities_FindContentControlIdList(uint8 * metadata, uint8 metadata_length, uint8 *ccid_list_length)
{
    uint8 *ccid_list = NULL;
    uint8 ccid_offset = 0;

    if (BapServerLtvUtilitiesFindLtvOffset(metadata, metadata_length, BAP_METADATA_LTV_TYPE_CCID_LIST, &ccid_offset))
    {
        /* CCID list is available in the metadata */
        *ccid_list_length = metadata[ccid_offset + LTV_LENGTH_OFFSET] - 1;
        ccid_list = &metadata[ccid_offset + LTV_VALUE_OFFSET];
    }

    return ccid_list;
}

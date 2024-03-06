/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "csr_util.h"
#include "csr_pmem.h"

#include "audio_announcement_parser_lib.h"
#include "audio_announcement_parser_debug.h"

/******************************************************************************/

/* Minumum length of the Broadcast Audio Announcement */
#define BROADCAST_AUDIO_ANNOUNCEMENT_ADVERT_LENGTH_MIN  (7)
/* Minumum length of the Basic Audio Announcement */
#define BASIC_AUDIO_ANNOUNCEMENT_ADVERT_LENGTH_MIN  (18)
/* Minumum length of the Public Broadcast Audio Announcement */
#define PUBLIC_BROADCAST_AUDIO_ANNOUNCEMENT_ADVERT_LENGTH_MIN  (6)
/* Minumum length of the Broadcast Name */
#define PUBLIC_BROADCAST_AUDIO_ANNOUNCEMENT_BROADCAST_NAME_LEN_MIN  (4)

/* Minumum length of Level 2 in Basic Audio Announcement */
#define BASIC_AUDIO_ANNOUNCEMENT_LEVEL_2_LENGTH_MIN  (10)
/* Minumum length of Level 3 in Basic Audio Announcement */
#define BASIC_AUDIO_ANNOUNCEMENT_LEVEL_3_LENGTH_MIN  (2)

/* Presentation delay length */
#define PRESENTATION_DELAY_LENGTH  (3)
/* Company ID length */
#define COMPANY_ID_LENGTH  (2)
/* Vendor Specific Codec ID length */
#define VENDOR_SPECIFIC_CODEC_ID_LENGTH  (2)

/* Basic Audio Announcement Service UUID length*/
#define BASIC_AUDIO_ANNOUNCEMENT_UUID_LENGTH  (2)
/* Basic Audio Announcement Service UUID length*/
#define PUBLIC_BROADCAST_AUDIO_ANNOUNCEMENT_UUID_LENGTH  (2)

/* Length of the Sampling Frequency LTV structure */
#define SAMPLING_FREQUENCY_LTV_STRUCTURE_LENGTH  (3)
/* Length of the Frame Durations LTV structure */
#define FRAME_DURATION_LTV_STRUCTURE_LENGTH  (3)
/* Length of the Audio Channel Allocation LTV structure */
#define AUDIO_CHANNEL_ALLOCATION_LTV_STRUCTURE_LENGTH  (6)
/* Length of the Octets Per Codec Frame LTV structure */
#define OCTETS_PER_CODEC_FRAME_LTV_STRUCTURE_LENGTH  (4)
/* Length of the Codec Frame Blocks Per Sdu LTV structure */
#define CODEC_FRAME_BLOCKS_PER_SDU_LTV_STRUCTURE_LENGTH  (3)

/* Length of the Preferred Audio Context LTV structure */
#define PREFERRED_AUDIO_CONTEXT_LTV_STRUCTURE_LENGTH  (4)
/* Length of the Streaming Audio Context LTV structure */
#define STREAMING_AUDIO_CONTEXT_LTV_STRUCTURE_LENGTH  (4)
/* Length of the Language LTV structure */
#define LANGUAGE_LTV_STRUCTURE_LENGTH  (5)
/* Length of the Audio Active State LTV structure */
#define AUDIO_ACTIVE_STATE_LTV_STRUCTURE_LENGTH  (3)

/* Ad type for the Service Data - 16-bit UUID */
#define AD_TYPE_SERVICE_DATA_16_BIT_UUID  (0x16u)
/* Ad type for the Service Data - 16-bit UUID */
#define AD_TYPE_BROADCAST_NAME            (0x30u)
/* Ad type length */
#define AD_TYPE_LENGTH                    (0x01u)

/* Broadcast Audio Announcement Service UUID */
#define BROADCAST_AUDIO_ANNOUNCEMENT_SERVICE_UUID         (0x1852u)
/* Basic Audio Announcement Service UUID */
#define BASIC_AUDIO_ANNOUNCEMENT_SERVICE_UUID             (0x1851u)
/* Broadcast Audio Announcement Service UUID */
#define PUBLIC_BROADCAST_AUDIO_ANNOUNCEMENT_SERVICE_UUID  (0x1856u)

/* Coding Format value for Vendor Specific */
#define CODING_FORMAT_VENDOR_SPECIFIC  (0xFFu)

/* LTV structure type for the Sampling Frequency parameter */
#define SAMPLING_FREQUENCY_LTV_STRUCTURE_TYPE  (0x01u)
/* VS LTV structure type for the Sampling Frequency parameter */
#define SAMPLING_FREQUENCY_LTV_STRUCTURE_TYPE_VS (0x81u)
/* LTV structure type for the Frame Duration parameter */
#define FRAME_DURATION_LTV_STRUCTURE_TYPE  (0x02u)
/* LTV structure type for the Audio Channel Allocation parameter */
#define AUDIO_CHANNEL_ALLOCATION_LTV_STRUCTURE_TYPE  (0x03u)
/* LTV structure type for the Octets Per Codec Frame parameter */
#define OCTETS_PER_CODEC_FRAME_LTV_STRUCTURE_TYPE  (0x04u)
/* LTV structure type for the Codec Frame Blocks Per Sdu parameter */
#define CODEC_FRAME_BLOCKS_PER_SDU_LTV_STRUCTURE_TYPE  (0x05u)

/* LTV structure type for the Preferred Audio Contexts parameter */
#define PREFERRED_AUDIO_CONTEXTS_LTV_STRUCTURE_TYPE  (0x01u)
/* LTV structure type for the Streaming Audio Contexts parameter */
#define STREAMING_AUDIO_CONTEXTS_LTV_STRUCTURE_TYPE  (0x02u)
/* LTV structure type for the Program Info parameter */
#define PROGRAM_INFO_LTV_STRUCTURE_TYPE  (0x03u)
/* LTV structure type for the Language parameter */
#define LANGUAGE_LTV_STRUCTURE_TYPE  (0x04u)
/* LTV structure type for the Audio Active State parameter */
#define AUDIO_ACTIVE_STATE_LTV_STRUCTURE_TYPE  (0x08u)

/* PBP Features bitmasks */
#define PBP_FEATURES_ENCRYPTION (0x01u)
#define PBP_FEATURES_SQ         (0x02u)
#define PBP_FEATURES_HQ         (0x04u)

/******************************************************************************/

/* Data type to describe the type of LTV buffer to parse */
typedef uint8 LtvBufferType;

/* Values for the LtvBufferType type */
#define LTV_BUFFER_TYPE_SPECIFIC_CODEC_CONFIG (0x01u)
#define LTV_BUFFER_TYPE_METADATA              (0x02u)

/******************************************************************************/

uint8 *AudioAnnouncementParserBcastNameParsing(uint16 advDataLen,
                                               const uint8 *advData,
                                               uint8 *nameLen)
{
    uint8 adDataTypeLen = 0;
    const uint8 *advDataPtr = NULL;
    uint16 remainingAdvDataLen = 0;
    uint8 *name = NULL;

    if(!advData || !nameLen)
    {
        AUDIO_ANNOUNCEMENT_PARSER_ERROR("(AUDIO ANNOUNCEMENT PARSER) : One or more invalid paramters!\n");
    }
    else if(advDataLen >= PUBLIC_BROADCAST_AUDIO_ANNOUNCEMENT_BROADCAST_NAME_LEN_MIN)
    {
        (*nameLen) = 0;
        advDataPtr = advData;
        remainingAdvDataLen = advDataLen;

        while(remainingAdvDataLen)
        {
            adDataTypeLen = (*advDataPtr++);

            if(adDataTypeLen >= (PUBLIC_BROADCAST_AUDIO_ANNOUNCEMENT_BROADCAST_NAME_LEN_MIN - 1) &&
                    (*advDataPtr) == AD_TYPE_BROADCAST_NAME)
            {
                (*nameLen) = (adDataTypeLen - AD_TYPE_LENGTH);
                name = CsrPmemAlloc(*nameLen);
                CsrMemCpy(name, ++advDataPtr,(*nameLen));

                AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Broadcast Audio Announcement found!\n");
                break;
            }

            advDataPtr += adDataTypeLen;
            remainingAdvDataLen -= (adDataTypeLen + 1);
        }
    }

    return name;
}

/******************************************************************************/
AudioAnnouncementParserStatus AudioAnnouncementParserBcastAudioAnnouncementParsing(uint16 advDataLen,
                                                                                       uint8 *advData,
                                                                                       uint32 *broadcastId)
{
    uint8 adDataTypeLen = 0;
    uint8 *advDataPtr = NULL;
    uint16 remainingAdvDataLen = 0;

    if(!advData || !broadcastId)
    {
        AUDIO_ANNOUNCEMENT_PARSER_ERROR("(AUDIO ANNOUNCEMENT PARSER) : One or more invalid paramters!\n");
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_PARAMETER;
    }

    if(advDataLen < BROADCAST_AUDIO_ANNOUNCEMENT_ADVERT_LENGTH_MIN)
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_LENGTH;

    (*broadcastId) = 0;
    advDataPtr = advData;
    remainingAdvDataLen = advDataLen;

    while(remainingAdvDataLen)
    {
        adDataTypeLen = (*advDataPtr++);

        if(adDataTypeLen >= (BROADCAST_AUDIO_ANNOUNCEMENT_ADVERT_LENGTH_MIN - 1) &&
           (*advDataPtr) == AD_TYPE_SERVICE_DATA_16_BIT_UUID)
        {
            uint8* serviceDataPtr = advDataPtr;
            uint16 serviceUuid = 0;

            serviceDataPtr++;

            serviceUuid = *(serviceDataPtr++);
            serviceUuid |= ((*(serviceDataPtr)) << 8);

            if(serviceUuid == BROADCAST_AUDIO_ANNOUNCEMENT_SERVICE_UUID)
            {
                AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Broadcast Audio Announcement found!\n");

                serviceDataPtr++;
                (*broadcastId) = CSR_GET_UINT24_FROM_LITTLE_ENDIAN(serviceDataPtr);

                AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Broadcast ID: 0x%08x!\n", (*broadcastId));

                return AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS;
            }

        }

        advDataPtr += adDataTypeLen;
        remainingAdvDataLen -= (adDataTypeLen + 1);
    }

    return AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND;
}

/******************************************************************************/
static uint8 * audioAnnouncementParserSpecificCodecConfigurationParsing(uint8 **dataToParse,
                                                                        uint16 advDataLen,
                                                                        uint8 *minLen,
                                                                        uint8 *codecSpecificConfigLen,
                                                                        AudioAnnouncementParserStatus *status)
{
    uint8 * codecSpecificConfig = NULL;

    (*codecSpecificConfigLen) = *(++(*dataToParse));
    AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Codec Specific Config Length 0x%02x!\n", (*codecSpecificConfigLen));

    (*minLen) += (*codecSpecificConfigLen);

    if(advDataLen < (*minLen))
    {
       (*status) = AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_LENGTH;
       return codecSpecificConfig;
    }

    if((*codecSpecificConfigLen))
    {
        codecSpecificConfig = CsrPmemAlloc((*codecSpecificConfigLen) * sizeof(uint8));
        CsrMemCpy(codecSpecificConfig, ++(*dataToParse), (*codecSpecificConfigLen));

        *dataToParse += ((*codecSpecificConfigLen) - 1);
    }

    return codecSpecificConfig;
}

/******************************************************************************/

static AudioAnnouncementParserStatus audioAnnouncementParserBisDataParsing(uint8 **bisDataToParse,
                                                                           uint16 advDataLen,
                                                                           uint8 numBis,
                                                                           uint8 *minLen,
                                                                           AudioAnnouncementParserBaseLvl3Data *bisData)
{
    AudioAnnouncementParserStatus status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS;
    uint8 j = 0;

    for(j=0; j<numBis; j++)
    {
        bisData[j].bisIndex = *(++(*bisDataToParse));
        AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : BIS Index %d\n", bisData[j].bisIndex);

        bisData[j].codecSpecificConfig = audioAnnouncementParserSpecificCodecConfigurationParsing(
                    bisDataToParse,
                    advDataLen,
                    minLen,
                    &bisData[j].codecSpecificConfigLen,
                    &status);
    }

    return status;
}

/******************************************************************************/

static AudioAnnouncementParserStatus audioAnnouncementParserSubGroupDataParsing(uint8 * subGroupDataToParse,
                                                                                uint16 advDataLen,
                                                                                uint8 numSubGroups,
                                                                                uint8 *minLen,
                                                                                AudioAnnouncementParserBaseLevel2Data *subGroupsData)
{
    uint8 i = 0;
    AudioAnnouncementParserStatus status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS;

    for(i=0; i<numSubGroups; i++)
    {
        subGroupsData[i].numBis = *(++subGroupDataToParse);
        AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Num Bis: %d!\n", subGroupsData[i].numBis);

        if(!subGroupsData[i].numBis)
        {
            status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_BAD_FORMAT;
            break;
        }

        (*minLen) += (subGroupsData[i].numBis - 1) * BASIC_AUDIO_ANNOUNCEMENT_LEVEL_3_LENGTH_MIN;

        if(advDataLen < (*minLen))
        {
            status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_LENGTH;
            break;
        }

        subGroupsData[i].codecId.codingFormat = *(++subGroupDataToParse);
        AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : CODEC ID: Coding Format: 0x%2x!\n", subGroupsData[i].codecId.codingFormat);

        subGroupDataToParse++;

        subGroupsData[i].codecId.companyId  = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(subGroupDataToParse);
        subGroupDataToParse += (COMPANY_ID_LENGTH - 1);
        AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : CODEC ID: Company ID: 0x%4x!\n", subGroupsData[i].codecId.companyId);

        if(subGroupsData[i].codecId.codingFormat == CODING_FORMAT_VENDOR_SPECIFIC &&
                subGroupsData[i].codecId.companyId)
        {
            status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_BAD_FORMAT;
            break;
        }

        subGroupDataToParse++;

        subGroupsData[i].codecId.vendorSpecificCodecId = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(subGroupDataToParse);
        subGroupDataToParse += (VENDOR_SPECIFIC_CODEC_ID_LENGTH - 1);
        AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : CODEC ID: Vedor Specific Codec ID: 0x%4x!\n", subGroupsData[i].codecId.vendorSpecificCodecId);

        if(subGroupsData[i].codecId.codingFormat == CODING_FORMAT_VENDOR_SPECIFIC &&
                subGroupsData[i].codecId.vendorSpecificCodecId)
        {
            status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_BAD_FORMAT;
            break;
        }

        subGroupsData[i].codecSpecificConfig = audioAnnouncementParserSpecificCodecConfigurationParsing(
                    &subGroupDataToParse,
                    advDataLen,
                    minLen,
                    &subGroupsData[i].codecSpecificConfigLen,
                    &status);

        if(status == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS)
        {
            subGroupsData[i].metadataLen = *(++subGroupDataToParse);
            AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Metadata Length 0x%02x!\n", subGroupsData[i].metadataLen);

            if(subGroupsData[i].metadataLen)
            {
                (*minLen) += subGroupsData[i].metadataLen;

                if(advDataLen < (*minLen))
                {
                    status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_LENGTH;
                    break;
                }

                subGroupsData[i].metadata = CsrPmemAlloc(subGroupsData[i].metadataLen * sizeof(uint8));
                CsrMemCpy(subGroupsData[i].metadata,
                          ++subGroupDataToParse,
                          subGroupsData[i].metadataLen);

                subGroupDataToParse += (subGroupsData[i].metadataLen - 1);
            }
            else
            {
                subGroupsData[i].metadata = NULL;
            }

            subGroupsData[i].bisData = CsrPmemAlloc(subGroupsData[i].numBis *
                                                    sizeof(AudioAnnouncementParserBaseLvl3Data));

            status = audioAnnouncementParserBisDataParsing(&subGroupDataToParse,
                                                           advDataLen,
                                                           subGroupsData[i].numBis,
                                                           minLen,
                                                           subGroupsData[i].bisData);
        }
    }

    return status;
}

/******************************************************************************/
static void audioAnnouncementParserBaseDataCleaning(AudioAnnouncementParserBaseData *baseData)
{
    if(baseData->subGroupsData)
    {
        uint8 i = 0;

        for(i=0; i<baseData->numSubgroups; i++)
        {
           if(baseData->subGroupsData[i].codecSpecificConfig)
               CsrPmemFree(baseData->subGroupsData[i].codecSpecificConfig);

           if(baseData->subGroupsData[i].metadata)
               CsrPmemFree(baseData->subGroupsData[i].metadata);

           if(baseData->subGroupsData[i].bisData)
           {
               uint8 j = 0;

               for(j=0; j<baseData->subGroupsData[i].numBis; j++)
               {
                   if(baseData->subGroupsData[i].bisData[j].codecSpecificConfig)
                       CsrPmemFree(baseData->subGroupsData[i].bisData[j].codecSpecificConfig);
               }

               CsrPmemFree(baseData->subGroupsData[i].bisData);
           }
        }

        CsrPmemFree(baseData->subGroupsData);
    }
}

/******************************************************************************/

AudioAnnouncementParserStatus AudioAnnouncementParserBasicAudioAnnouncementParsing(uint16 advDataLen,
                                                                                   uint8 *advData,
                                                                                   AudioAnnouncementParserBaseData *base)
{
    uint8 adDataTypeLen = 0;
    uint8 *advDataPtr = NULL;
    uint16 remainingAdvDataLen = 0;
    AudioAnnouncementParserBaseData baseData;
    AudioAnnouncementParserStatus status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS;
    bool isBaasFound = FALSE;

    if(!base || !advData)
    {
        AUDIO_ANNOUNCEMENT_PARSER_ERROR("(AUDIO ANNOUNCEMENT PARSER) : One or more invalid paramters!\n");
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_PARAMETER;
    }

    if(advDataLen < BASIC_AUDIO_ANNOUNCEMENT_ADVERT_LENGTH_MIN)
        status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_LENGTH;

    CsrMemSet(base, 0, sizeof(AudioAnnouncementParserBaseData));
    CsrMemSet(&baseData, 0, sizeof(AudioAnnouncementParserBaseData));

    if(status == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS)
    {
        advDataPtr = advData;
        remainingAdvDataLen = advDataLen;

        while(remainingAdvDataLen)
        {
            adDataTypeLen = (*advDataPtr++);

            if(adDataTypeLen >= (BASIC_AUDIO_ANNOUNCEMENT_ADVERT_LENGTH_MIN - 1) &&
               (*advDataPtr) == AD_TYPE_SERVICE_DATA_16_BIT_UUID)
            {
                uint8* serviceDataPtr = advDataPtr;
                uint16 serviceUuid = 0;

                serviceDataPtr++;

                serviceUuid = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(serviceDataPtr);
                serviceDataPtr += (BASIC_AUDIO_ANNOUNCEMENT_UUID_LENGTH - 1);

                if(serviceUuid == BASIC_AUDIO_ANNOUNCEMENT_SERVICE_UUID)
                {
                    uint8 minLen = BASIC_AUDIO_ANNOUNCEMENT_ADVERT_LENGTH_MIN;

                    AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Basic Audio Announcement found!\n");

                    isBaasFound = TRUE;

                    serviceDataPtr++;

                    baseData.presentationDelay = CSR_GET_UINT24_FROM_LITTLE_ENDIAN(serviceDataPtr);
                    serviceDataPtr += (PRESENTATION_DELAY_LENGTH - 1);

                    AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) :Presentation Delay: 0x%06x!\n", baseData.presentationDelay);

                    baseData.numSubgroups = *(++serviceDataPtr);
                    AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Num Sub Groups: %d!\n",baseData.numSubgroups);

                    if(!baseData.numSubgroups)
                    {
                        status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_BAD_FORMAT;
                        break;
                    }

                    minLen += (baseData.numSubgroups - 1) * (BASIC_AUDIO_ANNOUNCEMENT_LEVEL_2_LENGTH_MIN - 1);

                    if(advDataLen < minLen)
                    {
                        status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_LENGTH;
                        break;
                    }

                    baseData.subGroupsData = CsrPmemAlloc(baseData.numSubgroups *
                                                              sizeof(AudioAnnouncementParserBaseLevel2Data));
                    CsrMemSet(baseData.subGroupsData,
                              0,
                              baseData.numSubgroups * sizeof(AudioAnnouncementParserBaseLevel2Data));

                    status = audioAnnouncementParserSubGroupDataParsing(serviceDataPtr,
                                                                        advDataLen,
                                                                        baseData.numSubgroups,
                                                                        &minLen,
                                                                        baseData.subGroupsData);
                }
            }

            if(isBaasFound)
                break;

            advDataPtr += adDataTypeLen;
            remainingAdvDataLen -= (adDataTypeLen + 1);
        }

        if(!isBaasFound)
            status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND;
    }

    if(status == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS)
    {
        CsrMemCpy(base, &baseData, sizeof(AudioAnnouncementParserBaseData));
    }
    else
    {
        audioAnnouncementParserBaseDataCleaning(&baseData);
    }

    return status;
}

/******************************************************************************/

static bool audioAnnouncementParserCheckCodecConfigLtvStructureLenFromType(uint8 ltvStructureType,
                                                                           uint8 ltvStructureLen)
{
    switch(ltvStructureType)
    {
        case SAMPLING_FREQUENCY_LTV_STRUCTURE_TYPE:
        case SAMPLING_FREQUENCY_LTV_STRUCTURE_TYPE_VS:
            return(ltvStructureLen == (SAMPLING_FREQUENCY_LTV_STRUCTURE_LENGTH - 1));
        case FRAME_DURATION_LTV_STRUCTURE_TYPE:
            return(ltvStructureLen == (FRAME_DURATION_LTV_STRUCTURE_LENGTH - 1));
        case AUDIO_CHANNEL_ALLOCATION_LTV_STRUCTURE_TYPE:
            return(ltvStructureLen == (AUDIO_CHANNEL_ALLOCATION_LTV_STRUCTURE_LENGTH - 1));
        case OCTETS_PER_CODEC_FRAME_LTV_STRUCTURE_TYPE:
            return(ltvStructureLen == (OCTETS_PER_CODEC_FRAME_LTV_STRUCTURE_LENGTH - 1));
        case CODEC_FRAME_BLOCKS_PER_SDU_LTV_STRUCTURE_TYPE:
            return(ltvStructureLen == (CODEC_FRAME_BLOCKS_PER_SDU_LTV_STRUCTURE_LENGTH - 1));
        default:
            return FALSE;
    }
}

/******************************************************************************/

static bool audioAnnouncementParserCheckMetadataLtvStructureLenFromType(uint8 ltvStructureType,
                                                                        uint8 ltvStructureLen)
{
    switch(ltvStructureType)
    {
        case PREFERRED_AUDIO_CONTEXTS_LTV_STRUCTURE_TYPE:
            return(ltvStructureLen == (PREFERRED_AUDIO_CONTEXT_LTV_STRUCTURE_LENGTH - 1));
        case STREAMING_AUDIO_CONTEXTS_LTV_STRUCTURE_TYPE:
            return(ltvStructureLen == (STREAMING_AUDIO_CONTEXT_LTV_STRUCTURE_LENGTH - 1));
        case PROGRAM_INFO_LTV_STRUCTURE_TYPE:
            /* The length of Program Info is variable */
            return TRUE;
        case LANGUAGE_LTV_STRUCTURE_TYPE:
            return(ltvStructureLen == (LANGUAGE_LTV_STRUCTURE_LENGTH - 1));
        case AUDIO_ACTIVE_STATE_LTV_STRUCTURE_TYPE:
            return(ltvStructureLen == (AUDIO_ACTIVE_STATE_LTV_STRUCTURE_LENGTH - 1));
        default:
            return FALSE;
    }
}

/******************************************************************************/

static bool audioAnnouncementParserCheckLtvStructureLenFromType(uint8 ltvStructureType,
                                                                uint8 ltvStructureLen,
                                                                LtvBufferType ltvBufferType)
{
    bool result = FALSE;

    switch(ltvBufferType)
    {
        case LTV_BUFFER_TYPE_SPECIFIC_CODEC_CONFIG:
            result = audioAnnouncementParserCheckCodecConfigLtvStructureLenFromType(ltvStructureType,
                                                                                    ltvStructureLen);
        break;

        case LTV_BUFFER_TYPE_METADATA:
            result = audioAnnouncementParserCheckMetadataLtvStructureLenFromType(ltvStructureType,
                                                                                 ltvStructureLen);
        break;

        default:
        break;
    }

    return result;
}

/******************************************************************************/

static AudioAnnouncementParserStatus audioAnnouncementParserLtvStructureParsing(uint8 *dataToParse,
                                                                                uint8 ltvStructureType,
                                                                                LtvBufferType ltvBufferType,
                                                                                uint8 *ltvStructureLen,
                                                                                uint8 **ltvStructureValue)
{
    AudioAnnouncementParserStatus status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND;

    (*ltvStructureValue) = NULL;
    (*ltvStructureLen) = (*dataToParse);

    if(*ltvStructureLen)
    {
        if((*(++dataToParse)) == ltvStructureType &&
            audioAnnouncementParserCheckLtvStructureLenFromType(ltvStructureType,
                                                                *ltvStructureLen,
                                                                ltvBufferType))
        {
            (*ltvStructureValue) = ++dataToParse;
            status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS;
        }
    }
    else
    {
        status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_BAD_FORMAT;
    }

    return status;
}

/******************************************************************************/

AudioAnnouncementParserStatus AudioAnnouncementParserGetSamplingFreqFromCodecCnfg(
                                                                uint8 codecSpecificConfigLen,
                                                                uint8 *codecSpecificConfig,
                                                                AudioAnnouncementParserSamplingFreqType *samplingFrequency)
{
    AudioAnnouncementParserStatus status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND;
    uint8 remainingCodecConfigDataLen = codecSpecificConfigLen;
    uint8 *codecSpecificConfigPtr = codecSpecificConfig;

    if(!codecSpecificConfig || !samplingFrequency)
    {
        AUDIO_ANNOUNCEMENT_PARSER_ERROR("(AUDIO ANNOUNCEMENT PARSER) : One or more invalid paramters!\n");
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_PARAMETER;
    }

    if(codecSpecificConfigLen < SAMPLING_FREQUENCY_LTV_STRUCTURE_LENGTH)
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_LENGTH;

    (*samplingFrequency) = 0;

    while (remainingCodecConfigDataLen)
    {
        uint8 ltvStructureLen = 0;
        uint8 *ltvStructureValue;

        status = audioAnnouncementParserLtvStructureParsing(codecSpecificConfigPtr,
                                                            SAMPLING_FREQUENCY_LTV_STRUCTURE_TYPE,
                                                            LTV_BUFFER_TYPE_SPECIFIC_CODEC_CONFIG,
                                                            &ltvStructureLen,
                                                            &ltvStructureValue);

        if(status == AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND)
        {
            status = audioAnnouncementParserLtvStructureParsing(codecSpecificConfigPtr,
                                                                SAMPLING_FREQUENCY_LTV_STRUCTURE_TYPE_VS,
                                                                LTV_BUFFER_TYPE_SPECIFIC_CODEC_CONFIG,
                                                                &ltvStructureLen,
                                                                &ltvStructureValue);
        }

        if(status == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS && ltvStructureValue)
        {
            *samplingFrequency = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(ltvStructureValue);
            AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Sampling Frequency LTV structure found: 0x%04x\n", (*samplingFrequency));
            return status;
        }

        if(status != AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND)
            return status;

        codecSpecificConfigPtr += (ltvStructureLen + 1);
        remainingCodecConfigDataLen -= (ltvStructureLen + 1);
    }

    return status;
}

/******************************************************************************/

AudioAnnouncementParserStatus AudioAnnouncementParserGetFrameDurationFromCodecCnfg(
                                                              uint8 codecSpecificConfigLen,
                                                              uint8 *codecSpecificConfig,
                                                              AudioAnnouncementParserFrameDurationType *frameDuration)
{
    AudioAnnouncementParserStatus status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND;
    uint8 remainingCodecConfigDataLen = codecSpecificConfigLen;
    uint8 *codecSpecificConfigPtr = codecSpecificConfig;

    if(!codecSpecificConfig || !frameDuration)
    {
        AUDIO_ANNOUNCEMENT_PARSER_ERROR("(AUDIO ANNOUNCEMENT PARSER) : One or more invalid paramters!\n");
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_PARAMETER;
    }

    if(codecSpecificConfigLen < FRAME_DURATION_LTV_STRUCTURE_LENGTH)
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_LENGTH;

    (*frameDuration) = 0;

    while (remainingCodecConfigDataLen)
    {
        uint8 ltvStructureLen = 0;
        uint8 *ltvStructureValue;

        status = audioAnnouncementParserLtvStructureParsing(codecSpecificConfigPtr,
                                                            FRAME_DURATION_LTV_STRUCTURE_TYPE,
                                                            LTV_BUFFER_TYPE_SPECIFIC_CODEC_CONFIG,
                                                            &ltvStructureLen,
                                                            &ltvStructureValue);

        if(status == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS && ltvStructureValue)
        {
            *frameDuration = *ltvStructureValue;
            AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Frame Duration LTV structure found: 0x%02x\n", (*frameDuration));
            return status;
        }

        if(status != AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND)
            return status;

        codecSpecificConfigPtr += (ltvStructureLen + 1);
        remainingCodecConfigDataLen -= (ltvStructureLen + 1);
    }

    return status;
}

/******************************************************************************/

AudioAnnouncementParserStatus AudioAnnouncementParserGetAudioChannelAllocationFromCodecCnfg(
                                                                     uint8 codecSpecificConfigLen,
                                                                     uint8 *codecSpecificConfig,
                                                                     uint32 *audioChannelAllocation)
{
    AudioAnnouncementParserStatus status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND;
    uint8 remainingCodecConfigDataLen = codecSpecificConfigLen;
    uint8 *codecSpecificConfigPtr = codecSpecificConfig;

    if(!codecSpecificConfig || !audioChannelAllocation)
    {
        AUDIO_ANNOUNCEMENT_PARSER_ERROR("(AUDIO ANNOUNCEMENT PARSER) : One or more invalid paramters!\n");
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_PARAMETER;
    }

    if(codecSpecificConfigLen < AUDIO_CHANNEL_ALLOCATION_LTV_STRUCTURE_LENGTH)
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_LENGTH;

    (*audioChannelAllocation) = 0;

    while (remainingCodecConfigDataLen)
    {
        uint8 ltvStructureLen = 0;
        uint8 *ltvStructureValue;

        status = audioAnnouncementParserLtvStructureParsing(codecSpecificConfigPtr,
                                                            AUDIO_CHANNEL_ALLOCATION_LTV_STRUCTURE_TYPE,
                                                            LTV_BUFFER_TYPE_SPECIFIC_CODEC_CONFIG,
                                                            &ltvStructureLen,
                                                            &ltvStructureValue);

        if(status == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS && ltvStructureValue)
        {
            *audioChannelAllocation = CSR_GET_UINT32_FROM_LITTLE_ENDIAN(ltvStructureValue);
            AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Audio Channel Allocation LTV structure found: 0x%08x\n", (*audioChannelAllocation));
            return status;
        }

        if(status != AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND)
            return status;

        codecSpecificConfigPtr += (ltvStructureLen + 1);
        remainingCodecConfigDataLen -= (ltvStructureLen + 1);
    }

    return status;
}

/******************************************************************************/

AudioAnnouncementParserStatus AudioAnnouncementParserGetOctetsPerCodecFrameFromCodecCnfg(
                                                                  uint8 codecSpecificConfigLen,
                                                                  uint8 *codecSpecificConfig,
                                                                  uint16 *octetsPerCodecframe)
{
    AudioAnnouncementParserStatus status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND;
    uint8 remainingCodecConfigDataLen = codecSpecificConfigLen;
    uint8 *codecSpecificConfigPtr = codecSpecificConfig;

    if(!codecSpecificConfig || !octetsPerCodecframe)
    {
        AUDIO_ANNOUNCEMENT_PARSER_ERROR("(AUDIO ANNOUNCEMENT PARSER) : One or more invalid paramters!\n");
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_PARAMETER;
    }

    if(codecSpecificConfigLen < OCTETS_PER_CODEC_FRAME_LTV_STRUCTURE_LENGTH)
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_LENGTH;

    (*octetsPerCodecframe) = 0;

    while (remainingCodecConfigDataLen)
    {
        uint8 ltvStructureLen = 0;
        uint8 *ltvStructureValue;

        status = audioAnnouncementParserLtvStructureParsing(codecSpecificConfigPtr,
                                                            OCTETS_PER_CODEC_FRAME_LTV_STRUCTURE_TYPE,
                                                            LTV_BUFFER_TYPE_SPECIFIC_CODEC_CONFIG,
                                                            &ltvStructureLen,
                                                            &ltvStructureValue);

        if(status == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS && ltvStructureValue)
        {
            *octetsPerCodecframe = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(ltvStructureValue);
            AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Octets Per Codec Frame LTV structure found: 0x%04x\n", (*octetsPerCodecframe));
            return status;
        }

        if(status != AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND)
            return status;

        codecSpecificConfigPtr += (ltvStructureLen + 1);
        remainingCodecConfigDataLen -= (ltvStructureLen + 1);
    }

    return status;
}

/******************************************************************************/

AudioAnnouncementParserStatus AudioAnnouncementParserGetCodecFrameBlocksPerSduFromCodecCnfg(
                                                                     uint8 codecSpecificConfigLen,
                                                                     uint8 *codecSpecificConfig,
                                                                     uint8 *codecFrameBlocksPerSdu)
{
    AudioAnnouncementParserStatus status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND;
    uint8 remainingCodecConfigDataLen = codecSpecificConfigLen;
    uint8 *codecSpecificConfigPtr = codecSpecificConfig;

    if(!codecSpecificConfig || !codecFrameBlocksPerSdu)
    {
        AUDIO_ANNOUNCEMENT_PARSER_ERROR("(AUDIO ANNOUNCEMENT PARSER) : One or more invalid paramters!\n");
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_PARAMETER;
    }

    if(codecSpecificConfigLen < CODEC_FRAME_BLOCKS_PER_SDU_LTV_STRUCTURE_LENGTH)
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_LENGTH;

    (*codecFrameBlocksPerSdu) = 0;

    while (remainingCodecConfigDataLen)
    {
        uint8 ltvStructureLen = 0;
        uint8 *ltvStructureValue;

        status = audioAnnouncementParserLtvStructureParsing(codecSpecificConfigPtr,
                                                            CODEC_FRAME_BLOCKS_PER_SDU_LTV_STRUCTURE_TYPE,
                                                            LTV_BUFFER_TYPE_SPECIFIC_CODEC_CONFIG,
                                                            &ltvStructureLen,
                                                            &ltvStructureValue);

        if(status == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS && ltvStructureValue)
        {
            *codecFrameBlocksPerSdu = *ltvStructureValue;
            AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Codec Frame Blocks Per Sdu LTV structure found: 0x%02x\n", (*codecFrameBlocksPerSdu));
            return status;
        }

        if(status != AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND)
            return status;

        codecSpecificConfigPtr += (ltvStructureLen + 1);
        remainingCodecConfigDataLen -= (ltvStructureLen + 1);
    }

    return status;
}

/******************************************************************************/

AudioAnnouncementParserStatus AudioAnnouncementParserGetPrefAudioContextFromMetadata(
                                                                 uint8 metadataLen,
                                                                 uint8 *metadata,
                                                                 AudioAnnouncementParserAudioContextType *preferredAudioContext)
{
    AudioAnnouncementParserStatus status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND;
    uint8 remainingMetadataLen = metadataLen;
    uint8 *metadataPtr = metadata;

    if(!metadata || !preferredAudioContext)
    {
        AUDIO_ANNOUNCEMENT_PARSER_ERROR("(AUDIO ANNOUNCEMENT PARSER) : One or more invalid paramters!\n");
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_PARAMETER;
    }

    if(metadataLen < PREFERRED_AUDIO_CONTEXT_LTV_STRUCTURE_LENGTH)
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_LENGTH;

    (*preferredAudioContext) = 0;

    while (remainingMetadataLen)
    {
        uint8 ltvStructureLen = 0;
        uint8 *ltvStructureValue;

        status = audioAnnouncementParserLtvStructureParsing(metadataPtr,
                                                            PREFERRED_AUDIO_CONTEXTS_LTV_STRUCTURE_TYPE,
                                                            LTV_BUFFER_TYPE_METADATA,
                                                            &ltvStructureLen,
                                                            &ltvStructureValue);

        if(status == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS && ltvStructureValue)
        {
            *preferredAudioContext = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(ltvStructureValue);
            AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Preferred Audio Context LTV structure found: 0x%04x\n", (*preferredAudioContext));
            return status;
        }

        if(status != AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND)
            return status;

        metadataPtr += (ltvStructureLen + 1);
        remainingMetadataLen -= (ltvStructureLen + 1);
    }

    return status;
}

/******************************************************************************/

AudioAnnouncementParserStatus AudioAnnouncementParserGetStreamAudioContextFromMetadata(
                                                                 uint8 metadataLen,
                                                                 uint8 *metadata,
                                                                 AudioAnnouncementParserAudioContextType *streamingAudioContext)
{
    AudioAnnouncementParserStatus status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND;
    uint8 remainingMetadataLen = metadataLen;
    uint8 *metadataPtr = metadata;

    if(!metadata || !streamingAudioContext)
    {
        AUDIO_ANNOUNCEMENT_PARSER_ERROR("(AUDIO ANNOUNCEMENT PARSER) : One or more invalid paramters!\n");
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_PARAMETER;
    }

    if(metadataLen < STREAMING_AUDIO_CONTEXT_LTV_STRUCTURE_LENGTH)
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_LENGTH;

    (*streamingAudioContext) = 0;

    while (remainingMetadataLen)
    {
        uint8 ltvStructureLen = 0;
        uint8 *ltvStructureValue;

        status = audioAnnouncementParserLtvStructureParsing(metadataPtr,
                                                            STREAMING_AUDIO_CONTEXTS_LTV_STRUCTURE_TYPE,
                                                            LTV_BUFFER_TYPE_METADATA,
                                                            &ltvStructureLen,
                                                            &ltvStructureValue);

        if(status == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS && ltvStructureValue)
        {
            *streamingAudioContext = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(ltvStructureValue);
            AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Streaming Audio Context LTV structure found: 0x%04x\n", (*streamingAudioContext));
            return status;
        }

        if(status != AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND)
            return status;

        metadataPtr += (ltvStructureLen + 1);
        remainingMetadataLen -= (ltvStructureLen + 1);
    }

    return status;
}

/******************************************************************************/
uint8 *AudioAnnouncementParserGetProgramInfoFromMetadata(uint8 metadataLen,
                                                        uint8 *metadata,
                                                        uint8 *programInfoLen)
{
    AudioAnnouncementParserStatus status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND;
    uint8 remainingMetadataLen = metadataLen;
    uint8 *metadataPtr = metadata;
    uint8 *programInfo = NULL;

    if(!metadata || !programInfoLen)
    {
        AUDIO_ANNOUNCEMENT_PARSER_ERROR("(AUDIO ANNOUNCEMENT PARSER) : One or more invalid paramters!\n");
        return programInfo;
    }

    *programInfoLen = 0;

    while (remainingMetadataLen)
    {
        uint8 ltvStructureLen = 0;
        uint8 *ltvStructureValue;

        status = audioAnnouncementParserLtvStructureParsing(metadataPtr,
                                                            PROGRAM_INFO_LTV_STRUCTURE_TYPE,
                                                            LTV_BUFFER_TYPE_METADATA,
                                                            &ltvStructureLen,
                                                            &ltvStructureValue);

        if(status == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS && ltvStructureValue)
        {
            uint8 i, j = 0;

            AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Program Info LTV structure found!\n");

            (*programInfoLen) = ltvStructureLen - 1;
            j = (*programInfoLen) - 1;

            programInfo = CsrPmemAlloc((*programInfoLen) * sizeof(uint8));

            for(i=0; i<(*programInfoLen); i++,j--)
                programInfo[i] = ltvStructureValue[j];

            break;
        }

        if (!ltvStructureValue && status != AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND)
            break;

        metadataPtr += (ltvStructureLen + 1);
        remainingMetadataLen -= (ltvStructureLen + 1);
    }

    return programInfo;
}

/******************************************************************************/
char *AudioAnnouncementParserGetLanguageFromMetadata(uint8 metadataLen,
                                                     uint8 *metadata,
                                                     uint8 *languageLen)
{
    AudioAnnouncementParserStatus status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND;
    uint8 remainingMetadataLen = metadataLen;
    uint8 *metadataPtr = metadata;
    char *language = NULL;

    if(!metadata || !metadataLen || !languageLen)
        return language;

    *languageLen = 0;

    while (remainingMetadataLen)
    {
        uint8 ltvStructureLen = 0;
        uint8 *ltvStructureValue;

        status = audioAnnouncementParserLtvStructureParsing(metadataPtr,
                                                            LANGUAGE_LTV_STRUCTURE_TYPE,
                                                            LTV_BUFFER_TYPE_METADATA,
                                                            &ltvStructureLen,
                                                            &ltvStructureValue);

        if(status == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS && ltvStructureValue)
        {
            uint8 i, j = 0;

            AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Language LTV structure found!\n");

            (*languageLen) = ltvStructureLen - 1;
            j = (*languageLen) - 1;
            language = CsrPmemAlloc((*languageLen) * sizeof(uint8));

            for(i=0; i<(*languageLen); i++,j--)
                language[i] = ltvStructureValue[j];

            break;
        }

        if (!ltvStructureValue && status != AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND)
            break;

        metadataPtr += (ltvStructureLen + 1);
        remainingMetadataLen -= (ltvStructureLen + 1);
    }

    return language;
}

/******************************************************************************/

AudioAnnouncementParserStatus AudioAnnouncementParserGetAudioActiveStateFromMetadata(uint8 metadataLen,
                                                                                     uint8 *metadata,
                                                                                     AudioAnnouncementParserAudioActiveState *audioActiveState)
{
    AudioAnnouncementParserStatus status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND;
    uint8 remainingMetadataLen = metadataLen;
    uint8 *metadataPtr = metadata;

    if(!metadata || !audioActiveState)
    {
        AUDIO_ANNOUNCEMENT_PARSER_ERROR("(AUDIO ANNOUNCEMENT PARSER) : One or more invalid paramters!\n");
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_PARAMETER;
    }

    (*audioActiveState) = 0;

    if(metadataLen < AUDIO_ACTIVE_STATE_LTV_STRUCTURE_LENGTH)
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_LENGTH;

    while (remainingMetadataLen)
    {
        uint8 ltvStructureLen = 0;
        uint8 *ltvStructureValue;

        status = audioAnnouncementParserLtvStructureParsing(metadataPtr,
                                                            AUDIO_ACTIVE_STATE_LTV_STRUCTURE_TYPE,
                                                            LTV_BUFFER_TYPE_METADATA,
                                                            &ltvStructureLen,
                                                            &ltvStructureValue);

        if(status == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS && ltvStructureValue)
        {
            *audioActiveState = *ltvStructureValue;
            AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Audio Active State LTV structure found: 0x%04x\n", (*ltvStructureValue));
            return status;
        }

        if(status != AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND)
            return status;

        metadataPtr += (ltvStructureLen + 1);
        remainingMetadataLen -= (ltvStructureLen + 1);
    }

    return status;
}

/******************************************************************************/

AudioAnnouncementParserStatus AudioAnnouncementParserPublicBcastAudioAnnouncementParsing(
                                                                    uint16 advDataLen,
                                                                    uint8 *advData,
                                                                    AudioAnnouncementParserPbpDataType *pbpData)
{
    uint8 adDataTypeLen = 0;
    uint8 *advDataPtr = NULL;
    uint16 remainingAdvDataLen = 0;

    if(!advData || !pbpData)
    {
        AUDIO_ANNOUNCEMENT_PARSER_ERROR("(AUDIO ANNOUNCEMENT PARSER) : One or more invalid paramters!\n");
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_PARAMETER;
    }

    if(advDataLen < PUBLIC_BROADCAST_AUDIO_ANNOUNCEMENT_ADVERT_LENGTH_MIN)
        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_LENGTH;

    CsrMemSet(pbpData, 0, sizeof(AudioAnnouncementParserPbpDataType));

    advDataPtr = advData;
    remainingAdvDataLen = advDataLen;

    while(remainingAdvDataLen)
    {
        adDataTypeLen = (*advDataPtr++);

        if(adDataTypeLen >= (PUBLIC_BROADCAST_AUDIO_ANNOUNCEMENT_ADVERT_LENGTH_MIN - 1) &&
           (*advDataPtr) == AD_TYPE_SERVICE_DATA_16_BIT_UUID)
        {
            uint8* serviceDataPtr = ++advDataPtr;
            uint16 serviceUuid = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(serviceDataPtr);

            serviceDataPtr += (PUBLIC_BROADCAST_AUDIO_ANNOUNCEMENT_UUID_LENGTH -1);

            if(serviceUuid == PUBLIC_BROADCAST_AUDIO_ANNOUNCEMENT_SERVICE_UUID)
            {
                uint8 minLen = PUBLIC_BROADCAST_AUDIO_ANNOUNCEMENT_ADVERT_LENGTH_MIN;

                AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : Public Broadcast Audio Announcement found!\n");

                serviceDataPtr++;
                pbpData->publicBroadcastAnnouncementFeatures = *serviceDataPtr;

                pbpData->encryption = (*serviceDataPtr) & PBP_FEATURES_ENCRYPTION;
                AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : PBP Features Encryption: %d\n", pbpData->encryption);

                pbpData->audioQuality = (*serviceDataPtr) & PBP_FEATURES_SQ ? AUDIO_ANNOUNCEMENT_PARSER_AUDIO_QUALITY_TYPE_STANDARD : 0;
                pbpData->audioQuality |= (*serviceDataPtr) & PBP_FEATURES_HQ ? AUDIO_ANNOUNCEMENT_PARSER_AUDIO_QUALITY_TYPE_HIGH : 0;
                AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : PBP Features Audio Quality: 0x%02x\n", pbpData->audioQuality);

                pbpData->metadataLen = *(++serviceDataPtr);
                AUDIO_ANNOUNCEMENT_PARSER_INFO("(AUDIO ANNOUNCEMENT PARSER) : PBP Features Metadata Length: %d\n", pbpData->metadataLen);

                if(pbpData->metadataLen)
                {
                    minLen += pbpData->metadataLen;

                    if(advDataLen < minLen)
                    {
                        CsrMemSet(pbpData, 0, sizeof(AudioAnnouncementParserPbpDataType));
                        return AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_LENGTH;
                    }

                    pbpData->metadata = CsrPmemAlloc(pbpData->metadataLen * sizeof(uint8));
                    CsrMemCpy(pbpData->metadata, ++serviceDataPtr, pbpData->metadataLen);
                }
                else
                {
                    pbpData->metadata = NULL;
                }

                return AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS;
            }
        }

        advDataPtr += adDataTypeLen;
        remainingAdvDataLen -= (adDataTypeLen + 1);
    }

    return AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND;
}

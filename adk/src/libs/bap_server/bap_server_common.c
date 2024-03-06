/****************************************************************************
* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
* 
************************************************************************* ***/

#include "bap_server_common.h"
#include "bap_server_debug.h"

bool bapServerIsValidConectionId(BAP *bapInst, ConnId connectionId)
{
    uint8 i;
    for (i = 0; i< BAP_SERVER_MAX_CONNECTIONS; i++)
    {
        if(bapInst->cid[i] == connectionId)
            return TRUE;
    }
    return FALSE;
}

bool BapServerLtvUtilitiesFindLtvOffset(uint8 * ltvData, uint8 ltvDataLength,
                                                    uint8 type, uint8 * offset)
{
    uint8 ltvIndex = 0;

    *offset = 0;

    if (ltvData == NULL || ltvDataLength == 0)
    {
        return FALSE;
    }

    while(ltvIndex < ltvDataLength && ltvData[ltvIndex + BAP_LTV_LENGTH_OFFSET])
    {
        uint8 length = ltvData[ltvIndex + BAP_LTV_LENGTH_OFFSET];

        if(ltvData[ltvIndex + BAP_LTV_TYPE_OFFSET] == type)
        {
            *offset = ltvIndex;
            return TRUE;
        }
        else
        {
            ltvIndex += (1 + length);
        }
    }

    return FALSE;
}

bool bapServerIsLtvValueInRange(uint8 * data,
                                uint8 dataLength,
                                uint8 valueSize,
                                uint8 ltvType,
                                uint32 ltvMinValue,
                                uint32 ltvMaxValue)
{
    uint8 value[4] = {0,0,0,0};

    if (valueSize > 4)
        return FALSE; /* This function will only handle values that fit into a uint32 (or smaller ) data type */

    if (BapServerLtvUtilitiesFindLtvValue(data,
                                          dataLength,
                                          ltvType,
                                          &value[0],
                                          valueSize))
    {
        uint32 ltvValue = 0;
        int i;
        for (i = 0; i < valueSize; i++)
        {
            ltvValue |= ((uint32)value[i] << (i*8));
        }
        if ((ltvValue >= ltvMinValue) && (ltvValue <= ltvMaxValue))
        {
            return TRUE;
        }
    }
    else
    {
        /* Either:
         *   1) We cannot find the required 'Type' in the LTV fields
         *   2) The length of the LTV doesn't match the expected length for this LTV Type
         *
         * Unfortunately, bapServerLtvUtilitiesFindLtvValue() doesn't distinguish between these two situations.
         * Unfortunate because we want to reject Codec Config or Metadata if the length of an LTV
         * is wrong, but we do not want to reject just because an optional LTV is absent.
         *
         * For now, accept the Code Config/Metadata if bapServerLtvUtilitiesFindLtvValue() returns FALSE.
         */
        return TRUE;
    }

    return FALSE;
}

bool BapServerLtvUtilitiesFindLtvValue(uint8 * ltvData,
                                       uint8 ltvDataLength,
                                              uint8 type,
                                              uint8 * value,
                                       uint8 valueLength)
{
    bool ltvFound = FALSE;
    if(ltvData && ltvDataLength && value)
    {
        int ltvIndex = 0;
        while(ltvIndex < ltvDataLength && ltvFound == FALSE && ltvData[ltvIndex + BAP_LTV_LENGTH_OFFSET])
        {
            uint8 length = ltvData[ltvIndex + BAP_LTV_LENGTH_OFFSET];
            BAP_DEBUG_INFO(("BapServerLtvUtilitiesFindLtvValue: index=%d length=%d type=%d",
                ltvIndex, ltvData[ltvIndex + BAP_LTV_LENGTH_OFFSET], ltvData[ltvIndex + BAP_LTV_TYPE_OFFSET]));

            if(ltvData[ltvIndex + BAP_LTV_TYPE_OFFSET] == type)
            {
                if(ltvData[ltvIndex + BAP_LTV_LENGTH_OFFSET] == (valueLength + 1))
                {
                    uint8 i;
                    for(i = 0; i < valueLength; i++)
                    {
                        value[i] = ltvData[ltvIndex + BAP_LTV_VALUE_OFFSET + i];
                    }
                    ltvFound = TRUE;
                }
                else
                {
                    BAP_DEBUG_INFO(("BapServerLtvUtilitiesFindLtvValue: Unexpected length"));
                    break;
                }
            }
            else
            {
                ltvIndex += (1 + length);
            }
        }
    }
    else
    {
        BAP_DEBUG_INFO(("BapServerLtvUtilitiesFindLtvValue: Invalid LTV data"));
    }
    BAP_DEBUG_INFO(("BapServerLtvUtilitiesFindLtvValue: ltv_found=%d", ltvFound));
    return ltvFound;
}

uint32 BapServerLtvUtilitiesGetSampleRate(uint8 * config, uint8 configLength)
{
    uint32 sampleRate = 0;
    uint8 ltvValue = 0;
    if(BapServerLtvUtilitiesFindLtvValue(config, configLength, BAP_CODEC_CONFIG_LTV_TYPE_SAMPLING_FREQUENCY, &ltvValue, 1))
    {

        switch(ltvValue)
        {
            case 0x01:
                sampleRate = 8000;
                break;
            case 0x02:
                sampleRate = 11025;
                break;
            case 0x03:
                sampleRate = 16000;
                break;
            case 0x04:
                sampleRate = 22050;
                 break;
            case 0x05:
                sampleRate = 24000;
                break;
            case 0x06:
                sampleRate = 32000;
                break;
            case 0x07:
                sampleRate = 44100;
                break;
            case 0x08:
                sampleRate = 48000;
                break;
            case 0x09:
                sampleRate = 88200;
                break;
            case 0x0A:
                sampleRate = 96000;
                break;
            case 0x0B:
                sampleRate = 176400;
                break;
            case 0x0C:
                sampleRate = 192000;
                break;
            case 0x0D:
                sampleRate = 384000;
                break;

            default:
                BAP_DEBUG_INFO(("bapServerLtvUtilitiesGetSampleRateFromLc3CodecConfig: LTV value %d not handled", ltvValue));
                break;
        }
    }
    else
    {
        BAP_DEBUG_INFO(("BapServerLtvUtilitiesGetSampleRate not found"));
    }

    return sampleRate;
}

uint16 BapServerLtvUtilitiesGetFrameDuration(uint8 * config, uint8 configLength)
{
    uint16 frameDuration = 0;
    uint8 ltvValue = 0;
    if(BapServerLtvUtilitiesFindLtvValue(config, configLength,
                    BAP_CODEC_CONFIG_LTV_TYPE_FRAME_DURATION, &ltvValue, 1))
    {
        switch(ltvValue)
        {
            case 0x00:
                frameDuration = 7500;
                break;
            case 0x01:
                frameDuration = 10000;
                break;
            default:
                break;
        }
    }
    else
    {
        BAP_DEBUG_INFO(("BapServerLtvUtilitiesGetFrameDuration not found"));
    }
    return frameDuration;
}

AudioContextType BapServerLtvUtilitiesGetStreamingAudioContext(uint8 * metadata,
                                                          uint8 metadataLength)
{
    AudioContextType audioContext = AUDIO_CONTEXT_TYPE_UNKNOWN;
    uint8 ltvValue[2] = { 0, 0 };
    if(BapServerLtvUtilitiesFindLtvValue(metadata, metadataLength,
                BAP_METADATA_LTV_TYPE_STREAMING_AUDIO_CONTEXTS, ltvValue, 2))
    {
        audioContext = (((ltvValue[1] << 8) & 0xFF00) | ltvValue[0]);
    }

    BAP_DEBUG_INFO(("BapServerLtvUtilitiesGetStreamingAudioContext: audio_context :%d", audioContext));

    return audioContext;
}

uint32 BapServerLtvUtilitiesGetAudioChannelAllocation(uint8 * config,
                                                       uint8 configLength)
{
    uint32 audioChannelAllocation = 0;
    uint8 ltvValue[BAP_CODEC_CONFIG_LTV_SIZE_AUDIO_CHANNEL_ALLOCATION] = {0};
    uint8 i = 0;

    if(BapServerLtvUtilitiesFindLtvValue(config, configLength,
              BAP_CODEC_CONFIG_LTV_TYPE_AUDIO_CHANNEL_ALLOCATION, ltvValue,
              BAP_CODEC_CONFIG_LTV_SIZE_AUDIO_CHANNEL_ALLOCATION))
    {
        for (i=0; i<BAP_CODEC_CONFIG_LTV_SIZE_AUDIO_CHANNEL_ALLOCATION; i++)
        {
                audioChannelAllocation |= (ltvValue[i] << (i*8));
        }
    }

    return audioChannelAllocation;
}

uint16 BapServerLtvUtilitiesGetOctetsPerCodecFrame(uint8 * config,
                                                    uint8 configLength)
{
    uint16 octetsPerSdu = 0;
    uint8 ltvValue[BAP_CODEC_CONFIG_LTV_SIZE_OCTETS_PER_CODEC_FRAME] = {0};
    uint8 i = 0;

    if(BapServerLtvUtilitiesFindLtvValue(config, configLength, BAP_CODEC_CONFIG_LTV_TYPE_OCTETS_PER_FRAME_CODEC,
        ltvValue, BAP_CODEC_CONFIG_LTV_SIZE_OCTETS_PER_CODEC_FRAME))
    {
        for (i=0; i<BAP_CODEC_CONFIG_LTV_SIZE_OCTETS_PER_CODEC_FRAME; i++)
        {
            octetsPerSdu |= (ltvValue[i] << (i*8));
        }
    }

    return octetsPerSdu;
}

uint8 BapServerLtvUtilitiesGetCodecFrameBlocksPerSdu(uint8 * config, uint8 configLength)
{
    uint8 codecFrameBlocksPerSdu = 1; /* Defaults to 1 blocks per SDU */
    uint8 ltvValue = 0;

    if(BapServerLtvUtilitiesFindLtvValue(config, configLength, BAP_CODEC_CONFIG_LTV_TYPE_CODEC_FRAME_BLOCKS_PER_SDU, &ltvValue, 1))
    {
        codecFrameBlocksPerSdu = ltvValue;
    }
    else
    {
        BAP_DEBUG_INFO(("BapServerLtvUtilitiesGetCodecFrameBlocksPerSdu: LTV type %d not found",
            BAP_CODEC_CONFIG_LTV_TYPE_CODEC_FRAME_BLOCKS_PER_SDU));
    }

    return codecFrameBlocksPerSdu;
}

static bapServerResponse bapServerValidateMetadataLtvTypeFields(uint8* ltvData, uint8 ltvDataLength, uint8* invalidType)
{
    LTV ltv;
    uint8* dataPtr = ltvData;

    /* Step over each LTV */
    while ((dataPtr - ltvData) < ltvDataLength)
    {
        LTV_INITIALISE(&ltv, dataPtr);

        switch (LTV_TYPE(&ltv))
        {
            case BAP_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS:
                /* Fall through */
            case BAP_METADATA_LTV_TYPE_STREAMING_AUDIO_CONTEXTS:
                /* Fall through */
            case BAP_METADATA_LTV_TYPE_CCID_LIST:
                /* Fall through */
            case BAP_METADATA_LTV_TYPE_EXTENDED_METADATA:
                /* Fall through */
            case BAP_METADATA_LTV_TYPE_VENDOR_SPECIFIC:
                break;
                /* Fall through */
            case BAP_METADATA_LTV_TYPE_PROGRAM_INFO:
                break;
                /* Fall through */
            case BAP_METADATA_LTV_TYPE_LANGUAGE:
                break;
                /* Fall through */
            case BAP_METADATA_LTV_TYPE_PROGRAM_INFO_URI:
                break;
                /* Fall through */
            case BAP_METADATA_LTV_TYPE_PARENTAL_RATING:
                break;
            default:
            {
                /* This 'Type' is unrecognised */
                *invalidType = LTV_TYPE(&ltv);
                return GATT_ASCS_ASE_RESULT_REJECTED_METADATA;
            }
        }
        dataPtr = LTV_NEXT(&ltv);
    }
    return GATT_ASCS_ASE_RESULT_SUCCESS;
}
bapServerResponse BapServerValidateMetadataLtvs(uint8* metadata,
                                                            uint8 metadataLength,
                                                            uint8* invalidMetadataType)
{
    bapServerResponse ltvValidationResult = bapServerValidateMetadataLtvTypeFields(metadata,
                                                                                       metadataLength,
                                                                                       invalidMetadataType);
    if (ltvValidationResult != GATT_ASCS_ASE_RESULT_SUCCESS)
    {
        /* invalidMetadataType is already set from within the bapServerValidateMetadataLtvTypeField function */
        return ltvValidationResult;
    }

    if (!bapServerIsLtvValueInRange(metadata,
                                    metadataLength,
                                    sizeof(uint16),
                                    BAP_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS,
                                    0x0001,  /* spec defined min valid value: BAPS Assigned Numbers v11 */
                                    0x0800)) /* spec defined max valid value: BAPS Assigned Numbers v11 */
    {
        *invalidMetadataType = BAP_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS;
        return GATT_ASCS_ASE_RESULT_INVALID_METADATA;
    }

    if (!bapServerIsLtvValueInRange(metadata,
                                    metadataLength,
                                    sizeof(uint16),
                                    BAP_METADATA_LTV_TYPE_STREAMING_AUDIO_CONTEXTS,
                                    0x0001,  /* spec defined min valid value: BAPS Assigned Numbers v11 */
                                    0x0800)) /* spec defined max valid value: BAPS Assigned Numbers v11 */
    {
        *invalidMetadataType = BAP_METADATA_LTV_TYPE_STREAMING_AUDIO_CONTEXTS;
        return GATT_ASCS_ASE_RESULT_INVALID_METADATA;
    }

    return GATT_ASCS_ASE_RESULT_SUCCESS;
}
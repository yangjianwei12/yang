/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "gatt_pacs_server_private.h"
#include "gatt_pacs_server_debug.h"
#include "gatt_pacs_server_utils.h"


static bool validateLTVData(const uint8* vsConfig,
                           PacRecordLtvType type,
                           PacRecordLtvType inRecordtype,
                           void* value)
{
    bool status = FALSE;
    uint8 index = 0;

    if(vsConfig == NULL)
        return status;

    if(inRecordtype != type)
        return status;

    switch (type)
    {
        case SAMPLING_FREQUENCY_TYPE:
        {
            PacsSamplingFrequencyType freq =
                (vsConfig[index] & 0x00ff) |
                     ((vsConfig[index + 1] << 8) & 0xff00);

            if (freq & (*(PacsSamplingFrequencyType*)value))
                status = TRUE;
            break;
        }
        case SUPPORTED_FRAME_DURATION_TYPE:
        {
            PacsFrameDurationType frameDur =
                       (vsConfig[index] & 0xff);

            if (frameDur & (*(PacsFrameDurationType*)value))
                status = TRUE;
            break;
        }
        case AUDIO_CHANNEL_COUNTS_TYPE:
        {
            PacsFrameDurationType channelCount =
                              (vsConfig[index] & 0xff);

            if (channelCount & (*(PacsAudioChannelCountType*)value))
                status = TRUE;
            break;
        }
        case MAX_SUPPORTED_CODEC_FRAMES_PER_SDU_TYPE:
        {
            PacsFrameDurationType maxCodecPerFrame =
                                 (vsConfig[index] & 0xff);

            if (maxCodecPerFrame == (*(PacsSupportedMaxCodecFramePerSdu*)value))
                status = TRUE;
            break;
        }
        case SUPPORTED_OCTETS_PER_CODEC_FRAME_TYPE:
        {

            uint32 octectPerCodecFrame = (*(uint32*)vsConfig);

            if ((uint32)octectPerCodecFrame == (*(uint32*)value))
                status = TRUE;
            break;
        }
        /* TODO: Needs to be extended for additional fields
         * when defined for vendor codec
         */
        default:
            GATT_PACS_SERVER_DEBUG("\n validateLTVData: INVALID type \n");
            break;
    }
    
    return status;
}

static bool isLtvDataValuePresent(const GattPacsServerRecordType* record,
                                  PacRecordLtvType type,
                                  void* value)
{

    PacsSamplingFrequencyType freq;
    PacsFrameDurationType frameDur;
    PacsAudioChannelCountType count;
    PacsSupportedMaxCodecFramePerSdu codecFramesPerSduMax;
    uint32 octetPerCodecFrame;

    if(value == NULL ||
            record == NULL)
        return FALSE;

    freq = record->supportedSamplingFrequencies;
    frameDur = record->supportedFrameDuration;
    count = record->audioChannelCounts;
    codecFramesPerSduMax = record->supportedMaxCodecFramePerSdu;
    octetPerCodecFrame =(record->minSupportedOctetsPerCodecFrame << 16) |
                      (record->maxSupportedOctetsPerCodecFrame & 0x0000FFFF);

    if(validateLTVData((uint8*)&freq,
                       type,
                       SAMPLING_FREQUENCY_TYPE,
                       value))
        return TRUE;

    if(validateLTVData((uint8*)&frameDur,
                       type,
                       SUPPORTED_FRAME_DURATION_TYPE,
                       value))
        return TRUE;

    if(validateLTVData((uint8*)&count,
                       type,
                       AUDIO_CHANNEL_COUNTS_TYPE,
                       value))
        return TRUE;

    if(validateLTVData((uint8*)&codecFramesPerSduMax,
                       type,
                       MAX_SUPPORTED_CODEC_FRAMES_PER_SDU_TYPE,
                       value))
        return TRUE;

    if(validateLTVData((uint8*)&octetPerCodecFrame,
                       type,
                       SUPPORTED_OCTETS_PER_CODEC_FRAME_TYPE,
                       value))
        return TRUE;

    return FALSE;
}


static bool isVsLtvDataValuePresent(const GattPacsServerVSPacRecord* record,
                                    PacRecordLtvType type,
                                    void* value)
{
    bool status = FALSE;
    uint8 totalConfigLen;
    uint8 configLen;
    uint8 configType;
    uint8 index = 0;

    if(value == NULL ||
            record == NULL)
        return status;

    totalConfigLen = record->vsConfigLen;

    while (totalConfigLen)
    {
        configLen = record->vsConfig[index++];
        configType = record->vsConfig[index++];

        if(validateLTVData(&record->vsConfig[index],
                           type,
                           configType,
                           value))
        {
            status = TRUE;
            break;
        }

        index += (configLen - 1);

        if (totalConfigLen > (configLen + 1))
            totalConfigLen -= (configLen + 1); /* ConfigLen + size of length needs to removed*/
        else
            totalConfigLen = 0;
    }
    return status;
}


uint16 generatePacRecordHandle(GPACSS_T *pacs_server)
{
    uint16 index;
    for (index = 1; index < MAX_PAC_RECORD_HANDLES; index++)
    {
        if (pacs_server->data.pacs_record_handle_mask == DEFAULT_PAC_RECORD_HANDLE_MASK)
        {
            pacs_server->data.pacs_record_handle_mask |= 1 << (index-1);
            return index;
        }
        else if ((pacs_server->data.pacs_record_handle_mask & (1 << (index-1))) != (1 << (index-1)))
        {
            pacs_server->data.pacs_record_handle_mask |= 1 << (index-1);
            return index;
        }
     }
    return (uint16) PACS_RECORD_HANDLES_EXHAUSTED;
}

void removePacRecordHandle(GPACSS_T *pacs_server, uint16 pac_record_handle)
{
    pacs_server->data.pacs_record_handle_mask &= ~(1UL << (pac_record_handle -1));
}

void* getPacRecordList(GPACSS_T *pacs, bool isAptx, bool isSink)
{
    return ((isAptx)? (void*)(isSink ? pacs->data.vs_aptx_sink_pac_record:pacs->data.vs_aptx_source_pac_record)
                  : (void*)(isSink? pacs->data.sink_pack_record: pacs->data.source_pack_record));
}

bool isPacRecordWithLtvPresent(void *list,
                        PacsCodecIdType codecId,
                        PacsVendorCodecIdType vendorCodecId,
                        PacRecordLtvType ltvType,
                        void *value)
{
    bool status = FALSE;

    if(codecId == PACS_LC3_CODEC_ID)
    {
        pac_record_list *tempList = list;

        while(tempList != NULL)
        {
            if(tempList->pac_record->codecId == codecId &&
                        isLtvDataValuePresent(tempList->pac_record, ltvType, value))
            {
                status = TRUE;
                break;
            }
            tempList = tempList->next;
        }
    }
    else if(codecId == PACS_VENDOR_CODEC_ID)
    {
        pac_record_list_vs* tempList = list;

        /* Only APTX adpative is supported*/
        if(vendorCodecId !=
                PACS_RECORD_VENDOR_CODEC_APTX_ADAPTIVE_R3)
            return FALSE;

        while (tempList != NULL)
        {
            if (tempList->pac_record->codecId == codecId
                    && tempList->pac_record->vendorCodecId == vendorCodecId
                        && isVsLtvDataValuePresent(tempList->pac_record, ltvType, value))
            {
                status = TRUE;
                break;
            }
            tempList = tempList->next;
        }
    }
    return status;
}

bool pacsServerGetDeviceIndexFromCid(GPACSS_T *pacs_server, ConnectionIdType cid, uint8 *index)
{
    uint8 i;

    for (i=0; i< GATT_PACS_MAX_CONNECTIONS; i++)
    {
        if (cid == pacs_server->data.connected_clients[i].cid)
        {
            *index = i;
            return TRUE;
        }
    }

    return FALSE;
}
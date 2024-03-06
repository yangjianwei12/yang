/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "cap_client_bap_pac_record.h"
#include "cap_client_util.h"
#include "cap_client_debug.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

static CapClientAudioLocationInfo sinkAudioLocations[CAP_CLIENT_MAX_SUPPORTED_DEVICES];
static CapClientAudioLocationInfo sourceAudioLocations[CAP_CLIENT_MAX_SUPPORTED_DEVICES];

static uint32 remoteContext;

#define CAP_CLIENT_INVALID_INDEX 0xFF
#define LC3_EPC_SUPPORTED_FEATURES_TYPE          0x10
#define LC3_EPC_SUPPORTED_FEATURES_LEN           0x0B

#define IS_10MS_SUPPORTED(_FRMDUR)  (!!(_FRMDUR & 0x02))

#define IS_7P5MS_SUPPORTED(_FRMDUR)  (!!(_FRMDUR & 0x01))

#define IS_10MS_PREFERRED(_FRMDUR)  (!!(_FRMDUR & 0x20))

#define IS_7P5MS_PREFERRED(_FRMDUR)  (!!(_FRMDUR & 0x10))

#define SET_BITS_FOR_10MS(CONFIG)  \
          (CAP_CLIENT_STREAM_CAPABILITY_8_2 | CAP_CLIENT_STREAM_CAPABILITY_16_2 | CAP_CLIENT_STREAM_CAPABILITY_24_2 \
           | CAP_CLIENT_STREAM_CAPABILITY_32_2 | CAP_CLIENT_STREAM_CAPABILITY_8_2 | CAP_CLIENT_STREAM_CAPABILITY_441_2 \
               | CAP_CLIENT_STREAM_CAPABILITY_48_2 | CAP_CLIENT_STREAM_CAPABILITY_48_4 | CAP_CLIENT_STREAM_CAPABILITY_48_6)

#define SET_BITS_FOR_7P5MS(CONFIG)  \
          (CAP_CLIENT_STREAM_CAPABILITY_8_1 | CAP_CLIENT_STREAM_CAPABILITY_16_1 | CAP_CLIENT_STREAM_CAPABILITY_24_1 \
           | CAP_CLIENT_STREAM_CAPABILITY_32_1 | CAP_CLIENT_STREAM_CAPABILITY_8_1 | CAP_CLIENT_STREAM_CAPABILITY_441_1 \
               | CAP_CLIENT_STREAM_CAPABILITY_48_1 | CAP_CLIENT_STREAM_CAPABILITY_48_3 | CAP_CLIENT_STREAM_CAPABILITY_48_5)


/*! VS BAP PAC Codec Specific Configurations  */
#define CAP_VS_PAC_SAMPLING_FREQUENCY_LENGTH                 0x03
#define CAP_VS_PAC_SAMPLING_FREQUENCY_TYPE                   0x01
#define CAP_VS_PAC_AUDIO_CHANNEL_COUNTS_LENGTH               0x02
#define CAP_VS_PAC_AUDIO_CHANNEL_COUNTS_TYPE                 0x03
#define CAP_VS_PAC_SUPPORTED_OCTETS_PER_CODEC_FRAME_LENGTH   0x05
#define CAP_VS_PAC_SUPPORTED_OCTETS_PER_CODEC_FRAME_TYPE     0x04

typedef struct
{
    CapClientSreamCapability localCap;
    uint16    minOctetsPerCodecFrame;
    uint16    maxOctetsPerCodecFrame;
    CapClientFrameDuration     frameDuaration;
    CapClientAudioChannelCount channelCount;
    uint8     supportedMaxCodecFramesPerSdu;
} CapClientPacRecord;

static uint8 parseVsMetadata(uint8_t* vsMetadata, uint8_t vsMetadataTotalLen)
{
    uint8_t offset = 0;
    uint16_t companyId = 0;
    uint8_t vsMetadataLen = 0;
    uint8_t vsMetadataType = 0;
    uint8_t versionNum = 0;

    if (vsMetadataTotalLen == 0)
    {
        return 0;
    }

    companyId = (uint8_t)(vsMetadata[offset] | (vsMetadata[offset + 1] << 8));
    offset += sizeof(companyId);
    vsMetadataTotalLen -= sizeof(companyId);

    /* Process vs metadta LTV structure*/

    while (vsMetadataTotalLen)
    {
        vsMetadataLen = vsMetadata[offset++];
        vsMetadataType = vsMetadata[offset];
        if (LC3_EPC_SUPPORTED_FEATURES_TYPE == vsMetadataType &&
                    LC3_EPC_SUPPORTED_FEATURES_LEN == vsMetadataLen)
        {
           /*  First Octet of LC3_EPC structure is LC3_EPC version Number
            */
            versionNum = vsMetadata[offset+1];

           /* Remaining octets 2...10 are RFU
            * TODO: Process remaining octets when
            * use cases are added in future
            */
        }
        else
        {
           /*  Next LTV
            *  Do Nothing
            *  TODO:Process Additional LTVs (optional) in future
            *
            */
        }

        offset += vsMetadataLen;

        if (vsMetadataTotalLen >= (vsMetadataLen + 1))
            vsMetadataTotalLen -= (vsMetadataLen + 1);
        else
            vsMetadataTotalLen = 0;
    }

    return versionNum;
}

static uint8 capClientGetAudioLocationFindCid(CapClientAudioLocationInfo* audioLoc, uint32 cid)
{
    uint8 i;
    for (i = 0; i < CAP_CLIENT_MAX_SUPPORTED_DEVICES; i++)
    {
        if (audioLoc[i].cid == cid)
            return i;
    }
    return CAP_CLIENT_INVALID_INDEX;
}

static uint8 capClientGetAudioLocationFreeIndex(CapClientAudioLocationInfo *audioLoc)
{
    uint8 i;
    for(i = 0; i < CAP_CLIENT_MAX_SUPPORTED_DEVICES; i++)
    {
        if(audioLoc[i].cid == 0)
            return i;
    }
    return CAP_CLIENT_INVALID_INDEX;
}

static uint8 capClientGetRecordCount(CapClientGroupInstance *cap, bool isSink, CapClientContext useCase)
{
    uint8 recordCount = 0;
    CapClientBapPacRecord *record = NULL;
    CsrCmnList_t *list = isSink ? cap->sinkRecordList : cap->sourceRecordList ;

    record = (CapClientBapPacRecord*)(list->first);

    while(record)
    {
        if((useCase == CAP_CLIENT_CONTEXT_TYPE_PROHIBITED)
            || (useCase & record->preferredAudioContext)
             || (record->preferredAudioContext & CAP_CLIENT_CONTEXT_TYPE_UNSPECIFIED))
        {
            recordCount++;
        }
        record = record->next;
    }

    /* 
     * The setting are same for both lc3Epc and standard pac records
     * If EB has some record for lc3Epc then the same record is valid 
     * for lc3Epc as well
     */

    return recordCount;
}

static void capClientCachePacsRecordList(CapClientGroupInstance *cap,
                               bool isSink,
                               bool lc3Epc,
                               uint8 versionNum,
                               CapClientPacRecord pacRecord,
                               uint16 preferredContext,
                               uint16 streamingContext,
                               uint8 metadataLen,
                               uint8 *metadata)
{
    CsrCmnList_t *list;
    list = isSink ? cap->sinkRecordList : cap->sourceRecordList;
    CapClientBapPacRecord *record = CAP_CLIENT_LIST_ADD_RECORD(list);
    uint8 count = list->count;

    record->isLc3Epc = lc3Epc;
    record->preferredAudioContext = preferredContext;
    record->streamingAudioContext = streamingContext;
    record->recordNum = count;
    record->channelCount = pacRecord.channelCount;
    record->streamCapability = pacRecord.localCap;
    record->supportedMaxCodecFramesPerSdu = pacRecord.supportedMaxCodecFramesPerSdu;
    record->frameDuaration = pacRecord.frameDuaration;
    record->minOctetsPerCodecFrame = pacRecord.minOctetsPerCodecFrame;
    record->maxOctetsPerCodecFrame = pacRecord.maxOctetsPerCodecFrame;
    record->lc3EpcVersionNum = versionNum;
    record->metadataLen = 0;
    record->metadata = NULL;

    if (metadataLen && metadata)
    {
        record->metadataLen = metadataLen;
        record->metadata = (uint8*)CsrPmemZalloc(metadataLen);
        SynMemCpyS(record->metadata, metadataLen, metadata, metadataLen);
    }

    CAP_CLIENT_INFO("capClientCachePacsRecordList localCap =%x, channelCount : %x\n", pacRecord.localCap, pacRecord.channelCount);
}

static CapClientSreamCapability capClientBapRecordFrameDuration(uint8 frameDuration,
                                             CapClientSreamCapability  streamCapability)
{
    if(IS_10MS_SUPPORTED(frameDuration))
    {
        streamCapability = SET_BITS_FOR_10MS(streamCapability);
    }

    if(IS_7P5MS_SUPPORTED(frameDuration))
    {
        streamCapability = streamCapability | SET_BITS_FOR_7P5MS(streamCapability);
    }

    return streamCapability;
}

static CapClientSreamCapability capClientBapRecordSamplingFrequencySupported(uint16 samplingFreq,
                                            CapClientSreamCapability  streamCapability)
{
    uint16 unsupported = (~samplingFreq & 0x0000FFFF);

    if(unsupported & BAP_SAMPLING_FREQUENCY_8kHz)
    {
        streamCapability &= ~(CAP_CLIENT_STREAM_CAPABILITY_8_1 | CAP_CLIENT_STREAM_CAPABILITY_8_2);
    }

    if(unsupported & BAP_SAMPLING_FREQUENCY_16kHz)
    {
        streamCapability &= ~ (CAP_CLIENT_STREAM_CAPABILITY_16_1 | CAP_CLIENT_STREAM_CAPABILITY_16_2);
    }

    if(unsupported & BAP_SAMPLING_FREQUENCY_24kHz)
    {
        streamCapability &= ~(CAP_CLIENT_STREAM_CAPABILITY_24_1 | CAP_CLIENT_STREAM_CAPABILITY_24_2);
    }

    if(unsupported & BAP_SAMPLING_FREQUENCY_32kHz)
    {
        streamCapability &= ~(CAP_CLIENT_STREAM_CAPABILITY_32_1 | CAP_CLIENT_STREAM_CAPABILITY_32_2);
    }

    if(unsupported & BAP_SAMPLING_FREQUENCY_44_1kHz)
    {
        streamCapability &= ~(CAP_CLIENT_STREAM_CAPABILITY_441_1 | CAP_CLIENT_STREAM_CAPABILITY_441_2);
    }

    if(unsupported & BAP_SAMPLING_FREQUENCY_48kHz)
    {
        streamCapability &= ~((CAP_CLIENT_STREAM_CAPABILITY_48_1) |
                               (CAP_CLIENT_STREAM_CAPABILITY_48_2) |
                                 (CAP_CLIENT_STREAM_CAPABILITY_48_3)|
                                   (CAP_CLIENT_STREAM_CAPABILITY_48_4)|
                                     (CAP_CLIENT_STREAM_CAPABILITY_48_5)|
                                       (CAP_CLIENT_STREAM_CAPABILITY_48_6));
    }

    return streamCapability;
}

static CapClientSreamCapability capClientSetBitsForMinOctets(CapClientSreamCapability config,
                                                      uint16 minOctets)
{
    /* NOTE: Minimum octets will always be lesser than or equal to
     *  40, since 40 octets per frame is a mandatory setting
     *  */

    if(minOctets > CAP_CLIENT_STREAM_CAPABILITY_OCTETS_26)
    {
        config = (config & ~CAP_CLIENT_STREAM_CAPABILITY_8_1);
    }

    if(minOctets > CAP_CLIENT_STREAM_CAPABILITY_OCTETS_30)
    {
        config = (config & ~(CAP_CLIENT_STREAM_CAPABILITY_8_2 | CAP_CLIENT_STREAM_CAPABILITY_16_1));

    }

    return config;
}

static CapClientSreamCapability capClientSetBitsForMaxOctets(CapClientSreamCapability config,
                                                      uint16 maxOctets)
{
    /* NOTE: Maximum octets will always be Greater than or equal to
     * 60, since 60 octets per frame is a mandatory setting
     *  */

    if(maxOctets < CAP_CLIENT_STREAM_CAPABILITY_OCTETS_155)
    {
        config = (config & ~ (CAP_CLIENT_STREAM_CAPABILITY_48_6));
    }

    if(maxOctets < CAP_CLIENT_STREAM_CAPABILITY_OCTETS_130)
    {
        config = (config & ~ (CAP_CLIENT_STREAM_CAPABILITY_441_2 ));
    }

    if(maxOctets < CAP_CLIENT_STREAM_CAPABILITY_OCTETS_120)
    {
        config = (config & ~ (CAP_CLIENT_STREAM_CAPABILITY_48_4));
    }

    if(maxOctets < CAP_CLIENT_STREAM_CAPABILITY_OCTETS_117)
    {
        config = (config & ~ (CAP_CLIENT_STREAM_CAPABILITY_48_5));
    }

    if(maxOctets < CAP_CLIENT_STREAM_CAPABILITY_OCTETS_100)
    {
        config = (config & ~ (CAP_CLIENT_STREAM_CAPABILITY_48_2));
    }

    if(maxOctets < CAP_CLIENT_STREAM_CAPABILITY_OCTETS_97)
    {
        config = (config & ~ (CAP_CLIENT_STREAM_CAPABILITY_441_1));
    }

    if(maxOctets < CAP_CLIENT_STREAM_CAPABILITY_OCTETS_90)
    {
        config = (config & ~ (CAP_CLIENT_STREAM_CAPABILITY_48_3));
    }

    if(maxOctets < CAP_CLIENT_STREAM_CAPABILITY_OCTETS_80)
    {
        config = (config & ~ (CAP_CLIENT_STREAM_CAPABILITY_32_2));
    }

    if(maxOctets < CAP_CLIENT_STREAM_CAPABILITY_OCTETS_75)
    {
        config = (config & ~ (CAP_CLIENT_STREAM_CAPABILITY_48_1));
    }

    return config;
}

static CapClientSreamCapability capClientBapOctetsPerCodecFrame(uint16 minOctets,
                                                      uint16 maxOctets,
                                                      CapClientSreamCapability  streamCapability)
{
    streamCapability = capClientSetBitsForMinOctets(streamCapability, minOctets);
    streamCapability = capClientSetBitsForMaxOctets(streamCapability, maxOctets);

    return streamCapability;
}

static CapClientSreamCapability capClientInterpretLc3PacRecord(BapPacRecord* record, 
                                                            bool *lc3Epc,
                                                            uint8 *versionNum)
{
    CapClientSreamCapability localCap = 0x00000000;

    if (record && lc3Epc && versionNum)
    {
        /* In first function based on Frame duration we set all bits in 'localCap'. 
         * Based on unsupported frequencies and sdu size, the corresponding bits are unset in
         * in functions capClientBapRecordSamplingFrequencySupported and capClientBapOctetsPerCodecFrame
         * respectively. */

        localCap = capClientBapRecordFrameDuration(record->frameDuaration, localCap);

        localCap = capClientBapRecordSamplingFrequencySupported(record->samplingFrequencies, localCap);

        localCap = capClientBapOctetsPerCodecFrame(record->minOctetsPerCodecFrame,
                                                   record->maxOctetsPerCodecFrame,
                                                   localCap);

        *versionNum = parseVsMetadata(record->vendorSpecificMetadata,
                                   record->vendorSpecificMetadataLen);

        *lc3Epc = (*versionNum != 0);
    }

    return localCap;
}
    

static CapClientSreamCapability capClientInterpretAptxLitePacRecord(BapPacRecord* record,
    uint8 *channelCount, uint16 *minOctetsPerCodecFrame, uint16 *maxOctetsPerCodecFrame)
{
    CapClientSreamCapability localCap = 0x00000000;
    uint16    samplingFrequencies = BAP_SAMPLING_FREQUENCY_RFU;

    if (record)
    {
        uint8 value[CAP_VS_PAC_SAMPLING_FREQUENCY_LENGTH - 1];
        uint8 chanCount[CAP_VS_PAC_AUDIO_CHANNEL_COUNTS_LENGTH - 1];
        uint8 supportedOctets[CAP_VS_PAC_SUPPORTED_OCTETS_PER_CODEC_FRAME_LENGTH - 1];

        /* checking for sampling frequency in vendorSpecificConfig */
        if (capClientUtilsFindLtvValue(record->vendorSpecificConfig,
                                       record->vendorSpecificConfigLen,
                                       CAP_VS_PAC_SAMPLING_FREQUENCY_TYPE,
                                       &value[0],
                                      (CAP_VS_PAC_SAMPLING_FREQUENCY_LENGTH - 1)))
        {
            samplingFrequencies = value[0]| (value[1] << 8);
        }

        if ((samplingFrequencies & CAP_CLIENT_SAMPLING_FREQUENCY_48kHz)
            == CAP_CLIENT_SAMPLING_FREQUENCY_48kHz)
        {
            localCap |= CAP_CLIENT_STREAM_CAPABILITY_APTX_LITE_48_1;
        }

        if ((samplingFrequencies & CAP_CLIENT_SAMPLING_FREQUENCY_16kHz)
            == CAP_CLIENT_SAMPLING_FREQUENCY_16kHz)
        {
            localCap |= CAP_CLIENT_STREAM_CAPABILITY_APTX_LITE_16_1;
        }

        /* checking for channelCount in vendorSpecificConfig */
        if (capClientUtilsFindLtvValue(record->vendorSpecificConfig,
                                       record->vendorSpecificConfigLen,
                                       CAP_VS_PAC_AUDIO_CHANNEL_COUNTS_TYPE,
                                       &chanCount[0],
                                      (CAP_VS_PAC_AUDIO_CHANNEL_COUNTS_LENGTH - 1)))
        {
            *channelCount = chanCount[0];
        }

        /* checking for supported octets per code frame in vendorSpecificConfig */
        if (capClientUtilsFindLtvValue(record->vendorSpecificConfig,
                                       record->vendorSpecificConfigLen,
                                       CAP_VS_PAC_SUPPORTED_OCTETS_PER_CODEC_FRAME_TYPE,
                                       &supportedOctets[0],
                                       (CAP_VS_PAC_SUPPORTED_OCTETS_PER_CODEC_FRAME_LENGTH- 1)))
        {
            *minOctetsPerCodecFrame = supportedOctets[0] | (supportedOctets[1] << 8);
            *maxOctetsPerCodecFrame = supportedOctets[2] | (supportedOctets[3] << 8);
        }

    }

    return localCap;
}

static CapClientSreamCapability capClientInterpretAptxAdaptivePacRecord(BapPacRecord* record,
    uint8 *channelCount, uint16 *minOctetsPerCodecFrame, uint16 *maxOctetsPerCodecFrame)
{
    CapClientSreamCapability localCap = CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN;
    uint16    samplingFrequencies = BAP_SAMPLING_FREQUENCY_RFU;

    if (record)
    {
        uint8 value[CAP_VS_PAC_SAMPLING_FREQUENCY_LENGTH - 1];
        uint8 chanCount[CAP_VS_PAC_AUDIO_CHANNEL_COUNTS_LENGTH - 1];
        uint8 supportedOctets[CAP_VS_PAC_SUPPORTED_OCTETS_PER_CODEC_FRAME_LENGTH - 1];

        /* checking for sampling frequencies in vendorSpecificConfig */
        if (capClientUtilsFindLtvValue(record->vendorSpecificConfig,
                                       record->vendorSpecificConfigLen,
                                       CAP_VS_PAC_SAMPLING_FREQUENCY_TYPE,
                                       &value[0],
                                      (CAP_VS_PAC_SAMPLING_FREQUENCY_LENGTH - 1)))
        {
            samplingFrequencies = value[0]| (value[1] << 8);
        }

        if ((samplingFrequencies & CAP_CLIENT_SAMPLING_FREQUENCY_48kHz)
            == CAP_CLIENT_SAMPLING_FREQUENCY_48kHz)
        {
            localCap |= CAP_CLIENT_STREAM_CAPABILITY_APTX_ADAPTIVE_48_1;
        }

        if ((samplingFrequencies & CAP_CLIENT_SAMPLING_FREQUENCY_96kHz)
            == CAP_CLIENT_SAMPLING_FREQUENCY_96kHz)
        {
            localCap |= CAP_CLIENT_STREAM_CAPABILITY_APTX_ADAPTIVE_96;
        }

        /* checking for channelCount in vendorSpecificConfig */
        if (capClientUtilsFindLtvValue(record->vendorSpecificConfig,
                                       record->vendorSpecificConfigLen,
                                       CAP_VS_PAC_AUDIO_CHANNEL_COUNTS_TYPE,
                                       &chanCount[0],
                                      (CAP_VS_PAC_AUDIO_CHANNEL_COUNTS_LENGTH - 1)))
        {
            *channelCount = chanCount[0];
        }

        /* checking for supported octets per code frame in vendorSpecificConfig */
        if (capClientUtilsFindLtvValue(record->vendorSpecificConfig,
                                       record->vendorSpecificConfigLen,
                                       CAP_VS_PAC_SUPPORTED_OCTETS_PER_CODEC_FRAME_TYPE,
                                       &supportedOctets[0],
                                       (CAP_VS_PAC_SUPPORTED_OCTETS_PER_CODEC_FRAME_LENGTH- 1)))
        {
            *minOctetsPerCodecFrame = supportedOctets[0] | (supportedOctets[1] << 8);
            *maxOctetsPerCodecFrame = supportedOctets[2] | (supportedOctets[3] << 8);
        }

    }

    return localCap;
}


void capClientInterpretPacRecord(CapClientGroupInstance *cap,
    BapPacRecord **record, uint8 numOfRecords, uint8 recordtype)
{
    uint8 index, versionNum = 0;
    bool isSink = (BAP_AUDIO_SINK_RECORD == recordtype);
    bool lc3Epc = FALSE;
    uint8 channelCount = 0;
    uint16 minOctetsPerCodecFrame = 0;
    uint16 maxOctetsPerCodecFrame = 0;
    CapClientPacRecord pacRecord;

    for(index = 0; index < numOfRecords; index++)
    {
        memset(&pacRecord, 0, sizeof(CapClientPacRecord));

        if (record[index]->codecId == BAP_CODEC_ID_LC3)
        {
            pacRecord.localCap = capClientInterpretLc3PacRecord(record[index], &lc3Epc, &versionNum);
            pacRecord.channelCount = record[index]->channelCount;
            pacRecord.frameDuaration = record[index]->frameDuaration;
            pacRecord.minOctetsPerCodecFrame = record[index]->minOctetsPerCodecFrame;
            pacRecord.maxOctetsPerCodecFrame = record[index]->maxOctetsPerCodecFrame;
            pacRecord.supportedMaxCodecFramesPerSdu = record[index]->supportedMaxCodecFramesPerSdu;
        }
        else if (record[index]->codecId == BAP_CODEC_ID_VENDOR_DEFINED)
        {
            /* Not Qualcomm Specific Pac record then Do not Interpret*/
            if (record[index]->companyId != BAP_COMPANY_ID_QUALCOMM)
                return;

            if (record[index]->vendorCodecId == BAP_VS_CODEC_ID_APTX_LITE)
            {
                /* 
                   Right now only two frequencies are supported for Aptx Lite.
                   48Khz for Decoder, 16 Khz for encoder.
                */

                pacRecord.localCap = capClientInterpretAptxLitePacRecord(record[index],
                                                               &channelCount,
                                                               &minOctetsPerCodecFrame,
                                                               &maxOctetsPerCodecFrame);
                pacRecord.channelCount = channelCount;
                pacRecord.minOctetsPerCodecFrame = minOctetsPerCodecFrame;
                pacRecord.maxOctetsPerCodecFrame = maxOctetsPerCodecFrame;

            }
            else if (record[index]->vendorCodecId == BAP_VS_CODEC_ID_APTX_ADAPTIVE)
            {
                /* Interpreter for APTX adaptive */
                pacRecord.localCap = capClientInterpretAptxAdaptivePacRecord(record[index],
                                                                   &channelCount,
                                                         &minOctetsPerCodecFrame,
                                                         &maxOctetsPerCodecFrame);
                pacRecord.channelCount = channelCount;
                pacRecord.minOctetsPerCodecFrame = minOctetsPerCodecFrame;
                pacRecord.maxOctetsPerCodecFrame = maxOctetsPerCodecFrame;
            }
        }
        else
        {
            return;
        }

        capClientCachePacsRecordList(cap,
                           isSink,
                           lc3Epc,
                           versionNum,
                           pacRecord,
                           record[index]->preferredAudioContext,
                           record[index]->streamingAudioContext,
                           record[index]->vendorSpecificMetadataLen,
                           record[index]->vendorSpecificMetadata);
    }
}



void capClientFlushPacRecordsFromList(CapClientGroupInstance *cap, uint8 recordtype)
{
    uint8 i;
    CsrCmnList_t* list = (recordtype == BAP_AUDIO_SINK_RECORD) ? cap->sinkRecordList : cap->sourceRecordList;
    CsrCmnListElm_t* elem = list->first;
    uint8 count = list->count;

    for (i = 0; i <= count && elem; i++)
    {
        CsrCmnListElm_t* temp = elem->next;
        CAP_CLIENT_LIST_REMOVE_RECORD(list, elem);
        elem = temp;
    }
}


bool capClientIsRecordPresent(CapClientGroupInstance *cap, bool isSink)
{
    bool result = isSink ? (cap->sinkRecordList->count > 0):(cap->sourceRecordList->count > 0);
    return result;
}


CapClientLocalStreamCapabilities* capClientGetRemotePacRecord(CapClientGroupInstance *cap,
uint8 *count, bool isSink, CapClientContext useCase)
{
    uint8 index;
    CapClientLocalStreamCapabilities *result = NULL;
    CsrCmnList_t *list = isSink ? cap->sinkRecordList : cap->sourceRecordList;
    CapClientBapPacRecord *record = (CapClientBapPacRecord*)(list->first);

    *count = capClientGetRecordCount(cap, isSink, useCase);

    /* Populate results only if count is not zero */
    if (*count)
    {
        result = (CapClientLocalStreamCapabilities*)CsrPmemZalloc(*count * sizeof(CapClientLocalStreamCapabilities));

        for (index = 0; index < *count && record; record = record->next)
        {
            if ((useCase == CAP_CLIENT_CONTEXT_TYPE_PROHIBITED)
                || (useCase & record->preferredAudioContext)
                  || (record->preferredAudioContext & CAP_CLIENT_CONTEXT_TYPE_UNSPECIFIED))
            {
                result[index].capability = record->streamCapability;
                result[index].capability = (result[index].capability) |
                    (record->isLc3Epc ? CAP_CLIENT_STREAM_CAPABILITY_LC3_EPC: 0);
                result[index].context = record->preferredAudioContext;
                result[index].codecVersionNum = record->lc3EpcVersionNum;
                result[index].channelCount = record->channelCount;
                result[index].frameDuaration = record->frameDuaration;
                result[index].minOctetsPerCodecFrame = record->minOctetsPerCodecFrame;
                result[index].maxOctetsPerCodecFrame = record->maxOctetsPerCodecFrame;
                result[index].supportedMaxCodecFramesPerSdu = record->supportedMaxCodecFramesPerSdu;
                result[index].metadataLen = 0;
                result[index].metadata = NULL;

                if (record->metadataLen && record->metadata)
                {
                    result[index].metadataLen = record->metadataLen;
                    result[index].metadata = (uint8*)CsrPmemZalloc(record->metadataLen);
                    SynMemCpyS(result[index].metadata, record->metadataLen, 
                                   record->metadata, record->metadataLen);
                }
                index++;
            }
        }
    }

    return result;
}

uint8 capClientGetChannelCountForContext(CapClientGroupInstance *cap, CapClientContext useCase, bool sink)
{
    uint8 recordCount = 0;
    CsrCmnList_t *list = sink ? cap->sinkRecordList : cap->sourceRecordList;
    CapClientBapPacRecord *record = (CapClientBapPacRecord*)list->first;
    useCase = capClientMapCapContextWithCap(useCase);

    while(record)
    {
        if (((record->preferredAudioContext & useCase) == useCase)
            || (record->preferredAudioContext & CAP_CLIENT_CONTEXT_TYPE_UNSPECIFIED) == CAP_CLIENT_CONTEXT_TYPE_UNSPECIFIED)
        {
            recordCount = record->channelCount;
            break;
        }
        record = record->next;
    }

    CAP_CLIENT_INFO("capClientGetChannelCountForContext result : %x\n", recordCount);

    return recordCount;
}

CapClientBool capClientIsChannelCountSupported(uint8 channelCount, uint8 supportedCount)
{

    if ((channelCount & supportedCount) == supportedCount)
    {
        return TRUE;
    }

    return FALSE;
}

void capClientCacheRemoteAudioLocations(uint32 location, uint8 recordType, uint32 cid)
{
    CSR_UNUSED(location);
    CSR_UNUSED(cid);
    uint8 index;
    bool isSink = (BAP_AUDIO_SINK_RECORD == recordType);

    if(isSink)
    {
        index = capClientGetAudioLocationFindCid(sinkAudioLocations, cid);

        if(index != CAP_CLIENT_INVALID_INDEX)
        {
            sinkAudioLocations[index].location = location;
        }
        else
        {
            index = capClientGetAudioLocationFreeIndex(sinkAudioLocations);

            if(index != CAP_CLIENT_INVALID_INDEX)
            {
                sinkAudioLocations[index].cid = cid;
                sinkAudioLocations[index].location = location;
            }
        }
    }
    else
    {
        index = capClientGetAudioLocationFindCid(sourceAudioLocations, cid);
        if(index != CAP_CLIENT_INVALID_INDEX)
        {
            sourceAudioLocations[index].location = location;
        }
        else
        {
            index = capClientGetAudioLocationFreeIndex(sourceAudioLocations);
            if(index != CAP_CLIENT_INVALID_INDEX)
            {
                sourceAudioLocations[index].cid = cid;
                sourceAudioLocations[index].location = location;
            }
        }
    }

}

CapClientAudioLocationInfo* capClientGetRemoteAudioLocationsInfo(uint8 *count, bool isSink)
{
    uint8 index;
    CapClientAudioLocationInfo *audioLocation = (CapClientAudioLocationInfo*)
                    CsrPmemZalloc(CAP_CLIENT_MAX_SUPPORTED_DEVICES*sizeof(CapClientAudioLocationInfo));

    if(isSink)
    {
        index = capClientGetAudioLocationFreeIndex(sinkAudioLocations);

        if(index == 0xFF)
        {
            index = CAP_CLIENT_MAX_SUPPORTED_DEVICES;
        }

        *count = index;

        if(*count)
        {
            CsrMemCpy(audioLocation, &sinkAudioLocations[0], index*sizeof(CapClientAudioLocationInfo));
        }
    }
    else
    {
        index = capClientGetAudioLocationFreeIndex(sourceAudioLocations);

        if(index == 0xFF)
        {
            index = CAP_CLIENT_MAX_SUPPORTED_DEVICES;
        }

        *count = index;

        if(*count)
        {
             CsrMemCpy(audioLocation, &sourceAudioLocations[0], index*sizeof(CapClientAudioLocationInfo));
        }
    }
    return audioLocation;
}

uint32 capClientGetRemoteAudioLocationsForCid(uint32 cid, bool isSink)
{
    uint8 i;
    uint32 location = CAP_CLIENT_AUDIO_LOCATION_MONO;

    for (i = 0; i < CAP_CLIENT_MAX_SUPPORTED_DEVICES; i++)
    {
        if (isSink)
        {
            if (sinkAudioLocations[i].cid == cid)
            {
                location =  sinkAudioLocations[i].location;
            }
        }
        else
        {
            if (sourceAudioLocations[i].cid == cid)
            {
                location =  sourceAudioLocations[i].location;
            }
        }
    }
    CAP_CLIENT_INFO("capClientGetCachedAudioLoc location  = %x \n", location);
    return location;
}

void capClientResetCachedAudioLoc(uint32 cid)
{
    uint8 i;
    for (i = 0; i < CAP_CLIENT_MAX_SUPPORTED_DEVICES; i++)
    {
        if (sinkAudioLocations[i].cid == cid)
        {
            sinkAudioLocations[i].cid = 0x0;
            sinkAudioLocations[i].location = 0x00;
        }

        if (sourceAudioLocations[i].cid == cid)
        {
            sourceAudioLocations[i].cid = 0x0;
            sourceAudioLocations[i].location = 0x00;
        }

        CAP_CLIENT_INFO("capClientFreeCachedAudioLoc sinkAudioLocations  = %d, sinkAudioLocations cid : %x\n", sinkAudioLocations[i].location, sinkAudioLocations[i].cid);
        CAP_CLIENT_INFO("capClientFreeCachedAudioLoc sourceAudioLocations  = %d, sourceAudioLocations cid : %x\n", sinkAudioLocations[i].location, sinkAudioLocations[i].cid);
    }
}

void capClientCacheRemoteSupportedContext(uint32 context)
{
    remoteContext = context;
}

uint32 capClientGetRemoteSupportedContext(void)
{
    return remoteContext;
}

uint8 capClientGetPacRecordCount(CapClientGroupInstance *cap)
{
    return (cap->sinkRecordList->count + cap->sourceRecordList->count);
}
#endif /* INSTALL_LEA_UNICAST_CLIENT */

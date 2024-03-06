/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP UTILS interface implementation.
 */

/**
 * \addtogroup BAP_UTILS_PRIVATE
 * @{
 */


#include "tbdaddr.h"
#include "bap_utils.h"
#include "bap_profile.h"
#include "bap_pac_record.h"
#include "bap_client_list_util_private.h"
#include "bap_broadcast_assistant.h"
#include "bap_broadcast_src.h"
#include <stdio.h>

#define QBL_LTV_LENGTH_FIELD_LEN              (1)

#define AD_TYPE_FLAGS                                0x01
#define AD_TYPE_COMPLETE_LIST_UUID16      0x03
#define AD_TYPE_SERVICE_DATA_UUID16       0x16
#define AD_TYPE_BROADCAST_NAME              0x30

bool bapParseAdvDataForUuid16(uint8* data,
                              uint16 dataLength,
                              uint16 baasUuid,
                              uint32* baasServiceData,
                              uint8 baasServiceDataLen,
                              char *bigName,
                              uint8* bigNameLen,
                              uint8 *serviceData,
                              uint8* serviceDataLen)
{
    uint8 adType;
    uint16 adDataSize;
    bool status = FALSE;
    uint8 index = 0;
    uint8 i = 0;

    /* Loop through the advert data, and extract each ad-type to parse */
    while(data && dataLength)
    {
        /* first octet has the data size of the ad_type */
        adDataSize = data[0];
        if(adDataSize)
        {
            /* The next octet has the ad_type */
            adType = data[1];
            switch(adType)
            {
                case AD_TYPE_FLAGS:
                {
                    BAP_CLIENT_DEBUG("(BAP) : FLAGS = 0x%X\n",data[2]);
                }
                break;

                case AD_TYPE_SERVICE_DATA_UUID16:
                {
                    uint8 *dataPtr = &data[2];
                    uint16 uuidValue = 0;

                    uuidValue = (uint16) dataPtr[0] | (dataPtr[1] << 8);
                    BAP_CLIENT_DEBUG("(BAP) :Service Data UUID16 = 0x%04X\n",uuidValue);
                    if (uuidValue == baasUuid)
                    {
                       if (baasServiceData && baasServiceDataLen)
                       {
                           /* Service data related to UUID will occupy next octets after UUID itself */
                           index = 2;
           
                           for (i = 0; i < baasServiceDataLen; i++)
                           {
                               if ((dataPtr + index + i))
                               {
                                   *baasServiceData |= (uint32)(dataPtr[index + i] << (8 * i));
                               }
                           }
                           *baasServiceData = 0x00ffffff & *baasServiceData;
                           status = TRUE;
                       }
                    }
                    else if ((uuidValue == BAP_TELEPHONY_MEDIA_AUDIO_BROADCAST_ANNOUNCEMENT_UUID) ||
                             (uuidValue == BAP_GAMING_AUDIO_BROADCAST_ANNOUNCEMENT_UUID) ||
                             (uuidValue == BAP_PUBLIC_BROADCAST_ANNOUNCEMENT_UUID))
                    {
                         for (i = 0; i < ((data[0] + 1)); i++)
                            serviceData[(*serviceDataLen)++] = data[i];
                    }
                }
                break;

                case AD_TYPE_BROADCAST_NAME:
                {
                    uint8 iter;
                    uint8 *dataPtr = &data[2];

                    if (bigNameLen) 
                    {
                        *bigNameLen = adDataSize - 1;
                        for (iter = 0; iter < (*bigNameLen); iter++)
                            bigName[iter] = dataPtr[iter];

    					BAP_CLIENT_DEBUG("(BAP) :Broadcast Name length = 0x%02X\n",(*bigNameLen));
                    }
                }
                break;

                /* Ignore */
                default:
                break;
            }
        }

        /* Could be a superfluous data_length! */
        if((adDataSize + 1) <=  dataLength)
        {
            /* Increase the pointer to the next ad_type. Please note we need to
                add 1 to consider the placeholder of data_size */
            data += (adDataSize + 1);
            /* At the same time need to decrease the length as we don't expect to
                read more than required */
            dataLength -= (adDataSize + 1);
        }
        else
        {
            /* Looks like a invalid packet, just ignore it*/
            dataLength = 0;
        }
    }
    return status;
}

void qblLtvInitialise(QblLtv* ltv, uint8* ltvStart)
{
    buff_iterator_initialise(&ltv->iter, ltvStart);

    ltv->ltvFormattedDataStart = ltvStart;
    ltv->length = buff_iterator_get_octet(&ltv->iter);
    ltv->type   = buff_iterator_get_octet(&ltv->iter);

}
/*
 * TODO some code for qblLtvDecodeCodecSpecificCapabilities is almost identical to that of
 *      qblLtvDecodeCodecSpecificConfig - this needs to be investigated (when the spec
 *      settles down) to improve code re-use.
 */
bool qblLtvDecodeCodecSpecificCapabilities(QblLtv* ltv, BapCodecConfiguration* codecConfig)
{
    uint8 expectedLength = 0;

    switch (ltv->type)
    {
        case SAMPLING_FREQUENCY_TYPE:
            /* Fall through */
        case SAMPLING_FREQUENCY_TYPE_VS:
        {
            codecConfig->samplingFrequency = (uint8) buff_iterator_get_octet(&ltv->iter);
            expectedLength = SAMPLING_FREQUENCY_LENGTH;

        }
        break;
        case FRAME_DURATION_TYPE:
        {
            codecConfig->frameDuaration = (uint8) buff_iterator_get_octet(&ltv->iter);
            expectedLength = FRAME_DURATION_LENGTH;

        }
        break;
        case AUDIO_CHANNEL_ALLOCATION_TYPE:
            /* Fall through */
        case AUDIO_CHANNEL_ALLOCATION_TYPE_VS:
        {
            codecConfig->audioChannelAllocation = qblLtvGetValueU32le(ltv);
            expectedLength = AUDIO_CHANNEL_ALLOCATION_LENGTH;
        }
        break;
        case OCTETS_PER_CODEC_FRAME_TYPE:
        {
            codecConfig->octetsPerFrame = qblLtvGetValueU16le(ltv);
            expectedLength = OCTETS_PER_CODEC_FRAME_LENGTH;
        }
        break;
        case LC3_BLOCKS_PER_SDU_TYPE:
        {
            codecConfig->lc3BlocksPerSdu = (uint8) buff_iterator_get_octet(&ltv->iter);
            expectedLength = LC3_BLOCKS_PER_SDU_LENGTH;
        }
        break;
    }

    return (qblLtvLengthIsCorrect(ltv, expectedLength))? TRUE : FALSE;
}

#ifdef INSTALL_LEA_UNICAST_CLIENT
/*
 * TODO some code for qblLtvDecodeCodecSpecificCapabilities is almost identical to that of
 *      qblLtvDecodeCodecSpecificConfig - this needs to be investigated (when the spec
 *      settles down) to improve code re-use.
 */
bool qblLtvDecodeCodecSpecificConfig(QblLtv* ltv,
                                     BapAse* ase,
                                     uint8 codecId,
                                     AscsAseErrorStatus* errorStatus)
{
    bool result = TRUE;
    uint8 expectedLength = 0;

    /* set the op code once here (instead of in multiple places in the code below) */
    errorStatus->failedOpcode = ASE_OPCODE_CONFIG_CODEC;

    switch (codecId)
    {
    case BAP_CODEC_ID_LC3:
        /*
         * Decode an LTV formatted configuration element that is specific to the LC3 codec
         */
        switch (ltv->type)
        {
            case BAP_SAMPLING_FREQUENCY_TYPE:
            {
                ase->codecConfiguration.samplingFrequency = qblLtvGetValueU16le(ltv);
                expectedLength = BAP_SAMPLING_FREQUENCY_LENGTH;
            }
            break;
        }
        break;
    default:
        errorStatus->errorResponse = ascs_error_response_codec_id;
        errorStatus->responseCode = ascs_response_code_unsupported_parameter_value;

        result = FALSE;
        break;

    }

    if ( ! qblLtvLengthIsCorrect(ltv, expectedLength))
    {
        errorStatus->errorResponse = ascs_error_response_codec_specific_configuration_length;
        errorStatus->responseCode = ascs_response_code_invalid_parameter_value;

        result = FALSE;
    }

    return result;
}
#endif

uint16 qblLtvGetValueU16le(QblLtv* ltv)
{
    uint16 lsb = buff_iterator_get_octet(&ltv->iter);
    uint16 msb = buff_iterator_get_octet(&ltv->iter);

    return (msb << 8) | lsb;
}

uint32 qblLtvGetValueU24le(QblLtv* ltv)
{
    uint32 lsb    = buff_iterator_get_octet(&ltv->iter);
    uint32 middle = buff_iterator_get_octet(&ltv->iter);
    uint32 msb    = buff_iterator_get_octet(&ltv->iter);

    return (msb << 16) | (middle << 8) | lsb;
}

uint32 qblLtvGetValueU32le(QblLtv* ltv)
{
    uint32 lsb    = buff_iterator_get_octet(&ltv->iter);
    uint32 middle = buff_iterator_get_octet(&ltv->iter);
    uint32 msbl    = buff_iterator_get_octet(&ltv->iter);
    uint32 msbh    = buff_iterator_get_octet(&ltv->iter);

    return ((msbh << 24) | (msbl << 16) | (middle << 8) | lsb);
}

uint8* qblLtvGetNextLtvStart(QblLtv* ltv)
{
    return ltv->ltvFormattedDataStart + ltv->length + QBL_LTV_LENGTH_FIELD_LEN;
}

uint8 readUint8(uint8 **buf)
{
    return 0xFF & *((*buf)++);
}

uint16 readUint16(uint8 **buf)
{
    uint16 valLow = readUint8(buf);
    uint16 valHigh = readUint8(buf);

    return valLow | (valHigh << 8);
}

uint24 readUint24(uint8 **buf)
{
    uint16 valLow = readUint8(buf);
    uint16 valMiddle = readUint8(buf);
    uint16 valHigh = readUint8(buf);

    return valLow | ((uint24)valMiddle << 8) |((uint24)valHigh << 16);
}

uint32 readUint32(uint8 **buf)
{
    uint16 valLow = readUint16(buf);
    uint16 valHigh = readUint16(buf);

    return valLow | (((uint32)valHigh) << 16);
}

uint8 bapMapPacsSamplingFreqToAscsValue(uint16 sampFreq)
{
    uint8 samplingFreq = 0;
    switch(sampFreq)
    {
        case BAP_SAMPLING_FREQUENCY_8kHz:
            samplingFreq= SAMPLING_FREQUENCY_8kHz;
            break;
        case BAP_SAMPLING_FREQUENCY_11_025kHz:
            samplingFreq= SAMPLING_FREQUENCY_11_025kHz;
            break;
        case BAP_SAMPLING_FREQUENCY_16kHz:
            samplingFreq= SAMPLING_FREQUENCY_16kHz;
            break;
        case BAP_SAMPLING_FREQUENCY_22_05kHz:
            samplingFreq= SAMPLING_FREQUENCY_22_050kHz;
            break;
        case BAP_SAMPLING_FREQUENCY_24kHz:
            samplingFreq= SAMPLING_FREQUENCY_24kHz;
            break;
        case BAP_SAMPLING_FREQUENCY_32kHz:
            samplingFreq= SAMPLING_FREQUENCY_32kHz;
            break;
        case BAP_SAMPLING_FREQUENCY_44_1kHz:
            samplingFreq= SAMPLING_FREQUENCY_44_1kHz;
            break;
        case BAP_SAMPLING_FREQUENCY_48kHz:
            samplingFreq= SAMPLING_FREQUENCY_48kHz;
            break;
        case BAP_SAMPLING_FREQUENCY_88_2KHZ:
            samplingFreq= SAMPLING_FREQUENCY_88_200kHz;
            break;
        case BAP_SAMPLING_FREQUENCY_96kHz:
            samplingFreq= SAMPLING_FREQUENCY_96kHz;
            break;
        case BAP_SAMPLING_FREQUENCY_176_4kHz:
            samplingFreq= SAMPLING_FREQUENCY_176_420kHz;
            break;
        case BAP_SAMPLING_FREQUENCY_192kHz:
            samplingFreq= SAMPLING_FREQUENCY_192kHz;
            break;
        case BAP_SAMPLING_FREQUENCY_384kHz:
            samplingFreq= SAMPLING_FREQUENCY_384kHz;
            break;

        default:
            break;
    }

    return samplingFreq;
}

bool bapUtilsFindLtvValue(uint8 * ltvData,
                          uint8 ltvDataLength,
                          uint8 type,
                          uint8 * value,
                          uint8 valueLength)
{
    bool ltvFound = FALSE;
    if(ltvData && ltvDataLength && value)
    {
        int ltvIndex = 0;
        while(ltvIndex < ltvDataLength && ltvFound == FALSE && ltvData[ltvIndex])
        {
            uint8 length = ltvData[ltvIndex];
            BAP_CLIENT_DEBUG("bapUtilsFindLtvValue: index=%d length=%d type=%d",
                ltvIndex, ltvData[ltvIndex], ltvData[ltvIndex + 1]);

            if(ltvData[ltvIndex + 1] == type)
            {
                if(ltvData[ltvIndex] == (valueLength + 1))
                {
                    uint8 i;
                    for(i = 0; i < valueLength; i++)
                    {
                        value[i] = ltvData[ltvIndex + 2 + i];
                    }
                    ltvFound = TRUE;
                }
                else
                {
                    BAP_CLIENT_DEBUG("bapUtilsFindLtvValue: Unexpected length");
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
        BAP_CLIENT_DEBUG("bapUtilsFindLtvValue: Invalid LTV data");
    }

    return ltvFound;
}

void putMessageSynergy(CsrSchedQid q, CsrUint16 mi, void *mv)
{
    CsrSchedMessagePut(q,mi,mv);
}

#ifdef INSTALL_LEA_UNICAST_CLIENT
void bapUtilsCleanupBapConnection(BapConnection* connection)
{
    if(connection)
    {
        if(connection->ascs.srvcHndl)
        {
            /* Remove the ASCS Client instance */
            connection->numService++;
            GattAscsClientTerminate(connection->ascs.srvcHndl);
        }
    
        if(connection->pacs.srvcHndl)
        {
            /* Remove the PACS Client instance */
            connection->numService++;
            GattPacsClientTerminateReq(connection->pacs.srvcHndl);
        }
#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
        if(connection->bass.srvcHndl)
        {
            bapBroadcastAssistantDeinit(connection);
        }
#endif
    }
}
#endif

void bapUtilsSendBapInitCfm(phandle_t phandle,
                            uint32 cid,
                            BapResult result,
                            BapRole role)
{
    BapInitCfm*cfmPrim = CsrPmemZalloc(sizeof(BapInitCfm));

    cfmPrim->type = BAP_INIT_CFM;
    cfmPrim->handle = cid;
    cfmPrim->result = result;
    cfmPrim->role = role;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendBapDeinitCfm(phandle_t phandle,
                              uint32 cid,
                              BapRole role, 
                              BapResult result, 
                              BapHandles *handles)
{
    BapDeinitCfm* cfmPrim = CsrPmemZalloc(sizeof(BapDeinitCfm));


    cfmPrim->type = BAP_DEINIT_CFM;
    cfmPrim->result = result;
    cfmPrim->handle = cid;
    cfmPrim->role = role;
    cfmPrim->handles = CsrPmemZalloc(sizeof(BapHandles));

    if(handles)
    {
        if(handles->ascsHandles)
            cfmPrim->handles->ascsHandles = handles->ascsHandles;

        if(handles->pacsHandles)
            cfmPrim->handles->pacsHandles = handles->pacsHandles;

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
        if(handles->bassHandles)
            cfmPrim->handles->bassHandles = handles->bassHandles;
#endif
    }

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendRegisterPacsNotificationCfm(phandle_t phandle,
                                             uint32 cid,
                                             BapResult result)
{
    BapRegisterPacsNotificationCfm* cfmPrim = CsrPmemZalloc(sizeof(BapRegisterPacsNotificationCfm));

    cfmPrim->type = BAP_REGISTER_PACS_NOTIFICATION_CFM;
    cfmPrim->handle = cid;
    cfmPrim->result = result;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendPacRecordNotificationInd(phandle_t phandle,
                                          uint32 cid,
                                          BapPacsNotificationType notifyType,
                                          const BapPacRecord* record,
                                          uint8 recordCount)
{
    BapPacsAudioCapabilityNotificationInd* cfmPrim = CsrPmemZalloc(sizeof(BapPacsAudioCapabilityNotificationInd));

    cfmPrim->type = BAP_PACS_AUDIO_CAPABILITY_NOTIFICATION_IND;
    cfmPrim->handle = cid;
    cfmPrim->numPacRecords = recordCount;
    cfmPrim->recordType = (notifyType == BAP_PACS_NOTIFICATION_SINK_PAC_RECORD) ?
                                                 BAP_AUDIO_SINK_RECORD : BAP_AUDIO_SOURCE_RECORD;

    if (recordCount && (record != NULL))
    {
        uint8 i;
        for (i = 0; i < recordCount; i++)
        {
            cfmPrim->pacRecords[i] = (BapPacRecord*)(&record[i]);
        }
    }

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendPacsNotificationInd(phandle_t phandle,
                                     uint32 cid,
                                     BapPacsNotificationType notifyType,
                                     void* notifyValue)
{
    BapPacsNotificationInd* cfmPrim = CsrPmemZalloc(sizeof(BapPacsNotificationInd));

    cfmPrim->type = BAP_PACS_NOTIFICATION_IND;
    cfmPrim->handle = cid;
    cfmPrim->notifyType = notifyType;

    switch (notifyType)
    {
        case BAP_PACS_NOTIFICATION_AVAILABLE_AUDIO_CONTEXT:
        case BAP_PACS_NOTIFICATION_SUPPORTED_AUDIO_CONTEXT:
        {
            cfmPrim->valueLength = sizeof(BapPacAudioContextValue);
            cfmPrim->value = CsrPmemZalloc(sizeof(uint8) * cfmPrim->valueLength);
            CsrMemCpy(cfmPrim->value, (uint8*)notifyValue, cfmPrim->valueLength);
        }
        break;
        case BAP_PACS_NOTIFICATION_SINK_AUDIO_LOCATION:
        case BAP_PACS_NOTIFICATION_SOURCE_AUDIO_LOCATION:
        {
            cfmPrim->valueLength = sizeof(BapAudioLocation);
            cfmPrim->value = CsrPmemZalloc(sizeof(uint8) * cfmPrim->valueLength);
            CsrMemCpy(cfmPrim->value, (uint8*)notifyValue, cfmPrim->valueLength);
        }
        break;

        case BAP_PACS_NOTIFICATION_ALL:
        {
            /* Invalid case. Added to remove build warnings only  */
        }
        break;
    }

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendDiscoverRemoteAudioCapabilityCfm(phandle_t phandle,
                                                  uint32 cid,
                                                  BapResult result,
                                                  BapPacRecordType recordType,
                                                  uint8 numPacRecords,
                                                  const BapPacRecord*  pacRecords,
                                                  bool moreToCome)
{
    BapDiscoverRemoteAudioCapabilityCfm* cfmPrim = CsrPmemZalloc(sizeof(BapDiscoverRemoteAudioCapabilityCfm));

    cfmPrim->type = BAP_DISCOVER_REMOTE_AUDIO_CAPABILITY_CFM;
    cfmPrim->result = result;
    cfmPrim->recordType = recordType;
    cfmPrim->handle = cid;
    cfmPrim->moreToCome = moreToCome;

    /* Capping the max number of PAC record entries in cfm to 5 entries*/
    numPacRecords = (numPacRecords > MAX_PAC_RECORD_ENTRIES) ? MAX_PAC_RECORD_ENTRIES : numPacRecords;
    cfmPrim->numPacRecords = numPacRecords;

    if (numPacRecords && (pacRecords != NULL))
    {
        uint8 i;
        for (i = 0; i < numPacRecords; i++)
        {
            cfmPrim->pacRecords[i] = (BapPacRecord*)(&pacRecords[i]);
        }
    }

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendGetRemoteAudioLocationCfm(phandle_t phandle,
                                           uint32 cid,
                                           BapResult result,
                                           BapServerDirection direction,
                                           BapAudioLocation location)
{
    BapGetRemoteAudioLocationCfm* cfmPrim = CsrPmemZalloc(sizeof(BapGetRemoteAudioLocationCfm));

    cfmPrim->type = BAP_GET_REMOTE_AUDIO_LOCATION_CFM;
    cfmPrim->handle = cid;
    cfmPrim->result = result;
    cfmPrim->recordType = direction;
    cfmPrim->location = location;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendSetRemoteAudioLocationCfm(phandle_t phandle,
                                           uint32 cid,
                                           BapResult result)
{
    BapSetRemoteAudioLocationCfm* cfmPrim = CsrPmemZalloc(sizeof(BapSetRemoteAudioLocationCfm));

    cfmPrim->type = BAP_SET_REMOTE_AUDIO_LOCATION_CFM;
    cfmPrim->handle = cid;
    cfmPrim->result = result;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendDiscoverAudioContextCfm(phandle_t phandle,
                                         uint32 cid,
                                         BapResult result,
                                         BapPacAudioContext context,
                                         BapPacAudioContextValue contextValue)
{
    BapDiscoverAudioContextCfm* cfmPrim = CsrPmemZalloc(sizeof(BapDiscoverAudioContextCfm));

    cfmPrim->type = BAP_DISCOVER_AUDIO_CONTEXT_CFM;
    cfmPrim->handle = cid;
    cfmPrim->result = result;
    cfmPrim->context = context;
    cfmPrim->contextValue = contextValue;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendRegisterAseCfm(phandle_t phandle,
                                BapResult result,
                                uint8 aseId,
                                BapProfileHandle cid)
{
    BapUnicastClientRegisterAseNotificationCfm *cfmPrim = CsrPmemZalloc(sizeof(BapUnicastClientRegisterAseNotificationCfm));

    cfmPrim->type = BAP_UNICAST_CLIENT_REGISTER_ASE_NOTIFICATION_CFM;
    cfmPrim->result = result;
    cfmPrim->ase_id = aseId;
    cfmPrim->handle= cid;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendGetRemoteAseInfoCfm(phandle_t phandle,
                                     BapResult result,
                                     BapProfileHandle cid,
                                     uint16 numAses,
                                     uint8 * const aseIds,
                                     uint8 aseState,
                                     BapAseType type,
                                     uint8 aseInfoLen,
                                     uint8 *aseInfo)
{
    BapUnicastClientReadAseInfoCfm *cfmPrim = CsrPmemZalloc(sizeof(BapUnicastClientReadAseInfoCfm));

    cfmPrim->type = BAP_UNICAST_CLIENT_READ_ASE_INFO_CFM;
    cfmPrim->phandle = phandle;
    cfmPrim->result = result;
    cfmPrim->handle= cid;
    cfmPrim->numAses = (uint8) numAses;
    cfmPrim->aseIds = aseIds;
    cfmPrim->aseState = aseState;
    cfmPrim->aseType = type;
    cfmPrim->aseInfoLen = aseInfoLen;
    cfmPrim->aseInfo = NULL;
    if(aseInfoLen  && aseInfo)
    {
        cfmPrim->aseInfo = CsrPmemZalloc(aseInfoLen*sizeof(uint8));
        CsrMemCpy(cfmPrim->aseInfo, aseInfo, (aseInfoLen*sizeof(uint8)));
    }

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendAseCodecConfigureInd(phandle_t phandle,
                                      BapResult result,
                                      BapProfileHandle streamGroupId,
                                      uint32 presentationDelayMin,
                                      uint32 presentationDelayMax,
                                      BapAseInfo *aseInfo,
                                      uint8  peerFraming,
                                      uint8  peerPhy,
                                      uint8  peerRetransmissionEffort,
                                      uint16 peerTransportLatency,
                                      BapCodecConfiguration *codecParams,
                                      bool clientInitiated)
{
    BapUnicastClientCodecConfigureInd *indPrim = CsrPmemZalloc(sizeof(BapUnicastClientCodecConfigureInd));

    indPrim->type = BAP_UNICAST_CLIENT_CODEC_CONFIGURE_IND;

    indPrim->result = result;
    indPrim->handle = streamGroupId;
    indPrim->aseId = aseInfo->aseId;
    indPrim->aseState = aseInfo->aseState;

    indPrim->framing = peerFraming;
    indPrim->phy = peerPhy;
    indPrim->rtn = peerRetransmissionEffort;
    indPrim->transportLatency = peerTransportLatency;

    indPrim->presentationDelayMin = presentationDelayMin;
    indPrim->presentationDelayMax = presentationDelayMax;
    if(codecParams)
        memcpy( &indPrim->codecConfiguration, codecParams, sizeof(BapCodecConfiguration));
    indPrim->clientInitiated = clientInitiated;
    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, indPrim);
}

void bapUtilsSendStreamGroupCodecConfigureCfm(phandle_t phandle,
                                              BapResult result,
                                              BapProfileHandle streamGroupId)
{
    BapUnicastClientCodecConfigureCfm *cfmPrim = CsrPmemZalloc(sizeof(BapUnicastClientCodecConfigureCfm));

    cfmPrim->type = BAP_UNICAST_CLIENT_CODEC_CONFIGURE_CFM;

    cfmPrim->result = result;
    cfmPrim->handle = streamGroupId;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendAseQosConfigureInd(phandle_t phandle,
                                    BapResult result,
                                    BapProfileHandle streamGroupId,
                                    uint32 presentationDelayMicroseconds,
                                    BapAseInfo *aseInfo)
{
    BapUnicastClientQosConfigureInd *indPrim = CsrPmemZalloc(sizeof(BapUnicastClientQosConfigureInd));

    indPrim->type = BAP_UNICAST_CLIENT_QOS_CONFIGURE_IND;

    indPrim->result = result;
    indPrim->handle = streamGroupId;
    indPrim->aseId = aseInfo->aseId;
    indPrim->cisId = aseInfo->cisId;
    indPrim->aseState = aseInfo->aseState;
    indPrim->presentationDelayMicroseconds = presentationDelayMicroseconds;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, indPrim);
}


void bapUtilsSendStreamGroupQosConfigureCfm(phandle_t phandle,
                                            BapResult result,
                                            BapProfileHandle streamGroupId)
{
    BapUnicastClientQosConfigureCfm *cfmPrim = CsrPmemZalloc(sizeof(BapUnicastClientQosConfigureCfm));

    cfmPrim->type = BAP_UNICAST_CLIENT_QOS_CONFIGURE_CFM;

    cfmPrim->result = result;
    cfmPrim->handle = streamGroupId;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendAseEnableInd(phandle_t phandle,
                              BapResult result,
                              BapProfileHandle streamGroupId,
                              BapAseInfo *aseInfo,
                              uint8 metadataLength,
                              uint8* metadata)
{
    BapUnicastClientEnableInd *indPrim = CsrPmemZalloc(sizeof(BapUnicastClientEnableInd));

    indPrim->type = BAP_UNICAST_CLIENT_ENABLE_IND;

    indPrim->result = result;
    indPrim->handle = streamGroupId;
    indPrim->aseId = aseInfo->aseId;
    indPrim->cisId = aseInfo->cisId;
    indPrim->aseState = aseInfo->aseState;
    indPrim->metadataLength = metadataLength;

    if (indPrim->metadataLength)
    {
        indPrim->metadata = (uint8*) CsrPmemAlloc(indPrim->metadataLength);
        memcpy(indPrim->metadata, metadata, indPrim->metadataLength);
    }
    else
        indPrim->metadata = NULL;
		

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, indPrim);
}

void bapUtilsSendStreamGroupEnableCfm(phandle_t phandle,
                                      BapResult result,
                                      BapProfileHandle streamGroupId)
{
    BapUnicastClientEnableCfm *cfmPrim = CsrPmemZalloc(sizeof(BapUnicastClientEnableCfm));

    cfmPrim->type = BAP_UNICAST_CLIENT_ENABLE_CFM;

    cfmPrim->result = result;
    cfmPrim->handle = streamGroupId;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendAseUpdateMetadataInd(phandle_t phandle,
                                      BapResult result,
                                      BapProfileHandle streamGroupId,
                                      BapAseInfo *aseInfo,
                                      uint16 context,
                                      uint8 metadataLength,
                                      uint8* metadata)
{
    BapUnicastClientUpdateMetadataInd *indPrim = CsrPmemZalloc(sizeof(BapUnicastClientUpdateMetadataInd));

    indPrim->type = BAP_UNICAST_CLIENT_UPDATE_METADATA_IND;

    indPrim->result = result;
    indPrim->handle = streamGroupId;
    indPrim->aseId = aseInfo->aseId;
    indPrim->streamingAudioContexts = context;
    indPrim->metadataLength = metadataLength;

    if (indPrim->metadataLength)
    {
        indPrim->metadata = (uint8*) CsrPmemAlloc(indPrim->metadataLength);
        memcpy(indPrim->metadata, metadata, indPrim->metadataLength);
    }
    else
        indPrim->metadata = NULL;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, indPrim);
}

void bapUtilsSendStreamGroupMetadataCfm(phandle_t phandle,
                                        BapResult result,
                                        BapProfileHandle streamGroupId)
{
    BapUnicastClientUpdateMetadataCfm *cfmPrim = CsrPmemZalloc(sizeof(BapUnicastClientUpdateMetadataCfm));

    cfmPrim->type = BAP_UNICAST_CLIENT_UPDATE_METADATA_CFM;

    cfmPrim->result = result;
    cfmPrim->handle = streamGroupId;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendAseDisableInd(phandle_t phandle,
                               BapResult result,
                               BapProfileHandle streamGroupId,
                               BapAseInfo *aseInfo,
                               bool clientInitiated)
{
    BapUnicastClientDisableInd *indPrim = CsrPmemZalloc(sizeof(BapUnicastClientDisableInd));

    indPrim->type = BAP_UNICAST_CLIENT_DISABLE_IND;

    indPrim->result = result;
    indPrim->handle = streamGroupId;
    indPrim->aseId = aseInfo->aseId;
    indPrim->cisId = aseInfo->cisId;
    indPrim->aseState = aseInfo->aseState;
    indPrim->clientInitiated = clientInitiated;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, indPrim);
}

void bapUtilsSendStreamGroupDisableCfm(phandle_t phandle,
                                       BapResult result,
                                       BapProfileHandle streamGroupId)
{
    BapUnicastClientDisableCfm *cfmPrim = CsrPmemZalloc(sizeof(BapUnicastClientDisableCfm));

    cfmPrim->type = BAP_UNICAST_CLIENT_DISABLE_CFM;

    cfmPrim->result = result;
    cfmPrim->handle = streamGroupId;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendAseReleaseInd(phandle_t phandle,
                               BapResult result,
                               BapProfileHandle streamGroupId,
                               BapAseInfo *aseInfo,
                               bool clientInitiated)
{
    BapUnicastClientReleaseInd *indPrim = CsrPmemZalloc(sizeof(BapUnicastClientReleaseInd));

    indPrim->type = BAP_UNICAST_CLIENT_RELEASE_IND;

    indPrim->result = result;
    indPrim->handle = streamGroupId;
    indPrim->aseId = aseInfo->aseId;
    indPrim->cisId = aseInfo->cisId;
    indPrim->aseState = aseInfo->aseState;
    indPrim->clientInitiated = clientInitiated;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, indPrim);
}

void bapUtilsSendStreamGroupReleaseCfm(phandle_t phandle,
                                       BapResult result,
                                       BapProfileHandle streamGroupId)
{
    BapUnicastClientReleaseCfm *cfmPrim = CsrPmemZalloc(sizeof(BapUnicastClientReleaseCfm));

    cfmPrim->type = BAP_UNICAST_CLIENT_RELEASE_CFM;

    cfmPrim->result = result;
    cfmPrim->handle = streamGroupId;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendAseReleasedInd(phandle_t phandle,
                                BapProfileHandle streamGroupId,
                                BapAseInfo *aseInfo)
{
    BapUnicastClientReleasedInd *indPrim = CsrPmemZalloc(sizeof(BapUnicastClientReleasedInd));

    indPrim->type = BAP_UNICAST_CLIENT_RELEASED_IND;
    indPrim->handle = streamGroupId;
    indPrim->aseId = aseInfo->aseId;
    indPrim->cisId = aseInfo->cisId;
    indPrim->aseState = aseInfo->aseState;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, indPrim);
}

void bapUtilsSendAseReceiverReadyInd(phandle_t phandle,
                                     BapProfileHandle streamGroupId,
                                     uint8 ready_type,
                                     BapResult result,
                                     BapAseInfo *aseInfo,
                                     bool clientInitiated)
{
    BapUnicastClientReceiverReadyInd *indPrim = CsrPmemZalloc(sizeof(BapUnicastClientReceiverReadyInd));

    indPrim->type = BAP_UNICAST_CLIENT_RECEIVER_READY_IND;
    indPrim->handle = streamGroupId;
    indPrim->aseId = aseInfo->aseId;
    indPrim->readyType = ready_type;
    indPrim->result = result;
    indPrim->aseState = aseInfo->aseState;
    indPrim->clientInitiated = clientInitiated;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, indPrim);
}

void bapUtilsSendStreamGroupReceiverReadyCfm(phandle_t phandle,
                                             BapResult result,
                                             BapProfileHandle streamGroupId,
                                             uint8 readyType)
{
    BapUnicastClientReceiverReadyCfm *cfmPrim = CsrPmemZalloc(sizeof(BapUnicastClientReceiverReadyCfm));

    cfmPrim->type = BAP_UNICAST_CLIENT_RECEIVER_READY_CFM;

    cfmPrim->result = result;
    cfmPrim->handle = streamGroupId;
    cfmPrim->readyType = readyType;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendCigConfigureCfm(phandle_t phandle,
                                 CmIsocConfigureCigCfm *prim)
{
    BapUnicastClientCigConfigureCfm *cfmPrim = CsrPmemZalloc(sizeof(BapUnicastClientCigConfigureCfm));
    uint8 i;

    cfmPrim->type = BAP_UNICAST_CLIENT_CIG_CONFIGURE_CFM;
    cfmPrim->handle = phandle;

    if(prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
        cfmPrim->result = BAP_RESULT_SUCCESS;
    else
        cfmPrim->result = BAP_RESULT_ERROR;
    cfmPrim->cigId = prim->cig_id;
    cfmPrim->cisCount = prim->cis_count;

    for(i = 0; i< cfmPrim->cisCount; i++)
        cfmPrim->cisHandles[i] = prim->cis_handles[i];

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendCigTestConfigureCfm(phandle_t phandle,
                                     CmIsocConfigureCigTestCfm *prim)
{
    BapUnicastClientCigTestConfigureCfm *cfmPrim = CsrPmemAlloc(sizeof(BapUnicastClientCigTestConfigureCfm));
    uint8 i;

    cfmPrim->type = BAP_UNICAST_CLIENT_CIG_TEST_CONFIGURE_CFM;

    if(prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
        cfmPrim->result = BAP_RESULT_SUCCESS;
    else
        cfmPrim->result = BAP_RESULT_ERROR;

    cfmPrim->cigId = prim->cig_id;
    cfmPrim->cisCount = prim->cis_count;

    for(i = 0; i< cfmPrim->cisCount; i++)
        cfmPrim->cisTestHandles[i] = prim->cis_handles[i];

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendRemoveCigCfm(phandle_t phandle,
                              CmIsocRemoveCigCfm *prim)
{
    BapUnicastClientRemoveCigCfm *cfmPrim = CsrPmemZalloc(sizeof(BapUnicastClientRemoveCigCfm));

    cfmPrim->type = BAP_UNICAST_CLIENT_REMOVE_CIG_CFM;
    cfmPrim->handle = phandle;

    if(prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
        cfmPrim->result = BAP_RESULT_SUCCESS;
    else
        cfmPrim->result = BAP_RESULT_ERROR;
    cfmPrim->cigId = prim->cig_id;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendStreamGroupCisConnectInd(phandle_t phandle,
                                          BapResult result,
                                          BapProfileHandle streamGroupId,
                                          uint16 cisHandle,
                                          BapUnicastClientCisParam *cisParams,
                                          bool clientInitiated)
{
    BapUnicastClientCisConnectInd *cfmPrim = CsrPmemZalloc(sizeof(BapUnicastClientCisConnectInd));

    cfmPrim->type = BAP_UNICAST_CLIENT_CIS_CONNECT_IND;

    cfmPrim->result = result;
    cfmPrim->handle = streamGroupId;
    cfmPrim->cisHandle = cisHandle;
    memcpy(&cfmPrim->cisParams, cisParams, sizeof(BapUnicastClientCisParam));
    cfmPrim->clientInitiated = clientInitiated;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendStreamGroupCisConnectCfm(phandle_t phandle,
                                          BapResult result,
                                          BapProfileHandle streamGroupId)
{
    BapUnicastClientCisConnectCfm *cfmPrim = CsrPmemZalloc(sizeof(BapUnicastClientCisConnectCfm));

    cfmPrim->type = BAP_UNICAST_CLIENT_CIS_CONNECT_CFM;

    cfmPrim->result = result;
    cfmPrim->handle = streamGroupId;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendCisDisconnectInd(phandle_t phandle,
                                  uint16 reason,
                                  BapProfileHandle streamGroupId,
                                  uint16 cisHandle)
{
    BapUnicastClientCisDisconnectInd *cfmPrim = CsrPmemZalloc(sizeof(BapUnicastClientCisDisconnectInd));

    cfmPrim->type = BAP_UNICAST_CLIENT_CIS_DISCONNECT_IND;

    cfmPrim->reason = reason;
    cfmPrim->handle = streamGroupId;
    cfmPrim->cisHandle = cisHandle;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendCisDisconnectCfm(phandle_t phandle,
                                  BapResult result,
                                  BapProfileHandle streamGroupId,
                                  uint16 cisHandle)
{
    BapUnicastClientCisDisconnectCfm *cfmPrim = CsrPmemZalloc(sizeof(BapUnicastClientCisDisconnectCfm));

    cfmPrim->type = BAP_UNICAST_CLIENT_CIS_DISCONNECT_CFM;

    cfmPrim->result = result;
    cfmPrim->handle = streamGroupId;
    cfmPrim->cisHandle = cisHandle;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendSetupDatapathCfm(phandle_t phandle, 
                                  uint16 handle, 
                                  BapResult result,
                                  BapProfileHandle streamGroupId)
{
    BapSetupDataPathCfm *cfmPrim = CsrPmemZalloc(sizeof(BapSetupDataPathCfm));

    cfmPrim->type = BAP_CLIENT_SETUP_DATA_PATH_CFM;
    cfmPrim->handle = streamGroupId;
    cfmPrim->isoHandle = handle;
    cfmPrim->result = result;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapUtilsSendRemoveDatapathCfm(phandle_t phandle,
                                   uint16 handle,
                                   BapResult result)
{
    BapRemoveDataPathCfm *cfmPrim = CsrPmemZalloc(sizeof(BapRemoveDataPathCfm));

    cfmPrim->type = BAP_CLIENT_REMOVE_DATA_PATH_CFM;
    cfmPrim->handle = phandle;
    cfmPrim->isoHandle = handle ;
    cfmPrim->result = result;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapSetUtilsSendControlPointOpCfm(phandle_t phandle,
                                      BapProfileHandle handle,
                                      BapResult result)
{
    BapSetControlPointOpCfm *cfm = CsrPmemZalloc(sizeof(BapSetControlPointOpCfm));

    cfm->type = BAP_SET_CONTROL_POINT_OP_CFM;
    cfm->handle = handle;
    cfm->result = result;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfm);
}

/**@}*/

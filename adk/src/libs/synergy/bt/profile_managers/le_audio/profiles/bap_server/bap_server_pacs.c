/****************************************************************************
* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
* %%version
************************************************************************* ***/
#include "csr_bt_gatt_lib.h"
#include "csr_bt_gatt_prim.h"

#include "bap_server_private.h"
#include "bap_server_msg_handler.h"
#include "bap_server_debug.h"
#include "bap_server_init.h"
#include "bap_server_common.h"
#include "gatt_pacs_server.h"
#include "bap_server_pacs.h"
#include "bap_server_lib.h"
#include "bap_server_common.h"

registeredPacsRecords * BapSinkRecords = NULL;
registeredPacsRecords * BapSourceRecords = NULL;

static bool bapServerPacsUtilitiesIsCodecConfigSupportedBySink(ServiceHandle handle,
                                                               uint8 codingFormat,
                                                               uint16 companyId,
                                                               uint16 vendorSpecificCodecId)
{
    uint8 i;
    bool isSupported = FALSE;

    if (codingFormat == GATT_ASCS_CODEC_VENDOR_SPECIFIC_CODING_FORMAT)
    {
        /* A vendor specific will have a different pac record type */
        for(i = 0; ((i < BapSinkRecords->numberOfRegisteredRecords) && (isSupported == FALSE)); i++)
        {
            const GattPacsServerVSPacRecord * pac_record = GattPacsServerGetVSPacRecord(handle, BapSinkRecords->pacRecordHandles[i]);
            BAP_SERVER_INFO("bapServerPacsUtilitiesIsCodecConfigSupportedBySink checking VS handle[%d]=0x%x", i, BapSinkRecords->pacRecordHandles[i]);

            if(pac_record)
            {
                if(codingFormat == pac_record->codecId
                    && companyId == pac_record->companyId
                    && vendorSpecificCodecId == pac_record->vendorCodecId)
                {
                    isSupported = TRUE;
                    break;
                }
            }
        }
    }
    else
    {
        for(i = 0; ((i < BapSinkRecords->numberOfRegisteredRecords) && (isSupported == FALSE)); i++)
        {
            {
                const GattPacsServerRecordType * pac_record = GattPacsServerGetPacRecord( handle, BapSinkRecords->pacRecordHandles[i]);
                BAP_SERVER_INFO("bapServerPacsUtilitiesIsCodecConfigSupportedBySink checking handle[%d]=0x%x", i, BapSinkRecords->pacRecordHandles[i]);

                if(pac_record)
                {
                    if(codingFormat == pac_record->codecId
                        && companyId == pac_record->companyId
                        && vendorSpecificCodecId == pac_record->vendorCodecId)
                    {
                        isSupported = TRUE;
                        break;
                    }
                }
            }
        }
    }

    if (!isSupported)
    {
        BAP_SERVER_WARNING("bapServerPacsUtilitiesIsCodecConfigSupportedBySink no record found handle[%d]=0x%x",
                            i, BapSinkRecords->pacRecordHandles[i]);
    }
    return isSupported;
}

static bool bapServerPacsUtilitiesIsCodecConfigSupportedBySource(ServiceHandle handle,
                                                                 uint8 codingFormat,
                                                                 uint16 companyId,
                                                                 uint16 vendorSpecificCodecId)
{
    uint8 i;
    bool isSupported = FALSE;
    for(i = 0; ((i < BapSourceRecords->numberOfRegisteredRecords) && (isSupported == FALSE)); i++)
    {
        const GattPacsServerRecordType * pac_record = GattPacsServerGetPacRecord( handle, BapSourceRecords->pacRecordHandles[i]);
        if(pac_record)
        {
            if(codingFormat == pac_record->codecId
                && companyId == pac_record->companyId
                && vendorSpecificCodecId == pac_record->vendorCodecId)
            {
                isSupported = TRUE;
                break;
            }
        }
    }
    return isSupported;
}

static bool bapServerPacsUtilitiesIsCodecConfigSupported(ServiceHandle handle, GattAscsAseDirection direction,
                    uint8 ltvType,  void *ltvValue)
{
    bool isSupported = FALSE;
    registeredPacsRecords * BapPacsRecords = BapSourceRecords;
    uint8 i;

    if (direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK)
    {
        BapPacsRecords = BapSinkRecords;
    }
    
    for(i = 0; ((i < BapPacsRecords->numberOfRegisteredRecords) && (isSupported == FALSE)); i++)
    {
        const GattPacsServerRecordType * pac_record = GattPacsServerGetPacRecord( handle, BapPacsRecords->pacRecordHandles[i]);

        if(pac_record)
        {
            switch(ltvType)
            {
                case BAP_CODEC_CONFIG_LTV_TYPE_SAMPLING_FREQUENCY_VS:
                case BAP_CODEC_CONFIG_LTV_TYPE_SAMPLING_FREQUENCY:
                {
                    uint8 sampFreq = *(uint8 *)ltvValue;
                    if(pac_record->supportedSamplingFrequencies & (1 << (sampFreq - 1)))
                    {
                        isSupported = TRUE;
                    }
                }
                break;

                case BAP_CODEC_CONFIG_LTV_TYPE_FRAME_DURATION:
                {
                    uint8 frameDur = *(uint8 *)ltvValue;
                    if(pac_record->supportedFrameDuration & (1 << (frameDur - 1)))
                    {
                        isSupported = TRUE;
                    }
                }
                break;
                case BAP_CODEC_CONFIG_LTV_TYPE_AUDIO_CHANNEL_ALLOCATION_VS:
                case BAP_CODEC_CONFIG_LTV_TYPE_AUDIO_CHANNEL_ALLOCATION:
                {
                    uint32 audioChannel = *(uint32 *)ltvValue;
                    if(pac_record->audioChannelCounts & (1 << (audioChannel - 1)))
                    {
                        isSupported = TRUE;
                    }
                }
                break;

                default:
                    break;
            }
        }
        else
        {
            BAP_SERVER_DEBUG("bapServerPacsUtilitiesIsCodecConfigSupported no record found handle[%d]=0x%x",
                                i, BapPacsRecords->pacRecordHandles[i]);
        }
    }

    return isSupported;
}

bool bapServerIsCodecConfigSuppported(ServiceHandle handle,
                                      GattAscsServerConfigureCodecClientInfo * clientCodec,
                                      uint8 *samplingValue, uint8 *frameDurValue)
{
    bool supported = FALSE;

    if (clientCodec->direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK)
    {
        supported = bapServerPacsUtilitiesIsCodecConfigSupportedBySink(handle,
                                                                       clientCodec->codecId.codingFormat,
                                                                       clientCodec->codecId.companyId,
                                                                       clientCodec->codecId.vendorSpecificCodecId);
    }
    else
    {
        supported = bapServerPacsUtilitiesIsCodecConfigSupportedBySource(handle,
                                                                         clientCodec->codecId.codingFormat,
                                                                         clientCodec->codecId.companyId,
                                                                         clientCodec->codecId.vendorSpecificCodecId);
    }

    if(clientCodec->codecId.codingFormat == GATT_ASCS_CODEC_VENDOR_SPECIFIC_CODING_FORMAT)
    {
        if(BapServerLtvUtilitiesFindLtvValue(clientCodec->codecConfiguration, 
            clientCodec->codecConfigurationLength, BAP_CODEC_CONFIG_LTV_TYPE_SAMPLING_FREQUENCY_VS, samplingValue, 1))
        {
            return TRUE;
        }
        return FALSE;
    }
    
    /* Check for Sampling frequency LTV */
    if(BapServerLtvUtilitiesFindLtvValue(clientCodec->codecConfiguration, 
        clientCodec->codecConfigurationLength, BAP_CODEC_CONFIG_LTV_TYPE_SAMPLING_FREQUENCY, samplingValue, 1))
    {
        /* verify PAC server supported the sampling frequency */
        if(!bapServerPacsUtilitiesIsCodecConfigSupported(handle, clientCodec->direction,
            BAP_CODEC_CONFIG_LTV_TYPE_SAMPLING_FREQUENCY, samplingValue))
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    /* Check for frame duration LTV */
    if(!BapServerLtvUtilitiesFindLtvValue(clientCodec->codecConfiguration, 
        clientCodec->codecConfigurationLength, BAP_CODEC_CONFIG_LTV_TYPE_FRAME_DURATION, frameDurValue, 1))
    {
        return FALSE;
    }

    return supported;
}

uint16 bapServerPacsUtilitiesGetSinkAudioContextAvailability(void)
{
    return GattPacsServerGetAudioContextsAvailability(bapServerPacsUtilitiesGetPacsInstance(),
                                                      PACS_SERVER_IS_AUDIO_SINK);
}

uint16 bapServerPacsUtilitiesGetSourceAudioContextAvailability(void)
{
    return GattPacsServerGetAudioContextsAvailability(bapServerPacsUtilitiesGetPacsInstance(),
                                                      PACS_SERVER_IS_AUDIO_SOURCE);
}

static void bapServerOnPacsConfigChangeInd(BAP *bapInst,
    GattPacsServerConfigChangeInd * ind)
{
    if(ind->cid)
    {
        bapServerSendConfigChangeInd(bapInst,
                                     ind->cid,
                                     BAP_SERVER_CONFIG_PACS,
                                     ind->configChangeComplete);
    }
}

static void bapServerOnPacsReadAvailableAudioContextInd(BAP *bapInst,
    GattPacsServerAvailableAudioContextReadInd * ind)
{
    if(ind->cid)
    {
        BapServerAvailableAudioContextReadInd * message = NULL;

        message = (BapServerAvailableAudioContextReadInd*)CsrPmemZalloc(sizeof(BapServerAvailableAudioContextReadInd));
        message->type = BAP_SERVER_AVAILABLE_AUDIO_CONTEXT_READ_IND;
        message->connectionId = ind->cid;

        BapServerMessageSend(bapInst->appUnicastTask, message);
    }
}

void bapServerHandleGattPacsServerMsg(BAP *bapInst, void *message)
{
    GattPacsServerMessageId *prim = (GattPacsServerMessageId *)message;

    BAP_SERVER_INFO("bapServerHandleGattPacsServerMsg MESSAGE:GattPacsServerMessageId:0x%x", *prim);

    switch (*prim)
    {
        case GATT_PACS_SERVER_CONFIG_CHANGE_IND:
            bapServerOnPacsConfigChangeInd(bapInst, (GattPacsServerConfigChangeInd *)message);
            break;
        case GATT_PACS_SERVER_AVAILABLE_AUDIO_CONTEXT_READ_IND:
            bapServerOnPacsReadAvailableAudioContextInd(bapInst,
                                       (GattPacsServerAvailableAudioContextReadInd *)message);
            break;
        default:
            BAP_SERVER_PANIC("PANIC: BAP Server: unhandled PACS prim\n");
            break;
    }
}

static PacsRecordHandle BapServerAddPacRecordInternal(bapProfileHandle profileHandle, PacsDirectionType pacsDirection,
                    const void * pacRecord, bool vendor_specifc)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    registeredPacsRecords * pacRecords = NULL;
    PacsRecordHandle recordHandle = 0;
    
    if (!bapInst)
    {
        BAP_SERVER_PANIC("BapServerAddPacRecord: Invalid BAP profile handle\n");
        return 0;
    }
    
    /* Check if we are modifying the Sink or Source PAC record structure. Panic if we can't tell. */
    if (pacsDirection == PACS_DIRECTION_AUDIO_SINK)
    {
        pacRecords = BapSinkRecords;
    }
    else if (pacsDirection == PACS_DIRECTION_AUDIO_SOURCE)
    {
        pacRecords = BapSourceRecords;
    }
    else
    {
        BAP_SERVER_PANIC("PANIC: BapServerAddPacRecord: Invalid PAC record\n");
    }

    /* At this point the 'pacRecords' pointer must be valid. Next we determine if we are allocating or re-allocating memory. */
    if (pacRecords == NULL)
    {
        pacRecords = (registeredPacsRecords *)(calloc(1, (sizeof(registeredPacsRecords) + sizeof(uint16))));
    }
    else
    {
        pacRecords = (registeredPacsRecords *)(realloc(pacRecords, (sizeof(registeredPacsRecords) + (sizeof(uint16) * (pacRecords->numberOfRegisteredRecords+1)))));
    }

    if (pacRecords)
    {
        if (vendor_specifc == FALSE)
        {
            pacRecords->pacRecordHandles[pacRecords->numberOfRegisteredRecords] =
                GattPacsServerAddPacRecord(bapInst->pacsHandle,
                    pacsDirection,
                    (GattPacsServerRecordType*)pacRecord);
        }
        else
        {
            pacRecords->pacRecordHandles[pacRecords->numberOfRegisteredRecords] =
                GattPacsServerAddVSPacRecord(bapInst->pacsHandle,
                    pacsDirection,
                    (BapServerVSPacsRecord*)pacRecord);

        }
        recordHandle = pacRecords->pacRecordHandles[pacRecords->numberOfRegisteredRecords];

        if (pacRecords->pacRecordHandles[pacRecords->numberOfRegisteredRecords] < PACS_RECORD_ERRORCODE_BASE)
        {
            pacRecords->numberOfRegisteredRecords++;
        }
        else
        {
            BAP_SERVER_PANIC("PANIC: BapServerAddPacRecord: Buffer overflow\n");
        }
    }

    /* Assign local PAC record pointer to static PAC struct. */
    if (pacsDirection == PACS_DIRECTION_AUDIO_SINK)
    {
        BapSinkRecords = pacRecords;
    }
    else if (pacsDirection == PACS_DIRECTION_AUDIO_SOURCE)
    {
        BapSourceRecords = pacRecords;
    }

    return recordHandle;
}


PacsRecordHandle BapServerAddPacRecord(bapProfileHandle profileHandle, PacsDirectionType pacsDirection,
                    const BapServerPacsRecord * pacRecord)
{
    return (BapServerAddPacRecordInternal(profileHandle, pacsDirection, (void*)pacRecord, FALSE));
}

PacsRecordHandle BapServerAddVSPacRecord(bapProfileHandle profileHandle,
                               PacsDirectionType pacsDirection,
                               const BapServerVSPacsRecord *pacVSRecord)
{
    return (BapServerAddPacRecordInternal(profileHandle, pacsDirection, (void*)pacVSRecord, TRUE));
}

const BapServerPacsRecord * BapServerGetPacRecord(bapProfileHandle profileHandle,
                                                  uint16 pacRecordHandle)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if (bapInst)
    {
        return (BapServerPacsRecord *)GattPacsServerGetPacRecord(bapInst->pacsHandle,
                                                             pacRecordHandle);
    }

    return NULL;
}

const BapServerVSPacsRecord * BapServerGetVSPacRecord(bapProfileHandle profileHandle,
                                                  uint16 pacRecordHandle)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if (bapInst)
    {
        return (BapServerVSPacsRecord *)GattPacsServerGetVSPacRecord(bapInst->pacsHandle,
                                                             pacRecordHandle);
    }
    return NULL;
}

bool BapServerAddPacAudioLocation(bapProfileHandle profileHandle, 
                                  PacsDirectionType pacsDirection,
                                  AudioLocationType audioLocations)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    PacsServerDirectionType direction = (pacsDirection == PACS_DIRECTION_AUDIO_SINK)? PACS_SERVER_IS_AUDIO_SINK : PACS_SERVER_IS_AUDIO_SOURCE;

    if (bapInst)
    {
    return GattPacsServerAddAudioLocation(bapInst->pacsHandle,
                                          direction,
                                          audioLocations);
    }

    return FALSE;
}

AudioLocationType BapServerGetPacAudioLocation(bapProfileHandle profileHandle,
                                               PacsDirectionType pacsDirection)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    PacsServerDirectionType direction = (pacsDirection == PACS_DIRECTION_AUDIO_SINK)? PACS_SERVER_IS_AUDIO_SINK : PACS_SERVER_IS_AUDIO_SOURCE;

    if (bapInst)
    {
        return GattPacsServerGetAudioLocation(bapInst->pacsHandle,
                                          direction);
    }

    return 0;
}

bool BapServerAddPacAudioContexts(bapProfileHandle profileHandle,
                                  PacsDirectionType pacsDirection,
                                  AudioContextType audioContext,
                                  PacAudioContextType contexts)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    PacsServerDirectionType direction = (pacsDirection == PACS_DIRECTION_AUDIO_SINK)? PACS_SERVER_IS_AUDIO_SINK : PACS_SERVER_IS_AUDIO_SOURCE;
    PacsServerAudioContextType context = (contexts == PACS_SUPPORTED_AUDIO_CONTEXTS)? PACS_SERVER_SUPPORTED_AUDIO_CONTEXTS : PACS_SERVER_AVAILABLE_AUDIO_CONTEXTS;

    if (bapInst)
    {
        return GattPacsServerAddAudioContexts(bapInst->pacsHandle,
                                          direction, audioContext,
                                          context);
    }

    return FALSE;
}

AudioContextType BapServerGetPacAvailableContexts(bapProfileHandle profileHandle,
                                                  PacsDirectionType pacsDirection)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    PacsServerDirectionType direction = (pacsDirection == PACS_DIRECTION_AUDIO_SINK)? PACS_SERVER_IS_AUDIO_SINK : PACS_SERVER_IS_AUDIO_SOURCE;

    if (bapInst)
    {
        return GattPacsServerGetAudioContextsAvailability(bapInst->pacsHandle,
                                                      direction);
    }

    return 0;
}

bool BapServerRemovePacRecord(bapProfileHandle profileHandle,
                              PacsRecordHandle pacRecordHandle)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if (bapInst)
    {
        return GattPacsServerRemovePacRecord(bapInst->pacsHandle,
                                         pacRecordHandle);
    }

    return FALSE;
}

bool BapServerRemoveVSPacRecord(bapProfileHandle profileHandle,
                              PacsRecordHandle pacRecordHandle)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if (bapInst)
    {
        return GattPacsServerRemoveVSPacRecord(bapInst->pacsHandle,
                                         pacRecordHandle);
    }
    return FALSE;
}

bool BapServerRemovePacAudioLocation(bapProfileHandle profileHandle,
                                     PacsDirectionType pacsDirection,
                                     AudioLocationType audioLocations)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    if (bapInst)
    {
        return GattPacsServerRemoveAudioLocation(bapInst->pacsHandle,
                                             pacsDirection,
                                             audioLocations);
    }

    return FALSE;
}

bool BapServerRemovePacAudioContexts(bapProfileHandle profileHandle,
                                     PacsDirectionType pacsDirection,
                                     AudioContextType audioContext,
                                     PacAudioContextType contexts)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    if (bapInst)
    {
        return GattPacsServerRemoveAudioContexts(bapInst->pacsHandle,
                                             pacsDirection,
                                             audioContext,
                                             (PacsAudioContextType)contexts);
    }

    return FALSE;
}

bapStatus BapServerAddPacsConfig(bapProfileHandle profileHandle,
                                 ConnId connectionId,
                                 BapPacsConfig * config)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    bapStatus status = BAP_SERVER_STATUS_SUCCESS;
    
    if ((bapInst) && bapServerAddConnectionIdToList(bapInst, connectionId))
    {
        if (GattPacsServerAddConfig(bapInst->pacsHandle,
                                    connectionId,
                                    (GattPacsServerConfigType*) config) != CSR_BT_RESULT_CODE_SUCCESS)
        {
            status = BAP_SERVER_STATUS_FAILED;
            return status;
        }
        bapServerAddConfigToConnection(bapInst, connectionId);
    }

    return status;
}

BapPacsConfig * BapServerRemovePacsConfig(bapProfileHandle profileHandle,
                                          ConnId connectionId)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    BapPacsConfig * config = NULL;
    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        config = (BapPacsConfig *)GattPacsServerRemoveConfig(bapInst->pacsHandle,
                                                             connectionId);
        bapServerRemoveConnectionIdFromList(bapInst, connectionId);
    }

    return config;
}

bool BapServerSetSelectiveAvailableAudioContexts(
                               bapProfileHandle profileHandle,
                               ConnId connectionId,
                               uint16 sinkAudioContexts,
                               uint16 sourceAudioContexts)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        return GattPacsServerSetSelectiveAvailableAudioContexts(bapInst->pacsHandle,
                            connectionId, sinkAudioContexts, sourceAudioContexts);
    }
    return FALSE;
}

bool BapServerClearSelectiveAvailableAudioContexts(
                               bapProfileHandle profileHandle,
                               ConnId connectionId)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        return GattPacsServerClearSelectiveAvailableAudioContexts(bapInst->pacsHandle,
                                                        connectionId);
    }
    return FALSE;
}

void BapServerEnableAvailableAudioContextControl(bapProfileHandle profileHandle)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    if (bapInst)
    {
        GattPacsServerEnableAvailableAudioContextControl(bapInst->pacsHandle);
    }
}

bool BapServerAvailableAudioContextReadResponse(bapProfileHandle profileHandle,
                               ConnId connectionId,
                               AudioContextType sinkAudioContexts,
                               AudioContextType sourceAudioContexts)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        return GattPacsServerAvailableAudioContextReadResponse(
                                            bapInst->pacsHandle,
                                            connectionId,
                                            sinkAudioContexts,
                                            sourceAudioContexts);
    }
    return FALSE;
}


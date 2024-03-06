/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd.
* 
************************************************************************* ***/
#include "bap_server_private.h"
#include "bap_server_msg_handler.h"
#include "bap_server_debug.h"
#include "bap_server_init.h"
#include "bap_server_common.h"
#include "gatt_pacs_server.h"
#include "bap_server_pacs.h"
#include "bap_server.h"
#include "service_handle.h"

registeredPacsRecords * BapSinkRecords = NULL;
registeredPacsRecords * BapSourceRecords = NULL;

static bool bapServerPacsUtilitiesIsCodecConfigSupportedBySink(ServiceHandle handle,
                                                               uint8 coding_format,
                                                               uint16 company_id,
                                                               uint16 vendor_specific_codec_id)
{
    bool is_supported = FALSE;
    for(int i = 0; ((i < BapSinkRecords->numberOfRegisteredRecords) && (is_supported == FALSE)); i++)
    {
        BAP_DEBUG_INFO(("bapServerPacsUtilitiesIsCodecConfigSupportedBySink checking handle[%d]=0x%x", i, BapSinkRecords->pacRecordHandles[i]));
        const GattPacsServerRecordType * pac_record = GattPacsServerGetPacRecord( handle, BapSinkRecords->pacRecordHandles[i]);
        if(pac_record)
        {
            if(coding_format == pac_record->codecId
                && company_id == pac_record->companyId
                && vendor_specific_codec_id == pac_record->vendorCodecId)
            {
                is_supported = TRUE;
                break;
            }
        }
        else
        {
            BAP_DEBUG_INFO(("bapServerPacsUtilitiesIsCodecConfigSupportedBySink no record found handle[%d]=0x%x",
                i, BapSinkRecords->pacRecordHandles[i]));
        }
    }
    return is_supported;
}

static bool bapServerPacsUtilitiesIsCodecConfigSupportedBySource(ServiceHandle handle,
                                                                 uint8 coding_format,
                                                                 uint16 company_id,
                                                                 uint16 vendor_specific_codec_id)
{
    bool is_supported = FALSE;
    for(int i = 0; ((i < BapSourceRecords->numberOfRegisteredRecords) && (is_supported == FALSE)); i++)
    {
        BAP_DEBUG_INFO(("bapServerPacsUtilitiesIsCodecConfigSupportedBySource checking handle[%d]=0x%x", 
            i, BapSourceRecords->pacRecordHandles[i]));

        const GattPacsServerRecordType * pac_record = GattPacsServerGetPacRecord( handle, BapSourceRecords->pacRecordHandles[i]);
        if(pac_record)
        {
            if(coding_format == pac_record->codecId
                && company_id == pac_record->companyId
                && vendor_specific_codec_id == pac_record->vendorCodecId)
            {
                is_supported = TRUE;
                break;
            }
        }
        else
        {
            BAP_DEBUG_INFO(("bapServerPacsUtilitiesIsCodecConfigSupportedBySource no record found handle[%d]=0x%x",
                               i, BapSourceRecords->pacRecordHandles[i]));
        }
    }
    return is_supported;
}

bool bapServerIsCodecConfigSuppported(ServiceHandle handle,
                                      GattAscsServerConfigureCodecClientInfo * client_codec)
{
    bool supported = FALSE;

    if (client_codec->direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK)
    {
        supported = bapServerPacsUtilitiesIsCodecConfigSupportedBySink(handle,
                                                                       client_codec->codecId.codingFormat,
                                                                       client_codec->codecId.companyId,
                                                                       client_codec->codecId.vendorSpecificCodecId);
    }
    else if (client_codec->direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE)
    {
        supported = bapServerPacsUtilitiesIsCodecConfigSupportedBySource(handle,
                                                                         client_codec->codecId.codingFormat,
                                                                         client_codec->codecId.companyId,
                                                                         client_codec->codecId.vendorSpecificCodecId);
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

void bapServerHandleGattPacsServerMsg(BAP *bapInst, void *message)
{
   UNUSED(bapInst);
   UNUSED(message);
}

PacsRecordHandle BapServerAddPacRecord(bapProfileHandle profileHandle, PacsDirectionType pacsDirection,
                    const BapServerPacsRecord * pacRecord)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    registeredPacsRecords * pacRecords = NULL;
    PacsRecordHandle recordHandle = 0;

    if (!bapInst)
    {
        BAP_DEBUG_PANIC(("PANIC: BapServerAddPacRecord: Invalid BAP profile handle\n"));
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
        BAP_DEBUG_PANIC(("PANIC: BapServerAddPacRecord: Invalid PAC record\n"));
    }

    /* At this point the 'pacRecords' pointer must be valid. Next we determine if we are allocating or re-allocating memory. */
    if (pacRecords == NULL)
    {
        pacRecords = (registeredPacsRecords *)PanicNull(calloc(1, (sizeof(registeredPacsRecords) + sizeof(uint16))));
    }
    else
    {
        pacRecords = (registeredPacsRecords *)PanicNull(realloc(pacRecords, (sizeof(registeredPacsRecords) + (sizeof(uint16) * (pacRecords->numberOfRegisteredRecords+1)))));
    }

    pacRecords->pacRecordHandles[pacRecords->numberOfRegisteredRecords] =
            GattPacsServerAddPacRecord(bapInst->pacsHandle,
                                       pacsDirection,
                                       (GattPacsServerRecordType *)pacRecord);
    recordHandle = pacRecords->pacRecordHandles[pacRecords->numberOfRegisteredRecords];

    if (pacRecords->pacRecordHandles[pacRecords->numberOfRegisteredRecords] < PACS_RECORD_ERRORCODE_BASE)
    {
        BAP_DEBUG_INFO(("BapServerAddPacRecord: Registered Records=0x%x New Handle=0x%x",
                           pacRecords->numberOfRegisteredRecords,
                           pacRecords->pacRecordHandles[pacRecords->numberOfRegisteredRecords]));
        pacRecords->numberOfRegisteredRecords++;
    }
    else
    {
        BAP_DEBUG_PANIC(("PANIC: BapServerAddPacRecord: Buffer overflow\n"));
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
    else
    {
        BAP_DEBUG_PANIC(("PANIC: BapServerAddPacRecord: Invalid PAC record\n"));
    }

    return recordHandle;
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
    else
    {
        BAP_DEBUG_INFO(("BapServerGetPacRecord: BAP instance is null\n"));
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
    else
    {
        BAP_DEBUG_INFO(("BapServerAddPacAudioLocation: BAP instance is null\n"));
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
    else
    {
        BAP_DEBUG_INFO(("BapServerGetPacAudioLocation: BAP instance is null\n"));
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
    else
    {
        BAP_DEBUG_INFO(("BapServerAddPacAudioContexts: BAP instance is null\n"));
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
    else
    {
        BAP_DEBUG_INFO(("BapServerGetPacAvailableContexts: BAP instance is null\n"));
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
    else
    {
        BAP_DEBUG_INFO(("BapServerRemovePacRecord: BAP instance is null\n"));
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
    else
    {
        BAP_DEBUG_INFO(("BapServerRemovePacAudioLocation: BAP instance is null\n"));
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
    else
    {
        BAP_DEBUG_INFO(("BapServerRemovePacAudioContexts: BAP instance is null\n"));
    }

    return FALSE;
}

bapStatus BapServerAddPacsConfig(bapProfileHandle profileHandle,
                                ConnId connectionId,
                                BapPacsConfig * config)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    bapStatus status = BAP_SERVER_STATUS_SUCCESS;
    
    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        if(GattPacsServerAddConfig(bapInst->pacsHandle,
                                         connectionId,
                                         (GattPacsServerConfigType*) config) != gatt_status_success) 
        {
            status = BAP_SERVER_STATUS_FAILED;
            BAP_DEBUG_PANIC(("PANIC: BapServerAddPacsConfig add PACS config failed status=%d", status));
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
        BAP_DEBUG_INFO(("BapServerRemovePacsConfig cid=0x%x", connectionId));
        bapServerRemoveConnectionIdFromList(bapInst, connectionId);
    }

    return config;
}


/*******************************************************************************

Copyright (C) 2019-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP GATT Message Handler interface implementation.
 */

/**
 * \addtogroup BAP
 * @{
 */

#include <stdio.h>
#include <string.h>
#include "bap_client_list_container_cast.h"
#include "bap_stream_group.h"
#include "bap_utils.h"
#include "bap_client_lib.h"
#include "bap_connection.h"
#include "bap_client_connection.h"
#include "bap_client_debug.h"

#include "bap_client_list_util_private.h"
#include "bap_gatt_msg_handler.h"
#include "gatt_ascs_client_private.h"

#include "csr_bt_tasks.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
static bool bapClientUpdateCodecSpecificConfigValue(BapPacRecord* record,
                                                    uint8 length,
                                                    uint8 type,
                                                    uint8* value);

static bool bapClientParsePacRecords(GattPacsClientRecord* pacRecord,
                                     BapPacRecord* bapPacRecord);


void bapClientGattAseDiscovery(BapConnection * const connection,
                               uint8 aseId,
                               BapAseType aseType)
{
#ifdef adk_gatt
    if(ascs_client->ases_ase_handle != 0xffff)
        GattAscsReadAseAudioInfoReq(ascs_client, ascs_client->ases_ase_handle);
#endif
    if(aseId == ASE_ID_ALL)
    {
        if(!GattAscsReadAseInfoReq(connection->ascs.srvcHndl, (AseCharType)aseType))
        {
            bapClientConnectionHandleReadAseInfoCfm(connection,
                                                    0,
                                                    NULL,
                                                    0,
													aseType);
        }
    }
    else
    {
        GattAscsClientReadAseStateReq(connection->ascs.srvcHndl, aseId, (AseCharType)aseType);
    }
}

void bapClientGattRegisterAseNotification(BapConnection * const connection,
                                          uint8 aseId,
                                          bool notifyType)
{
    if(aseId != INVALID_ASE_ID)
    {
        GattAscsRegisterForNotification(connection->ascs.srvcHndl,
                                        aseId,
                                        notifyType);
    }
}

void bapClientGattInitCfm(BAP * const bap,
                          GattAscsClientInitCfm *cfm)
{
    BapResult result = BAP_RESULT_ERROR;
    BapConnection *connection = NULL;

    if (bapClientFindConnectionByCid(bap, cfm->cid, &connection))
    {
        result = (cfm->status == GATT_ASCS_CLIENT_STATUS_SUCCESS) ? BAP_RESULT_SUCCESS : BAP_RESULT_ERROR;
        connection->bapInitStatus |= result;

        BAP_CLIENT_INFO("\n(BAP):ASCS clientHandle: %x", cfm->clientHandle);
        /* Update Service handle on success */
        if (result == BAP_RESULT_SUCCESS)
            connection->ascs.srvcHndl = cfm->clientHandle;

#ifdef adk
            /*GattAscsRegisterForNotification(connection->ascs.srvc_hndl,
                                        ascsc->ases_ase_ccd_handle, TRUE);*/
#endif

        connection->numService--;
        /* verify if all the Discovered mendatory Services are initialised */
        if(connection->numService == 0)
        {
            if ( connection->bapInitStatus != BAP_RESULT_SUCCESS)
                bapUtilsCleanupBapConnection(connection);

            bapUtilsSendBapInitCfm(connection->rspPhandle,
                                   cfm->cid,
                                   connection->bapInitStatus,
                                   connection->role);
        }
    }
}

void bapClientGattHandleAsesReadCfm(BAP * const bap,
                                    GattAscsClientReadAseInfoCfm *cfm)
{
    (void)bap;
    if (cfm->status == GATT_ASCS_CLIENT_STATUS_SUCCESS )
    {
        BapConnection *connection = NULL;
        if (bapClientFindConnectionByAscsSrvcHndl(bap, cfm->clientHandle, &connection))
        {
            if(cfm->noOfAse > 0)
            {
                uint8 numberOfAses = (CsrUint8) cfm->noOfAse;
                bapClientConnectionHandleReadAseInfoCfm(connection,
                                                        numberOfAses,
                                                        &cfm->aseId[0],
                                                        0,
                                                        (BapAseType)cfm->charType);
            }
        }
    }
}

void bapClientGattHandleAsesReadStateCfm(BAP * const bap,
                                         GattAscsClientReadAseStateCfm *cfm)
{
    (void)bap;
    if (cfm->status == GATT_ASCS_CLIENT_STATUS_SUCCESS )
    {
        BapConnection *connection = NULL;
        if (bapClientFindConnectionByAscsSrvcHndl(bap, cfm->clientHandle, &connection))
        {
            if(cfm->sizeValue >= 2)
            {
                uint8 aseState = (CsrUint8) cfm->value[1];
                uint8 *aseInfo = NULL;

                if(cfm->sizeValue > 2)
                    aseInfo = &cfm->value[2];

                bapUtilsSendGetRemoteAseInfoCfm(connection->rspPhandle,
                                                BAP_RESULT_SUCCESS,
                                                connection->cid,
                                                1,
                                                &cfm->value[0],
                                                aseState,
                                                0,
                                                (cfm->sizeValue-2),
                                                aseInfo);
            }
        }
    }
}

void bapClientGattHandleAseWriteCfm(BAP * const bap,
                                    GattAscsClientWriteAseCfm *cfm)
{
    (void)bap;
    if (cfm->status == GATT_ASCS_CLIENT_STATUS_SUCCESS )
    {
        BapConnection *connection = NULL;
        if (bapClientFindConnectionByAscsSrvcHndl(bap, cfm->clientHandle, &connection))
        {
            bapUtilsSendRegisterAseCfm(connection->rspPhandle,
                                       cfm->status,
                                       cfm->aseId,
                                       connection->cid);
        }
    }
}

void bapClientGattHandleNotification(BAP * const bap,
                                     GattAscsClientIndicationInd *ind)
{
    BapConnection* connection = NULL;
    BapAse* ase = NULL;
    (void)bap;

    /* ASCS Notification minimum value length size check*/
    if(ind->sizeValue < ASE_CP_CMD_HDR_SIZE)
    {
        return;
    }

    bapClientFindConnectionByAscsSrvcHndl(bap, ind->clientHandle, &connection);

    if(ind->aseId) /* ASE Notification */
    {
        if (connection)
        {
            ase = bapConnectionFindAseByAseId(connection, ind->aseId);
        }

        if (ase && (ind->sizeValue > 0))
        {
            BapClientConnection* client_connection = CONTAINER_CAST(connection,
                                        BapClientConnection, connection);
            bapClientConnectionRcvAscsMsg(client_connection, ind->value);
        }
    }
    else /* ASE Control Point Notification */
    {
        uint8 aseId = 0;

        if(ind->sizeValue > ASE_CP_NOTIFY_ASE_ID_OFFSET)
        {
            aseId = ind->value[ASE_CP_NOTIFY_ASE_ID_OFFSET];
        }

        if (connection)
        {
            ase = bapConnectionFindAseByAseId(connection, aseId);
        }

        if (ase && (ind->sizeValue > ASE_CP_NOTIFY_ASE_ID_OFFSET))
        {
            BapClientConnection* client_connection = CONTAINER_CAST(connection,
                                        BapClientConnection, connection);
            bapStreamGroupHandleAscsCpMsg(client_connection->streamGroup,
                                          ind->value);
        }
    }

    if( ind->sizeValue && ind->value)
    {
        CsrPmemFree(ind->value);
    }
}


void bapClientGattPacsClientInitCfm(BAP* const bap,
                                    GattPacsClientInitCfm* cfm)
{
    BapConnection* connection = NULL;
    BapResult result = BAP_RESULT_ERROR;

    if (bapClientFindConnectionByCid(bap, cfm->cid, &connection))
    {
        result = (cfm->status == GATT_PACS_CLIENT_STATUS_SUCCESS) ? BAP_RESULT_SUCCESS : BAP_RESULT_ERROR;
        connection->bapInitStatus |= result;

        BAP_CLIENT_INFO("\n(BAP):PACS clientHandle: 0x%x \n", cfm->clientHandle);
        /* Update Service handle on success */
        if (result == BAP_RESULT_SUCCESS)
            connection->pacs.srvcHndl = cfm->clientHandle;

        bap->controller.state = BAP_CONTROLLER_STATE_CONNECTED;

        connection->numService--;
        /* verify if all the Discovered mendatory Services are initialised */
        if(connection->numService == 0)
        {
            if ( connection->bapInitStatus != BAP_RESULT_SUCCESS)
                bapUtilsCleanupBapConnection(connection);

            bapUtilsSendBapInitCfm(connection->rspPhandle,
                                   connection->cid,
                                   connection->bapInitStatus,
                                   connection->role);
        }
    }
}

void bapClientGattHandleRegisterPacsNotificationCfm(BAP* const bap,
                                                    GattPacsClientNotificationCfm* cfm)
{
    BapConnection* connection = NULL;
    BapResult result = BAP_RESULT_ERROR;

    if (bapClientFindConnectionByPacsSrvcHndl(bap, cfm->clientHandle, &connection))
    {
        result = (cfm->status == GATT_PACS_CLIENT_STATUS_SUCCESS) ? BAP_RESULT_SUCCESS : BAP_RESULT_ERROR;

        bapUtilsSendRegisterPacsNotificationCfm(connection->rspPhandle,
                                                connection->cid,
                                                result);
    }
}

static bool bapClientUpdateCodecSpecificConfigValue(BapPacRecord* record,
                                                    uint8 length,
                                                    uint8 type,
                                                    uint8* value)
{
    bool status = FALSE;

    /* Fill default value for supported_max_frames_per_sdu
    * In the absence of the Supported_Max_Codec_Frames_Per_SDU shall be
    filled as equivalent to a Supported_Max_Codec_Frames_Per_SDU value of
    1 codec frame per audio channel per SDU maximum.
    */

    record->supportedMaxCodecFramesPerSdu = 1;
    record->vendorSpecificConfigLen = 0;
    record->vendorSpecificConfig = NULL;

    switch (type)
    {
        case BAP_PAC_SAMPLING_FREQUENCY_TYPE:
            {
                if (length == BAP_PAC_SAMPLING_FREQUENCY_LENGTH)
                {
                    record->samplingFrequencies = value[0] | (value[1] << 8);
                    status = TRUE;
                }
            }
            break;
        case BAP_PAC_SUPPORTED_FRAME_DURATION_TYPE:
            {
                if (length == BAP_PAC_SUPPORTED_FRAME_DURATION_LENGTH)
                {
                    record->frameDuaration = value[0];
                    status = TRUE;
                }
            }
            break;
        case BAP_PAC_AUDIO_CHANNEL_COUNTS_TYPE:
            {
                if (length == BAP_PAC_AUDIO_CHANNEL_COUNTS_LENGTH)
                {
                    record->channelCount = value[0];
                    status = TRUE;
                }
            }
            break;
        case BAP_PAC_SUPPORTED_OCTETS_PER_CODEC_FRAME_TYPE:
            {
                if (length == BAP_PAC_SUPPORTED_OCTETS_PER_CODEC_FRAME_LENGTH)
                {
                    record->minOctetsPerCodecFrame = value[0] | (value[1] << 8);
                    record->maxOctetsPerCodecFrame = value[2] | (value[3] << 8);
                    status = TRUE;
                }
            }
            break;
        case BAP_PAC_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU_TYPE:
            {
                if (length == BAP_PAC_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU_LEN)
                {
                    record->supportedMaxCodecFramesPerSdu = value[0];
                    status = TRUE;
                }
            }
            break;
        default:
            BAP_CLIENT_WARNING("(bap_client_gatt) Unhandled type: %d", type);
            break;
    }
    return status;
}


static bool bapClientUpdateMetadata(BapPacRecord* record,
                                    uint8 length,
                                    uint8 type,
                                    uint8* value)
{
    bool status = FALSE;

    switch (type)
    {
        case BAP_PAC_PREFERRED_AUDIO_CONTEXT:
        {
            if (length == BAP_PAC_PREFERRED_AUDIO_CONTEXT_LEN)
            {
                record->preferredAudioContext = value[0] | (value[1] << 8);
                status = TRUE;
            }
            break;
        }
        case BAP_PAC_STREAMING_AUDIO_CONTEXT:
        {
            if (length == BAP_PAC_STREAMING_AUDIO_CONTEXT_LEN)
            {
                record->streamingAudioContext = value[0] | (value[1] << 8);
                status = TRUE;
            }
            break;
        }
        case BAP_PAC_VENDOR_SPECIFIC_METADATA:
        {
            if (length)
            {
                record->vendorSpecificMetadataLen = length - 1;
                record->vendorSpecificMetadata = (uint8*)CsrPmemZalloc(length - 1);
                memcpy(record->vendorSpecificMetadata, value, length - 1);
                status = TRUE;
            }
            break;
        }
        default:
            break;
    }

    return status;
}

static void bapClientInitializeMetadata(BapPacRecord* record)
{
    record->vendorSpecificMetadataLen = 0;
    record->preferredAudioContext = 0;
    record->streamingAudioContext = 0;
}

static bool bapClientParsePacRecords(GattPacsClientRecord* pacRecord,
                                     BapPacRecord* bapPacRecord)
{
    bool status = TRUE;
    uint8 i;
    uint8 offset = 0;
    uint8 configLength = 0;
    uint8 configType = 0;
    uint8 configTotalLength = 0;
    uint8 metadataType = 0;
    uint8 metadataLength = 0;
    uint8 metadataTotalLength = 0;

    for (i = 0; i < pacRecord->pacRecordCount; i++)
    {
        bapPacRecord[i].codecId = pacRecord->value[offset++];
        bapPacRecord[i].companyId = pacRecord->value[offset] | (pacRecord->value[offset+1] << 8);
        offset += 2;
        bapPacRecord[i].vendorCodecId = pacRecord->value[offset] | (pacRecord->value[offset+1] << 8);
        offset += 2;
        /* Parsing for Codec Specific configurations */
        configTotalLength = pacRecord->value[offset++];

        if (bapPacRecord[i].codecId == BAP_CODEC_ID_LC3)
        {

            while (configTotalLength)
            {
                /* LTV structure processing */
                /* Config length */
                configLength = pacRecord->value[offset++];
                /* Config type */
                configType = pacRecord->value[offset];
                /* Config value */
                if (bapClientUpdateCodecSpecificConfigValue(&bapPacRecord[i], configLength, configType, &pacRecord->value[offset + 1]))
                {
                    offset += configLength;
                }
                else
                {
                    status = FALSE;
                    break;
                }
                /* Update config total length */
                if (configTotalLength >= (configLength + 1))
                    configTotalLength -= (configLength + 1);
                else
                    configTotalLength = 0;
            }
        }
        else if(bapPacRecord[i].codecId == BAP_CODEC_ID_VENDOR_DEFINED)
        {
            /*Do not Parse Vendor Specific Codec */

            /* Vendor specific codec is packed in vendorSpecificConfig field
             *  of BapClientPacRecord and send for app to decode
             */

            if (bapPacRecord->companyId == BAP_COMPANY_ID_QUALCOMM)
            {
                bapPacRecord[i].vendorSpecificConfigLen = configTotalLength;

                if (configTotalLength)
                {
                    bapPacRecord[i].vendorSpecificConfig = (uint8*)CsrPmemZalloc(configTotalLength);
                    CsrMemCpy(bapPacRecord[i].vendorSpecificConfig, &(pacRecord->value[offset]), configTotalLength);
                    offset += configTotalLength;
                }
            }
            else
            {
                /* Ignore Vendor specific configuration */
                status = TRUE;
                break;
            }
        }
        else
        {
            status = FALSE;
            break;
        }

        if ((bapPacRecord->companyId == BAP_COMPANY_ID_QUALCOMM)
                     || (bapPacRecord->codecId == BAP_CODEC_ID_LC3))
        {
            metadataTotalLength = pacRecord->value[offset++];

            /* Initialize metadata */
            bapClientInitializeMetadata(&bapPacRecord[i]);

            /* Parse Metadata*/
            while (metadataTotalLength)
            {
                /* Metadata LTV structure processing */

                /* Metadata length*/
                metadataLength = pacRecord->value[offset++];
                /* Metadata type*/
                metadataType = pacRecord->value[offset];
                /* Metadata value */
                if (bapClientUpdateMetadata(&bapPacRecord[i], metadataLength, metadataType, &pacRecord->value[offset + 1]))
                {
                    offset += metadataLength;
                }
                else
                {
                    status = FALSE;
                    break;
                }

                if (metadataTotalLength >= (metadataLength + 1))
                    metadataTotalLength -= (metadataLength + 1);
                else
                    metadataTotalLength = 0;
            }
        }
    }
    return status;
}

void bapClientGattHandleReadPacRecordCfm(BAP* const bap,
                                         GattPacsClientReadPacRecordCfm* cfm)
{
    BapConnection* connection = NULL;
    BapResult result = BAP_RESULT_ERROR;
    BapPacRecordType recordType;
    BapPacRecords   pacRecords;
    bool status = FALSE;

    if (bapClientFindConnectionByPacsSrvcHndl(bap, cfm->clientHandle, &connection))
    {
        result = (cfm->status == GATT_PACS_CLIENT_STATUS_SUCCESS) ? BAP_RESULT_SUCCESS : BAP_RESULT_ERROR;
        recordType = (cfm->type == GATT_PACS_CLIENT_SINK) ? BAP_AUDIO_SINK_RECORD : BAP_AUDIO_SOURCE_RECORD;

        pacRecords.numPacRecords = cfm->record.pacRecordCount;

        if (pacRecords.numPacRecords > 0)
        {
            pacRecords.pacRecords = CsrPmemZalloc(pacRecords.numPacRecords * sizeof(BapPacRecord));
        }
        else
        {
            pacRecords.pacRecords = NULL;
        }

        status = bapClientParsePacRecords(&cfm->record, pacRecords.pacRecords);
       
        if (result == BAP_RESULT_SUCCESS)
            result = status ? BAP_RESULT_SUCCESS : BAP_RESULT_NOT_SUPPORTED;

        bapUtilsSendDiscoverRemoteAudioCapabilityCfm(connection->rspPhandle,
                                                     connection->cid,
                                                     result,
                                                     recordType,
                                                     pacRecords.numPacRecords,
                                                     pacRecords.pacRecords,
                                                     cfm->moreToCome);
    }
}

void bapClientGattHandleReadAudioLocationCfm(BAP* const bap,
                                             GattPacsClientReadAudioLocationCfm* cfm)
{
    BapConnection* connection = NULL;
    BapResult result = BAP_RESULT_ERROR;
    BapServerDirection direction;

    if (bapClientFindConnectionByPacsSrvcHndl(bap, cfm->clientHandle, &connection))
    {
        result = (cfm->status == GATT_PACS_CLIENT_STATUS_SUCCESS) ? BAP_RESULT_SUCCESS : BAP_RESULT_ERROR;
        direction = (cfm->type == GATT_PACS_CLIENT_SINK) ? BAP_SERVER_DIRECTION_SINK : BAP_SERVER_DIRECTION_SOURCE;

        bapUtilsSendGetRemoteAudioLocationCfm(connection->rspPhandle,
                                              connection->cid,
                                              result,
                                              direction,
                                              cfm->location);
    }
}

void bapClientGattHandleWriteAudioLocationCfm(BAP* const bap,
                                              GattPacsClientWriteAudioLocationCfm* cfm)
{
    BapConnection* connection = NULL;
    BapResult result = BAP_RESULT_ERROR;

    if (bapClientFindConnectionByPacsSrvcHndl(bap, cfm->clientHandle, &connection))
    {
        result = (cfm->status == GATT_PACS_CLIENT_STATUS_SUCCESS) ? BAP_RESULT_SUCCESS : BAP_RESULT_ERROR;

        bapUtilsSendSetRemoteAudioLocationCfm(connection->rspPhandle,
                                              connection->cid,
                                              result);
    }
}

void bapClientGattHandleReadAudioContextCfm(BAP* const bap,
                                            GattPacsClientReadAudioContextCfm* cfm)
{
    BapConnection* connection = NULL;
    BapResult result = BAP_RESULT_ERROR;
    BapPacAudioContext context;
    BapPacAudioContextValue value;

    if (bapClientFindConnectionByPacsSrvcHndl(bap, cfm->clientHandle, &connection))
    {
        result = (cfm->status == GATT_PACS_CLIENT_STATUS_SUCCESS) ? BAP_RESULT_SUCCESS : BAP_RESULT_ERROR;
        context = (cfm->context == GATT_PACS_CLIENT_AVAILABLE) ? BAP_PAC_AVAILABLE_AUDIO_CONTEXT : BAP_PAC_SUPPORTED_AUDIO_CONTEXT;

        value.sinkContext = cfm->value & 0x0000FFFF;
        value.sourceContext = (cfm->value >> 16) & 0x0000FFFF;

        bapUtilsSendDiscoverAudioContextCfm(connection->rspPhandle,
                                            connection->cid,
                                            result,
                                            context,
                                            value);
    }
}


void bapClientGattHandlePacRecordNotificationInd(BAP* const bap,
                                                 GattPacsClientPacRecordNotificationInd* ind)
{
    BapConnection* connection = NULL;
    BapPacsNotificationType notifyType;
    BapPacRecords   pacRecords;

    if (bapClientFindConnectionByPacsSrvcHndl(bap, ind->clientHandle, &connection))
    {
        notifyType = (ind->type == GATT_PACS_CLIENT_SINK) ?
            BAP_PACS_NOTIFICATION_SINK_PAC_RECORD : BAP_PACS_NOTIFICATION_SOURCE_PAC_RECORD;

        pacRecords.numPacRecords = ind->record.pacRecordCount;
        pacRecords.pacRecords = CsrPmemAlloc(pacRecords.numPacRecords * sizeof(BapPacRecord));

        if (bapClientParsePacRecords(&ind->record, pacRecords.pacRecords))
            bapUtilsSendPacRecordNotificationInd(connection->rspPhandle,
                                                 connection->cid,
                                                 notifyType,
                                                 pacRecords.pacRecords,
                                                 pacRecords.numPacRecords);
    }
}

void bapClientGattHandleAudioLocationNotificationInd(BAP* const bap,
                                                      GattPacsClientAudioLocationNotificationInd* ind)
{
    BapConnection* connection = NULL;
    BapPacsNotificationType notifyType;

    if (bapClientFindConnectionByPacsSrvcHndl(bap, ind->clientHandle, &connection))
    {
        notifyType = (ind->type == GATT_PACS_CLIENT_SINK) ?
            BAP_PACS_NOTIFICATION_SINK_AUDIO_LOCATION : BAP_PACS_NOTIFICATION_SOURCE_AUDIO_LOCATION;

        bapUtilsSendPacsNotificationInd(connection->rspPhandle,
                                        connection->cid,
                                        notifyType,
                                        (void*)&ind->location);
    }
}

void bapClientGattHandleAudioContextNotificationInd(BAP* const bap,
                                                    GattPacsClientAudioContextNotificationInd* ind)
{
    BapConnection* connection = NULL;
    BapPacsNotificationType notifyType;

    if (bapClientFindConnectionByPacsSrvcHndl(bap, ind->clientHandle, &connection))
    {
        notifyType = (ind->context == GATT_PACS_CLIENT_AVAILABLE) ?
            BAP_PACS_NOTIFICATION_AVAILABLE_AUDIO_CONTEXT : BAP_PACS_NOTIFICATION_SUPPORTED_AUDIO_CONTEXT;

        bapUtilsSendPacsNotificationInd(connection->rspPhandle,
                                        connection->cid,
                                        notifyType,
                                        (void*)&ind->contextValue);
    }
}
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
/** @}*/

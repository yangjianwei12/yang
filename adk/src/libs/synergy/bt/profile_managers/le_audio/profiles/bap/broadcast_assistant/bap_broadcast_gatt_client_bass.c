/*******************************************************************************

Copyright (C) 2019-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP Broadcast Assistant GATT Message Handler interface implementation.
 */

/**
 * \addtogroup BAP
 * @{
 */

#include <stdio.h>
#include <string.h>
#include "bap_client_list_container_cast.h"
#include "bap_utils.h"
#include "bap_client_debug.h"

#include "csr_bt_tasks.h"

#include "bap_broadcast_assistant.h"
#include "bap_broadcast_assistant_utils.h"
#include "bap_broadcast_src.h"
#include "bap_broadcast_gatt_client_bass.h"

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT

static Bool findConnectionByBassSrvcHndl(BAP* const bap,
                                         ServiceHandle bass_srvc_hndl,
                                         struct BapConnection** const connection)
{
    BapClientListElement* listElement;

    for (listElement = bapClientListPeekFront(&bap->connectionList);
        listElement != NULL;
        listElement = bapClientListElementGetNext(listElement))
    {
        *connection = CONTAINER_CAST(listElement, BapConnection, listElement);

        if ((*connection)->bass.srvcHndl == bass_srvc_hndl)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static void bapGattBassClientInitCfm(BAP* const bap,
                                     GattBassClientInitCfm* cfm)
{
    BapConnection* connection = NULL;
    BapResult result = BAP_RESULT_ERROR;

    if (bapClientFindConnectionByCid(bap, cfm->cid, &connection))
    {
        result = (cfm->status == GATT_BASS_CLIENT_STATUS_SUCCESS) ?
                            BAP_RESULT_SUCCESS : BAP_RESULT_ERROR;

        connection->bapInitStatus |= result; 
        /* Update Service handle on success */
        if (result == BAP_RESULT_SUCCESS)
            connection->bass.srvcHndl = cfm->clntHndl;

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

static void bapGattBassClientBroadcastReceiveStateSetNtfCfm(BAP* const bap,
                                                            GattBassClientBroadcastReceiveStateSetNtfCfm* cfm)
{
    BapConnection* connection = NULL;
    BapResult result = BAP_RESULT_ERROR;

    if (findConnectionByBassSrvcHndl(bap, cfm->clntHndl, &connection))
    {
        result = (cfm->status == GATT_BASS_CLIENT_STATUS_SUCCESS) ?
                             BAP_RESULT_SUCCESS : BAP_RESULT_ERROR;

        bapBroadcastAssistantUtilsSendBroadcastReceiveStateSetNtfCfm(connection->rspPhandle,
                                                                     connection->cid,
                                                                     result,
                                                                     cfm->sourceId);
    }
}

static void bapGattBassClientReadBroadcastReceiveStateCccCfm(BAP* const bap,
                                                             GattBassClientReadBroadcastReceiveStateCccCfm* cfm)
{
    BapConnection* connection = NULL;
    BapResult result = BAP_RESULT_ERROR;

    if (findConnectionByBassSrvcHndl(bap, cfm->clntHndl, &connection))
    {
        uint8 *value = NULL;
        result = (cfm->status == GATT_BASS_CLIENT_STATUS_SUCCESS) ?
                             BAP_RESULT_SUCCESS : BAP_RESULT_ERROR;

        if (cfm->sizeValue != 0)
        {
            value = CsrPmemZalloc(cfm->sizeValue*sizeof(uint8));
            memcpy(value, &(cfm->value[0]), cfm->sizeValue*sizeof(uint8));
        }

        bapBroadcastAssistantUtilsSendReadBroadcastReceiveStateCccCfm(connection->rspPhandle,
                                                                      connection->cid,
                                                                      result,
                                                                      cfm->sourceId,
                                                                      cfm->sizeValue,
                                                                      value);
    }
}

static void bapGattBassClientBroadcastReceiveStateInd(BAP* const bap,
                                                      GattBassClientBroadcastReceiveStateInd* ind)
{
    BapConnection* connection = NULL;

    if (findConnectionByBassSrvcHndl(bap, ind->clntHndl, &connection))
    {
        uint8 *badCode = NULL;
        BapSubgroupInfo *subGroupInfo = NULL;

        if (ind->brsValue.bigEncryption == BAP_ASSISTANT_BAD_CODE)
        {
            badCode = CsrPmemZalloc(BAP_BROADCAST_CODE_SIZE * sizeof(uint8));
            memcpy(badCode, &(ind->brsValue.badcode[0]), BAP_BROADCAST_CODE_SIZE * sizeof(uint8));

            CsrPmemFree(ind->brsValue.badcode);
        }

        if (ind->brsValue.numSubGroups != 0)
        {
            uint8 i;

            subGroupInfo =
                (BapSubgroupInfo *) CsrPmemZalloc(ind->brsValue.numSubGroups * sizeof(BapSubgroupInfo));

            for (i = 0; i < ind->brsValue.numSubGroups; i++)
            {
                subGroupInfo[i].bisSyncState = ind->brsValue.subGroupsData[i].bisSync;
                subGroupInfo[i].metadataLen = ind->brsValue.subGroupsData[i].metadataLen;

                if (ind->brsValue.subGroupsData[i].metadataLen != 0)
                {
                    subGroupInfo[i].metadataValue = CsrPmemZalloc(ind->brsValue.subGroupsData[i].metadataLen *sizeof(uint8));
                    memcpy(subGroupInfo[i].metadataValue,
                           ind->brsValue.subGroupsData[i].metadata,
                           ind->brsValue.subGroupsData[i].metadataLen *sizeof(uint8)
                           );
                    CsrPmemFree(ind->brsValue.subGroupsData[i].metadata);
                }
            }

            CsrPmemFree(ind->brsValue.subGroupsData);
        }

        /* Check if the Source Address and advSid is same as one which Assistant */
        if (bapBroadcastAssistantValidateBrsParams(connection,
            ind->brsValue.sourceAddress.addr, ind->brsValue.sourceAddress.type, ind->brsValue.advSid))
        {
            if (bapBroadcastAssistantIsSourceIdPending(connection))
            {
                bapBroadcastAssistantSetSourceId(connection, ind->brsValue.sourceId);
            }

            bapBroadcastAssistantUtilsSendBroadcastReceiveStateInd(connection->rspPhandle,
                                                                   connection->cid,
                                                                   ind->brsValue.sourceId,
                                                                   ind->brsValue.sourceAddress.addr,
                                                                   ind->brsValue.sourceAddress.type,
                                                                   ind->brsValue.advSid,
                                                                   ind->brsValue.paSyncState,
                                                                   ind->brsValue.bigEncryption,
                                                                   badCode,
                                                                   ind->brsValue.numSubGroups,
                                                                   subGroupInfo,
                                                                   ind->brsValue.broadcastId);

            /* Identify what operation is required from PA sync, BIS sync and
            * BIG encryption state
            */

            if (ind->brsValue.paSyncState == BAP_ASSISTANT_PA_STATE_SYNC_INFO_REQ)
            {
                /* Start PAST procedure to transfer syncinfo */
                bapBroadcastAssistantStartSyncInfoReq(connection);
            }
            else if(ind->brsValue.paSyncState == BAP_ASSISTANT_PA_STATE_SYNC &&
                ind->brsValue.bigEncryption == BAP_ASSISTANT_BROADCAST_CODE_REQ)
            {
                bool responseOp;
                bapBroadcastAssistantGetControlPointOp(connection, &responseOp, NULL);

                if (bapBroadcastIsSourceCollocated(connection))
                {
                    uint8 *broadcastCode = CsrPmemZalloc(BAP_BROADCAST_CODE_SIZE);
                    uint8 sourceId = connection->broadcastAssistant.sourceId;
                    uint8 advHandle = (uint8) bapBroadcastGetSyncHandle(connection);

                    if(bapBroadcastGetCodefromSrc(bap, advHandle, broadcastCode))
                    {

                        GattBassClientSetBroadcastCodeRequest(connection->bass.srvcHndl,
                                                              sourceId,
                                                              broadcastCode,
                                                              responseOp);
                    }
                    else
                    {
                        BAP_CLIENT_ERROR("(BAP) bapGattBassClientBroadcastReceiveStateInd: Failed to get BRD SRC Codes\n");
                    }

                    CsrPmemFree(broadcastCode);
                }
                else
                {
                    /* For non-collocated, the codes have to be obtained from
                     * application.
                     * Send indication to application to provide it
                     */
                     bapBroadcastAssistantUtilsSendSetBroadcastCodeInd(connection->rspPhandle,
                                                                       connection->cid,
                                                                       ind->brsValue.sourceId,
                                                                       BROADCAST_CODE_REQUESTED);
                }
            }
            else if(ind->brsValue.paSyncState == BAP_ASSISTANT_PA_STATE_SYNC &&
                ind->brsValue.bigEncryption == BAP_ASSISTANT_BAD_CODE)
            {
                BAP_CLIENT_ERROR("(BAP) bapGattBassClientBroadcastReceiveStateInd: BAD code Ind\n");

                if (!bapBroadcastIsSourceCollocated(connection))
                {
                    /* For non-collocated, the codes have to be obtained from
                     * application/remote device.
                     * Bad code can happen becuase of user input or OOB getting code
                     * Send indication to application to get it
                     * For collocated we dont expect Code to be wrong
                     */
                     bapBroadcastAssistantUtilsSendSetBroadcastCodeInd(connection->rspPhandle,
                                                                       connection->cid,
                                                                       ind->brsValue.sourceId,
                                                                       BROADCAST_CODE_REQUESTED |
                                                                       BROADCAST_CODE_BAD_CODE);
                }
            }

        }
        else
        {
            /* Ignore msg? */
            BAP_CLIENT_ERROR("(BAP) bapGattBassClientBroadcastReceiveStateInd: Wrong addr/advSid\n");
        }
    }
}

static void bapGattBassClientBroadcastReceiveStateCfm(BAP* const bap,
                                                      GattBassClientReadBroadcastReceiveStateCfm* cfm)
{
    BapConnection* connection = NULL;
    BapResult result = BAP_RESULT_ERROR;

    if (findConnectionByBassSrvcHndl(bap, cfm->clntHndl, &connection))
    {
        uint8 *badCode = NULL;
        BapSubgroupInfo *subGroupInfo = NULL;

        result = (cfm->status == GATT_BASS_CLIENT_STATUS_SUCCESS) ?
            BAP_RESULT_SUCCESS : BAP_RESULT_ERROR;

        if (cfm->brsValue.bigEncryption == BAP_ASSISTANT_BAD_CODE)
        {
            badCode = CsrPmemZalloc(BAP_BROADCAST_CODE_SIZE * sizeof(uint8));
            memcpy(badCode, &(cfm->brsValue.badcode[0]), BAP_BROADCAST_CODE_SIZE * sizeof(uint8));
            CsrPmemFree(cfm->brsValue.badcode);
        }

        if (cfm->brsValue.numSubGroups != 0)
        {
            uint8 i;

            subGroupInfo =
                (BapSubgroupInfo *) CsrPmemZalloc(cfm->brsValue.numSubGroups * sizeof(BapSubgroupInfo));

            for (i = 0; i < cfm->brsValue.numSubGroups; i++)
            {
                subGroupInfo[i].bisSyncState = cfm->brsValue.subGroupsData[i].bisSync;
                subGroupInfo[i].metadataLen = cfm->brsValue.subGroupsData[i].metadataLen;

                if (cfm->brsValue.subGroupsData[i].metadataLen != 0)
                {
                    subGroupInfo[i].metadataValue = CsrPmemZalloc(cfm->brsValue.subGroupsData[i].metadataLen *sizeof(uint8));
                    memcpy(subGroupInfo[i].metadataValue,
                           cfm->brsValue.subGroupsData[i].metadata,
                           cfm->brsValue.subGroupsData[i].metadataLen *sizeof(uint8)
                           );

                    CsrPmemFree(cfm->brsValue.subGroupsData[i].metadata);
                }
            }

            CsrPmemFree(cfm->brsValue.subGroupsData);
        }

        bapBroadcastAssistantUtilsSendBroadcastReceiveStateCfm(connection->rspPhandle,
                                                               connection->cid,
                                                               result,
                                                               cfm->brsValue.sourceId,
                                                               cfm->brsValue.sourceAddress.addr,
                                                               cfm->brsValue.sourceAddress.type,
                                                               cfm->brsValue.advSid,
                                                               cfm->brsValue.paSyncState,
                                                               cfm->brsValue.bigEncryption,
                                                               badCode,
                                                               cfm->brsValue.numSubGroups,
                                                               subGroupInfo,
                                                               cfm->brsValue.broadcastId);
    }

}

static void bapGattBassClientAddSourceCfm(BAP* const bap,
                                          GattBassClientAddSourceCfm* cfm)
{
    BapConnection* connection = NULL;
    BapResult result = BAP_RESULT_ERROR;

    if (findConnectionByBassSrvcHndl(bap, cfm->clntHndl, &connection))
    {
        result = (cfm->status == GATT_BASS_CLIENT_STATUS_SUCCESS) ?
            BAP_RESULT_SUCCESS : BAP_RESULT_ERROR;

        /* Update the assistant state */
        bapBroadcastAssistantUpdateState(connection, BAP_ASSISTANT_STATE_IDLE);

        /* Set the sourceId pending flag */
        bapBroadcastAssistantSetSourceIdPending(connection);

        bapBroadcastAssistantUtilsSendAddSourceCfm(connection->rspPhandle,
                                                   connection->cid,
                                                   result);
    }
}

static void bapGattBassClientModifySourceCfm(BAP* const bap,
                                             GattBassClientModifySourceCfm* cfm)
{
    BapConnection* connection = NULL;
    BapResult result = BAP_RESULT_ERROR;

    if (findConnectionByBassSrvcHndl(bap, cfm->clntHndl, &connection))
    {
        result = (cfm->status == GATT_BASS_CLIENT_STATUS_SUCCESS) ?
            BAP_RESULT_SUCCESS : BAP_RESULT_ERROR;

        /* Update the assistant state */
        bapBroadcastAssistantUpdateState(connection, BAP_ASSISTANT_STATE_IDLE);

        bapBroadcastAssistantUtilsSendModifySourceCfm(connection->rspPhandle,
                                                      connection->cid,
                                                      result);
    }
}

static void bapGattBassClientSetBroadcastCodeCfm(BAP* const bap,
                                                 GattBassClientSetBroadcastCodeCfm* cfm)
{
    BapConnection* connection = NULL;

    if (findConnectionByBassSrvcHndl(bap, cfm->clntHndl, &connection))
    {
        /* Update the assistant state */
        bapBroadcastAssistantUpdateState(connection, BAP_ASSISTANT_STATE_IDLE);
    }
}

static void bapGattBassClientRemoveSourceCfm(BAP* const bap,
                                             GattBassClientRemoveSourceCfm* cfm)
{
    BapConnection* connection = NULL;
    BapResult result = BAP_RESULT_ERROR;

    if (findConnectionByBassSrvcHndl(bap, cfm->clntHndl, &connection))
    {
        result = (cfm->status == GATT_BASS_CLIENT_STATUS_SUCCESS) ?
            BAP_RESULT_SUCCESS : BAP_RESULT_ERROR;

        /* Update the assistant state */
        bapBroadcastAssistantUpdateState(connection, BAP_ASSISTANT_STATE_IDLE);

        /* Set sourceIdPending flag as sourceId is not assigned */
        bapBroadcastAssistantSetSourceIdPending(connection);

        bapBroadcastAssistantUtilsSendRemoveSourceCfm(connection->rspPhandle,
                                                      connection->cid,
                                                      result);
    }
}

void handleBassPrimitive(BAP* const bap, uint16 primitive_id, void* primitive)
{

    GattBassClientMessageId* prim = (GattBassClientMessageId*)primitive;

    switch (*prim)
    {
        case GATT_BASS_CLIENT_INIT_CFM:
        {
            GattBassClientInitCfm* cfm = (GattBassClientInitCfm*)primitive;
            bapGattBassClientInitCfm(bap, cfm);
        }
        break;

        case GATT_BASS_CLIENT_BROADCAST_RECEIVE_STATE_SET_NTF_CFM:
        {
            GattBassClientBroadcastReceiveStateSetNtfCfm* cfm =
                 (GattBassClientBroadcastReceiveStateSetNtfCfm*)primitive;
            bapGattBassClientBroadcastReceiveStateSetNtfCfm(bap, cfm);
        }
        break;

        case GATT_BASS_CLIENT_READ_BROADCAST_RECEIVE_STATE_CCC_CFM:
        {
            GattBassClientReadBroadcastReceiveStateCccCfm* cfm =
                 (GattBassClientReadBroadcastReceiveStateCccCfm*)primitive;
            bapGattBassClientReadBroadcastReceiveStateCccCfm(bap, cfm);
        }
        break;
        case GATT_BASS_CLIENT_BROADCAST_RECEIVE_STATE_IND:
        {
            GattBassClientBroadcastReceiveStateInd* ind =
                 (GattBassClientBroadcastReceiveStateInd*)primitive;
            bapGattBassClientBroadcastReceiveStateInd(bap, ind);
        }
        break;

        case GATT_BASS_CLIENT_READ_BROADCAST_RECEIVE_STATE_CFM:
        {
            GattBassClientReadBroadcastReceiveStateCfm* cfm =
                (GattBassClientReadBroadcastReceiveStateCfm*)primitive;
            bapGattBassClientBroadcastReceiveStateCfm(bap, cfm);
        }
        break;

        case GATT_BASS_CLIENT_REMOTE_SCAN_STOP_CFM:
        {
        }
        break;

        case GATT_BASS_CLIENT_REMOTE_SCAN_START_CFM:
        {
        }
        break;

        case GATT_BASS_CLIENT_ADD_SOURCE_CFM:
        {
            GattBassClientAddSourceCfm* cfm = (GattBassClientAddSourceCfm*)primitive;
            bapGattBassClientAddSourceCfm(bap, cfm);
        }
        break;

        case GATT_BASS_CLIENT_MODIFY_SOURCE_CFM:
        {
            GattBassClientModifySourceCfm* cfm = (GattBassClientModifySourceCfm*)primitive;
            bapGattBassClientModifySourceCfm(bap, cfm);
        }
        break;


        case GATT_BASS_CLIENT_SET_BROADCAST_CODE_CFM:
        {
            GattBassClientSetBroadcastCodeCfm* cfm = (GattBassClientSetBroadcastCodeCfm*)primitive;
            bapGattBassClientSetBroadcastCodeCfm(bap, cfm);
        }
        break;


        case GATT_BASS_CLIENT_REMOVE_SOURCE_CFM:
        {
            GattBassClientRemoveSourceCfm* cfm = (GattBassClientRemoveSourceCfm*)primitive;
            bapGattBassClientRemoveSourceCfm(bap, cfm);
        }
        break;

        case GATT_BASS_CLIENT_TERMINATE_CFM:
        {
            BapConnection *connection;
            uint8 i,j;
            GattBassClientTerminateCfm *ind = (GattBassClientTerminateCfm*)primitive;
            if(ind->status == GATT_BASS_CLIENT_STATUS_SUCCESS)
            {
                if (findConnectionByBassSrvcHndl(bap, ind->clntHndl, &connection))
                {
                    connection->handles.bassHandles = CsrPmemZalloc(sizeof(BapBassClientDeviceData));
                    connection->handles.bassHandles->broadcastReceiveStateHandle =
                                          (uint16*)CsrPmemZalloc(ind->broadcastSourceNum*sizeof(uint16));
                    connection->handles.bassHandles->broadcastReceiveStateHandleCcc =
                                          (uint16*)CsrPmemZalloc(ind->broadcastSourceNum*sizeof(uint16));

                    connection->handles.bassHandles->broadcastSourceNum = ind->broadcastSourceNum;
                    connection->handles.bassHandles->broadcastAudioScanControlPointHandle
                                                                = ind->broadcastAudioScanControlPointHandle;

                    connection->numService--;

                    for(i=0, j=0; i < ind->broadcastSourceNum; i++)
                    {
                        connection->handles.bassHandles->broadcastReceiveStateHandle[i]
                                                       = ind->broadcastReceiveStateHandles[j++];
                        connection->handles.bassHandles->broadcastReceiveStateHandleCcc[i]
                                                       = ind->broadcastReceiveStateHandles[j++];
                    }

                    if(connection->numService == 0)
                    {
                        bapClientSendDeinitCfmSuccess(bap, connection);
                    }
                }
            }
        }
        break;
        default:
            break;
    }

    CSR_UNUSED(primitive_id);
}
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */
/** @}*/


/*******************************************************************************

Copyright (C)2020-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP Broadcast Assistant init implementation.
 */

/**
 * \addtogroup BAP_PRIVATE
 * @{
 */

#include <stdio.h>
#include <string.h>

#include "dm_prim.h"
#include "dmlib.h"
#include "bap_client_lib.h"

#include "bap_utils.h"
#include "bap_connection.h"
#include "bap_client_debug.h"
/*#include "../bap_gatt_msg_handler.h"*/
#include "bap_client_list_util_private.h"

#include "bap_broadcast_assistant.h"

#include "csr_bt_common.h"
#include "csr_bt_gatt_lib.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "gatt_bass_client.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "csr_bt_cm_prim.h"
#include "csr_bt_cm_lib.h"
#include "gatt_service_discovery_lib.h"

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
/*Service handle initialization failure*/
#define BASS_INVALID_SERVICE_HANDLE ((ServiceHandle)(0x0000))
#define ADV_IN_PAST_MATCHES_ADV_EXT_IND      (0 << 0)
#define ADV_IN_PAST_NOT_MATCHES_ADV_EXT_IND  (1 << 0)
#define ADV_IN_PAST_MATCHES_SRC_ADDRESS      (0 << 1)
#define ADV_IN_PAST_NOT_MATCHES_SRC_ADDRESS  (1 << 1)

void bapBroadcastAssistantInit(BapConnection* connection)
{
    /* Initialise Assistant */
    if (connection)
    {
        connection->role |= BAP_ROLE_BROADCAST_ASSISTANT;
        connection->bass.srvcHndl = BASS_INVALID_SERVICE_HANDLE;
        connection->bass.bassStartHandle = 0;
        connection->bass.bassEndHandle = 0;

        memset(&(connection->broadcastAssistant), 0 , sizeof(BroadcastAssistant));
        connection->broadcastAssistant.assistantState = BAP_ASSISTANT_STATE_IDLE;
        connection->broadcastAssistant.sourceIdPending = TRUE;
        connection->broadcastAssistant.controlOpResponse = FALSE;
        connection->broadcastAssistant.longWrite = FALSE;
    }
}

void bapBroadcastAssistantDeinit(BapConnection* connection)
{
    if(connection && connection->bass.srvcHndl)
    {
        uint8 i;
        /* Remove the BASS Client instance */
        connection->numService++;
        GattBassClientTerminateReq(connection->bass.srvcHndl);

        connection->role &= ~(BAP_ROLE_BROADCAST_ASSISTANT);

        /* Delete Assistant structures */
        memset(&(connection->broadcastAssistant), 0 , sizeof(BroadcastAssistant));
        connection->broadcastAssistant.assistantState = BAP_ASSISTANT_STATE_IDLE;
        connection->broadcastAssistant.sourceIdPending = TRUE;
        connection->broadcastAssistant.controlOpResponse = FALSE;
        connection->broadcastAssistant.longWrite = FALSE;

        for (i = 0; i < connection->broadcastAssistant.numbSubGroups; i++)
        {
            if((connection->broadcastAssistant.subgroupInfo[i].metadataLen) &&
               (connection->broadcastAssistant.subgroupInfo[i].metadataValue))
            {
                CsrPmemFree(connection->broadcastAssistant.subgroupInfo[i].metadataValue);
            }
        }
    }
}

void bapBassClientSrvcInit(BapConnection* connection, CsrBtConnId cid, uint16 startHndl,
	                       uint16 endHndl, BapBassClientDeviceData *data)
{
    GattBassClientInitData params;

    if(connection)
    {
        /* Protect the Client from initialising multiple instance from same Device */
        if(connection->bass.bassStartHandle == 0)
        {
            connection->bass.bassStartHandle = startHndl;
            connection->bass.bassEndHandle = endHndl;
            connection->bass.srvcHndl = BASS_INVALID_SERVICE_HANDLE;
            connection->numService++;

            params.cid = cid;
            params.startHandle = startHndl;
            params.endHandle = endHndl;

            GattBassClientInitReq(CSR_BT_BAP_IFACEQUEUE, &params, data);
        }
        else
        {
            BAP_CLIENT_DEBUG("(BAP) BASS service already started\n\n");
        }

    }
}

BroadcastAssistant *bapBroadcastAssistantGetInstFromConn(BapConnection* connection)
{
    return &(connection->broadcastAssistant);
}

void bapBroadcastAssistantUpdateState(BapConnection* connection,
                                      bapAssistantState  assistantState)
{
    BroadcastAssistant  *broadcastAssistant =
        bapBroadcastAssistantGetInstFromConn(connection);

    broadcastAssistant->assistantState = assistantState;
}

uint8 bapBroadcastAssistantGetState(BapConnection* connection)
{
    BroadcastAssistant  *broadcastAssistant =
        bapBroadcastAssistantGetInstFromConn(connection);

    return broadcastAssistant->assistantState;
}


void bapBroadcastAssistantAddSourceParam(BapConnection* connection, GattBassClientAddSourceParam *params,
	                                     uint16 syncHandle, bool srcCollocated)
{
    uint8 i;
    BroadcastAssistant  *broadcastAssistant =
        bapBroadcastAssistantGetInstFromConn(connection);

    broadcastAssistant->advSid = params->advSid;
    broadcastAssistant->paSyncState = params->paSyncState;
    broadcastAssistant->srcCollocated = srcCollocated;
    broadcastAssistant->syncHandle = syncHandle;
    broadcastAssistant->broadcastId = params->broadcastId;

    broadcastAssistant->advertiseAddType = params->sourceAddress.type;
    bd_addr_copy(&(broadcastAssistant->sourceAddress), &(params->sourceAddress.addr));


    if(broadcastAssistant->advertiseAddType == 0x00 ||
        broadcastAssistant->advertiseAddType == 0x01)
    {
        broadcastAssistant->serviceDataOctet0 = ADV_IN_PAST_MATCHES_SRC_ADDRESS;
    }
    else
    {
        broadcastAssistant->serviceDataOctet0 = ADV_IN_PAST_NOT_MATCHES_SRC_ADDRESS;
    }

    broadcastAssistant->serviceDataOctet0 |= ADV_IN_PAST_MATCHES_ADV_EXT_IND;

    broadcastAssistant->numbSubGroups = params->numSubGroups;

    for (i = 0; i < broadcastAssistant->numbSubGroups; i++)
    {
        broadcastAssistant->subgroupInfo[i].bisSyncState = params->subGroupsData[i].bisSync;
        broadcastAssistant->subgroupInfo[i].metadataLen = params->subGroupsData[i].metadataLen;
        if (params->subGroupsData[i].metadataLen !=0)
        {
             broadcastAssistant->subgroupInfo[i].metadataValue =
                CsrPmemZalloc(params->subGroupsData[i].metadataLen);

             memcpy(broadcastAssistant->subgroupInfo[i].metadataValue,
                    params->subGroupsData[i].metadata,
                    params->subGroupsData[i].metadataLen);
        }
    }
}

void bapBroadcastAssistantModifySourceParam(BapConnection* connection, GattBassClientModifySourceParam *params,
	                                        uint16 syncHandle, bool srcCollocated,
    uint8 advSid)
{
    uint8 i;
    BroadcastAssistant  *broadcastAssistant =
        bapBroadcastAssistantGetInstFromConn(connection);

    broadcastAssistant->paSyncState = params->paSyncState;
    broadcastAssistant->advSid = advSid;
    broadcastAssistant->srcCollocated = srcCollocated;
    broadcastAssistant->syncHandle = syncHandle;

    if(broadcastAssistant->advertiseAddType == 0x00)
    {
        broadcastAssistant->serviceDataOctet0 = ADV_IN_PAST_MATCHES_ADV_EXT_IND |
                                                ADV_IN_PAST_MATCHES_SRC_ADDRESS;
    }
    else if(broadcastAssistant->advertiseAddType == 0x01)
    {
        broadcastAssistant->serviceDataOctet0 = ADV_IN_PAST_NOT_MATCHES_ADV_EXT_IND |
                                                ADV_IN_PAST_MATCHES_SRC_ADDRESS;
    }
    else
    {
        /* type 2 or 3 */
        broadcastAssistant->serviceDataOctet0 = ADV_IN_PAST_NOT_MATCHES_ADV_EXT_IND |
                                                ADV_IN_PAST_NOT_MATCHES_SRC_ADDRESS;
    }

    broadcastAssistant->numbSubGroups = params->numSubGroups;

    for (i = 0; i < broadcastAssistant->numbSubGroups; i++)
    {
        /* free the old metadata */
        if(broadcastAssistant->subgroupInfo[i].metadataValue != NULL)
        {
            CsrPmemFree(broadcastAssistant->subgroupInfo[i].metadataValue);
            broadcastAssistant->subgroupInfo[i].metadataValue = NULL;
        }

        broadcastAssistant->subgroupInfo[i].bisSyncState = params->subGroupsData[i].bisSync;
        broadcastAssistant->subgroupInfo[i].metadataLen = params->subGroupsData[i].metadataLen;
        if (params->subGroupsData[i].metadataLen !=0)
        {
             broadcastAssistant->subgroupInfo[i].metadataValue =
                CsrPmemZalloc(params->subGroupsData[i].metadataLen);

             memcpy(broadcastAssistant->subgroupInfo[i].metadataValue,
                    params->subGroupsData[i].metadata,
                    params->subGroupsData[i].metadataLen);
        }
    }

}

void bapBroadcastAssistantRemoveSourceId(BapConnection* connection,
                                         uint16 sourceId)
{
    BroadcastAssistant  *broadcastAssistant =
        bapBroadcastAssistantGetInstFromConn(connection);
    uint8 i;

    if (sourceId == broadcastAssistant->sourceId)
    {
        broadcastAssistant->sourceId = 0;
        for (i = 0; i < broadcastAssistant->numbSubGroups; i++)
        {
            /* free the metadata */
            if(broadcastAssistant->subgroupInfo[i].metadataValue != NULL)
            {
                CsrPmemFree(broadcastAssistant->subgroupInfo[i].metadataValue);
                broadcastAssistant->subgroupInfo[i].metadataValue = NULL;
            }
        }
    }
}


bool bapBroadcastAssistantValidateBrsParams(BapConnection* connection,
                                            BD_ADDR_T addr,
                                            uint8 advertiseAddType,
                                            uint8 advSid)
{
    CSR_UNUSED(connection);
    CSR_UNUSED(addr);
    CSR_UNUSED(advertiseAddType);
    CSR_UNUSED(advSid);
    return TRUE;
}

bool bapBroadcastAssistantIsSourceIdPending(BapConnection* connection)
{
    BroadcastAssistant  *broadcastAssistant =
        bapBroadcastAssistantGetInstFromConn(connection);

    if (broadcastAssistant->sourceIdPending)
        return TRUE;

    return FALSE;
}

void bapBroadcastAssistantSetSourceId(BapConnection* connection,
                                      uint8 sourceId)
{
    BroadcastAssistant  *broadcastAssistant =
        bapBroadcastAssistantGetInstFromConn(connection);

    broadcastAssistant->sourceId = sourceId;
}


void bapBroadcastAssistantSetSourceIdPending(BapConnection* connection)
{
    BroadcastAssistant  *broadcastAssistant =
        bapBroadcastAssistantGetInstFromConn(connection);

    broadcastAssistant->sourceIdPending = TRUE;
}

void bapBroadcastAssistantStartSyncInfoReq(BapConnection* connection)
{
    BroadcastAssistant  *broadcastAssistant =
        bapBroadcastAssistantGetInstFromConn(connection);
    TYPED_BD_ADDR_T peer_address;
    uint16 serviceData;

    CsrBtGattClientUtilFindAddrByConnId(connection->cid, &peer_address);

    BAP_CLIENT_DEBUG("(BAP) PeerAddress %04x:%02x:%06x\n",
                    peer_address.addr.nap,
                    peer_address.addr.uap,
                    peer_address.addr.lap);

    BAP_CLIENT_DEBUG(" (BAP) Broadcast Src collocated :%x\n", broadcastAssistant->srcCollocated);

    serviceData = (broadcastAssistant->sourceId << 8) | (broadcastAssistant->serviceDataOctet0);

    /* Identify if the source_address if of collocated or standalone SRC*/
    if(broadcastAssistant->srcCollocated)
    {
        uint8 advHandle = (uint8) broadcastAssistant->syncHandle;
        BAP_CLIENT_DEBUG("advHandle =%x\n", advHandle);
        CmPeriodicAdvSetTransferReqSend(CSR_BT_BAP_IFACEQUEUE, peer_address, serviceData, advHandle);
    }
    else
    {
        BAP_CLIENT_DEBUG("syncHandle =%x\n", broadcastAssistant->syncHandle);
        CmPeriodicScanSyncTransferReqSend(CSR_BT_BAP_IFACEQUEUE,
                                          peer_address,
                                          serviceData,
                                          broadcastAssistant->syncHandle);
    }
}

void bapBroadcastAssistantUpdateControlPointOp(BapConnection* connection,
                                               bool  controlOpResponse,
                                               bool longWrite)
{
    BroadcastAssistant  *broadcastAssistant =
        bapBroadcastAssistantGetInstFromConn(connection);
    if( broadcastAssistant )
    {
        broadcastAssistant->controlOpResponse= controlOpResponse;
        broadcastAssistant->longWrite = longWrite;
    }
}

void bapBroadcastAssistantGetControlPointOp(BapConnection* connection,
                                            bool* controlOpResponse,
                                            bool* longWrite)
{
    BroadcastAssistant  *broadcastAssistant =
        bapBroadcastAssistantGetInstFromConn(connection);

    if(controlOpResponse)
        *controlOpResponse = broadcastAssistant->controlOpResponse;
    if(longWrite)
        *longWrite = broadcastAssistant->longWrite;
}

void bapBroadcastAssistantAddSyncParam(BapConnection* connection,
                                       TYPED_BD_ADDR_T sourceAddress,
                                       uint8 advSid)
{
    BroadcastAssistant  *broadcastAssistant =
        bapBroadcastAssistantGetInstFromConn(connection);

    broadcastAssistant->advSid = advSid;
    broadcastAssistant->advertiseAddType = sourceAddress.type;

    bd_addr_copy(&(broadcastAssistant->sourceAddress), &(sourceAddress.addr));
}

void bapBroadcastAssistantGetSyncParams(BapConnection* connection,
                                        TYPED_BD_ADDR_T *sourceAddress,
                                         uint8 *advSid)
{
    BroadcastAssistant  *broadcastAssistant =
        bapBroadcastAssistantGetInstFromConn(connection);

    *advSid = broadcastAssistant->advSid;

    tbdaddr_copy_from_bd_addr(sourceAddress,
                              broadcastAssistant->advertiseAddType,
                              &(broadcastAssistant->sourceAddress));
}

bool bapBroadcastIsSourceCollocated(BapConnection* connection)
{
    BroadcastAssistant  *broadcastAssistant =
        bapBroadcastAssistantGetInstFromConn(connection);

    return broadcastAssistant->srcCollocated;
}

uint16 bapBroadcastGetSyncHandle(BapConnection* connection)
{
    BroadcastAssistant  *broadcastAssistant =
        bapBroadcastAssistantGetInstFromConn(connection);

    return broadcastAssistant->syncHandle;
}
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

/**@}*/

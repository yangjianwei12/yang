/*******************************************************************************

Copyright (C) 2020-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#include <string.h>
#include <stdio.h>
#include "bap_client_lib.h"
#include "tbdaddr.h"
#include "csr_bt_profiles.h"
#include "csr_bt_tasks.h"
#include "csr_bt_common.h"
#include "bap_utils.h"
#include "bap_client_debug.h"
#include "bap_client_list_util_private.h"

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
/* BAP Broadcast Assistant Downstream primitives */

void BapBroadcastAssistantStartScanReq(BapProfileHandle handle,
                                       uint8 flags,
                                       uint16 filterContext,
                                       uint8 scanFlags,
                                       uint8 ownAddressType,
                                       uint8 scanningFilterPolicy)
{
    BapInternalBroadcastAssistantStartScanReq *pPrim = CsrPmemZalloc(sizeof(BapInternalBroadcastAssistantStartScanReq));

    pPrim->type = BAP_INTERNAL_BROADCAST_ASSISTANT_START_SCAN_REQ;
    pPrim->handle = handle;
    pPrim->flags = flags;
    pPrim->filterContext = filterContext;
    pPrim->scanFlags = scanFlags;
    pPrim->ownAddressType = ownAddressType;
    pPrim->scanningFilterPolicy = scanningFilterPolicy;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}


void BapBroadcastAssistantStopScanReq(BapProfileHandle handle, uint16 scanHandle)
{
    BapInternalBroadcastAssistantStopScanReq *pPrim = 
        CsrPmemZalloc(sizeof(BapInternalBroadcastAssistantStopScanReq));

    pPrim->type = BAP_INTERNAL_BROADCAST_ASSISTANT_STOP_SCAN_REQ;
    pPrim->handle = handle;
    pPrim->scanHandle = scanHandle;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapBroadcastAssistantSyncToSrcStartReq(BapProfileHandle handle,
                                            TYPED_BD_ADDR_T *addrt,
                                            uint8 advSid)
{
    BapInternalBroadcastAssistantSyncToSrcStartReq *pPrim =
        CsrPmemZalloc(sizeof(BapInternalBroadcastAssistantSyncToSrcStartReq));

    pPrim->type = BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_START_REQ;
    pPrim->handle = handle;

    tbdaddr_copy(&pPrim->addrt, addrt);

    pPrim->advSid = advSid;
    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapBroadcastAssistantSyncToSrcCancelReq(BapProfileHandle handle)
{
    BapInternalBroadcastAssistantSyncToSrcCancelReq *pPrim = 
        CsrPmemZalloc(sizeof(BapInternalBroadcastAssistantSyncToSrcCancelReq));

    pPrim->type = BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_REQ;
    pPrim->handle = handle;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapBroadcastAssistantSyncToSrcTerminateReq(BapProfileHandle handle,
                                                uint16 syncHandle)
{
    BapInternalBroadcastAssistantSyncToSrcTerminateReq *pPrim = 
        CsrPmemZalloc(sizeof(BapInternalBroadcastAssistantSyncToSrcTerminateReq));

    pPrim->type = BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_TERMINATE_REQ;
    pPrim->handle = handle;
    pPrim->syncHandle = syncHandle;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapBroadcastAssistantBRSRegisterForNotificationReq(BapProfileHandle  handle,
                                                        uint8 sourceId,
                                                        bool allSources,
                                                        bool notificationsEnable)
{
    BapInternalBroadcastAssistantBrsRegisterForNotifcationReq *pPrim = 
        CsrPmemZalloc(sizeof(BapInternalBroadcastAssistantBrsRegisterForNotifcationReq));

    pPrim->type = BAP_INTERNAL_BROADCAST_ASSISTANT_BRS_REGISTER_FOR_NOTIFICATION_REQ;
    pPrim->handle = handle;
    pPrim->sourceId= sourceId;
    pPrim->allSources= allSources;
    pPrim->notificationsEnable = notificationsEnable;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapBroadcastAssistantReadBRSCccReq(BapProfileHandle  handle,
                                        uint8 sourceId,
                                        bool allSources)
{
    BapInternalBroadcastAssistantReadBrsCccReq *pPrim = 
        CsrPmemZalloc(sizeof(BapInternalBroadcastAssistantReadBrsCccReq));

    pPrim->type = BAP_INTERNAL_BROADCAST_ASSISTANT_READ_BRS_CCC_REQ;
    pPrim->handle = handle;
    pPrim->sourceId= sourceId;
    pPrim->allSources= allSources;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}


void BapBroadcastAssistantReadBRSReq(BapProfileHandle  handle,
                                     uint8 sourceId,
                                     bool allSources)
{
    BapInternalBroadcastAssistantReadBrsReq *pPrim = 
        CsrPmemZalloc(sizeof(BapInternalBroadcastAssistantReadBrsReq));

    pPrim->type = BAP_INTERNAL_BROADCAST_ASSISTANT_READ_BRS_REQ;
    pPrim->handle = handle;
    pPrim->sourceId= sourceId;
    pPrim->allSources= allSources;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapBroadcastAssistantAddSrcReq(BapProfileHandle handle,
                                    BD_ADDR_T *sourceAddrt,
                                    uint8 advertiserAddressType,
                                    bool srcCollocated,
                                    uint16 syncHandle,
                                    uint8 sourceAdvSid,
                                    uint8 paSyncState,
                                    uint16 paInterval,
                                    uint32 broadcastId,
                                    uint8 numbSubGroups,
                                    BapSubgroupInfo *subgroupInfo)
{
    uint8 i;
    BapInternalBroadcastAssistantAddSrcReq *pPrim =
        CsrPmemZalloc(sizeof(BapInternalBroadcastAssistantAddSrcReq));

    pPrim->type = BAP_INTERNAL_BROADCAST_ASSISTANT_ADD_SRC_REQ;
    pPrim->handle = handle;

    bd_addr_copy(&pPrim->sourceAddrt, sourceAddrt);
    pPrim->advertiserAddressType = advertiserAddressType;

    pPrim->srcCollocated = srcCollocated;
    pPrim->syncHandle = syncHandle;
    pPrim->sourceAdvSid= sourceAdvSid;
    pPrim->paSyncState = paSyncState;
    pPrim->paInterval = paInterval;
    pPrim->numbSubGroups = numbSubGroups;
    pPrim->broadcastId = broadcastId;

    for (i = 0; i < (numbSubGroups > BAP_MAX_SUPPORTED_NUM_SUBGROUPS ?
                        BAP_MAX_SUPPORTED_NUM_SUBGROUPS: numbSubGroups); i++)
    {
        pPrim->subgroupInfo[i] = CsrPmemZalloc(sizeof(BapSubgroupInfo));
        pPrim->subgroupInfo[i]->bisSyncState = subgroupInfo[i].bisSyncState;
        pPrim->subgroupInfo[i]->metadataLen = subgroupInfo[i].metadataLen;

        if (subgroupInfo[i].metadataLen != 0)
        {
            pPrim->subgroupInfo[i]->metadataValue = 
                CsrPmemZalloc(subgroupInfo[i].metadataLen*sizeof(uint8));

            memcpy(pPrim->subgroupInfo[i]->metadataValue,
                   subgroupInfo[i].metadataValue,
                   subgroupInfo[i].metadataLen*sizeof(uint8));
        }
    }

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapBroadcastAssistantModifySrcReq(BapProfileHandle handle,
                                       uint8 sourceId,
                                       bool srcCollocated,
                                       uint16 syncHandle,
                                       uint8 sourceAdvSid,
                                       uint8 paSyncState,
                                       uint16 paInterval,
                                       uint8 numbSubGroups,
                                       BapSubgroupInfo *subgroupInfo)
{
    uint8 i;
    BapInternalBroadcastAssistantModifySrcReq *pPrim =
        CsrPmemZalloc(sizeof(BapInternalBroadcastAssistantModifySrcReq));

    pPrim->type = BAP_INTERNAL_BROADCAST_ASSISTANT_MODIFY_SRC_REQ;
    pPrim->handle = handle;
    pPrim->sourceId= sourceId;

    pPrim->srcCollocated = srcCollocated;
    pPrim->syncHandle = syncHandle;
    pPrim->sourceAdvSid = sourceAdvSid;
    pPrim->paSyncState = paSyncState;
    pPrim->paInterval = paInterval;
    pPrim->numbSubGroups = numbSubGroups;


    for (i = 0; i < (numbSubGroups > BAP_MAX_SUPPORTED_NUM_SUBGROUPS ?
                        BAP_MAX_SUPPORTED_NUM_SUBGROUPS: numbSubGroups); ++i)
    {
        pPrim->subgroupInfo[i] = CsrPmemZalloc(sizeof(BapSubgroupInfo));
        pPrim->subgroupInfo[i]->bisSyncState = subgroupInfo[i].bisSyncState;
        pPrim->subgroupInfo[i]->metadataLen = subgroupInfo[i].metadataLen;

        if (subgroupInfo[i].metadataLen != 0)
        {
            pPrim->subgroupInfo[i]->metadataValue = 
                CsrPmemZalloc(subgroupInfo[i].metadataLen*sizeof(uint8));

            memcpy(pPrim->subgroupInfo[i]->metadataValue,
                   subgroupInfo[i].metadataValue,
                   subgroupInfo[i].metadataLen*sizeof(uint8));
        }
    }

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);

}

void BapBroadcastAssistantRemoveSrcReq(BapProfileHandle handle,
                                       uint8 sourceId)
{
    BapInternalBroadcastAssistantRemoveSrcReq *pPrim = 
        CsrPmemZalloc(sizeof(BapInternalBroadcastAssistantRemoveSrcReq));

    pPrim->type = BAP_INTERNAL_BROADCAST_ASSISTANT_REMOVE_SRC_REQ;
    pPrim->handle = handle;
    pPrim->sourceId= sourceId;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapBroadcastAssistantSetCodeRsp(BapProfileHandle handle,
                                     uint8 sourceId,
                                     uint8 *broadcastCode)
{
    BapInternalBroadcastAssistantSetCodeRsp *pPrim = 
        CsrPmemZalloc(sizeof(BapInternalBroadcastAssistantSetCodeRsp));

    pPrim->type = BAP_INTERNAL_BROADCAST_ASSISTANT_SET_CODE_RSP;
    pPrim->handle = handle;
    pPrim->sourceId= sourceId;

    if (broadcastCode != NULL)
    {
        pPrim->broadcastCode = CsrPmemZalloc(BAP_BROADCAST_CODE_SIZE * sizeof(uint8));
        memcpy(pPrim->broadcastCode, broadcastCode, BAP_BROADCAST_CODE_SIZE * sizeof(uint8));
        CsrPmemFree(broadcastCode);
    }

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
    CSR_UNUSED(broadcastCode);
}
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */


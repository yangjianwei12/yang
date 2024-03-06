/*******************************************************************************

Copyright (C) 2020-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP Broadcast Assistant UTILS interface implementation..
 */

/**
 * \addtogroup BAP_BROADCAST_ASSISTANT_UTILS_PRIVATE
 * @{
 */

#include <stdio.h>
#include <string.h>

#include "bap_client_lib.h"
#include "tbdaddr.h"
#include "csr_bt_common.h"
#include "bap_utils.h"
#include "bap_broadcast_assistant_utils.h"

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT

void bapBroadcastAssistantUtilsSendReadBroadcastReceiveStateCccCfm(phandle_t phandle,
                                                                   BapProfileHandle handle,
                                                                   BapResult result,
                                                                   uint8 sourceId,
                                                                   uint16 size_value,
                                                                   uint8 *value)
{
    BapBroadcastAssistantReadBrsCccCfm *cfm =
        CsrPmemZalloc(sizeof(BapBroadcastAssistantReadBrsCccCfm));

    cfm->type = BAP_BROADCAST_ASSISTANT_READ_BRS_CCC_CFM;
    cfm->handle = handle;
    cfm->result = result;
    cfm->sourceId = sourceId;
    cfm->size_value = size_value;

    if (cfm->size_value)
        cfm->value = value;
    else
        cfm->value[0] = 0;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfm);
}

void bapBroadcastAssistantUtilsSendBroadcastReceiveStateSetNtfCfm(phandle_t phandle,
                                                                  BapProfileHandle handle,
                                                                  BapResult result,
                                                                  uint8 sourceId)
{
    BapBroadcastAssistantBrsRegisterForNotifcationCfm *cfm =
        CsrPmemZalloc(sizeof(BapBroadcastAssistantBrsRegisterForNotifcationCfm));

    cfm->type = BAP_BROADCAST_ASSISTANT_BRS_REGISTER_FOR_NOTIFICATION_CFM;
    cfm->handle = handle;
    cfm->result = result;
    cfm->sourceId = sourceId;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfm);
}

void bapBroadcastAssistantUtilsSendBroadcastReceiveStateCfm(phandle_t phandle,
                                                            BapProfileHandle handle,
                                                            BapResult result,
                                                            uint8 sourceId,
                                                            BD_ADDR_T sourceAddress,
                                                            uint8 advertiseAddType,
                                                            uint8 advSid,
                                                            uint8 paSyncState,
                                                            uint8 bigEncryption,
                                                            uint8 *badCode,
                                                            uint8 numSubGroups,
                                                            BapSubgroupInfo *subGroupInfo,
                                                            uint32 broadcastId)
{
    BapBroadcastAssistantReadBrsCfm *cfm = CsrPmemZalloc(sizeof(BapBroadcastAssistantReadBrsCfm));

    cfm->type = BAP_BROADCAST_ASSISTANT_READ_BRS_CFM;
    cfm->handle = handle;
    cfm->result = result;
    cfm->sourceId = sourceId;

    bd_addr_copy(&cfm->sourceAddress, &sourceAddress);
    cfm->advertiseAddType = advertiseAddType;
    cfm->advSid = advSid;
    cfm->paSyncState = paSyncState;
    cfm->bigEncryption = bigEncryption;
    cfm->broadcastId = broadcastId;

    if (bigEncryption == BROADCAST_CODE_BAD_CODE)
    {
        cfm->badCode = badCode;
    }
    else
    {
        cfm->badCode = NULL;
    }

    cfm->numSubGroups = numSubGroups;

    if (numSubGroups != 0)
    {
        cfm->subGroupInfo = subGroupInfo;
    }
    else
    {
        cfm->subGroupInfo = NULL;
    }

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfm);
}

void bapBroadcastAssistantUtilsSendBroadcastReceiveStateInd(phandle_t phandle,
                                                            BapProfileHandle handle,
                                                            uint8 sourceId,
                                                            BD_ADDR_T sourceAddress,
                                                            uint8 advertiseAddType,
                                                            uint8 advSid,
                                                            uint8 paSyncState,
                                                            uint8 bigEncryption,
                                                            uint8 *badCode,
                                                            uint8 numSubGroups,
                                                            BapSubgroupInfo *subGroupInfo,
                                                            uint32 broadcastId)
{
    BapBroadcastAssistantBrsInd *ind = CsrPmemZalloc(sizeof(BapBroadcastAssistantBrsInd));

    ind->type = BAP_BROADCAST_ASSISTANT_BRS_IND;
    ind->handle = handle;
    ind->sourceId = sourceId;

    bd_addr_copy(&(ind->sourceAddress), &sourceAddress);
    ind->advertiseAddType = advertiseAddType;
    ind->advSid = advSid;
    ind->paSyncState = paSyncState;
    ind->bigEncryption = bigEncryption;
    ind->broadcastId = broadcastId;

    if (bigEncryption == 0x03)
    {
        ind->badCode = badCode;
    }
    else
    {
        ind->badCode = NULL;
    }

    ind->numSubGroups = numSubGroups;

    if (numSubGroups != 0)
    {
        ind->subGroupInfo = subGroupInfo;
    }
    else
    {
        ind->subGroupInfo = NULL;
    }

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, ind);
}

void bapBroadcastAssistantUtilsSendStartScanCfm(phandle_t phandle,
                                                BapProfileHandle handle,
                                                BapResult result,
                                                uint16 scanHandle)
{
    BapBroadcastAssistantStartScanCfm *cfm =
        CsrPmemZalloc(sizeof(BapBroadcastAssistantStartScanCfm));

    cfm->type = BAP_BROADCAST_ASSISTANT_START_SCAN_CFM;
    cfm->handle = handle;
    cfm->result = result;
    cfm->scanHandle = scanHandle;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfm);
}

void bapBroadcastAssistantUtilsSendStopScanCfm(phandle_t phandle,
                                               BapProfileHandle handle,
                                               BapResult result)
{
    BapBroadcastAssistantStopScanCfm *cfm =
        CsrPmemZalloc(sizeof(BapBroadcastAssistantStopScanCfm));

    cfm->type = BAP_BROADCAST_ASSISTANT_STOP_SCAN_CFM;
    cfm->handle = handle;
    cfm->result = result;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfm);
}

void bapBroadcastAssistantUtilsSendAddSourceCfm(phandle_t phandle,
                                                BapProfileHandle handle,
                                                BapResult result)
{
    BapBroadcastAssistantAddSrcCfm *cfm =
        CsrPmemZalloc(sizeof(BapBroadcastAssistantAddSrcCfm));

    cfm->type = BAP_BROADCAST_ASSISTANT_ADD_SOURCE_CFM;
    cfm->handle = handle;
    cfm->result = result;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfm);
}

void bapBroadcastAssistantUtilsSendModifySourceCfm(phandle_t phandle,
                                                   BapProfileHandle handle,
                                                   BapResult result)
{
    BapBroadcastAssistantModifySrcCfm *cfm =
        CsrPmemZalloc(sizeof(BapBroadcastAssistantModifySrcCfm));

    cfm->type = BAP_BROADCAST_ASSISTANT_MODIFY_SOURCE_CFM;
    cfm->handle = handle;
    cfm->result = result;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfm);
}

void bapBroadcastAssistantUtilsSendSetBroadcastCodeInd(phandle_t phandle,
                                                       BapProfileHandle handle,
                                                       uint8 sourceId,
                                                       uint8 flags)
{
    BapBroadcastAssistantSetCodeInd *ind =
        CsrPmemZalloc(sizeof(BapBroadcastAssistantSetCodeInd));

    ind->type = BAP_BROADCAST_ASSISTANT_SET_CODE_IND;
    ind->handle = handle;
    ind->sourceId = sourceId;
    ind->flags = flags;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, ind);
}

void bapBroadcastAssistantUtilsSendRemoveSourceCfm(phandle_t phandle,
                                                   BapProfileHandle handle,
                                                   BapResult result)
{
    BapBroadcastAssistantRemoveSrcCfm *cfm =
        CsrPmemZalloc(sizeof(BapBroadcastAssistantRemoveSrcCfm));

    cfm->type = BAP_BROADCAST_ASSISTANT_REMOVE_SOURCE_CFM;
    cfm->handle = handle;
    cfm->result = result;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfm);
}

void bapBroadcastAssistantUtilsSendSyncToSrcStartCfm(phandle_t phandle,
                                                     BapProfileHandle handle,
                                                     uint16 syncHandle,
                                                     uint8 advSid,
                                                     uint8 advClockAccuracy,
                                                     uint8 advPhy,
                                                     uint16 periodicAdvInterval,
                                                     TYPED_BD_ADDR_T  addrt,
                                                     BapResult result)
{
    BapBroadcastAssistantSyncToSrcStartCfm *cfm =
        CsrPmemZalloc(sizeof(BapBroadcastAssistantSyncToSrcStartCfm));

    cfm->type = BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_START_CFM;
    cfm->handle = handle;
    cfm->result = result;
    cfm->syncHandle = syncHandle;
    cfm->advSid = advSid;
    cfm->advClockAccuracy = advClockAccuracy;
    cfm->advPhy = advPhy;
    cfm->periodicAdvInterval = periodicAdvInterval;

    memcpy(&(cfm->addrt), &addrt, sizeof(TYPED_BD_ADDR_T));

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfm);
}

void bapBroadcastAssistantUtilsSendSyncToSrcCancelCfm(phandle_t phandle,
                                                      BapProfileHandle handle,
                                                      BapResult result)
{
    BapBroadcastAssistantSyncToSrcCancelCfm *cfm =
        CsrPmemZalloc(sizeof(BapBroadcastAssistantSyncToSrcCancelCfm));

    cfm->type = BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_CFM;
    cfm->handle = handle;
    cfm->result = result;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfm);
}

void bapBroadcastAssistantUtilsSendSyncToSrcTerminateCfm(phandle_t phandle,
                                                         BapProfileHandle handle,
                                                         uint16 syncHandle,
                                                         BapResult result)
{
    BapBroadcastAssistantSyncToSrcTerminateCfm *cfm =
        CsrPmemZalloc(sizeof(BapBroadcastAssistantSyncToSrcTerminateCfm));

    cfm->type = BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_TERMINATE_CFM;
    cfm->handle = handle;
    cfm->result = result;
    cfm->syncHandle = syncHandle;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfm);
}

void bapBroadcastAssistantUtilsSendSyncLossInd(phandle_t phandle,
                                               BapProfileHandle handle,
                                               uint16 syncHandle)
{
    BapBroadcastAssistantSyncLossInd *ind =
        CsrPmemZalloc(sizeof(BapBroadcastAssistantSyncLossInd));

    ind->type = BAP_BROADCAST_ASSISTANT_SYNC_LOSS_IND;
    ind->handle = handle;
    ind->syncHandle = syncHandle;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, ind);
}
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */
/** @}*/


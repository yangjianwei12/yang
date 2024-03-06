/****************************************************************************
* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
* %%version
************************************************************************* ***/

#include "csr_bt_gatt_lib.h"
#include "csr_bt_gatt_prim.h"

#include "bap_server_private.h"
#include "bap_server_debug.h"
#include "bap_server_init.h"
#include "bap_server_lib.h"
#include "bap_server_common.h"
#include "bap_server_scan_delegator.h"
#include "bap_server_msg_handler.h"

static void bapServerOnScanningStateInd(BAP *bapInst,
                                        GattBassServerScanningStateInd* ind)
{
    BapServerBassScanningStateInd * message = NULL;

    message = (BapServerBassScanningStateInd*)CsrPmemZalloc(sizeof(BapServerBassScanningStateInd));
    message->type = BAP_SERVER_BASS_SCANNING_STATE_IND;
    message->cid = ind->cid;
    message->clientScanningState= ind->clientScanningState;

    BapServerMessageSend(bapInst->appBroadcastTask, message);
}

static void bapServerOnAddSourceInd(BAP *bapInst,
                                    GattBassServerAddSourceInd * ind)
{
    BapServerBassAddSourceInd * message = NULL;

    BAP_SERVER_DEBUG("BapServerOnAddSourceInd paSync=0x%x sourceAdvSid=0x%x addr=[%x %x:%x:%lx]",
                     ind->paSync,
                     ind->sourceAdvSid,
                     ind->advertiserAddress.type,
                     ind->advertiserAddress.addr.uap,
                     ind->advertiserAddress.addr.nap,
                     ind->advertiserAddress.addr.lap);

    message = (BapServerBassAddSourceInd*)CsrPmemZalloc(sizeof(BapServerBassAddSourceInd));

    message->type = BAP_SERVER_BASS_ADD_SOURCE_IND;
    message->source.cid = ind->cid;
    message->source.paSync = ind->paSync;
    message->source.advertiserAddress.type = ind->advertiserAddress.type;
    message->source.advertiserAddress.addr.uap = ind->advertiserAddress.addr.uap;
    message->source.advertiserAddress.addr.nap = ind->advertiserAddress.addr.nap;
    message->source.advertiserAddress.addr.lap = ind->advertiserAddress.addr.lap;
    message->source.broadcastId = ind->broadcastId;
    message->source.sourceAdvSid = ind->sourceAdvSid;
    message->source.paInterval = ind->paInterval;
    message->source.numSubGroups = ind->numSubGroups;
    if (message->source.numSubGroups)
    {
        message->source.subGroupsData = ind->subGroupsData;
    }
    else
    {
        message->source.subGroupsData = NULL;
    }

    BapServerMessageSend(bapInst->appBroadcastTask, message);
}

static void bapServerOnModifySourceInd(BAP *bapInst,
                                       GattBassServerModifySourceInd * ind)
{
    BapServerBassModifySourceInd * message = NULL;

    message = (BapServerBassModifySourceInd*)CsrPmemZalloc(sizeof(BapServerBassModifySourceInd));

    message->type = BAP_SERVER_BASS_MODIFY_SOURCE_IND;
    message->source.cid = ind->cid;
    message->source.sourceId = ind->sourceId;
    message->source.paSyncState = ind->paSyncState;
    message->source.paInterval = ind->paInterval;
    message->source.numSubGroups = ind->numSubGroups;
    if (message->source.numSubGroups)
    {
        message->source.subGroupsData = ind->subGroupsData;
    }
    else
    {
        message->source.subGroupsData = NULL;
    }
    BapServerMessageSend(bapInst->appBroadcastTask, message);
}

static void bapServerOnBroadcastCodeInd(BAP *bapInst,
                                        GattBassServerBroadcastCodeInd * ind)
{
    BapServerBassBroadcastCodeInd * message = NULL;

    message = (BapServerBassBroadcastCodeInd*)CsrPmemZalloc(sizeof(BapServerBassBroadcastCodeInd));

    message->type = BAP_SERVER_BASS_BROADCAST_CODE_IND;
    message->code.cid = ind->cid;
    message->code.sourceId = ind->sourceId;
    message->code.broadcastCode = CsrPmemZalloc(SCAN_DELEGATOR_BROADCAST_CODE_SIZE);
    memcpy(message->code.broadcastCode, ind->broadcastCode, SCAN_DELEGATOR_BROADCAST_CODE_SIZE);

    BapServerMessageSend(bapInst->appBroadcastTask, message);
}

static void bapServerOnRemoveSourceInd(BAP *bapInst,
                                       GattBassServerRemoveSourceInd * ind)
{    
    BapServerBassRemoveSourceInd * message = NULL;

    message = (BapServerBassRemoveSourceInd*)CsrPmemZalloc(sizeof(BapServerBassRemoveSourceInd));

    message->type = BAP_SERVER_BASS_REMOVE_SOURCE_IND;
    message->source.cid = ind->cid;
    message->source.sourceId = ind->sourceId;

    BapServerMessageSend(bapInst->appBroadcastTask, message);
}

static void bapServerOnBassConfigChangeInd(BAP *bapInst,
    GattBassServerConfigChangeInd * ind)
{
    if(ind->cid)
    {
        bapServerSendConfigChangeInd(bapInst,
                                     ind->cid,
                                     BAP_SERVER_CONFIG_BASS,
                                     ind->configChangeComplete);
    }
}

void bapServerHandleGattBassServerMsg(BAP *bapInst,
                                      void *message)
{
    GattBassServerMessageId *prim = (GattBassServerMessageId *)message;
    BAP_SERVER_INFO("bapServerHandleGattBassServerMsg MESSAGE:GattBassServerMessageId:0x%x", *prim);

    switch (*prim)
    {
        case GATT_BASS_SERVER_SCANNING_STATE_IND:
            bapServerOnScanningStateInd(bapInst,(GattBassServerScanningStateInd *)message);
            break;
        case GATT_BASS_SERVER_ADD_SOURCE_IND:
            bapServerOnAddSourceInd(bapInst, (GattBassServerAddSourceInd *)message);
            break;
        case GATT_BASS_SERVER_MODIFY_SOURCE_IND:
            bapServerOnModifySourceInd(bapInst, (GattBassServerModifySourceInd *)message);
            break;
        case GATT_BASS_SERVER_BROADCAST_CODE_IND:
            bapServerOnBroadcastCodeInd(bapInst, (GattBassServerBroadcastCodeInd *)message);
            break;
        case GATT_BASS_SERVER_REMOVE_SOURCE_IND:
            bapServerOnRemoveSourceInd(bapInst, (GattBassServerRemoveSourceInd *)message);
            break;
        case GATT_BASS_SERVER_CONFIG_CHANGE_IND:
            bapServerOnBassConfigChangeInd(bapInst, (GattBassServerConfigChangeInd *)message);
            break;
        default:
            BAP_SERVER_WARNING("PANIC: BAP Server: unhandled bass prim\n");
            break;
    }
}

uint8 * BapServerGetSourceIdsReq(bapProfileHandle profileHandle,
                                 uint16 *sourceIdNum)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if (bapInst)
    {
        return GattBassServerGetSourceIdsRequest(bapInst->bassHandle,
                                                 sourceIdNum);
    }

    return NULL;
}


bapStatus  BapServerAddBroadcastSourceReq(bapProfileHandle profileHandle,
                                          uint8 *sourceId,
                                          BapServerBassReceiveState *sourceInfo)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if (bapInst)
    {
        return GattBassServerAddBroadcastSourceRequest(bapInst->bassHandle,
                                                       sourceId,
                                                       (GattBassServerReceiveState*)sourceInfo);
    }

    return BAP_SERVER_STATUS_FAILED;
}

bapStatus BapServerGetBroadcastReceiveStateReq(bapProfileHandle profileHandle,
                                               uint8 sourceId,
                                               BapServerBassReceiveState *state)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if (bapInst)
    {
        return GattBassServerGetBroadcastReceiveStateRequest(bapInst->bassHandle,
                                                             sourceId,
                                                             (GattBassServerReceiveState*)state);
    }
    return BAP_SERVER_STATUS_FAILED;
}

bapStatus  BapServerModifyBroadcastSourceReq(bapProfileHandle profileHandle,
                                             uint8 sourceId,
                                             BapServerBassReceiveState *sourceInfo)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if (bapInst)
    {
        return GattBassServerModifyBroadcastSourceRequest(bapInst->bassHandle,
                                                          sourceId,
                                                          (GattBassServerReceiveState*)sourceInfo);
    }

    return BAP_SERVER_STATUS_FAILED;
}

bapStatus  BapServerRemoveBroadcastSourceReq(bapProfileHandle profileHandle,
                                             uint8 sourceId)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if (bapInst)
    {
        return GattBassServerRemoveBroadcastSourceRequest(bapInst->bassHandle,
                                                          sourceId);
    }
    return BAP_SERVER_STATUS_FAILED;
}

uint8* BapServerGetBroadcastCodeReq(bapProfileHandle profileHandle,
                                    uint8 sourceId)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if (bapInst)
    {
        return GattBassServerGetBroadcastCodeRequest(bapInst->bassHandle,
                                                     sourceId);
    }

    return NULL;
}

bapStatus BapServerSetBroadcastCodeReq(bapProfileHandle profileHandle,
                                       uint8 sourceId,
                                       uint8 *broadcastCode)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if (bapInst)
    {
        return GattBassServerSetBrodcastCodeRequest(bapInst->bassHandle,
                                                    sourceId,
                                                    broadcastCode);
    }
    return BAP_SERVER_STATUS_FAILED;
}

bool BapServerIsAnyClientConnected(bapProfileHandle profileHandle)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if (bapInst)
    {
        return GattBassServerIsAnyClientConnected(bapInst->bassHandle);
    }

    return FALSE;
}

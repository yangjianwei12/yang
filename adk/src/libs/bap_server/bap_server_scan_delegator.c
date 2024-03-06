/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd.
* 
************************************************************************* ***/

#include "bap_server_debug.h"
#include "bap_server.h"
#include "bap_server_common.h"
#include "bap_server_private.h"
#include "bap_server_scan_delegator.h"

static void bapServerOnScanningStateInd(BAP *bapInst,
                                        GattBassServerScanningStateInd* ind)
{
    if(ind)
    {
        BAP_DEBUG_INFO(("BapServerOnScanningStateInd scanning_state=0x%x", ind->clientScanningState));

        BapServerBassScanningStateInd * message = NULL;
        message = (BapServerBassScanningStateInd*)PanicUnlessMalloc(sizeof(BapServerBassScanningStateInd));

        message->type = BAP_SERVER_BASS_SCANNING_STATE_IND;
        message->cid = ind->cid;
        message->clientScanningState= ind->clientScanningState;

        BapServerMessageSend(bapInst->appBroadcastTask, message);
    }
    else
    {
        BAP_DEBUG_INFO(("BapServerOnScanningStateInd. NULL ind"));
    }
}

static void bapServerOnAddSourceInd(BAP *bapInst,
                                    GattBassServerAddSourceInd * ind)
{
    if(ind)
    {
        BAP_DEBUG_INFO(("BapServerOnAddSourceInd pa_sync=0x%x source_adv_sid=0x%x addr=[%x %x:%x:%lx]",
                        ind->paSync,
                        ind->sourceAdvSid,
                        ind->advertiseAddressType,
                        ind->advertiserAddress.uap,
                        ind->advertiserAddress.nap,
                        ind->advertiserAddress.lap));

        BapServerBassAddSourceInd * message = NULL;
        message = (BapServerBassAddSourceInd*)PanicUnlessMalloc(sizeof(BapServerBassAddSourceInd));

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
        if(message->source.numSubGroups)
        {
            message->source.subGroupsData = ind->subGroupsData;
        }
        else
        {
            message->source.subGroupsData = NULL;
        }

        BapServerMessageSend(bapInst->appBroadcastTask, message);
    }
    else
    {
        BAP_DEBUG_INFO(("BapServerOnAddSourceInd. NULL ind"));
    }
}

static void bapServerOnModifySourceInd(BAP *bapInst,
                                       GattBassServerModifySourceInd * ind)
{
    if(ind)
    {
        BAP_DEBUG_INFO(("BapServerOnModifySourceInd source_id=0x%x pa_sync=0x%x",
                        ind->sourceId,
                        ind->paSyncState));

        BapServerBassModifySourceInd * message = NULL;
        message = (BapServerBassModifySourceInd*)PanicUnlessMalloc(sizeof(BapServerBassModifySourceInd));

        message->type = BAP_SERVER_BASS_MODIFY_SOURCE_IND;
        message->source.cid = ind->cid;
        message->source.sourceId = ind->sourceId;
        message->source.paSyncState = ind->paSyncState;
        message->source.paInterval = ind->paInterval;
        message->source.numSubGroups = ind->numSubGroups;
        if(message->source.numSubGroups)
        {
            message->source.subGroupsData = ind->subGroupsData;
        }
        else
        {
            message->source.subGroupsData = NULL;
        }

        BapServerMessageSend(bapInst->appBroadcastTask, message);
    }
    else
    {
        BAP_DEBUG_INFO(("BapServerOnModifySourceInd. NULL ind"));
    }
}

static void bapServerOnBroadcastCodeInd(BAP *bapInst,
                                        GattBassServerBroadcastCodeInd * ind)
{
    if(ind)
    {
        BAP_DEBUG_INFO(("BapServerOnBroadcastCodeInd source_id=0x%x", ind->sourceId));

        BapServerBassBroadcastCodeInd * message = NULL;
        message = (BapServerBassBroadcastCodeInd*)PanicUnlessMalloc(sizeof(BapServerBassBroadcastCodeInd));

        message->type = BAP_SERVER_BASS_BROADCAST_CODE_IND;
        message->code.cid = ind->cid;
        message->code.sourceId = ind->sourceId;
        message->code.broadcastCode = PanicUnlessMalloc(SCAN_DELEGATOR_BROADCAST_CODE_SIZE);
        memcpy(message->code.broadcastCode, ind->broadcastCode, SCAN_DELEGATOR_BROADCAST_CODE_SIZE);

        BapServerMessageSend(bapInst->appBroadcastTask, message);
    }
    else
    {
        BAP_DEBUG_INFO(("BapServerOnBroadcastCodeInd. NULL ind"));
    }
}

static void bapServerOnRemoveSourceInd(BAP *bapInst,
                                       GattBassServerRemoveSourceInd * ind)
{    
    if(ind)
    {
        BAP_DEBUG_INFO(("BapServerOnRemoveSourceInd source_id=0x%x", ind->sourceId));

        BapServerBassRemoveSourceInd * message = NULL;
        message = (BapServerBassRemoveSourceInd*)PanicUnlessMalloc(sizeof(BapServerBassRemoveSourceInd));

        message->type = BAP_SERVER_BASS_REMOVE_SOURCE_IND;
        message->source.cid = ind->cid;
        message->source.sourceId = ind->sourceId;

        BapServerMessageSend(bapInst->appBroadcastTask, message);
    }
    else
    {
        BAP_DEBUG_INFO(("BapServerOnRemoveSourceInd. NULL ind"));
    }
}

void bapServerHandleGattBassServerMsg(BAP *bapInst, MessageId id,
                                      void *message)
{
    switch (id)
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
        default:
            BAP_DEBUG_PANIC(("PANIC: BAP Server: unhandled bass prim\n"));
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
    else
        BAP_DEBUG_INFO(("BapServerGetSourceIdsReq : BAP instance is NULL"));

    return NULL;
}


bapStatus  BapServerAddBroadcastSourceReq(bapProfileHandle profileHandle,
                                          uint8* sourceId,
                                          BapServerBassReceiveState *sourceInfo)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if (bapInst)
    {
        return GattBassServerAddBroadcastSourceRequest(bapInst->bassHandle,
                                                       sourceId,
                                                       (GattBassServerReceiveState*)sourceInfo);
    }
    else
        BAP_DEBUG_INFO(("BapServerAddBroadcastSourceReq : BAP instance is NULL"));

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
    else
        BAP_DEBUG_INFO(("BapServerGetBroadcastReceiveStateReq : BAP instance is NULL"));

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
    else
        BAP_DEBUG_INFO(("BapServerModifyBroadcastSourceReq : BAP instance is NULL"));

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
    else
        BAP_DEBUG_INFO(("BapServerRemoveBroadcastSourceReq : BAP instance is NULL"));

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
    else
        BAP_DEBUG_INFO(("BapServerGetBroadcastCodeReq : BAP instance is NULL"));

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
    else
        BAP_DEBUG_INFO(("BapServerSetBroadcastCodeReq : BAP instance is NULL"));

    return BAP_SERVER_STATUS_FAILED;
}

bool BapServerIsAnyClientConnected(bapProfileHandle profileHandle)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    if (bapInst)
    {
        return GattBassServerIsAnyClientConnected(bapInst->bassHandle);
    }
    else
        BAP_DEBUG_INFO(("BapServerIsAnyClientConnected : BAP instance is NULL"));

    return FALSE;
}

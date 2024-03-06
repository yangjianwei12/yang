/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd.
* 
************************************************************************* ***/

#include "bap_server_private.h"
#include "bap_server_msg_handler.h"
#include "bap_server_debug.h"
#include "bap_server_init.h"
#include "bap_server_common.h"
#include "bap_server.h"
#include "connection_no_ble.h"

static void bapServerSendSetupIsoDataPathCfm(Task appTask,
                                             uint16 isoHandle,
                                             uint8  resultCode)
{
    BapServerSetupDataPathCfm * message = NULL;

    message = (BapServerSetupDataPathCfm*)PanicUnlessMalloc(sizeof(BapServerSetupDataPathCfm));

    BAP_DEBUG_INFO(("bapServerSendSetupIsoDataPathCfm isoHandle =0x%x ", isoHandle));

    /* Fill BAP upstream primitive */
    message->type = BAP_SERVER_SETUP_DATA_PATH_CFM;
    message->isoHandle= isoHandle;
    if(resultCode == hci_success)
        message->status= BAP_SERVER_STATUS_SUCCESS;
    else
        message->status= (bapStatus)resultCode;
    BapServerMessageSend(appTask, message);
}

static void bapServerSendRemoveIsoDataPathCfm(BAP *bapInst,
                                              uint16 isoHandle,
                                              bapStatus status)
{
    BapServerRemoveDataPathCfm * message = NULL;

    message = (BapServerRemoveDataPathCfm*)PanicUnlessMalloc(sizeof(BapServerRemoveDataPathCfm));

    BAP_DEBUG_INFO(("bapServerSendRemoveIsoDataPathCfm isoHandle =0x%x ", isoHandle));

    /* Fill BAP upstream primitive */
    message->type = BAP_SERVER_REMOVE_DATA_PATH_CFM;
    message->isoHandle= isoHandle;
    message->status = status;
    
    BapServerMessageSend(bapInst->pendingTask, message);
}

static void bapServerOnBigInfoAdvReportInd(BAP *bapInst,
                                           CL_DM_BLE_BIGINFO_ADV_REPORT_IND_T * ind)
{
    BapServerBigInfoAdvReportInd * message = NULL;
    BAP_DEBUG_INFO(("bapServerOnBigInfoAdvReportInd Sync Handle=0x%x", ind->sync_handle));

    message = (BapServerBigInfoAdvReportInd*)PanicUnlessMalloc(sizeof(BapServerBigInfoAdvReportInd));

    /* Fill BAP upstream primitive */
    message->type = BAP_SERVER_BIGINFO_ADV_REPORT_IND;
    message->syncHandle = ind->sync_handle;
    message->numBis = ind->num_bis;
    message->nse = ind->nse;
    message->isoInterval = ind->iso_interval;
    message->bn = ind->bn;
    message->pto = ind->pto;
    message->irc = ind->irc;
    message->maxPdu = ind->max_pdu;
    message->sduInterval = ind->sdu_interval;
    message->maxSdu = ind->max_sdu;
    message->phy = ind->phy;
    message->framing = ind->framing;
    message->encryption = ind->encryption;

    BapServerMessageSend(bapInst->appSinkTask, message);
}



static void bapServerOnSetupIsoDataPathCfm(BAP *bapInst,
                                           CL_DM_ISOC_SETUP_ISOCHRONOUS_DATA_PATH_CFM_T * cfm)
{
    BAP_DEBUG_INFO(("bapServerOnSetupIsoDataPathCfm status=%d cisHandle=0x%x", cfm->status, cfm->handle));

    bapServerSendSetupIsoDataPathCfm(bapInst->pendingTask,
                                        cfm->handle,
                                        cfm->status);
}

static void bapServerSendIsoBigSyncCfm(BAP *bapInst,
                                       CL_DM_ISOC_BIG_CREATE_SYNC_CFM_T *msg)
{
    BapServerIsocBigCreateSyncCfm * message = NULL;

    message = (BapServerIsocBigCreateSyncCfm*)PanicUnlessMalloc(sizeof(BapServerIsocBigCreateSyncCfm));

    BAP_DEBUG_INFO(("bapServerSendIsoBigSyncCfm isoHandle =0x%x ", msg->big_handle));

    /* Fill BAP upstream primitive */
    message->type = BAP_SERVER_ISOC_BIG_CREATE_SYNC_CFM;
    if(msg->status == hci_success)
        message->status= BAP_SERVER_STATUS_SUCCESS;
    else
        message->status= msg->status;

    message->bigHandle = msg->big_handle;
    message->numBis = msg->num_bis;
    message->bigParams.transportLatencyBig = msg->transport_latency_big;
    message->bigParams.maxPdu = msg->max_pdu;
    message->bigParams.isoInterval = msg->iso_interval;
    message->bigParams.phy = 1;   /* TODO API parity */
    message->bigParams.nse = msg->nse;
    message->bigParams.bn = msg->bn;
    message->bigParams.pto = msg->pto;
    message->bigParams.irc = msg->irc;
    message->bisHandles =  msg->bis_handle;

    BapServerMessageSend(bapInst->appSinkTask, message);
}

static void bapServerSendIsoBigTerminateSyncInd(BAP *bapInst,
                                                CL_DM_ISOC_BIG_TERMINATE_SYNC_IND_T *msg)
{
    BapServerIsocBigTerminateSyncInd * message = NULL;

    message = (BapServerIsocBigTerminateSyncInd*)PanicUnlessMalloc(sizeof(BapServerIsocBigTerminateSyncInd));

    BAP_DEBUG_INFO(("bapServerSendIsoBigTerminateSyncInd isoHandle =0x%x ", msg->big_handle));

    /* Fill BAP upstream primitive */
    message->type = BAP_SERVER_ISOC_BIG_TERMINATE_SYNC_IND;
    if(msg->status_or_reason == hci_success)
        message->status= BAP_SERVER_STATUS_SUCCESS;
    else
        message->status= (bapStatus)msg->status_or_reason;

    message->bigHandle = msg->big_handle;
    BapServerMessageSend(bapInst->appSinkTask, message);
}

void bapServerHandleClMsg(BAP *bapInst, MessageId id,
                          void *message)
{
    switch (id)
    {
        case CL_DM_ISOC_REGISTER_CFM:
            BAP_DEBUG_INFO(("CL_DM_ISOC_REGISTER_CFM\n"));
            break;
        case CL_DM_BLE_BIGINFO_ADV_REPORT_IND:
            bapServerOnBigInfoAdvReportInd(bapInst,(CL_DM_BLE_BIGINFO_ADV_REPORT_IND_T *)message);
            break;
        case CL_DM_ISOC_SETUP_ISOCHRONOUS_DATA_PATH_CFM:
            {
                bapServerOnSetupIsoDataPathCfm(bapInst, (CL_DM_ISOC_SETUP_ISOCHRONOUS_DATA_PATH_CFM_T *)message);
            }
            break;
        case CL_DM_ISOC_REMOVE_ISO_DATA_PATH_CFM:
            {
                CL_DM_ISOC_REMOVE_ISO_DATA_PATH_CFM_T *msg = (CL_DM_ISOC_REMOVE_ISO_DATA_PATH_CFM_T *)message;
                bapStatus status = BAP_SERVER_STATUS_SUCCESS;
                if(msg->status != hci_success)
                {
                    status = (bapStatus)msg->status;
                }

                bapServerSendRemoveIsoDataPathCfm(bapInst,
                                                  msg->handle,
                                                  status);
            }
            break;
        case CL_DM_ISOC_BIG_CREATE_SYNC_CFM:
            {
                bapServerSendIsoBigSyncCfm(bapInst,
                                          (CL_DM_ISOC_BIG_CREATE_SYNC_CFM_T *)message);
            }
            break;
        case CL_DM_ISOC_BIG_TERMINATE_SYNC_IND:
            {
                bapServerSendIsoBigTerminateSyncInd(bapInst,
                                                   (CL_DM_ISOC_BIG_TERMINATE_SYNC_IND_T *)message);
            }
            break;

        default:
            BAP_DEBUG_PANIC(("bapServerHandleCLsMsg Unhandled CM Primitive: 0x%x", id));
            break;
    }

}


void BapServerSetupIsoDataPathReq(bapProfileHandle profileHandle,
                                  BapIsoDataType isoDataType,
                                  const BapServerSetupDataPathReq *dataPathParameters)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if (bapInst)
    {
        bapInst->pendingTask = ( isoDataType == BAP_ISO_UNICAST ) ?
            bapInst->appUnicastTask : bapInst->appSinkTask;

        ConnectionIsocSetupIsochronousDataPathRequest(&bapInst->libTask,
                                      dataPathParameters->isoHandle,
                                      dataPathParameters->dataPathDirection,
                                      dataPathParameters->dataPathId);
    }
    else
    {
        BAP_DEBUG_PANIC(("BapServerSetupIsoDataPathReq Invalid bap profile handle: 0x%x", profileHandle));
    }
}

void BapServerRemoveIsoDataPathReq(bapProfileHandle profileHandle,
                                   uint16 isoHandle,
                                   BapIsoDataType isoDataType,
                                   uint8 dataPathDirection)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if (bapInst)
    {
        bapInst->pendingTask = ( isoDataType == BAP_ISO_UNICAST ) ?
            bapInst->appUnicastTask : bapInst->appSinkTask;

        ConnectionIsocRemoveIsoDataPathRequest(&bapInst->libTask,
                                       isoHandle,
                                       dataPathDirection);
    }
    else
    {
        BAP_DEBUG_PANIC(("BapServerRemoveIsoDataPathReq Invalid bap profile handle: 0x%x", profileHandle));
    }
}

void BapServerUnicastCisDisconnectReq(bapProfileHandle profileHandle,
                                      uint16 cisHandle,
                                      uint8 disconnectReason)
{
    UNUSED(profileHandle);
    UNUSED(cisHandle);
    UNUSED(disconnectReason);
}

void BapServerBroadcastBigCreateSyncReq(Task appTask,
                                        bapProfileHandle profileHandle,
                                        uint16 syncHandle,
                                        uint16 bigSyncTimeout,
                                        uint8 bigHandle,
                                        uint8 mse,
                                        uint8 encryption,
                                        uint8 *broadcastCode,
                                        uint8 numBis,
                                        uint8 *bis)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    if (bapInst)
    {
        if(appTask != NULL)
        {
            bapInst->appSinkTask = appTask;
        }
        
        ConnectionIsocBigCreateSyncRequest(&bapInst->libTask,
                                   syncHandle,
                                   bigSyncTimeout,
                                   bigHandle,
                                   mse,
                                   encryption,
                                   broadcastCode,
                                   numBis,
                                   bis);
    }
    else
    {
        BapServerIsocBigCreateSyncCfm * message = NULL;
        message = (BapServerIsocBigCreateSyncCfm*)PanicUnlessMalloc(sizeof(BapServerIsocBigCreateSyncCfm));

        message->type = BAP_SERVER_ISOC_BIG_CREATE_SYNC_CFM;
        message->status= BAP_SERVER_STATUS_INVALID_PARAMETER;

        message->bigHandle = bigHandle;
        message->numBis = numBis;
        memset(&message->bigParams, 0, sizeof(BapSereverBigParam));
        if(numBis)
        {
            message->bisHandles =  PanicUnlessMalloc(numBis * sizeof(uint16));
            memcpy(message->bisHandles, bis, numBis);
        }
        
        BapServerMessageSend(appTask, message);
    }
}

void BapServerBroadcastBigTerminateSyncReq(bapProfileHandle profileHandle,
                                           uint8 bigHandle)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    if (bapInst)
    {
        ConnectionIsocBigTerminateSyncRequest( bigHandle);
    }
}


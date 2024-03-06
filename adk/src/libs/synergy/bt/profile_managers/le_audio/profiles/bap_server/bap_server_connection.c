/****************************************************************************
* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
* %%version
************************************************************************* ***/

#include "bap_server_private.h"
#include "bap_server_msg_handler.h"
#include "bap_server_debug.h"
#include "bap_server_init.h"
#include "bap_server_common.h"
#include "bluetooth.h"
#include "bap_server_lib.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_gatt_client_util_lib.h"

typedef struct
{
    uint16 cisHandle;
    uint8 cisId;
} connectionInProgress;

static connectionInProgress cisInprogress;

static void bapServerUnicastOnCisConnectInd(CmIsocCisConnectInd * ind)
{
    uint8 status = HCI_ERROR_UNSPECIFIED;

    cisInprogress.cisHandle = ind->cis_handle;
    cisInprogress.cisId = ind->cis_id;
    status = CSR_BT_RESULT_CODE_SUCCESS;

    BAP_SERVER_DEBUG("bapServerUnicastOnCisConnectInd sending response cig=0x%x cis=0x%x cisHandle=0x%x", ind->cig_id, ind->cis_id, ind->cis_handle);
    CmIsocCisConnectRspSend(CSR_BT_BAP_SERVER_IFACEQUEUE,
                            ind->cis_handle,
                            status);
}

static void bapServerSendCisDisconnected(BAP *bapInst,
                                         uint16 cisHandle,
                                         uint8 reason)
{
    BapServerCisDisconnectedInd * message = NULL;

    message = (BapServerCisDisconnectedInd *)CsrPmemZalloc(sizeof(BapServerCisDisconnectedInd));

    BAP_SERVER_DEBUG("bapServerSendCisDisconnected cisId=0x%x ", cisInprogress.cisId);

    /* Fill BAP upstream primitive */
    message->type = BAP_SERVER_CIS_DISCONNECTED_IND;
    message->cisHandle = cisHandle;
    message->reason = reason;
    BapServerMessageSend(bapInst->appUnicastTask, message);
    if(cisInprogress.cisHandle == cisHandle)
    {
        memset(&cisInprogress, 0, sizeof(connectionInProgress));
    }
}

static void bapServerSendSetupIsoDataPathCfm(AppTask appTask,
                                             uint16 isoHandle,
                                             CsrBtResultCode  resultCode)
{
    BapServerSetupDataPathCfm * message = NULL;

    message = (BapServerSetupDataPathCfm*)CsrPmemZalloc(sizeof(BapServerSetupDataPathCfm));

    /* Fill BAP upstream primitive */
    message->type = BAP_SERVER_SETUP_DATA_PATH_CFM;
    message->isoHandle= isoHandle;
    if(resultCode == CSR_BT_RESULT_CODE_SUCCESS)
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

    message = (BapServerRemoveDataPathCfm*)CsrPmemZalloc(sizeof(BapServerRemoveDataPathCfm));

    /* Fill BAP upstream primitive */
    message->type = BAP_SERVER_REMOVE_DATA_PATH_CFM;
    message->isoHandle= isoHandle;
    message->status = status;
    
    BapServerMessageSend(bapInst->pendingTask, message);
}

static void bapServerUnicastOnCisConnectCfm(BAP *bapInst, CmIsocCisConnectCfm * cfm)
{
    if(cfm->resultCode == CSR_BT_RESULT_CODE_SUCCESS)
    {
        BapServerCisEstablishedInd * message =(BapServerCisEstablishedInd*)
                                CsrPmemZalloc(sizeof(BapServerCisEstablishedInd));

        BAP_SERVER_DEBUG("bapServerUnicastOnCisConnectCfm cisId=0x%x cis Handle=0x%x",
                         cisInprogress.cisId, cisInprogress.cisHandle);

        message->type = BAP_SERVER_CIS_ESTABLISHED_IND;
        message->cisId = cisInprogress.cisId;
        message->cisHandle = cfm->cis_handle;
        message->cisParams.cigSyncDelay = cfm->cis_params.cig_sync_delay;
        message->cisParams.cisSyncDelay = cfm->cis_params.cis_sync_delay;
        message->cisParams.transportLatencyMtoS = cfm->cis_params.transport_latency_m_to_s;
        message->cisParams.transportLatencyStoM = cfm->cis_params.transport_latency_s_to_m;
        message->cisParams.maxPduMtoS = cfm->cis_params.max_pdu_m_to_s;
        message->cisParams.maxPduStoM = cfm->cis_params.max_pdu_s_to_m;
        message->cisParams.isoInterval = cfm->cis_params.iso_interval;
        message->cisParams.phyMtoS = cfm->cis_params.phy_m_to_s;
        message->cisParams.phyStoM = cfm->cis_params.phy_s_to_m;
        message->cisParams.nse = cfm->cis_params.nse;
        message->cisParams.bnMtoS = cfm->cis_params.bn_m_to_s;
        message->cisParams.bnStoM = cfm->cis_params.bn_s_to_m;
        message->cisParams.ftMtoS = cfm->cis_params.ft_m_to_s;
        message->cisParams.ftStoM = cfm->cis_params.ft_s_to_m;
        message->connectionId = CsrBtGattClientUtilFindConnIdByAddr(&cfm->tp_addr.addrt);

        BapServerMessageSend(bapInst->appUnicastTask, message);
    }
    else
    {
        memset(&cisInprogress, 0, sizeof(connectionInProgress));
    }
}

static void bapServerOnBigInfoAdvReportInd(BAP *bapInst,
                                           CmBleBigInfoAdvReportInd * ind)
{
    BapServerBigInfoAdvReportInd * message = NULL;
    BAP_SERVER_DEBUG("bapServerOnBigInfoAdvReportInd Sync Handle=0x%x", ind->syncHandle);

    message = (BapServerBigInfoAdvReportInd*)CsrPmemZalloc(sizeof(BapServerBigInfoAdvReportInd));

    /* Fill BAP upstream primitive */
    message->type = BAP_SERVER_BIGINFO_ADV_REPORT_IND;
    message->syncHandle = ind->syncHandle;
    message->numBis = ind->numBis;
    message->nse = ind->bigParams.nse;
    message->isoInterval = ind->bigParams.iso_interval;
    message->bn = ind->bigParams.bn;
    message->pto = ind->bigParams.pto;
    message->irc = ind->bigParams.irc;
    message->maxPdu = ind->maxSdu;
    message->sduInterval = ind->sduInterval;
    message->maxSdu = ind->maxSdu;
    message->phy = ind->bigParams.phy;
    message->framing = ind->framing;
    message->encryption = ind->encryption;

    BapServerMessageSend(bapInst->appSinkTask, message);
}

static void bapServerOnSetupIsoDataPathCfm(BAP *bapInst,
                                           CmIsocSetupIsoDataPathCfm * cfm)
{
    bapServerSendSetupIsoDataPathCfm(bapInst->pendingTask, cfm->handle,
                                        cfm->resultCode);
}

static void bapServerUnicastOnCisDisconnectCfm(BAP *bapInst,
                                               uint16  cisHandle,
                                               bapStatus status)
{
    BapServerCisDisconnectedCfm * message = NULL;

    message = (BapServerCisDisconnectedCfm *)CsrPmemZalloc(sizeof(BapServerCisDisconnectedCfm));

    /* Fill BAP upstream primitive */
    message->type = BAP_SERVER_CIS_DISCONNECTED_CFM;
    message->cisHandle = cisHandle;
    message->status = status;
    BapServerMessageSend(bapInst->appUnicastTask, message);

    if(cisInprogress.cisHandle == cisHandle)
    {
        memset(&cisInprogress, 0, sizeof(connectionInProgress));
    }
}

static void bapServerUnicastOnCisDisconnectInd(BAP *bapInst,
                                               CmIsocCisDisconnectInd * ind)
{
     bapServerSendCisDisconnected(bapInst, ind->cis_handle, ind->reason);
}


static void bapServerSendIsoBigSyncCfm(BAP *bapInst,
                                       CmIsocBigCreateSyncCfm *msg)
{
    BapServerIsocBigCreateSyncCfm * message = NULL;

    message = (BapServerIsocBigCreateSyncCfm*)CsrPmemZalloc(sizeof(BapServerIsocBigCreateSyncCfm));

    /* Fill BAP upstream primitive */
    message->type = BAP_SERVER_ISOC_BIG_CREATE_SYNC_CFM;
    if(msg->resultCode == CSR_BT_RESULT_CODE_SUCCESS)
        message->status= BAP_SERVER_STATUS_SUCCESS;
    else
        message->status= msg->resultCode;

    message->bigHandle = msg->big_handle;
    message->numBis = msg->num_bis;
    memcpy(&message->bigParams, &msg->big_params, sizeof(CmBigParam));
    if(msg->num_bis)
    {
        message->bisHandles =  CsrPmemZalloc(msg->num_bis* sizeof(uint16));
        memcpy(message->bisHandles, msg->bis_handles, (msg->num_bis* sizeof(uint16)));
    }
    
    BapServerMessageSend(bapInst->appSinkTask, message);
}

static void bapServerSendIsoBigTerminateSyncInd(BAP *bapInst,
                                                CmIsocBigTerminateSyncInd *msg)
{
    BapServerIsocBigTerminateSyncInd * message = NULL;

    message = (BapServerIsocBigTerminateSyncInd*)CsrPmemZalloc(sizeof(BapServerIsocBigTerminateSyncInd));

    /* Fill BAP upstream primitive */
    message->type = BAP_SERVER_ISOC_BIG_TERMINATE_SYNC_IND;
    if(msg->resultCode == CSR_BT_RESULT_CODE_SUCCESS)
        message->status= BAP_SERVER_STATUS_SUCCESS;
    else
        message->status= msg->resultCode;

    message->bigHandle = msg->big_handle;
    BapServerMessageSend(bapInst->appSinkTask, message);
}

void bapServerHandleCMMsg(BAP *bapInst,
                          void *message)
{
    CsrBtCmPrim *primType = (CsrBtCmPrim *) message;

    BAP_SERVER_DEBUG("bapServerHandleCMMsg MESSAGE:CsrBtCmPrim:0x%x", *primType);

    switch (*primType)
    {
        case CSR_BT_CM_ISOC_REGISTER_CFM:
             break;
        case CSR_BT_CM_ISOC_CIS_CONNECT_CFM:
            {
                bapServerUnicastOnCisConnectCfm(bapInst, (CmIsocCisConnectCfm *)message);
            }
            break;
        case CSR_BT_CM_ISOC_CIS_CONNECT_IND:
            {
                bapServerUnicastOnCisConnectInd((CmIsocCisConnectInd *)message);
            }
            break;
        case CSR_BT_CM_ISOC_CIS_DISCONNECT_CFM:
            {
                CmIsocCisDisconnectCfm *cfm = (CmIsocCisDisconnectCfm *)message;
                bapStatus status = BAP_SERVER_STATUS_SUCCESS;
                if(cfm->resultCode != CSR_BT_RESULT_CODE_SUCCESS )
                {
                    /* map to HCI error code */
                    status = (bapStatus)cfm->resultCode;
                }
                bapServerUnicastOnCisDisconnectCfm(bapInst,
                                                   cfm->cis_handle,
                                                   status);
            }
            break;
        case CSR_BT_CM_ISOC_CIS_DISCONNECT_IND:
            {
                bapServerUnicastOnCisDisconnectInd(bapInst, (CmIsocCisDisconnectInd *)message);
            }
            break;
        case CSR_BT_CM_ISOC_SETUP_ISO_DATA_PATH_CFM:
            {
                bapServerOnSetupIsoDataPathCfm(bapInst, (CmIsocSetupIsoDataPathCfm *)message);
            }
            break;
        case CSR_BT_CM_ISOC_REMOVE_ISO_DATA_PATH_CFM:
            {
                CmIsocRemoveIsoDataPathCfm *msg = (CmIsocRemoveIsoDataPathCfm *)message;
                bapStatus status = BAP_SERVER_STATUS_SUCCESS;
                if(msg->resultCode != CSR_BT_RESULT_CODE_SUCCESS)
                {
                    status = msg->resultCode;
                }

                bapServerSendRemoveIsoDataPathCfm(bapInst,
                                                  msg->handle,
                                                  status);
            }
            break;
        case CSR_BT_CM_ISOC_BIG_CREATE_SYNC_CFM:
            {
                bapServerSendIsoBigSyncCfm(bapInst,
                                          (CmIsocBigCreateSyncCfm *)message);
            }
            break;
        case CSR_BT_CM_BLE_BIGINFO_ADV_REPORT_IND:
            bapServerOnBigInfoAdvReportInd(bapInst,(CmBleBigInfoAdvReportInd *)message);
            break;

        case CSR_BT_CM_ISOC_BIG_TERMINATE_SYNC_IND:
            {
                bapServerSendIsoBigTerminateSyncInd(bapInst,
                                                   (CmIsocBigTerminateSyncInd *)message);
            }
            break;

        default:
            BAP_SERVER_WARNING("bapServerHandleCMMsg Unhandled CM Primitive: 0x%x", *primType);
            break;
    }

    CsrBtCmFreeUpstreamMessageContents(CSR_BT_CM_PRIM, (void *) message);
}


void BapServerSetupIsoDataPathReq(bapProfileHandle profileHandle,
                                  BapIsoDataType isoDataType,
                                  const BapServerSetupDataPathReq *dataPathParameters)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    uint8 codecId[BAP_CODEC_ID_SIZE];

    if (bapInst)
    {
        bapInst->pendingTask = ( isoDataType == BAP_ISO_UNICAST ) ?
            bapInst->appUnicastTask : bapInst->appSinkTask;

        CsrMemCpy(codecId, dataPathParameters->codecId, BAP_CODEC_ID_SIZE);

        CmIsocSetupIsoDataPathReqSend(CSR_BT_BAP_SERVER_IFACEQUEUE,
                                      dataPathParameters->isoHandle,
                                      dataPathParameters->dataPathDirection,
                                      dataPathParameters->dataPathId,
                                      codecId,
                                      dataPathParameters->controllerDelay,
                                      dataPathParameters->codecConfigLength,
                                      dataPathParameters->codecConfigParams);
    }
    else
    {
        BAP_SERVER_DEBUG("BapServerSetupIsoDataPathReq Invalid bap profile handle");
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

        CmIsocRemoveIsoDataPathReqSend(CSR_BT_BAP_SERVER_IFACEQUEUE,
                                       isoHandle,
                                       dataPathDirection);
    }
    else
    {
        BAP_SERVER_DEBUG("BapServerRemoveIsoDataPathReq Invalid bap profile handle");
    }
}

void BapServerUnicastCisDisconnectReq(bapProfileHandle profileHandle,
                                      uint16 cisHandle,
                                      uint8 disconnectReason)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if (bapInst)
    {
        CmIsocCisDisconnectReqSend(CSR_BT_BAP_SERVER_IFACEQUEUE,
                                   cisHandle,
                                   disconnectReason);
    }
    else
    {
        BAP_SERVER_DEBUG("BapServerUnicastCisDisconnectReq Invalid bap profile handle");
    }
}

void BapServerBroadcastBigCreateSyncReq(AppTask appTask,
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
        if(appTask != CSR_SCHED_QID_INVALID)
        {
            bapInst->appSinkTask = appTask;
        }
        
        CmIsocBigCreateSyncReqSend(CSR_BT_BAP_SERVER_IFACEQUEUE,
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
        message = (BapServerIsocBigCreateSyncCfm*)CsrPmemZalloc(sizeof(BapServerIsocBigCreateSyncCfm));

        message->type = BAP_SERVER_ISOC_BIG_CREATE_SYNC_CFM;
        message->status= BAP_SERVER_STATUS_INVALID_PARAMETER;

        message->bigHandle = bigHandle;
        message->numBis = numBis;
        memset(&message->bigParams, 0, sizeof(BapSereverBigParam));
        if(numBis)
        {
            message->bisHandles =  CsrPmemAlloc(numBis * sizeof(uint16));
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
        CmIsocBigTerminateSyncReqSend(CSR_BT_BAP_SERVER_IFACEQUEUE,
                                      bigHandle);
    }
}


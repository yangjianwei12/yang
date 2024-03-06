/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "cap_client_util.h"
#include "cap_client_common.h"
#include "cap_client_ase.h"
#include "cap_client_csip_handler.h"
#include "cap_client_debug.h"
#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
#include "cap_client_broadcast_assistant_periodic_scan.h"
#include "cap_client_broadcast_assistant_add_modify_src_req.h"

static CapClientPeriodicScanParameters scanParams;
static CapClientRegisterForNoifyParams notiParams;


/******************************************* STOP SCAN ************************************************/
void capClientHandleBroadcastAssistantStopScanCfm(CAP_INST* inst,
                                                  Msg msg,
                                                  CapClientGroupInstance* cap)
{
    AppTask appTask;
    BapBroadcastAssistantStopScanCfm* cfm = NULL;
    BapProfileHandle bapHandle = 0;
    BapInstElement* bap = NULL;
    CapClientResult result = CAP_CLIENT_RESULT_NULL_INSTANCE;


    if (inst == NULL)
    {
        CAP_CLIENT_PANIC("\n capClientHandleBroadcastAssistantStartScanCfm: NULL instance \n");
        /* Do nothing */
        return;
    }

    appTask = inst->profileTask;

    if (cap == NULL)
    {
        CAP_CLIENT_INFO("\n capClientHandleBroadcastAssistantStartScanCfm: BAP NULL instance \n");
        /* Do nothing */
        return;
    }

    if (capClientBcastAsstOpComplete(cap))
    {
        /* if all the counters are clear then there is no need to proceed, just return*/
        CAP_CLIENT_INFO("\n capClientHandleBroadcastAssistantStartScanCfm: BAP NULL instance \n");
        return;
    }

    cfm = (BapBroadcastAssistantStopScanCfm*)msg;

    bapHandle = cfm->handle;

    bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, bapHandle);

    if (bap == NULL )
    {

        CAP_CLIENT_ERROR("\n capHandleBroadcastAssistantStopScanCfm: NULL instance \n");
        result = CAP_CLIENT_RESULT_FAILURE_BAP_ERR;
        capClientBcastAsstResetState(cap, CAP_CLIENT_BCAST_ASST_STATE_IDLE);
        capClientSendBcastCommonCfmMsgSend(appTask,
                                          inst,
                                          NULL,
                                          0,
                                          result,
                                          CAP_CLIENT_BCAST_ASST_STOP_SRC_SCAN_CFM);

        return;
    }

    result = capClientBroadcastSourceGetResultCode(cfm->result);
    bap->recentStatus = result;

    if (bap->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
    {
        capClientIncrementSuccessCounter(&bap->operationCounter);
    }
    else if (bap->recentStatus == CAP_CLIENT_RESULT_INPROGRESS)
    {
        /* Do Nothing */
        return;
    }
    else
    {
        capClientIncrementErrorCounter(&bap->operationCounter);
    }

    if (capClientBcastAsstOpComplete(cap))
    {
        /* Reset the Counters and Pending Operation */
        capClientBcastAsstResetState(cap, CAP_CLIENT_BCAST_ASST_STATE_IDLE);
        capClientSendBcastCommonCfmMsgSend(appTask,
                                          inst,
                                          cap,
                                          cap->requestCid,
                                          result,
                                          CAP_CLIENT_BCAST_ASST_STOP_SRC_SCAN_CFM);
    }

}

void capClientBcastAsstStopScanReqSend(BapInstElement* bap,
                                      uint32 cid,
                                      AppTask appTask)
{
    CSR_UNUSED(appTask);

    do {

        capClientIncrementOpCounter(&bap->operationCounter);

        CAP_CLIENT_INFO("\n capClientBcastAsstStopScanReqSend: Counter :%d \n",
            bap->operationCounter.opReqCount);

        BapBroadcastAssistantStopScanReq(bap->bapHandle, scanParams.scanHandle);

        bap = bap->next;
    } while (bap && (cid == 0));
}


void handleBroadcastAssistantStopSrcScanReq(CAP_INST* inst, const Msg msg)
{
    BapInstElement* bap;
    uint16 result;
    CapClientGroupInstance* cap = NULL;
    CapClientInternalBcastAsstStopSrcScanReq* req =
        (CapClientInternalBcastAsstStopSrcScanReq*)msg;
    AppTask appTask = req->profileTask;
    /*
     * if groupId is not same Switch the group return
     * by sending error Response
     * */
    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    result = capClientBroadcastAssistantValidOperation(req->groupId, req->profileTask, inst, cap);

    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
        CAP_CLIENT_INFO("\n handleBroadcastAssistantStopSrcScanReq: result =%x \n", result);
        capClientSendBcastCommonCfmMsgSend(appTask, inst, NULL, 0, result, CAP_CLIENT_BCAST_ASST_STOP_SRC_SCAN_CFM);
        return;
    }

    if (cap == NULL)
    {
        CAP_CLIENT_INFO("\n handleBroadcastAssistantStopSrcScanReq: NULL instance \n");
        return;
    }

    if (capClientBcastAsistantGetState(cap) != CAP_CLIENT_BCAST_ASST_STATE_START_SCAN)
    {
        result = CAP_CLIENT_RESULT_CAP_BUSY;
        CAP_CLIENT_INFO("\n handleBroadcastAssistantStopSrcScanReq: INVALIDE STATE \n");
        capClientSendBcastCommonCfmMsgSend(appTask, inst, NULL, 0, result, CAP_CLIENT_BCAST_ASST_STOP_SRC_SCAN_CFM);
        return;
    }

    /* co ordinated set?
     *
     * Based on if co ordinated Set or not decide number of ASEs required
     * and then start BAP procedures
     *
     * */

    if (req->cid)
    {
        bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, req->cid);

        if (bap == NULL)
        {
            result = CAP_CLIENT_RESULT_INVALID_PARAMETER;
            CAP_CLIENT_INFO("\n handleBroadcastAssistantStopSrcScanReq: UKNOWN CID \n");
            capClientSendBcastCommonCfmMsgSend(appTask, inst, NULL, 0, result, CAP_CLIENT_BCAST_ASST_STOP_SRC_SCAN_CFM);
            return;
        }
    }
    cap->requestCid = req->cid;
    inst->profileTask = req->profileTask;
    scanParams.scanHandle = req->scanHandle;

    cap->pendingOp = CAP_CLIENT_BAP_BASS_SCAN_STOP;
    capClientBcastAsistantSetState(cap, CAP_CLIENT_BCAST_ASST_STATE_STOP_SCAN);
    capClientSendBapBcastAsstReq(cap, inst, capClientBcastAsstStopScanReqSend);
}


/******************************************* START SCAN ************************************************/
void capClientBcastAsstStartScanReqSend(BapInstElement* bap,
                                               uint32 cid,
                                               AppTask appTask)
{

    do {

        capClientIncrementOpCounter(&bap->operationCounter);

        CAP_CLIENT_INFO("\n CapClientBcastAsstStartScanReqSend: Counter :%d \n",
                                                    bap->operationCounter.opReqCount);

        BapBroadcastAssistantStartScanReq(bap->bapHandle,
                                         (uint8)scanParams.bcasSrcType,
                                         (uint16)scanParams.filterContext,
                                         scanParams.scanFlags,
                                         scanParams.ownAddressType,
                                         scanParams.scanningFilterPolicy);

        if (bap->bass)
            bap->bass->reportToTask = appTask;

        bap = bap->next;
    } while (bap && (cid == 0));
}

void capClientHandleBroadcastAssistantSrcReportInd(CAP_INST *inst,
                                BapBroadcastAssistantSrcReportInd *ind,
                                CapClientGroupInstance* cap)
{
    CSR_UNUSED(cap);
    BapInstElement* bap = NULL;
    MAKE_CAP_CLIENT_MESSAGE(CapClientBcastAsstSrcReportInd);
    message->advHandle = ind->advHandle;
    message->advSid = ind->advSid;
    message->collocated = ind->collocated;
    message->handle = inst->activeGroupId;
    message->numSubgroup = ind->numSubgroup;
    message->sourceAddrt = ind->sourceAddrt;
    message->broadcastId = ind->broadcastId;
    message->serviceDataLen = ind->serviceDataLen;
    message->bigNameLen = ind->bigNameLen;

    bap = (BapInstElement*)
        CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, ind->handle);

    if (bap == NULL)
    {
        /*Can't send Report indication*/
        CsrPmemFree(message);
        message = NULL;
        return;
    }

    if ((bap == NULL) ||
           (bap->bass == NULL) ||
               (bap->bass->reportToTask == CSR_SCHED_QID_INVALID))
    {
        /*Can't send Report indication*/
        CsrPmemFree(message);
        message = NULL;
        return;
    }

    if (ind->numSubgroup && ind->subgroupInfo)
    {
        message->subgroupInfo = ind->subgroupInfo;
    }
    else
    {
        message->subgroupInfo = NULL;
    }

    if (ind->bigNameLen && ind->bigName)
    {
        message->bigName = ind->bigName;
    }
    else
    {
        message->bigName = NULL;
    }

    if (ind->serviceDataLen && ind->serviceData)
    {
        message->serviceData = ind->serviceData;
    }
    else
    {
        message->serviceData = NULL;
    }

    /*TODO: Free memory in ind*/
    CapClientMessageSend(bap->bass->reportToTask, CAP_CLIENT_BCAST_ASST_SRC_REPORT_IND, message);
}

void capClientSendBroadcastAssistantStartScanSrcCfm(AppTask appTask,
                                               CAP_INST *inst,
                                               CapClientGroupInstance *cap,
                                               uint32 cid,
                                               CapClientResult result,
                                               uint16 scanHandle)
{
    BapInstElement* bap = NULL;
    MAKE_CAP_CLIENT_MESSAGE(CapClientBcastAsstStartSrcScanCfm);
    message->groupId = inst->activeGroupId;
    message->statusLen = 0;
    message->status = NULL;
    message->scanHandle = scanHandle;

    if (cap && (result == CAP_CLIENT_RESULT_SUCCESS))
    {
        bap = (BapInstElement*)cap->bapList.first;
        message->statusLen = cap->bapList.count;
        CAP_CLIENT_CLEAR_PENDING_OP(cap->pendingOp);

        if (cid)
        {
            bap = (BapInstElement*)
                CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cid);
            message->statusLen = 1;
        }

        if (message->statusLen)
            message->status = (CapClientDeviceStatus*)
                 CsrPmemZalloc(sizeof(CapClientDeviceStatus) * message->statusLen);

        if (bap && message->status)
        {
            uint8 i = 0;

            do
            {
                message->status[i].cid = bap->cid;
                message->status[i].result = bap->recentStatus;

                if (bap->recentStatus != CAP_CLIENT_RESULT_SUCCESS)
                    result = bap->recentStatus;

                bap = bap->next;
                i++;
            } while (bap && cid == 0);
        }
        else
        {
            CsrPmemFree(message->status);
            message->status = NULL;
            message->statusLen = 0;
            message->scanHandle = 0;
            message->result = CAP_CLIENT_RESULT_FAILURE_BAP_ERR;
        }
    }
    else
    {
        message->scanHandle = 0;
    }
    message->result = result;


    CapClientMessageSend(appTask, CAP_CLIENT_BCAST_ASST_START_SRC_SCAN_CFM, message);
}

void capClientHandleBroadcastAssistantStartScanCfm(CAP_INST *inst,
                                           Msg msg,
                                           CapClientGroupInstance* cap)
{
    BapBroadcastAssistantStartScanCfm *startCfm = NULL;
    BapProfileHandle bapHandle = 0;
    BapInstElement *bap = NULL;
    CapClientResult result = CAP_CLIENT_RESULT_NULL_INSTANCE;
    uint16 scanHandle = 0;


    if(inst == NULL)
    {
        CAP_CLIENT_PANIC("\n capClientHandleBroadcastAssistantStartScanCfm: NULL instance \n");
        /* Do nothing */
        return;
    }

    if (cap == NULL)
    {
        CAP_CLIENT_INFO("\n capClientHandleBroadcastAssistantStartScanCfm: NULL instance \n");
        /* Do nothing */
        return;
    }

    if (capClientBcastAsstOpComplete(cap))
    {
        /* if all the counters are clear then there is no need to proceed, just return*/
        CAP_CLIENT_INFO("\n capClientHandleBroadcastAssistantStartScanCfm: BAP NULL instance \n");
        return;
    }

    startCfm = (BapBroadcastAssistantStartScanCfm*)msg;
    scanHandle = startCfm->scanHandle;
    bapHandle = startCfm->handle;

    bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, bapHandle);

    if(bap == NULL)
    {
        CAP_CLIENT_ERROR("\n capHandleBroadcastAssistantStarScanCfm: NULL instance \n");
        result = CAP_CLIENT_RESULT_FAILURE_BAP_ERR;
        capClientBcastAsstResetState(cap, CAP_CLIENT_BCAST_ASST_STATE_IDLE);
        capClientSendBroadcastAssistantStartScanSrcCfm(inst->profileTask,
                                                      inst,
                                                      NULL,
                                                      cap->requestCid,
                                                      result, 
                                                      0);
        return;
    }

    result = capClientBroadcastSourceGetResultCode(startCfm->result);
    bap->recentStatus = result;

    if(bap->recentStatus == CAP_CLIENT_RESULT_INPROGRESS
        || bap->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
    {
        /* Do Nothing */
        capClientIncrementSuccessCounter(&bap->operationCounter);
    }
    else
    {
        capClientIncrementErrorCounter(&bap->operationCounter);
    }

    if(capClientBcastAsstOpComplete(cap))
    {
        /* Reset the Counters and Pending Operation */
        capClientBcastAsstResetState(cap, CAP_CLIENT_BCAST_ASST_STATE_START_SCAN);
        capClientSendBroadcastAssistantStartScanSrcCfm(inst->profileTask,
                                                       inst, 
                                                       cap, 
                                                       cap->requestCid, 
                                                       CAP_CLIENT_RESULT_SUCCESS,
                                                       scanHandle);

    }
}

void handleBroadcastAssistantStartSrcScanReq(CAP_INST* inst, const Msg msg)
{
    BapInstElement* bap;
    uint16 result;
    CapClientGroupInstance* cap = NULL;
    CapClientInternalBcastAsstStartSrcScanReq* req =
        (CapClientInternalBcastAsstStartSrcScanReq*)msg;
    AppTask appTask = req->profileTask;
    /*
     * if groupId is not same Switch the group return
     * by sending error Response
     * */

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    result = capClientBroadcastAssistantValidOperation(req->groupId, req->profileTask, inst, cap);

    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
        capClientSendBroadcastAssistantStartScanSrcCfm(appTask,
                                                      inst,
                                                      NULL,
                                                      req->cid,
                                                      result, 
                                                      0);
        return;
    }


    if (cap == NULL)
    {
        CAP_CLIENT_INFO("\n handleBroadcastAssistantStartSrcScanReq: NULL instance \n");
        return;
    }

    if (capClientBcastAsistantGetState(cap) != CAP_CLIENT_BCAST_ASST_STATE_IDLE)
    {
        result = CAP_CLIENT_RESULT_CAP_BUSY;
        capClientSendBroadcastAssistantStartScanSrcCfm(appTask,
                                                      inst,
                                                      NULL,
                                                      req->cid,
                                                      result, 0);
        return;
    }

    /* co ordinated set?
     *
     * Based on if co ordinated Set or not decide number of ASEs required
     * and then start BAP procedures
     *
     * */
    if (req->cid)
    {
        bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, req->cid);

        if (bap == NULL)
        {

            capClientSendBroadcastAssistantStartScanSrcCfm(appTask,
                inst,
                NULL,
                req->cid,
                CAP_CLIENT_RESULT_INVALID_PARAMETER, 0);
            CAP_CLIENT_INFO("\n handleBroadcastAssistantSrcScanReq: Invalid BAP Handle \n");
            return;
        }
    }

    /* Cache the scan Parameters*/
    scanParams.filterContext = req->filterContext;
    scanParams.ownAddressType = req->ownAddressType;
    scanParams.scanFlags = req->scanFlags;
    scanParams.scanningFilterPolicy = req->scanningFilterPolicy;
    scanParams.bcasSrcType = req->bcastSrcType;
    scanParams.bcastType = req->bcastType;

    cap->requestCid = req->cid;
    inst->profileTask = req->profileTask;

    cap->pendingOp = CAP_CLIENT_BAP_BASS_SCAN_START;
    capClientBcastAsistantSetState(cap, CAP_CLIENT_BCAST_ASST_STATE_START_SCAN);

    capClientSendBapBcastAsstReq(cap, inst, capClientBcastAsstStartScanReqSend);
}

/*******************************************************REGISTER FOR NOTIFICATIONS*****************************************************/
void capClientHandleBroadcastAssistantRegisterNotificationCfm(CAP_INST* inst,
                                  Msg msg,
                                  CapClientGroupInstance* cap)
{
    BapBroadcastAssistantBrsRegisterForNotifcationCfm* cfm = 
                               (BapBroadcastAssistantBrsRegisterForNotifcationCfm*)msg;
    BapInstElement* bap = NULL;
    CapClientResult result = CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR;

    if (inst == NULL)
    {
        CAP_CLIENT_ERROR("\n capClientHandleBroadcastAssistantRegisterNotificationCfm: NULL instance \n");
        /* Do nothing */
        return;
    }

    if (cap == NULL)
    {
        CAP_CLIENT_ERROR("\n capClientHandleBroadcastAssistantRegisterNotificationCfm: CAP NULL instance \n");
        capClientSendBcastCommonCfmMsgSend(inst->profileTask,
                                          inst,
                                          NULL,
                                          0,
                                          CAP_CLIENT_RESULT_NULL_INSTANCE,
                                          CAP_CLIENT_BCAST_ASST_REGISTER_NOTIFICATION_CFM);
        return;
    }

    /* Reject the api call if the call is not from registered app*/

    /* co ordinated set?
     *
     * Based on if co ordinated Set or not decide number of ASEs required
     * and then start BAP procedures
     *
     * */
    bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cfm->handle);

    if (bap == NULL)
    {
        CAP_CLIENT_ERROR("\n capClientHandleBroadcastAssistantRegisterNotificationCfm: Invalid BAP Handle \n");
        capClientSendBcastCommonCfmMsgSend(inst->profileTask,
                                          inst,
                                          NULL,
                                          0,
                                          CAP_CLIENT_RESULT_INVALID_PARAMETER,
                                          CAP_CLIENT_BCAST_ASST_REGISTER_NOTIFICATION_CFM);
        return;
    }

    result = capClientBroadcastSourceGetResultCode(cfm->result);
    bap->recentStatus = result;

    /* Check first the operation count */
    if (bap->operationCounter.opReqCount != 0)
    {
        if (bap->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
        {
            capClientIncrementSuccessCounter(&bap->operationCounter);
        }
        else
        {
            capClientIncrementErrorCounter(&bap->operationCounter);
        }
    }

    if (capClientBcastAsstOpComplete(cap))
    {
        capClientResetOpCounter(cap);
        CAP_CLIENT_CLEAR_PENDING_OP(cap->pendingOp);

        CAP_CLIENT_INFO("\n capClientHandleBroadcastAssistantRegisterNotificationCfm: Success \n");
        capClientSendBcastCommonCfmMsgSend(inst->profileTask,
                                          inst,
                                          cap,
                                          cap->requestCid,
                                          CAP_CLIENT_RESULT_SUCCESS,
                                          CAP_CLIENT_BCAST_ASST_REGISTER_NOTIFICATION_CFM);
    }/*else more confirmations are pending from remote */

}

void capClientSendBapBassRegisterNotificationReq(BapInstElement* bap)
{
    BapBroadcastAssistantBRSRegisterForNotificationReq(bap->bapHandle,
                                                      notiParams.sourceId, 
                                                      notiParams.allSources, 
                                                      notiParams.notificationEnable);
}

void capClientBcastAsstNtfReqSend(BapInstElement* bap,
                                    uint32 cid,
                                    AppTask appTask)
{
    CapClientGroupInstance* cap =
        (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(bap->groupId);

    do{
        uint8 i = 0;
        /* Get the number of broadcast source handles from gatt bass client device data */
        GattBassClientDeviceData* bassHandles = capClientGetHandlesInfo(bap->cid, cap, CAP_CLIENT_PROFILE_BASS);

        if (bassHandles)
        {
            if (notiParams.allSources == TRUE)
            {
                for (i = 0; i< bassHandles->broadcastSourceNum; i++)
                  capClientIncrementOpCounter(&bap->operationCounter);
            }
            else
              capClientIncrementOpCounter(&bap->operationCounter);

            CsrPmemFree(bassHandles);
        }

        BapBroadcastAssistantBRSRegisterForNotificationReq(bap->bapHandle,
                                                          notiParams.sourceId,
                                                          notiParams.allSources,
                                                          notiParams.notificationEnable);
        if (bap->bass)
            bap->bass->reportToTask = appTask;
        bap = bap->next;
    }while(bap && (cid == 0));
}

void handleBassRegisterNotificationReq(CAP_INST* inst, const Msg msg)
{
    BapInstElement* bap;
    CapClientGroupInstance* cap = NULL;
    uint16 result;
    CapClientInternalBcastAsstRegNotificationReq* req =
                         (CapClientInternalBcastAsstRegNotificationReq*)msg;


    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);
    result = capClientBroadcastAssistantValidOperation(req->groupId, req->profileTask, inst, cap);
    
    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
        capClientSendBcastCommonCfmMsgSend(req->profileTask,
                                          inst,
                                          NULL,
                                          req->cid,
                                          result,
                                          CAP_CLIENT_BCAST_ASST_REGISTER_NOTIFICATION_CFM);
        return;
    }

    /* co ordinated set?
     *
     * Based on if co ordinated Set or not decide number of ASEs required
     * and then start BAP procedures
     *
     * */

    cap->requestCid = req->cid;

    if (req->cid)
    {
        bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, req->cid);

        if (bap == NULL)
        {
            CAP_CLIENT_ERROR("\n handleBassRegisterNotificationReq:Invalid BAP Handle\n");
            capClientSendBcastCommonCfmMsgSend(req->profileTask,
                                              inst,
                                              NULL,
                                              0,
                                              CAP_CLIENT_RESULT_INVALID_PARAMETER,
                                              CAP_CLIENT_BCAST_ASST_REGISTER_NOTIFICATION_CFM);
            return;
        }
    }

    notiParams.allSources = req->allSources;
    notiParams.notificationEnable = req->notificationEnable;
    notiParams.sourceId = req->sourceId;
    inst->profileTask = req->profileTask;

    cap->pendingOp = CAP_CLIENT_BAP_BASS_REG_NOTIFY;
    capClientSendBapBcastAsstReq(cap, inst, capClientBcastAsstNtfReqSend);
}

/************************************************** READ BROADCAST RECEIVE STATE *********************************************/

static void capClientSendBcastAsstReadBrsCfm(AppTask profileTask,
                                      ServiceHandle groupId,
                                      uint32 cid,
                                      CapClientResult result)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientBcastAsstReadReceiveStateCfm);
    message->result = result;
    message->groupId = groupId;
    message->cid = cid;
    CapClientMessageSend(profileTask, CAP_CLIENT_BCAST_ASST_READ_RECEIVE_STATE_CFM, message);
}


void capClientHandleBroadcastAssistantReadBrsCfm(CAP_INST *inst,
                                   BapBroadcastAssistantReadBrsCfm* cfm,
                                   CapClientGroupInstance *cap)
{
    uint8 i;
    BapInstElement* bap = NULL;
    MAKE_CAP_CLIENT_MESSAGE(CapClientBcastAsstReadReceiveStateInd);
    message->numSubGroups = 0;
    message->subGroupInfo = NULL;
    message->badCode = NULL;
    
    if (cfm)
    {
        bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cfm->handle);
        CAP_CLIENT_INFO("\n(CAP) capClientHandleBroadcastAssistantReadBrsCfm: Result: 0x%x \n", cfm->result);

        if (bap && bap->operationCounter.opReqCount
                && (cfm->result == BAP_RESULT_SUCCESS))
        {
            capClientIncrementSuccessCounter(&bap->operationCounter);
        }
        else if (bap && cfm->result == BAP_RESULT_INPROGRESS)
        {
            /*Ignore*/
        }
        else if (bap && bap->operationCounter.opReqCount)
        {
            capClientIncrementErrorCounter(&bap->operationCounter);

        }
        else if (bap && capClientBcastAsstOpComplete(cap))
        {
            CAP_CLIENT_INFO("\n(CAP) capClientHandleBroadcastAssistantReadBrsCfm: Message Ignored.Reason: OpCounter Reset \n");
            capClientBcastAsstResetState(cap, cap->bcastAsstState);
            CsrPmemFree(message);
        }
        else
        {
            CAP_CLIENT_INFO("\n(CAP) capClientHandleBroadcastAssistantReadBrsCfm: Invalide Handle \n");
            capClientBcastAsstResetState(cap, cap->bcastAsstState);
            capClientSendBcastAsstReadBrsCfm(inst->profileTask, inst->activeGroupId,
                                                cap->requestCid, CAP_CLIENT_RESULT_FAILURE_BAP_ERR);
            CsrPmemFree(message);
            return;
        }
  

    }

    if (cfm && cfm->result == CAP_CLIENT_RESULT_SUCCESS)
    {
        message->advertiseAddType = cfm->advertiseAddType;
        message->advSid = cfm->advSid;
        message->bigEncryption = cfm->bigEncryption;
        message->broadcastId = cfm->broadcastId;
        message->groupId = inst->activeGroupId;
        message->cid = cfm->handle;
        message->numSubGroups = cfm->numSubGroups;
        message->sourceId = cfm->sourceId;
        message->sourceAddress.lap = cfm->sourceAddress.lap;
        message->sourceAddress.uap = cfm->sourceAddress.uap;
        message->sourceAddress.nap = cfm->sourceAddress.nap;
        message->paSyncState = cfm->paSyncState;
        message->result = capClientBroadcastSourceGetResultCode(cfm->result);

        if (cfm->badCode)
        {
            message->badCode = cfm->badCode;
            cfm->badCode = NULL;
        }

        if (cfm->numSubGroups)
        {
            message->numSubGroups = cfm->numSubGroups;
            message->subGroupInfo = (CapClientSubgroupInfo*)
                CsrPmemZalloc(cfm->numSubGroups * sizeof(CapClientSubgroupInfo));

            for (i = 0; i < cfm->numSubGroups; i++)
            {
                if (cfm->subGroupInfo)
                {
                    message->subGroupInfo[i].bisIndex = cfm->subGroupInfo[i].bisSyncState;
                    if (cfm->subGroupInfo[i].metadataLen && cfm->subGroupInfo[i].metadataValue)
                    {
    
                        message->subGroupInfo[i].metadataLen = cfm->subGroupInfo[i].metadataLen;
                        message->subGroupInfo[i].metadataValue =
                            CsrPmemZalloc(message->subGroupInfo[i].metadataLen * sizeof(uint8));
    
                        SynMemCpyS(message->subGroupInfo[i].metadataValue,
                                          message->subGroupInfo[i].metadataLen,
                                               cfm->subGroupInfo[i].metadataValue,
                                                   message->subGroupInfo[i].metadataLen);
    
                        CsrPmemFree(cfm->subGroupInfo[i].metadataValue);
                    }
                }
            }

            CsrPmemFree(cfm->subGroupInfo);
        }
    }
        CapClientMessageSend(inst->profileTask, CAP_CLIENT_BCAST_ASST_READ_RECEIVE_STATE_IND, message);

    if (capClientBcastAsstOpComplete(cap))
    {
        capClientBcastAsstResetState(cap, cap->bcastAsstState);
        capClientSendBcastAsstReadBrsCfm(inst->profileTask, inst->activeGroupId, 
                                        cap->requestCid,CAP_CLIENT_RESULT_SUCCESS);
    }

}

void capClientSendBroadcastAssistantReadBrsReq(BapInstElement* bap)
{
    BapBroadcastAssistantReadBRSReq(bap->bapHandle, 0, TRUE);
}

void handleReadBroadcastReceiveStateReq(CAP_INST* inst, const Msg msg)
{
    BapInstElement* bap;
    uint16 result = CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR;
    CapClientGroupInstance* cap = NULL;
    CapClientInternalBcastAsstReadReceiveStateReq* req =
        (CapClientInternalBcastAsstReadReceiveStateReq*)msg;
    GattBassClientDeviceData* bassHandles = NULL;
    /*
     * if groupId is not same Switch the group return
     * by sending error Response
     * */

    if (inst == NULL)
    {
        result = CAP_CLIENT_RESULT_NULL_INSTANCE;
        capClientSendBcastAsstReadBrsCfm(req->profileTask, 0, req->cid, result);
        CAP_CLIENT_INFO("\n handleReadBroadcastReceiveStateReq: NULL instance \n");
        return;
    }

    if (req->groupId != inst->activeGroupId)
    {
        result = CAP_CLIENT_RESULT_INVALID_PARAMETER;
        capClientSendBcastAsstReadBrsCfm(req->profileTask, req->groupId, req->cid, result);
        CAP_CLIENT_INFO("\n handleReadBroadcastReceiveStateReq: Unable to change GroupId \n");
        return;
    }

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    if (cap == NULL)
    {
        result = CAP_CLIENT_RESULT_INVALID_GROUPID;
        capClientSendBcastAsstReadBrsCfm(req->profileTask, req->groupId, req->cid, result);
        CAP_CLIENT_INFO("\n handleReadBroadcastReceiveStateReq: NULL instance \n");
        return;
    }

    /* Reject the api call if the call is not from registered app*/

    if (req->profileTask != inst->profileTask)
    {
        result = CAP_CLIENT_RESULT_TASK_NOT_REGISTERED;
        capClientSendBcastAsstReadBrsCfm(req->profileTask, inst->activeGroupId, req->cid, result);
        CAP_CLIENT_INFO("\n handleReadBroadcastReceiveStateReq: Invalid Profile Task \n");
        return;
    }

    /* co ordinated set?
     *
     * Based on if co ordinated Set or not decide number of ASEs required
     * and then start BAP procedures
     *
     * */
    bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, req->cid);

    if (bap == NULL)
    {
        result = CAP_CLIENT_RESULT_TASK_NOT_REGISTERED;
        capClientSendBcastAsstReadBrsCfm(req->profileTask, req->groupId, req->cid, result);
        CAP_CLIENT_INFO("\n handleReadBroadcastReceiveStateReq: Invalid BAP Handle \n");
        return;
    }

    cap->requestCid = req->cid;

    /* Get the number of broadcast source number from gatt bass client device data */
    bassHandles = capClientGetHandlesInfo(cap->requestCid, cap, CAP_CLIENT_PROFILE_BASS);

    if (bassHandles)
    {
        CAP_CLIENT_SET_OP_COUNTER_VALUE(bap->operationCounter, (uint8)bassHandles->broadcastSourceNum);
        /* Read All sources */
        capClientSendBroadcastAssistantReadBrsReq(bap);
        CsrPmemFree(bassHandles);
    }
    else
    {
        result = CAP_CLIENT_RESULT_INVALID_PARAMETER;
        capClientSendBcastAsstReadBrsCfm(req->profileTask, req->groupId, req->cid, result);
        CAP_CLIENT_INFO("\n handleReadBroadcastReceiveStateReq: Invalid btConnId \n");
    }
}
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "csip.h"
#include "cap_client_csip_handler.h"
#include "cap_client_util.h"
#include "cap_client_init.h"
#include "cap_client_bap_pac_record.h"
#include "cap_client_init_stream_and_control_req.h"
#include "cap_client_add_new_dev.h"
#include "cap_client_available_audio_context_req.h"
#include "cap_client_discover_audio_capabilities_req.h"
#include "cap_client_start_stream_req.h"
#include "cap_client_remove_device_req.h"
#include "cap_client_unicast_connect_req.h"
#include "cap_client_unicast_disconnect_req.h"
#include "cap_client_update_audio_req.h"
#include "cap_client_vcp_operation_req.h"
#include "cap_client_stop_stream_req.h"
#include "cap_client_broadcast_assistant_periodic_scan.h"
#include "cap_client_broadcast_assistant_sync_to_adv.h"
#include "cap_client_broadcast_assistant_add_modify_src_req.h"
#include "cap_client_broadcast_assistant_remove_src_req.h"
#include "cap_client_broadcast_src.h"
#include "cap_client_debug.h"
#include "cap_client_common.h"
#include "cap_client_private.h"
#include "cap_client_micp_handler.h"
#include "cap_client_micp_operation_req.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

#define CAP_CSIP_READ_ALL  (CAP_CLIENT_CSIP_LOCK | CAP_CLIENT_CSIP_SIZE | CAP_CLIENT_CSIP_RANK | CAP_CLIENT_CSIP_SIRK)


static void capClientSendCsipReadCsInfoReq(CsrCmnListElm_t* elem, void* value)
{
    CsipInstElement* cElem = (CsipInstElement*)elem;
    CAP_INST* cap = (CAP_INST*)value;
    CapClientGroupInstance* gElem = NULL;


    /* If any of the Recent Procedure has failed */
    if (cElem->recentStatus)
    {
        if (!cap->addNewDevice)
            capClientSendInitCfm(cap, CAP_CLIENT_RESULT_FAILURE_CSIP_ERR);
        else
            capClientSendAddNewDeviceCfm(cap->activeGroupId,
                               cap->deviceCount,
                               CAP_CLIENT_RESULT_FAILURE_CSIP_ERR,
                               cap->appTask,
                               NULL);

        return;
    }

    if (cElem->discoveryComplete)
    {
        /* if Discovery is complete */
        gElem = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(cap->activeGroupId);

        if (gElem && gElem->setSize != gElem->currentDeviceCount)
            gElem->setSize = gElem->currentDeviceCount;

        if (!cap->addNewDevice)
            capClientSendInitCfm(cap, CAP_CLIENT_RESULT_SUCCESS);
        else
            capClientSendAddNewDeviceCfm(cap->activeGroupId,
                            cap->deviceCount,
                            CAP_CLIENT_RESULT_SUCCESS,
                            cap->appTask,
                            gElem);
    }

    /*Save current Opeartion*/
    if (cap->addNewDevice)
    {
        cElem->currentOperation = CAP_CLIENT_INTERNAL_ADD_NEW_DEV_REQ;
    }
    else
    {
        cElem->currentOperation = CAP_CLIENT_INTERNAL_INIT_REQ;
    }

    if (cap->csipReadType & CAP_CLIENT_CSIP_LOCK)
    {
        cap->csipRequestCount++;
        CsipReadCSInfoRequest(cElem->csipHandle, CSIP_LOCK);
    }
    else if (cap->csipReadType & CAP_CLIENT_CSIP_SIZE)
    {
        cap->csipRequestCount++;
        CsipReadCSInfoRequest(cElem->csipHandle, CSIP_SIZE);
    }
    else if (cap->csipReadType & CAP_CLIENT_CSIP_RANK)
    {
        cap->csipRequestCount++;
        CsipReadCSInfoRequest(cElem->csipHandle, CSIP_RANK);
    }
    else if (cap->csipReadType & CAP_CLIENT_CSIP_SIRK)
    {
        cap->csipRequestCount++;
        CsipReadCSInfoRequest(cElem->csipHandle, CSIP_SIRK);
    }
    else
    {
        /* Do Nothing */
    }
}

static void capClientCsipHandleInitCfm(Msg msg,
                               CAP_INST *inst,
                               CapClientGroupInstance *groupElem)
{
    CsipInitCfm* cfm = (CsipInitCfm*)msg;
    CsipInstElement* csip =
         (CsipInstElement*)CAP_CLIENT_GET_CSIP_ELEM_FROM_CID(groupElem->csipList, cfm->cid);

    CAP_CLIENT_INFO("\ncapClientCsipHandleInitCfm: init Recvd!!, result :%d\n", cfm->status);

    if (cfm->status == CSIP_STATUS_SUCCESS)
    {

        /* Check if there is already any entry of the CSIP profile
         * with same profile handle in the Active Group and update the
         * profile handle
         * */

        csip->csipHandle = cfm->prflHndl;
    }

    if(cfm->status != CSIP_STATUS_IN_PROGRESS)
        inst->csipRequestCount--;

    csip->recentStatus = capClientGetCapClientResult(cfm->status, CAP_CLIENT_CSIP);


    if(!inst->csipRequestCount)
    {
        inst->csipReadType = (0x0F & CAP_CSIP_READ_ALL);
        CsrCmnListIterate(&groupElem->csipList, capClientSendCsipReadCsInfoReq ,inst);
    }
}

static void capClientInitAddDeviceCfm(CAP_INST* inst, CapClientGroupInstance* cap, CapClientResult result)
{
    /* Send the confirmation to the App for cap client init/add new device */
    if(!inst->csipReadType && !inst->csipRequestCount)
    {
       if (!inst->addNewDevice)
       {
           /* Send CAP INIT CFM */
           capClientSendInitCfm(inst, result);
       }
       else
       {
           /* Send CAP ADD_NEW_DEV CFM */
           inst->addNewDevice = FALSE;

           capClientSendAddNewDeviceCfm(inst->activeGroupId,
                                 inst->deviceCount,
                                 result,
                                 cap->appTask,
                                 (CapClientGroupInstance *)(cap));
       }
    }
}

static void capClientCsisHandleReadCsInfoCfm(Msg msg,
                                      CAP_INST *inst,
                                      CapClientGroupInstance *cap)
{
    CsipReadCsInfoCfm *cfm = (CsipReadCsInfoCfm*)msg;

    /*Check if there is already any entry of the CSIP profile
     * with same profile handle in the Active Group
     * */
    CsipInstElement* csip =
            (CsipInstElement*)CAP_CLIENT_GET_CSIP_ELEM_FROM_PHANDLE(cap->csipList, cfm->prflHndl);

    if(csip == NULL)
    {
        if(!inst->addNewDevice)
            capClientSendInitCfm(inst, CAP_CLIENT_RESULT_FAILURE_CSIP_ERR);
        else
            capClientSendAddNewDeviceCfm(inst->activeGroupId,
                      inst->deviceCount,
                      CAP_CLIENT_RESULT_FAILURE_CSIP_ERR,
                      inst->appTask,
                      NULL);
        return;
    }

    /* update the Status*/
    csip->recentStatus = capClientGetCapClientResult(cfm->status, CAP_CLIENT_CSIP);

    CAP_CLIENT_INFO("capClientCsisHandleReadCsInfoCfm: Info type: 0x%x, status: 0x%x", cfm->csInfoType, cfm->status);

    if (cfm->csInfoType == CSIP_SIRK && inst->csipRequestCount)
    {
        inst->csipRequestCount--;

        if(inst->csipRequestCount == 0)
        {
            inst->csipReadType &= ~(0x0f & CAP_CLIENT_CSIP_SIRK);
        }

        if (cfm->status == CSIP_STATUS_SUCCESS)
        {
            uint8 i;

            cap->sirkType = cfm->value[0];

            /* Cap need to decrypt the sirk in case of encrypted sirk and once sirk is decrypted, then need to send cap init cfm to App*/
            CsrMemCpy(cap->sirk, &(cfm->value[1]), CAP_CLIENT_SIRK_SIZE);

            if (cfm->value[0] == PLAIN_SIRK)
            {
                for (i = 1; i < cfm->sizeValue; i++)
                    CAP_CLIENT_INFO(" 0x%x", cfm->value[i]);
                CAP_CLIENT_INFO("\n\n");
            }
            else if (cfm->value[0] == ENCRYPTED_SIRK)
            {
                 CAP_CLIENT_INFO(" capClientCsisHandleReadCsInfoCfm :Encrypted sirk, perform decrypt SIRK : %x csip->cid %x\n", cfm->value[0], csip->cid);

                 /* Call the CSIP decrypt API to get SIRK decrypted */
                 if (csip && csip->csipHandle)
                 {
                    CsipDecryptSirk(csip->csipHandle, cap->sirk);
                 }
                 return;
            }
        }
    }
    else if (cfm->csInfoType == CSIP_SIZE && inst->csipRequestCount)
    {
        inst->csipRequestCount--;

        if (inst->csipRequestCount == 0)
            inst->csipReadType &= ~(0x0f & CAP_CLIENT_CSIP_SIZE);

        if (cfm->status == CSIP_STATUS_SUCCESS)
        {
            cap->setSize = cfm->value[0];

            if (inst->deviceCount > cap->setSize)
            {
                cap->currentDeviceCount = inst->deviceCount;
                cap->setSize = inst->deviceCount;

                if (!inst->addNewDevice)
                    capClientSendInitCfm(inst, CAP_CLIENT_RESULT_INVALID_OPERATION);
                else
                    capClientSendAddNewDeviceCfm(inst->activeGroupId,
                        inst->deviceCount,
                        CAP_CLIENT_RESULT_INVALID_OPERATION,
                        inst->appTask,
                        NULL);
                return;
            }
        }
    }
    else if (cfm->csInfoType == CSIP_RANK && inst->csipRequestCount)
    {
        inst->csipRequestCount--;

        if (inst->csipRequestCount == 0)
            inst->csipReadType &= ~(0x0f & CAP_CLIENT_CSIP_RANK);

        if (cfm->status == CSIP_STATUS_SUCCESS)
        {
            csip->rank = cfm->value[0];
        }
    }
    else if (cfm->csInfoType == CSIP_LOCK && inst->csipRequestCount)
    {
        inst->csipRequestCount--;

        if (inst->csipRequestCount == 0)
            inst->csipReadType &= ~(0x0f & CAP_CLIENT_CSIP_LOCK);

        if (cfm->status == CSIP_STATUS_SUCCESS)
        {
            csip->lock = cfm->value[0];
        }
    }

    if(inst->csipReadType && !inst->csipRequestCount)
    {
        CsrCmnListIterate(&cap->csipList, capClientSendCsipReadCsInfoReq ,inst);
    }

    capClientInitAddDeviceCfm(inst, cap, CAP_CLIENT_RESULT_SUCCESS);

    CsrPmemFree(cfm->value);
}


static void capClientSendCoordinatedSetUnlockCfm(CAP_INST* inst,
    CsipInstElement* csip, CapClientResult result)
{
    uint8 i = 0;
    MAKE_CAP_CLIENT_MESSAGE(CapClientUnlockCoordinatedSetCfm);

    message->groupId = inst->activeGroupId;
    message->deviceStatusLen = inst->deviceCount;
    message->result = result;

    if (result == CAP_CLIENT_RESULT_SUCCESS && csip)
    {
        message->deviceStatus = (CapClientDeviceStatus*)CsrPmemZalloc(inst->deviceCount * sizeof(CapClientDeviceStatus));

        for (i = 0; i < inst->deviceCount && csip; i++)
        {
            message->deviceStatus[i].cid = csip->cid;
            message->deviceStatus[i].result = csip->recentStatus;
            csip = csip->next;
        }
    }

    CapClientMessageSend(inst->appTask,
                        CAP_CLIENT_UNLOCK_COORDINATED_SET_CFM,
                        message);
}

static void capClientSendReadCSInfoCfm(Msg msg, CAP_INST* inst, CsipInstElement* csip, CapClientResult result)
{
    CSR_UNUSED(csip);
    CsipReadCsInfoCfm* cfm = (CsipReadCsInfoCfm*)msg;
    MAKE_CAP_CLIENT_MESSAGE(CapClientCsipReadCfm);

    message->groupId = inst->activeGroupId;
    message->result = result;

    if (cfm)
    {
        message->csipChar = cfm->csInfoType;
        message->csipSizeValue = cfm->sizeValue;
        if (cfm->sizeValue)
        {
            message->csipValue = (uint8*)CsrPmemZalloc(cfm->sizeValue * sizeof(uint8));
            memcpy(message->csipValue, cfm->value, cfm->sizeValue);
            CsrPmemFree(cfm->value);
        }
    }

    CapClientMessageSend(inst->appTask,
                        CAP_CLIENT_CSIP_READ_CFM,
                        message);
}

static void capClientCsipHandleLockInd(CAP_INST *inst,
                                       CapClientGroupInstance *cap,
                                       CsipLockStatusInd *cfm)
{
    CsipInstElement *csip = (CsipInstElement*) CAP_CLIENT_GET_CSIP_ELEM_FROM_PHANDLE(cap->csipList, cfm->prflHndl);

    CAP_CLIENT_INFO("(CAP) : CSIP Handle: 0x%04x,Lock status : 0x%02x \n", cfm->prflHndl,cfm->lockStatus);

    if (csip)
    {
        MAKE_CAP_CLIENT_MESSAGE(CapClientLockStateInd);

        /* Send CSIP lock State Indication to application */
        message->groupId = inst->activeGroupId;
        message->cid = csip->cid;
        message->lockState = cfm->lockStatus;

        CapClientMessageSend(inst->profileTask, CAP_CLIENT_LOCK_STATE_IND, message);
    }
}

void handleCoordinatedSetUnlockReq(CAP_INST* inst, const Msg msg)
{
    CsipInstElement* csip = NULL;
    uint16 result = CAP_CLIENT_RESULT_INVALID_OPERATION;
    CapClientGroupInstance* cap = NULL;
    CapClientInternalUnlockCoordinatedSetReq* req =
        (CapClientInternalUnlockCoordinatedSetReq*)msg;


    /*
     * TODO:if groupId is not same Switch the group return
     * by sending error Response
     * */

    if (req->groupId != inst->activeGroupId)
    {
        result = CAP_CLIENT_RESULT_INVALID_OPERATION;
        CAP_CLIENT_INFO("\n handleCoordinatedSetUnlockReq:  GroupId not active \n");
        capClientSendCoordinatedSetUnlockCfm(inst, NULL, result);

        return;
    }

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    if (cap == NULL)
    {
        result = CAP_CLIENT_RESULT_NULL_INSTANCE;
        CAP_CLIENT_INFO("\n handleCoordinatedSetUnlockReq: Invalid Group Id\n");
        capClientSendCoordinatedSetUnlockCfm(inst, NULL, result);
        return;
    }

    csip = (CsipInstElement*)cap->csipList.first;

    if (csip && capClientIsGroupCoordinatedSet(cap) && (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED))
    {
        cap->pendingOp = CAP_CLIENT_CSIP_CLEANUP;
        capClientSetCsisLockState(csip, &inst->csipRequestCount, FALSE);
    }
    else
    {
        /* Send Cfm*/
        capClientSendCoordinatedSetUnlockCfm(inst, NULL, CAP_CLIENT_RESULT_SUCCESS);
    }
}


static void capClientCsipTriggerBapVcpProcedure(CAP_INST* inst,
                                        CapClientGroupInstance *gInst,
                                        CapClientResult result)

{
    BapInstElement *bap = (BapInstElement*)((CsrCmnListElm_t*)(gInst->bapList).first);
    CsipInstElement* csip = (CsipInstElement*)((CsrCmnListElm_t*)(gInst->csipList).first);
    VcpInstElement* vcp =  (VcpInstElement*)(gInst->vcpList.first);

#ifndef EXCLUDE_CSR_BT_MICP_MODULE

    MicpInstElement* micp = NULL;
    /* First check if the MICP list is existing */
    if (gInst->micpList)
        micp = (MicpInstElement*)(gInst->micpList->first);
#endif

    if(gInst->pendingOp == CAP_CLIENT_BAP_UNICAST_CONNECT)
    {
        if(csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
            capClientSendAllUnicastConfigCodecReq(gInst, csip, bap, inst);
        }
        else if(csip->lock == CAP_CLIENT_LOCK_STATE_DISABLED)
        {
            capClientSendUnicastClientConnectCfm(inst->profileTask, inst, gInst, result);
        }
    }
    else if(gInst->pendingOp == CAP_CLIENT_BAP_UNICAST_START_STREAM)
    {

        if(csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
            capClientBapAseEnableReqSend(csip, bap, gInst,inst);
        }
        else if(csip->lock == CAP_CLIENT_LOCK_STATE_DISABLED)
        {
            capClientSendUnicastStartStreamCfm(inst->profileTask, inst, gInst, result);
        }
    }
    else if(gInst->pendingOp == CAP_CLIENT_BAP_UNICAST_AUDIO_UPDATE)
    {
        if(csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
            capClientSendUnicastAudioUpdateReq(bap, gInst, inst);
        }
        else if(csip->lock == CAP_CLIENT_LOCK_STATE_DISABLED)
        {
            capClientUpdateAudioCfmSend(inst->profileTask, 
                                        inst, 
                                        result,
                                        gInst->useCase, bap, gInst);
        }
    }
    else if (gInst->pendingOp == CAP_CLIENT_UNICAST_DISCONNECT)
    {
        if (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
            capClientSendInternalPendingOpReq(gInst->pendingOp);
        }
        else if (csip->lock == CAP_CLIENT_LOCK_STATE_DISABLED)
        {
            capClientSendUnicastClientDisConnectCfm(inst->profileTask, inst, 
                                                   gInst, result);
        }
    }
#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
    else if(gInst->pendingOp == CAP_CLIENT_BAP_BASS_SCAN_START)
    {
        if(csip->lock == CAP_CLIENT_LOCK_STATE_DISABLED)
        {
            /*CAP: Add logs*/
        }
        else if(csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
            capClientSendBapBcastAsstReq(gInst, inst, capClientBcastAsstStartScanReqSend);
        }
    }
    else if (gInst->pendingOp == CAP_CLIENT_BAP_BASS_SCAN_STOP)
    {
        if (csip->lock == CAP_CLIENT_LOCK_STATE_DISABLED)
        {
            /*CAP: Add logs*/
        }
        else if (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
            capClientSendBapBcastAsstReq(gInst, inst, capClientBcastAsstStopScanReqSend);
        }
    }
    else if(gInst->pendingOp == CAP_CLIENT_BAP_BASS_ADD_SRC)
    {
        if(csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
            capClientSendBapBcastAsstReq(gInst, inst, capClientBcastAsstAddSrcReqSend);

        }
        else
        {

        }
    }
    else if(gInst->pendingOp == CAP_CLIENT_BAP_BASS_MODIFY_SRC)
    {
        if(csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
            capClientSendBapBcastAsstReq(gInst, inst, capClientBcastAsstmodifySrcReqSend);

        }
        else
        {

        }
    }
    else if (gInst->pendingOp == CAP_CLIENT_BAP_BASS_REMOVE_SRC)
    {
        if (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
            capClientSendBapBcastAsstReq(gInst, inst, capClientBcastAsstRemoveSrcReqSend);

        }
        else
        {

        }
    }
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */
    else if(gInst->pendingOp == CAP_CLIENT_VCP_MUTE)
    {
        CAP_CLIENT_CLEAR_PENDING_OP(gInst->pendingOp);

        if(csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
            capClientSetVcpMuteStateReq(vcp, (bool)gInst->volumeSetting, inst, gInst);
        }
        else if(csip->lock == CAP_CLIENT_LOCK_STATE_DISABLED)
        {
            capClientSendMuteCfm(inst->vcpProfileTask, inst->activeGroupId,
                    result, inst->deviceCount, vcp, inst);
        }
    }
    else if(gInst->pendingOp == CAP_CLIENT_VCP_SET_VOL)
    {
        CAP_CLIENT_CLEAR_PENDING_OP(gInst->pendingOp);

        if(csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
            capClientSetAbsoluteVolumeReq((CsrCmnListElm_t*)vcp, gInst->volumeSetting, inst, gInst);
        }
    }

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
    else if(gInst->pendingOp == CAP_CLIENT_MICP_MUTE)
    {
        CAP_CLIENT_CLEAR_PENDING_OP(gInst->pendingOp);

        if(csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
            capClientSetMicpMuteStateReq(micp, inst, gInst);
        }
        else if(csip->lock == CAP_CLIENT_LOCK_STATE_DISABLED)
        {
            capClientSendMicMuteCfm(inst->micpProfileTask,
                                       inst->activeGroupId,
                                       gInst,
                                       gInst->requestCid,
                                       result);
        }
    }
#endif

    else if(gInst->pendingOp == CAP_CLIENT_CSIP_CLEANUP)
    {
        /* Send the Cfm to Application */
        if (csip->lock == CAP_CLIENT_LOCK_STATE_DISABLED)
        {
            CAP_CLIENT_CLEAR_PENDING_OP(gInst->pendingOp);
            capClientSendCoordinatedSetUnlockCfm(inst, csip, result);
        }
        else
        {
            CAP_CLIENT_INFO("\n(CAP) Invalid Cleanup CSIP lock state");
        }
    }
    else if(gInst->pendingOp == CAP_CLIENT_BAP_UNICAST_DISABLE)
    {
        if(csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
             capClientUnicastDisableReleaseReq(gInst, inst);
        }
        else if(csip->lock == CAP_CLIENT_LOCK_STATE_DISABLED)
        {
            capClientSendUnicastStopStreamCfm(inst->profileTask, 
                                             inst, 
                                             result,
                                             FALSE, bap, gInst);
        }

    }
    else if(gInst->pendingOp == CAP_CLIENT_BAP_UNICAST_RELEASE)
    {
        if(csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
             capClientUnicastDisableReleaseReq(gInst, inst);
        }
        else if(csip->lock == CAP_CLIENT_LOCK_STATE_DISABLED)
        {
            if (gInst->stopComplete && capClientAsesReleaseComplete(gInst))
            {
                gInst->stopComplete = FALSE;
                capClientSendUnicastStopStreamCfm(inst->profileTask, inst, 
                                                 result,
                                                 TRUE, bap, gInst);
            }
        }
    }
#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
    else if(gInst->pendingOp == CAP_CLIENT_BAP_BASS_REG_NOTIFY)
    {
         if (csip->lock == CAP_CLIENT_LOCK_STATE_DISABLED)
         {
         }
         else if (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
         {
             bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(gInst->bapList, gInst->requestCid);
             capClientSendBapBassRegisterNotificationReq(bap);
         }
    }
    else if (gInst->pendingOp == CAP_CLIENT_BAP_BASS_SET_CODE)
    {
        if (csip->lock == CAP_CLIENT_LOCK_STATE_DISABLED)
        {
        }
        else if (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
             bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(gInst->bapList, gInst->requestCid);
             BapBroadcastAssistantSetCodeRsp(gInst->requestCid, bap->bass->sourceId, gInst->broadcastCode);
        }
    }
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */
    else
    {

    }
}

static void handleCsipLockCfm(CAP_INST *inst,
                              CapClientGroupInstance *cap,
                              CsipInstElement *csip)
{
    /* When All cfms are received */
    if (inst->csipRequestCount == 0)
    {
        CsipInstElement *temp = (CsipInstElement*) ((CsrCmnListElm_t*) (cap->csipList.first));
        bool lockState = TRUE;

        while (temp)
        {
            if (temp->recentStatus != CSIP_STATUS_SUCCESS)
            {
                lockState = FALSE;
                break;
            }
            temp = temp->next;
        }

        if (lockState)
        { /* All lock/unlock attempt have passed */
            if (csip->pendingLock == CAP_CLIENT_LOCK_STATE_ENABLED)
            {
                cap->csipStatus = CAP_CLIENT_RESULT_SUCCESS;
            }
            capClientCsipTriggerBapVcpProcedure(inst, cap, cap->csipStatus);
        }
        else
        {
            /* If the lock acquisition is failed for any of the device in the group then send the error message to the Upper layer
             * before sending the confirmation first need to unlock the lock for all the devices in the group */
            if (csip->pendingLock == CAP_CLIENT_LOCK_STATE_ENABLED)
            {
                cap->csipStatus = CAP_CLIENT_RESULT_CSIP_LOCK_UNAVAILABLE;
                capClientSetCsisLockState(csip, &inst->csipRequestCount, FALSE);

                if (inst->csipRequestCount == 0)
                { /* Lock release was not sent for any device, we need to send error cfm to app
                   * from here only, for that we also need to set lock value to disabled for each device */
                    temp = (CsipInstElement*) ((CsrCmnListElm_t*) (cap->csipList.first));

                    while (temp)
                    {
                        temp->lock = CAP_CLIENT_LOCK_STATE_DISABLED;
                        temp = temp->next;
                    }
                    capClientCsipTriggerBapVcpProcedure(inst, cap, cap->csipStatus);
                }
            }
            else
            {
                capClientCsipTriggerBapVcpProcedure(inst, cap, cap->csipStatus);
            }
        }
    }
}

static void capClientCsipHandleLockCfm(CAP_INST *inst,
                                  CsipSetLockCfm *cfm,
                                  CapClientGroupInstance *groupElem)
{
    CsipInstElement *csip = NULL;

    csip = (CsipInstElement*)
             CAP_CLIENT_GET_CSIP_ELEM_FROM_PHANDLE(groupElem->csipList, cfm->prflHndl);

    if (cfm->status != CSIP_STATUS_IN_PROGRESS)
    {
        inst->csipRequestCount--;
    }
    csip->recentStatus = capClientGetCapClientResult(cfm->status , CAP_CLIENT_CSIP);

    /* update Lock status */
    if (csip->recentStatus == CSIP_STATUS_SUCCESS)
    {
        csip->lock = csip->pendingLock;
    }

    handleCsipLockCfm(inst, groupElem, csip);
}

static void capClientHandleCsipReadCfm(CAP_INST *inst, 
                                      Msg msg, 
                                      CapClientGroupInstance* groupElem)
{
    /* Check for the pending operation if its CSIP READ , App has triggered this operation */
    CsipReadCsInfoCfm* cfm = (CsipReadCsInfoCfm*)msg;
    CsipInstElement *csip = (CsipInstElement*)
                  CAP_CLIENT_GET_CSIP_ELEM_FROM_PHANDLE(groupElem->csipList, cfm->prflHndl);

    if (csip && (csip->currentOperation == CAP_CLIENT_INTERNAL_CSIP_READ_REQ))
    {
        /* Clear the CAP_CSIP_READ_LOCK flag */
        csip->currentOperation = CAP_CLIENT_INTERNAL_INVALID_OPERATION;
        capClientSendReadCSInfoCfm(msg, inst, csip, CAP_CLIENT_RESULT_SUCCESS);
    }
    else if (csip && ((csip->currentOperation == CAP_CLIENT_INTERNAL_INIT_REQ)
                || (csip->currentOperation == CAP_CLIENT_INTERNAL_ADD_NEW_DEV_REQ)))
    {
        csip->currentOperation = CAP_CLIENT_INTERNAL_INVALID_OPERATION;
        capClientCsisHandleReadCsInfoCfm(msg, inst, groupElem);
    }
    else
    {
        CAP_CLIENT_INFO("\n (CAP) capClientHandleCsipReadCfm:Invalid Opeartion!!\n");

        if (!inst->addNewDevice)
        {
            capClientSendInitCfm(inst, CAP_CLIENT_RESULT_FAILURE_CSIP_ERR);
        }
        else
        {
            capClientSendAddNewDeviceCfm(inst->activeGroupId,
                0,
                CAP_CLIENT_RESULT_FAILURE_CSIP_ERR,
                inst->appTask,
                groupElem);
        }
    }
}

static void capClientSortProfilesList(CapClientGroupInstance *cap)
{
    if (cap->currentDeviceCount == cap->setSize && cap->profileListSort == FALSE)
    {
        cap->profileListSort = TRUE;
        CAP_CLIENT_SORT_CSIP_LIST(cap->csipList);
        CAP_CLIENT_SORT_BAP_LIST(cap->bapList);
        CAP_CLIENT_SORT_VCP_LIST(cap->vcpList);
    }
}

void capClientSetCsisLockState(CsipInstElement *csip, uint8 *csipRequestCount, bool lockState)
{
    /* Call Csip Lock device Api to lock/unlock all the devices */
    uint8 value = lockState ? CAP_CLIENT_LOCK_STATE_ENABLED: CAP_CLIENT_LOCK_STATE_DISABLED;

    /* Before doing the procedure check all devices are there in the group 
     * If all devices are present in the group then CAP need to abide CSIP ordered access
     * For the procedure including CSIP lock */
    CapClientGroupInstance *cap = NULL;
    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(csip->groupId);

    if (cap)
    {
        capClientSortProfilesList(cap);

         /* When the list will get sort, csip node will get moved so get the first node again */
        if (cap->profileListSort == TRUE)
        {
            csip = (CsipInstElement *)cap->csipList.first;
        }
    }

    while (csip)
    {
         if (csip->csipHandle != CAP_CLIENT_INVALID_SERVICE_HANDLE && csip->recentStatus == CSIP_STATUS_SUCCESS)
         {
             (*csipRequestCount)++;
             CsipSetLockRequest(csip->csipHandle, lockState);
             csip->pendingLock = value;
         }
         csip = csip->next;
    }
}

void capClientHandleCsipMsg(CAP_INST *inst, Msg msg)
{
    CsipInstElement *csip = NULL;
    CsrBtGattPrim *prim = (CsrBtGattPrim *)msg;
    CapClientGroupInstance *cap = CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    if (cap == NULL)
    {
        CAP_CLIENT_INFO("capHandleCsipMsg: cap is NULL");
        return;
    }

    switch (*prim)
    {
        /* Handle BAP related primitives here */
        case CSIP_INIT_CFM:
        {
            capClientCsipHandleInitCfm(msg, inst, cap);
        }
        break;

        case CSIP_DESTROY_CFM:
        {
            capClientRemoveGroup(msg, inst, CAP_CLIENT_CSIP);
        }
        break;

        case CSIP_READ_CS_INFO_CFM:
        {
            capClientHandleCsipReadCfm(inst, msg, cap);
        }
        break;

        case CSIP_CS_SET_NTF_CFM:
        {
            CAP_CLIENT_INFO("(LEA) : CSIP_CS_SET_NTF_CFM  Status\n");
        }
        break;

        case CSIP_SET_LOCK_CFM:
        {
            CsipSetLockCfm *cfm = (CsipSetLockCfm*)msg;
            CAP_CLIENT_INFO("(LEA) : CSIP_SET_LOCK_CFM  Status : 0x%04x \n", cfm->status);

            /* Handle Set lock Cfm message*/
            capClientCsipHandleLockCfm(inst, cfm, cap);

        }
        break;

        case CSIP_LOCK_STATUS_IND:
        {
            CsipLockStatusInd *cfm = (CsipLockStatusInd*)msg;
            CAP_CLIENT_INFO("(LEA) : CSIP_LOCK_STATUS_IND  LOCK Status : 0x%04x \n", cfm->lockStatus);
            /* update lock status */
            csip = (CsipInstElement*)CAP_CLIENT_GET_CSIP_ELEM_FROM_PHANDLE(cap->csipList, cfm->prflHndl);
            csip->lock = cfm->lockStatus;

            /* Handle Set lock Ind message*/
            capClientCsipHandleLockInd(inst, cap, cfm);
        }
        break;

        case CSIP_SIZE_CHANGED_IND:
        {
            CAP_CLIENT_INFO("(LEA) : CSIP_SIZE_CHANGED_IND  New Size value\n");
        }
        break;

        case CSIP_SIRK_CHANGED_IND:
        {
            CAP_CLIENT_INFO("(LEA) : CSIP_SIRK_CHANGED_IND\n");
        }
        break;

        case CSIP_SIRK_DECRYPT_CFM:
        {
            CsipSirkDecryptCfm *cfm = (CsipSirkDecryptCfm *)msg;
            CapClientResult result = CAP_CLIENT_RESULT_SUCCESS_SIRK_DECRYPT_ERR;

            if (cfm->status == CAP_CLIENT_RESULT_SUCCESS)
            {
                uint8 i;

                cap->sirkType = PLAIN_SIRK;
                result = CAP_CLIENT_RESULT_SUCCESS;

                /* We Need to store the SIRK from MSB->LSB */
                for(i = 0; i < CSIP_SIRK_SIZE; i++)
                {
                    cap->sirk[i] = cfm->sirkValue[CSIP_SIRK_SIZE - 1 -i];
                }
            }

            capClientInitAddDeviceCfm(inst, cap, result);
        }
        break;

        default:
             CAP_CLIENT_WARNING("\n(LE Audio App) : CSIP_PRIM 0x%x Not handled\n", *prim);
        break;

    }
}

void handleCsipReadLock(CAP_INST* inst, const Msg msg)
{
    /*
     * First Check if the Group is A co-ordinated Set
     * Note: Group is a co ordinated Set if the CSIS instance has
     * setSize of more than 1 and has a valid CSIP handle
     * */
     CapClientGroupInstance *cap = NULL;
     CsipInstElement *csip  = NULL;
     CapClientResult result;
     CapClientInternalCsipReadReq *req =(CapClientInternalCsipReadReq*)msg;

     /* if groupId is not same Switch the group
      *
      * Note: capSetNewActiveGroup sends CapActiveGroupChangeInd to the
      * application internally
      * */

     cap = capClientSetNewActiveGroup(inst, req->groupId, FALSE);

     /* If Group Id does not match with the list of cap groupiDs send
      * Send CAP_RESULT_INVALID_GROUPID
      * */

     if (cap == NULL)
     {
         capClientSendReadCSInfoCfm(NULL, inst, NULL, CAP_CLIENT_RESULT_INVALID_GROUPID);
         return;
     }

     result = capClientValidateCapState(cap->capState, req->type);

     if (result != CAP_CLIENT_RESULT_SUCCESS)
     {
         capClientSendReadCSInfoCfm(NULL, inst, csip, result);
         CAP_CLIENT_INFO("\n handleCsipReadLock: invalid state transition \n");
         return;
     }

     if(req->groupId != inst->activeGroupId)
     {
         capClientSendReadCSInfoCfm(NULL, inst, csip, CAP_CLIENT_RESULT_INVALID_OPERATION);
         return;
     }

    csip = (CsipInstElement*)CAP_CLIENT_GET_CSIP_ELEM_FROM_CID(cap->csipList, req->cid);

    if (csip)
    {
        /* Obtain Lock on the Devices */
        csip->currentOperation = req->type;
        /* Increment the csip operation count */
        CAP_CLIENT_INFO("\n(CAP) : csipReadNotify CsipReadCSInfoRequest\n");
        CsipReadCSInfoRequest(csip->csipHandle, req->csipCharType);
    }
    else
    {
        capClientSendReadCSInfoCfm(NULL, inst, NULL, CAP_CLIENT_RESULT_FAILURE_CSIP_ERR);
    }
}


/* This is utility function for enabling/disabling CSIP lock CCD, which can be used in future to enable other
 * CSIP characteristics also */
void capClientCsipEnableCcd(CAP_INST *inst, uint8 enable)
{
    CapClientGroupInstance* cap = CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);
    CsipInstElement* csip = NULL;

    if (cap)
    {
        csip = (CsipInstElement*)cap->csipList.first;
    }

    while (csip)
    {
        CAP_CLIENT_INFO("\n(CAP) : capClientCsipEnableCcd CsipCSRegisterForLockNotificationReq\n");
        CsipCSRegisterForNotificationReq(csip->csipHandle, CSIP_LOCK, enable);
        CsipCSRegisterForNotificationReq(csip->csipHandle, CSIP_SIZE, enable);
        CsipCSRegisterForNotificationReq(csip->csipHandle, CSIP_SIRK, enable);
        csip = csip->next;
    }
}
#endif /* INSTALL_LEA_UNICAST_CLIENT */

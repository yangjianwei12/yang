/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_sched.h"
#include "csr_pmem.h"
#include "csr_env_prim.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_util.h"
#include "csr_log_text_2.h"

#include "csr_bt_sdc_support.h"
#include "csr_bt_cm_prim.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_hfg_prim.h"
#include "csr_bt_hfg_main.h"
#include "csr_bt_hfg_proto.h"
#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_hfg_streams.h"
#endif /* CSR_STREAMS_ENABLE */

#define MAX_SERVICE_RECORDS_SEARCH    10

/* Util: Start service discovery */
void CsrBtHfgStartSdcSearch(HfgInstance_t *inst)
{
    if (!inst->pendingSearch)
    {
        CmnCsrBtLinkedListStruct *sdpTagList = NULL;
        CsrUint16 shIndex;
        CsrBool searchBothTypes = FALSE;
        dm_security_level_t secOutgoing;

        inst->searchHfg.num     = 0;
        inst->searchAg.num      = 0;

        HFG_CHANGE_STATE(inst->oldState, Connect_s);
        HFG_CHANGE_STATE(inst->state, ServiceSearch_s);
        inst->pendingSearch = TRUE;
        inst->searchAndCon = TRUE;

        if (inst->linkType != CSR_BT_HFG_CONNECTION_AG)
        {
            searchBothTypes = TRUE;
        }

        if(searchBothTypes)
        {
            sdpTagList = CsrBtUtilSdrCreateServiceHandleEntryFromUuid32(sdpTagList, CSR_BT_HF_PROFILE_UUID, &shIndex);

            CsrBtUtilSdrInsertLocalServerChannel(sdpTagList, shIndex, inst->serverChannel);
            CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_SUPPORTED_FEATURES_ATTRIBUTE_IDENTIFIER, NULL, 0);
            CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_SERVICE_NAME_ATTRIBUTE_IDENTIFIER, NULL, 0);
            CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER, NULL, 0);
        }

       /* Always look for HS devices... */
        sdpTagList = CsrBtUtilSdrCreateServiceHandleEntryFromUuid32(sdpTagList, CSR_BT_HS_PROFILE_UUID, &shIndex);
        if (NULL != sdpTagList)
        {
            CsrBtUtilSdrInsertLocalServerChannel(sdpTagList, shIndex, inst->serverChannel);
            CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_REMOTE_AUDIO_VOLUME_CONTROL_ATTRIBUTE_IDENTIFIER, NULL, 0);
        }
        else
        {
            inst->pendingSearch = FALSE;
        }

#ifndef INSTALL_HFG_CUSTOM_SECURITY_SETTINGS
        CsrBtScSetSecOutLevel(&secOutgoing,
                              CSR_BT_SEC_DEFAULT,
                              CSR_BT_HANDSFREE_GW_MANDATORY_SECURITY_OUTGOING,
                              CSR_BT_HANDSFREE_GW_DEFAULT_SECURITY_OUTGOING,
                              CSR_BT_RESULT_CODE_HFG_SUCCESS,
                              CSR_BT_RESULT_CODE_HFG_UNACCEPTABLE_PARAMETER);
#else
        secOutgoing = CsrBtHfgGetMainInstance(inst)->secOutgoing;
#endif /* INSTALL_HFG_CUSTOM_SECURITY_SETTINGS*/

        CsrBtUtilRfcConStart((void *)inst,
                        CsrBtHfgGetMainInstance(inst)->sdpHfgSearchConData,
                        sdpTagList,
                        inst->address,
                        secOutgoing,
                        FALSE,
                        NULL,
                        CSR_BT_HFG_PROFILE_DEFAULT_MTU_SIZE,
                        CsrBtHfgGetMainInstance(inst)->modemStatus,
                        0,
                        CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                               CSR_BT_HANDSFREE_DEFAULT_ENC_KEY_SIZE_VAL));
    }
    /*else?*/
}


/* Disconnected while service search */
void CsrBtHfgServiceSearchCmDisconnectIndHandler(HfgInstance_t *inst)
{
    CsrBtCmDisconnectInd *prim;
    CsrBool sendResp;
    prim = (CsrBtCmDisconnectInd*)inst->msg;
    sendResp = !prim->localTerminated;

    /* Special error-code */
    if(!prim->status)
    {
        /* Retry */
        CsrBtCmDisconnectReqSend(prim->btConnId);
    }
    else
    {
        CsrBtCmLogicalChannelTypeReqSend(CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL,inst->address,prim->btConnId);
        if (inst->pendingDisconnect)
        {
            CsrBtHfgSendHfgDisconnectInd(inst, prim->localTerminated, prim->reasonCode, prim->reasonSupplier);
            CsrBtHfgLinkConnectFailed(inst);
        }
        else if(inst->pendingCancel)
        {
            if (inst->state == Connected_s)
            {
                CsrBtHfgSendHfgDisconnectInd(inst, prim->localTerminated, CSR_BT_RESULT_CODE_HFG_CANCELLED_CONNECT_ATTEMPT, CSR_BT_SUPPLIER_HFG);
                CsrBtHfgLinkConnectFailed(inst);
            }
            else if ((CsrBtUtilRfcConVerifyCmMsg(inst->msg)) && (inst->searchAndCon))
            {
                CsrBtUtilRfcConCmMsgHandler(inst, CsrBtHfgGetMainInstance(inst)->sdpHfgSearchConData, inst->msg);
                /* To avoid trying to free them when already freed...*/
                inst->msg = NULL;
                /* Disconnect response already sent by CsrBtUtilRfcConCmMsgHandler(..) */
                sendResp = FALSE; 
            }
            else if ((CsrBtUtilSdcVerifyCmMsg(inst->msg)) && (inst->pendingSearch))
            {
                CsrBtUtilSdcCmMsgHandler(inst, CsrBtHfgGetMainInstance(inst)->sdpHfgSearchData, inst->msg);
                /* To avoid trying to free them when already freed...*/
                inst->msg = NULL;
                /* Disconnect response already sent by CsrBtUtilSdcCmMsgHandler(..) */
                sendResp = FALSE; 
            }
            else
            {
                /* App does not not about connection, so simply
                 * make it look like it was cancelled */
                CsrBtHfgSendHfgServiceConnectInd(inst,
                                                 CSR_BT_RESULT_CODE_HFG_CANCELLED_CONNECT_ATTEMPT,
                                                 CSR_BT_SUPPLIER_HFG);
                CsrBtHfgLinkConnectFailed(inst);
            }
        }
        else if(inst->pendingSearch)
        {
            /* This means that we are in Activate_s */
            if(!inst->searchAndCon)
            {
                CsrBtUtilSdcSearchCancel((void*)inst,CsrBtHfgGetMainInstance(inst)->sdpHfgSearchData);
            }
            inst->pendingPeerDisconnect = TRUE;
        }
        else
        {
            /* This means were are in the SLC AT sequence */
            if (inst->state == Connected_s)
            {
                /* This means that HfgServiceConnectInd has been sent */
                CsrBtHfgSendHfgDisconnectInd(inst, prim->localTerminated, prim->reasonCode, prim->reasonSupplier);
            }
            else
            {
                CsrBtHfgSendHfgServiceConnectInd(inst,
                                                 CSR_BT_RESULT_CODE_HFG_CONNECT_ATTEMPT_FAILED,
                                                 CSR_BT_SUPPLIER_HFG);
            }
            CsrBtHfgLinkConnectFailed(inst);
        }
    }
#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
    if (sendResp)
    {
        /* For remote disconnections, profile needs to respond to CSR_BT_CM_DISCONNECT_IND. */
        CsrBtCmRfcDisconnectRspSend(prim->btConnId);
    }
#else
    CSR_UNUSED(sendResp);
#endif
}

/* Cancel connect in service search */
void CsrBtHfgServiceSearchHfgCancelConnectReqHandler(HfgInstance_t *inst)
{
    if(inst->pendingSearch)
    {
        if (inst->searchAndCon)
        {
            CsrBtUtilRfcConCancel((void*)inst,CsrBtHfgGetMainInstance(inst)->sdpHfgSearchConData);
            inst->pendingCancel = TRUE;
        }
        else
        {
            CsrBtUtilSdcSearchCancel((void*)inst,CsrBtHfgGetMainInstance(inst)->sdpHfgSearchData);
            CsrBtHfgLinkConnectFailed(inst);
            /*inst->state = Activate_s;*/
        }
    }
    else
    {
        /* Connection already established, so send disconnect */
        CsrBtCmDisconnectReqSend(inst->hfgConnId);
        inst->pendingCancel = TRUE;
    }
}

/* Disconnect while service search */
void CsrBtHfgServiceSearchHfgDisconnectReqHandler(HfgInstance_t *inst)
{
    if (inst->pendingSearch)
    {
        /* Hfg is doing SDC search. Set disconnectFlag and wait for next signal */
        inst->pendingDisconnect = TRUE;
    }
    else
    {
        /* HFG is in the AT sequence. Save signal and restore after AT sequence */
        CsrBtHfgSaveMessage(inst);
    }
}

/* Message in service search can't be handled now */
void CsrBtHfgServiceSearchHfgXReqHandler(HfgInstance_t *inst)
{
    /* Determine if CSR_BT_HFG_SERVICE_CONNECT_IND has been sent to app. If
     * it has not been sent then ignore this request otherwise save
     * the message and restore after AT sequence has finished */
    if(inst->pendingSearch ||
       (inst->atState == At0Idle_s))
    {
        /* CSR_BT_HFG_SERVICE_CONNECT_IND has not been sent, ignore message */
    }
    else
    {
        CsrBtHfgSaveMessage(inst);
    }
}

/* Send the manual status indicator at continue AT sequence */
void CsrBtHfgServiceSearchHfgManualIndicatorResHandler(HfgInstance_t *inst)
{
    CsrBtHfgManualIndicatorRes *prim;
    CsrUint16 len;

    prim = (CsrBtHfgManualIndicatorRes*)inst->msg;
    len = (CsrUint16)(prim->indicatorsLength > CSR_BT_CIEV_NUMBER_OF_INDICATORS
                     ? CSR_BT_CIEV_NUMBER_OF_INDICATORS
                     : prim->indicatorsLength);

    /* Store the indicators */
    SynMemCpyS(inst->ind.ciev, CSR_BT_CIEV_NUMBER_OF_INDICATORS, prim->indicators, len);
    SynMemCpyS(CsrBtHfgGetMainInstance(inst)->ind.ciev, CSR_BT_CIEV_NUMBER_OF_INDICATORS, prim->indicators, len);

    /* Send the +CIND, OK and continue the AT sequence */
    HFG_CHANGE_STATE(inst->atState, At4CindStatus_s);
    CsrBtHfgSendAtCindStatus(inst);
    CsrBtHfgSendAtOk(inst);
}



/***********************************************************************************/
static CsrBool csrBtHfgSdpGetBluetoothProfileDescriptorList(CmnCsrBtLinkedListStruct *bll_p,
                                                        CsrUint16  serviceHandleIndex,
                                                        CsrUint16  *version)
{
    CsrBool    retBool = FALSE;

    CsrUint8  *att_p;
    CsrUintFast16  nofAttributes, x;
    CsrUint16 attDataLen, nofBytesToAttribute, emptyAttSize, consumedBytes, totalConsumedBytes = 0, tempVar;
    CsrUint32  returnValue, protocolValue;

    if (TRUE == CsrBtUtilSdrGetNofAttributes(bll_p, serviceHandleIndex, &nofAttributes))
    {
        for (x=0; x<nofAttributes; x++)
        {
            att_p = CsrBtUtilSdrGetAttributePointer(bll_p, serviceHandleIndex, x, &nofBytesToAttribute);

            if (att_p)
            {
                /* Check if the UUID in the 'outer' attribute struct is correct */
                SynMemCpyS(&tempVar, SDR_ENTRY_SIZE_SERVICE_UINT16, att_p + SDR_ENTRY_INDEX_ATTRIBUTE_UUID, SDR_ENTRY_SIZE_SERVICE_UINT16);
                if (CSR_BT_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER == tempVar)
                {
                    CsrBtUtilSdrGetEmptyAttributeSize(&emptyAttSize);
                    SynMemCpyS(&tempVar, SDR_ENTRY_SIZE_SERVICE_UINT16, att_p, SDR_ENTRY_SIZE_SERVICE_UINT16);
                    attDataLen = tempVar - emptyAttSize + SDR_ENTRY_SIZE_TAG_LENGTH;

                    /* First extract the attribute uuid from the attribute SDP data */
                    if (TRUE == CsrBtUtilSdpExtractUint(att_p + SDR_ENTRY_INDEX_ATTRIBUTE_DATA,
                                                  attDataLen,
                                                  &returnValue,
                                                  &consumedBytes,
                                                  FALSE))
                    {
                        /* Check if the UUID in the 'inner' attribute sdp data struct is correct */
                        if (CSR_BT_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER == returnValue)
                        {
                            attDataLen = attDataLen - consumedBytes;
                            totalConsumedBytes += consumedBytes;
                            /* first find the protocol UUID */
                            if (TRUE == CsrBtUtilSdpExtractUint(att_p + SDR_ENTRY_INDEX_ATTRIBUTE_DATA + totalConsumedBytes,
                                                          attDataLen,
                                                          &protocolValue,
                                                          &consumedBytes,
                                                          TRUE))
                            {
                                attDataLen = attDataLen - consumedBytes;
                                totalConsumedBytes += consumedBytes;
                                /* Now find the value */
                                if (TRUE == CsrBtUtilSdpExtractUint(att_p + SDR_ENTRY_INDEX_ATTRIBUTE_DATA + totalConsumedBytes,
                                                              attDataLen,
                                                              &returnValue,
                                                              &consumedBytes,
                                                              TRUE))
                                {
                                    attDataLen = attDataLen - consumedBytes;
                                    totalConsumedBytes += consumedBytes;

                                    if (CSR_BT_HF_PROFILE_UUID == protocolValue)
                                    {
                                        *version = (CsrUint16)returnValue;
                                        retBool = TRUE;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return retBool;
}

static void csrBtHfgUtilCarryOnAfterSdp(HfgInstance_t *instData, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    /* We need to store this for release resources */
    instData->searchConnectResult = resultCode;

    CSR_LOG_TEXT_INFO((CsrBtHfgLto, 0, "csrBtHfgUtilCarryOnAfterSdp result = 0x%x, supplier 0x%x pendingDisconnect 0x%x pendingCancel 0x%x pendingCancelOutgoingConn 0x%x ", 
        resultCode, resultSupplier, instData->pendingDisconnect, instData->pendingCancel, instData->pendingCancelOutgoingConn)); 

    if(instData->pendingDisconnect ||
       instData->pendingCancel || 
       instData->pendingCancelOutgoingConn)
    {
        if(resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
           resultSupplier == CSR_BT_SUPPLIER_CM)
        {
            CsrBtCmDisconnectReqSend(instData->hfgConnId);
        }
        else
        {
            if(instData->pendingDisconnect)
            {
                /* Disconnect pending completed */
                CsrBtHfgSendHfgDisconnectInd(instData, TRUE, resultCode, resultSupplier);
                instData->pendingDisconnect = FALSE;
                CsrBtHfgLinkConnectFailed(instData);
            }
            else if(instData->pendingCancel) /* pendingCancel */
            {
                /* Can't cancel while cancelling... */
                CsrBtHfgSendHfgServiceConnectInd(instData, CSR_BT_RESULT_CODE_HFG_CANCELLED_CONNECT_ATTEMPT, CSR_BT_SUPPLIER_HFG);
                instData->pendingCancel = FALSE;
                CsrBtHfgLinkConnectFailed(instData);
            }
            else  /* pendingCancelOutgoingConn */
            {
                /* When an incoming connection has been processed as a result we have cancelled 
                   the SDP request for an outgoing connection. This needs to be reset */
                instData->pendingCancelOutgoingConn = FALSE;
                /* The variable mainInst->activeConnection would have been set to a incoming connection index as part of the CsrBtHfgMainActiveCmConnectAcceptCfmHandler() call
                 * but inside the function CsrBtHfgLinkConnectFailed, we are setting the mainInst->activeConnection to the outgoing connection
                 * but this would again get set to incoming connection as part of CsrBtHfgLinkConnectSuccess() and inbetween these two events, we don't access the link
                 * based on activeConnection, so there should not be any impact on any other functionality.
                 */
                CsrBtHfgLinkConnectFailed(instData);
            }
        }
    }
    else if(instData->pendingPeerDisconnect)
    {
        CsrBtHfgSendHfgServiceConnectInd(instData,
                                         CSR_BT_RESULT_CODE_HFG_CONNECT_ATTEMPT_FAILED,
                                         CSR_BT_SUPPLIER_HFG);
        CsrBtHfgLinkConnectFailed(instData);
        instData->pendingPeerDisconnect = FALSE;
    }
    else
    {
        if(resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
           resultSupplier == CSR_BT_SUPPLIER_CM)
        {

            /* Kick-start RFCOMM flow control */
            CsrBtHfgSendCmControlReq(instData);

            if(instData->linkType == CSR_BT_HFG_CONNECTION_AG)
            {
                /* Old-style AG connection. The SLC has already been
                 * fully established! */
                CsrBtHfgSendHfgHouseCleaning(instData);
                HFG_CHANGE_STATE(instData->oldState, ServiceSearch_s);
                HFG_CHANGE_STATE(instData->state, Connected_s);
                instData->serviceName[0] = '\0';
                HFG_CHANGE_STATE(instData->atState, At10End_s);
                instData->pendingSearch = FALSE;
                CsrBtHfgSendHfgServiceConnectInd(instData, CSR_BT_RESULT_CODE_HFG_SUCCESS, CSR_BT_SUPPLIER_HFG);
                CsrBtHfgLinkConnectSuccess(instData);
            }
            else
            {
                /* This is a HFG connection -- the HF will now start
                 * the AT sequence, so don't do anything at this
                 * point... */
                HFG_CHANGE_STATE(instData->atState, At0Idle_s);
                if (instData->conGuardTimer != 0)
                {
                    void *mv;
                    CsrUint16 mi;
                    CsrSchedTimerCancel(instData->conGuardTimer ,&mi, &mv);
                }
                instData->conGuardTimer = CsrSchedTimerSet((CsrTime)CSR_BT_HFG_CONNECTION_GUARD_TIME, CsrBtHfgConnectionTimeout, 0,(void*)instData);
                    
                /* Make sure to handle the AT command if it is already stored in the primitive queue */
                CsrBtHfgSendHfgHouseCleaning(instData);
            }
#ifdef CSR_STREAMS_ENABLE
                CsrBtHfgStreamsRegister(CsrBtHfgGetMainInstance(instData), instData->hfgConnId);
#endif
        }
        else
        {
            CsrBtHfgSendHfgServiceConnectInd(instData, CSR_BT_RESULT_CODE_HFG_SDC_SEARCH_FAILED, CSR_BT_SUPPLIER_HFG);
            CsrBtHfgLinkConnectFailed(instData);
        }
    }
}


void CsrBtHfgSdcResultHandler(void               * inst,
                                CmnCsrBtLinkedListStruct * sdpTagList,
                                CsrBtDeviceAddr          deviceAddr,
                                CsrBtResultCode          resultCode,
                                CsrBtSupplier      resultSupplier)
{
    HfgInstance_t *instData = (HfgInstance_t *) inst;
    CsrBtUuid32    tmpUuid = 0;

    CSR_UNUSED(deviceAddr);
    instData->pendingSearch = FALSE;
    instData->searchAndCon  = FALSE;

    if (!instData->pendingDisconnect && !instData->pendingPeerDisconnect)
    {
        if ((resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS) && (sdpTagList != NULL))
        {
            CsrUintFast16    numOfSdpRecords = CsrBtUtilBllGetNofEntriesEx(sdpTagList);
            CsrUintFast16    sdpRecordIndex;
            CsrUint16    tmpResult;
            CsrUint16    dummy1, dummy2; /* Currently CSR_UNUSED */
            CsrUint32    returnValue;
            CsrUint8     *string;
            CsrUint16    stringLen;
            CsrUint16    version = 0;

            for (sdpRecordIndex = 0; sdpRecordIndex < numOfSdpRecords; sdpRecordIndex++)
            {
                if (CsrBtUtilSdrGetServiceUuid32AndResult(sdpTagList,
                                                sdpRecordIndex,
                                                &tmpUuid,
                                                &tmpResult,
                                                &dummy1,
                                                &dummy2))
                {
                    if (tmpResult == CSR_BT_RESULT_CODE_CM_SUCCESS)
                    {
                        CsrBtUuid32 serviceHandleTemp = 0;

                        if ((tmpUuid == CSR_BT_HF_PROFILE_UUID) &&
                            ((instData->linkType == CSR_BT_HFG_CONNECTION_HFG) || (instData->linkType == CSR_BT_HFG_CONNECTION_UNKNOWN)))
                        { /* Handsfree connection */
                            instData->linkType = CSR_BT_HFG_CONNECTION_HFG;

                          if (TRUE == CsrBtUtilSdrGetServiceHandle(sdpTagList, sdpRecordIndex, &serviceHandleTemp))
                           {
                              instData->searchHfg.recordId = serviceHandleTemp;
                              instData->searchHfg.num++;
                           }

                          if (TRUE == CsrBtUtilSdrGetUintAttributeDataValueFromAttributeUuid(sdpTagList, sdpRecordIndex,
                                                        CSR_BT_SUPPORTED_FEATURES_ATTRIBUTE_IDENTIFIER, &returnValue))
                          {
                              instData->remSupFeatures = (CsrUint16) returnValue;
                          }
                          if (TRUE == CsrBtUtilSdrGetStringAttributeFromAttributeUuid(sdpTagList, sdpRecordIndex, CSR_BT_SERVICE_NAME_ATTRIBUTE_IDENTIFIER, &string, &stringLen))
                          {
                              if (stringLen > CSR_BT_MAX_FRIENDLY_NAME_LEN)
                              {
                                  stringLen = CSR_BT_MAX_FRIENDLY_NAME_LEN;
                              }
                              SynMemCpyS(instData->serviceName, sizeof(instData->serviceName), string, stringLen);
                              CsrUtf8StrTruncate(instData->serviceName, stringLen);
                          }
                          if (TRUE == csrBtHfgSdpGetBluetoothProfileDescriptorList(sdpTagList,(CsrUint16) sdpRecordIndex, &version))
                          {
                              instData->remoteVersion = version;
                          }
                        }
                        else if ((tmpUuid == CSR_BT_HS_PROFILE_UUID) &&
                            ((instData->linkType == CSR_BT_HFG_CONNECTION_AG) || (instData->linkType == CSR_BT_HFG_CONNECTION_UNKNOWN)))
                        {  /* HSP connection  */
                           instData->linkType = CSR_BT_HFG_CONNECTION_AG;
                           if (TRUE == CsrBtUtilSdrGetServiceHandle(sdpTagList, sdpRecordIndex, &serviceHandleTemp))
                           {
                               instData->searchAg.recordId = serviceHandleTemp;
                               instData->searchAg.num++;
                           }
                           if (TRUE == CsrBtUtilSdrGetUintAttributeDataValueFromAttributeUuid(sdpTagList, sdpRecordIndex,
                                                        CSR_BT_REMOTE_AUDIO_VOLUME_CONTROL_ATTRIBUTE_IDENTIFIER, &returnValue))
                           {
                              instData->remSupFeatures = CSR_BT_HF_SUPPORT_REMOTE_VOLUME_CONTROL;
                           }
                           else
                           {
                              instData->remSupFeatures = 0;
                           }
                        }
                    }
                }
            }
        }
    }
    else
    {
        instData->searchHfg.num = 0;
        instData->searchAg.num  = 0;
    }

    CsrBtUtilBllFreeLinkedList(&sdpTagList, CsrBtUtilBllPfreeWrapper);

    /* Now that we have handled the SDC close message, decide what to do based on whether the
       service discovery operation went well or not, and on whether the application has decided to cancel
       or diconnect before connection establishement */
    csrBtHfgUtilCarryOnAfterSdp(instData, resultCode, resultSupplier);
}

void CsrBtHfgRfcSdcConResultHandler(void                  *inst,
                              CsrUint8               localServerCh,
                              CsrUint32                    hfgConnId,
                              CsrBtDeviceAddr                deviceAddr,
                              CsrUint16                    maxFrameSize,
                              CsrBool                      validPortPar,
                              RFC_PORTNEG_VALUES_T        portPar,
                              CsrBtResultCode             resultCode,
                              CsrBtSupplier         resultSupplier,
                              CmnCsrBtLinkedListStruct    *sdpTagList)
{
    HfgInstance_t *instData = (HfgInstance_t *) inst;
    CSR_UNUSED(validPortPar);
    CSR_UNUSED(portPar);  

    CSR_LOG_TEXT_INFO((CsrBtHfgLto, 0, "CsrBtHfgRfcSdcConResultHandler code = 0x%x, suppler 0x%x", resultCode, resultSupplier)); 

    if (resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
        resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        instData->hfgConnId = hfgConnId;
        instData->address = deviceAddr;
        instData->serverChannel = localServerCh;
        instData->maxFrameSize = maxFrameSize;
        CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL,deviceAddr,
                    hfgConnId);
    }

    CsrBtHfgSdcResultHandler(instData, sdpTagList, deviceAddr, resultCode, resultSupplier);
}

void CsrBtHfgSdcSelectServiceHandler(void                    * instData,
                                     void                    * cmSdcRfcInstData,
                                     CsrBtDeviceAddr         deviceAddr,
                                     CsrUint8           serverChannel,
                                     CsrUint16                entriesInSdpTaglist,
                                     CmnCsrBtLinkedListStruct * sdpTagList)
{
    CsrUint16 *serviceHandleIndexList = CsrPmemAlloc(sizeof(CsrUint16) * MAX_SERVICE_RECORDS_SEARCH);
    /*CmnCsrBtLinkedListStruct *tmpSdpTagList = sdpTagList;*/
    CsrBtUuid32    tmpUuid = 0;
    CsrUintFast16  sdpRecordIndex;
    CsrUint16      idx = 0;
    CsrUint16    tmpResult;
    CsrUint16    dummy1, dummy2; /* Currently CSR_UNUSED */

    CSR_UNUSED(deviceAddr);
    CSR_UNUSED(serverChannel);
    /* First find all the HF records*/
    for (sdpRecordIndex = 0; ((sdpRecordIndex < entriesInSdpTaglist) && (idx < MAX_SERVICE_RECORDS_SEARCH)); sdpRecordIndex++)
    {
        if (CsrBtUtilSdrGetServiceUuid32AndResult(sdpTagList,
                                        (CsrUint16)sdpRecordIndex,
                                        &tmpUuid,
                                        &tmpResult,
                                        &dummy1,
                                        &dummy2))
        {
            if (tmpResult == SDR_SDC_SEARCH_SUCCESS)
            {
                if (tmpUuid == CSR_BT_HF_PROFILE_UUID)
                {
                    serviceHandleIndexList[idx] = (CsrUint16)sdpRecordIndex;
                    idx++;
                }
            }
        }
    }

    /* And then all the HS records... */
    for (sdpRecordIndex = 0; ((sdpRecordIndex < entriesInSdpTaglist) && (idx < MAX_SERVICE_RECORDS_SEARCH)); sdpRecordIndex++)
    {
        if (CsrBtUtilSdrGetServiceUuid32AndResult(sdpTagList,
                                        (CsrUint16)sdpRecordIndex,
                                        &tmpUuid,
                                        &tmpResult,
                                        &dummy1,
                                        &dummy2))
        {
            if (tmpResult == SDR_SDC_SEARCH_SUCCESS)
            {
                if (tmpUuid == CSR_BT_HS_PROFILE_UUID)
                {
                    serviceHandleIndexList[idx] = (CsrUint16)sdpRecordIndex;
                    idx++;
                }
            }
        }
    }


    /* Select the preferred service or services to connect to in prioritized order*/
    CsrBtUtilRfcConSetServiceHandleIndexList(instData, cmSdcRfcInstData, serviceHandleIndexList,idx);
}



/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_sched.h"
#include "csr_pmem.h"
#include "bluetooth.h"
#include "hci_prim.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_util.h"
#include "csr_log_text_2.h"

#include "csr_bt_hf_main.h"
#include "csr_bt_hf_prim.h"
#include "csr_bt_hf_util.h"
#include "csr_bt_hf_lib.h"
#include "csr_bt_hf_at_inter.h"
#include "csr_bt_sdc_support.h"
#include "csr_bt_hfhs_data_sef.h"
#include "csr_bt_hf_call_sef.h"
#include "csr_bt_hf_connect_sef.h"

/*************************************************************************************
    An AT-command is received. Send it to HFG via CM.
************************************************************************************/
void CsrBtHfConnectedStateHfAtCmdReqHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfAtCmdReq *prim;
    prim = (CsrBtHfAtCmdReq *) instData->recvMsgP;
    instData->linkData[instData->index].data->dataReceivedInConnected = TRUE;
    if ((instData->linkData[instData->index].lastAtCmdSent == idleCmd) && ((instData->mainConfig & CSR_BT_HF_AT_MODE_TRANSPARENT_ENABLE) == 0x00000000))
    {
        instData->linkData[instData->index].lastAtCmdSent = other;
    }
    CsrBtHfHsSendCmDataReq(instData, (CsrUint16)CsrStrLen((char *)prim->atCmdString),(CsrUint8 *)prim->atCmdString);
}

/*************************************************************************************
    Handler for CSR_BT_HF_CALL_HANDLING_REQ in Connected State
************************************************************************************/
void CsrBtHfConnectedStateHfChldReqHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfCallHandlingReq * prim;
    CsrUint16 length;
    CsrUint8    * callHdlString;
    prim = (CsrBtHfCallHandlingReq *) instData->recvMsgP;

    if (prim->command < CSR_BT_BTRH_PUT_ON_HOLD)
    {/* CHLD_CMD */
        callHdlString = CsrPmemAlloc(CHLD_COMMAND_LENGTH);
        CsrMemSet(callHdlString,0,CHLD_COMMAND_LENGTH);
        length = CHLD_VALUE_INDEX + 1;
        SynMemCpyS(callHdlString, CHLD_COMMAND_LENGTH, CHLD_COMMAND, length);
        length -= 1;
        switch (prim->command)
        {/* CHLD COMMANDS */
            case CSR_BT_RELEASE_ALL_HELD_CALL:
                {
                    callHdlString[length] = '0';
                    length++;
                    break;
                }
            case CSR_BT_RELEASE_ACTIVE_ACCEPT:
                {
                    callHdlString[length] = '1';
                    length++;
                    break;
                }
            case CSR_BT_RELEASE_SPECIFIED_CALL:
                {
                    callHdlString[length] = '1';
                    length++;
                    if (prim->index < 10)
                    {
                        callHdlString[length] = prim->index + '0';
                        length++;
                    }
                    else if (prim->index < 100)
                    {
                        callHdlString[length] = (prim->index / 10) + '0';
                        length++;
                        callHdlString[length] = (prim->index % 10) + '0';
                        length++;
                    }
                    else
                    {
                        /* index > 100 */
                        CsrPmemFree (callHdlString);
                        return;
                    }
                    break;
                }
            case CSR_BT_HOLD_ACTIVE_ACCEPT:
                {
                    callHdlString[length] = '2';
                    length++;
                    break;
                }
            case CSR_BT_REQUEST_PRIVATE_WITH_SPECIFIED:
                {
                    callHdlString[length] = '2';
                    length++;
                    if (prim->index < 10)
                    {
                        callHdlString[length] = prim->index + '0';
                        length++;
                    }
                    else if (prim->index < 100)
                    {
                        callHdlString[length] = (prim->index / 10) + '0';
                        length++;
                        callHdlString[length] = (prim->index % 10) + '0';
                        length++;
                    }
                    else
                    {
                        /* index > 100 */
                        CsrPmemFree (callHdlString);
                        return;
                    }
                    break;
                }
            case CSR_BT_ADD_CALL:
                {
                    callHdlString[length] = '3';
                    length++;
                    break;
                }
            case CSR_BT_CONNECT_TWO_CALLS:
                {
                    callHdlString[length] = '4';
                    length++;
                    break;
                }
            default:
                {
                    CsrPmemFree (callHdlString);
                    return;
                }
        }
        if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
        {
            instData->linkData[instData->index].lastAtCmdSent = chld;
        }
    }
    else
    {/* BTRH */
        callHdlString = CsrPmemAlloc(BTRH_COMMAND_LENGTH);
        CsrMemSet(callHdlString,0,BTRH_COMMAND_LENGTH);
        if (prim->command < CSR_BT_BTRH_READ_STATUS)
        {
            length = BTRH_VALUE_INDEX + 1;
        }
        else
        {
            length = BTRH_VALUE_INDEX;
        }
        SynMemCpyS(callHdlString, BTRH_COMMAND_LENGTH, BTRH_COMMAND, length);
        length -= 1;
        if (prim->command < CSR_BT_BTRH_READ_STATUS)
        {/* Commands: BTRH_PUT_ON_HOLD, BTRH_ACCEPT_INCOMING or BTRH_REJECT_INCOMING ahsll be mapped to '0','1' or '2' respectively*/
            callHdlString[length] = prim->command - CSR_BT_BTRH_PUT_ON_HOLD + '0';
        }
        else
        {/* Query BTRH status */
            callHdlString[length] = '?';
        }
        length++;
        if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
        {
            instData->linkData[instData->index].lastAtCmdSent = btrh;
        }
    }

    callHdlString[length] = '\r';
    length++;

    CsrBtHfHsSendCmDataReq(instData, length, callHdlString);
}

/*************************************************************************************
    Handler for CSR_BT_HF_CALL_ANSWER_REQ in Connected State
************************************************************************************/
void CsrBtHfConnectedStateHfAnswerReqHandler(HfMainInstanceData_t *instData)
{
    HfInstanceData_t *linkPtr = &instData->linkData[instData->index];

    if (linkPtr->linkType == CSR_BT_HF_CONNECTION_HS)
    {
        sendCkpd(instData);
    }
    else
    {
        CsrUint8         *dataPtr;
        CsrUint16       answerStrLen;

        answerStrLen = (CsrUint16) (CsrStrLen(ANSWER));
        dataPtr = (CsrUint8 *) CsrStrDup(ANSWER);
        CsrBtHfHsSendCmDataReq(instData, answerStrLen, dataPtr);
    }
    if (linkPtr->lastAtCmdSent == idleCmd)
    {
        linkPtr->lastAtCmdSent = answer;
    }
}

/*************************************************************************************
    Handler for CSR_BT_HF_CALL_END_REQ in Connected State
************************************************************************************/
void CsrBtHfConnectedStateHfCallEndReqHandler(HfMainInstanceData_t *instData)
{
    HfInstanceData_t *linkPtr = &instData->linkData[instData->index];

    if (linkPtr->linkType == CSR_BT_HF_CONNECTION_HS)
    {
        sendCkpd(instData);
    }
    else
    {
        CsrUint8   *dataPtr;
        CsrUint16 rejectStrLen;

        rejectStrLen = (CsrUint16) (CsrStrLen(REJECT));
        dataPtr      = (CsrUint8 *) CsrStrDup(REJECT);
        CsrBtHfHsSendCmDataReq(instData, rejectStrLen, dataPtr);
    }
    if (linkPtr->lastAtCmdSent == idleCmd)
    {
        linkPtr->lastAtCmdSent = callEnd;
    }
}

/*************************************************************************************
    Handler for CSR_BT_HF_SPEAKER_GAIN_STATUS_REQ in Connected State
************************************************************************************/
void CsrBtHfConnectedStateHfSpeakerGainStatusReqHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfSpeakerGainStatusReq    *prim;
    CsrUint8                  *dataPtr;
    CsrUint16                spkStrLen;

    prim      = (CsrBtHfSpeakerGainStatusReq *) instData->recvMsgP;
    spkStrLen = (CsrUint16) (CsrStrLen(SPEAKER_GAIN));

    dataPtr   = (CsrUint8 *) CsrStrDup(SPEAKER_GAIN);

    if(prim->gain > MAX_SPEAKER_GAIN)
    {
        prim->gain = MAX_SPEAKER_GAIN;
    }
    dataPtr[SPEAKER_GAIN_VALUE_INDEX]   = (char) ((prim->gain / 10) + '0');
    dataPtr[SPEAKER_GAIN_VALUE_INDEX+1] = (char) ((prim->gain % 10) + '0');
    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = vgs;
    }
    CsrBtHfHsSendCmDataReq(instData, spkStrLen, dataPtr);
}

/*************************************************************************************
    Handler for CSR_BT_HF_MIC_GAIN_STATUS_REQ in Connected State
************************************************************************************/
void CsrBtHfConnectedStateHfMicGainStatusReqHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfMicGainStatusReq    *prim;
    CsrUint8              *dataPtr;
    CsrUint16            micStrLen;

    prim      = (CsrBtHfMicGainStatusReq *) instData->recvMsgP;
    micStrLen = (CsrUint16) (CsrStrLen(MICROPHONE_GAIN));
    dataPtr   = (CsrUint8 *) CsrStrDup(MICROPHONE_GAIN);

    if(prim->gain > MAX_MICROPHONE_GAIN)
    {
        prim->gain = MAX_MICROPHONE_GAIN;
    }
    dataPtr[MICROPHONE_GAIN_VALUE_INDEX]   = (char) ((prim->gain / 10) + '0');
    dataPtr[MICROPHONE_GAIN_VALUE_INDEX+1] = (char) ((prim->gain % 10) + '0');
    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = vgm;
    }
    CsrBtHfHsSendCmDataReq(instData, micStrLen, dataPtr);
}

void CsrBtHfConnectedStateHfAudioReqHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfAudioConnectReq *prim;
    HfInstanceData_t *linkPtr = &instData->linkData[instData->index];

    prim = (CsrBtHfAudioConnectReq *) instData->recvMsgP;

    linkPtr->lastAudioReq = CSR_BT_HF_AUDIO_ON;

    if (linkPtr->audioPending == FALSE)
    { /* No audio connection in progress */
        if (linkPtr->scoHandle == HF_SCO_UNUSED)
        { /* No audio connection exists yet: establish one */
            linkPtr->audioPending            = TRUE;

            if (((linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_CODEC_NEGOTIATION) &&
                (instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_CODEC_NEGOTIATION))
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
                || (linkPtr->hfQceCodecId != CSR_BT_HF_QCE_UNSUPPORTED)
#endif
               )
            {
                CsrBtHfSendAtBcc(instData);
            }
            else
            {
                linkPtr->scoConnectAcceptPending = FALSE;
                CsrBtCmScoCancelReqSend(CSR_BT_HF_IFACEQUEUE, linkPtr->hfConnId);

                linkPtr->pcmSlot       = prim->pcmSlot;
                linkPtr->pcmReassign   = prim->pcmRealloc;

                if (linkPtr->remoteVersion >= CSR_BT_FIRST_HFP_ESCO)
                {
                    CsrBtHfSendCmScoConnectReq(linkPtr, CSR_BT_ESCO_DEFAULT_CONNECT);
                }
                else
                {
                    CsrBtHfSendCmScoConnectReq(linkPtr, CSR_BT_SCO_DEFAULT_1P1);
                }
            }
        }
        else
        { /* SCO connection already exists, just send confirmation back with SCO connection limit exceeded. */
            CsrBtHfSendHfAudioConnectInd(instData, linkPtr->pcmSlot, 0, 0, 0, 0, 0, 0,
                                         CSR_BT_RESULT_CODE_HF_SYNCHRONOUS_CONNECTION_LIMIT_EXCEEDED,
                                         0xDEAD, CSR_BT_SUPPLIER_HF, CSR_BT_HF_AUDIO_CONNECT_CFM);
        }
    }
    CsrPmemFree(prim->audioParameters);
}


void CsrBtHfXStateMapScoPcmIndHandler(HfMainInstanceData_t *instData)
{
    HfInstanceData_t *linkPtr = &instData->linkData[instData->index];
    CsrBtCmMapScoPcmInd *prim;
    prim     = (CsrBtCmMapScoPcmInd *) instData->recvMsgP;


    if (TRUE == linkPtr->pcmMappingReceived)
    {
        CsrBtCmMapScoPcmResSend(linkPtr->hfConnId, HCI_SUCCESS, NULL,
                           linkPtr->pcmSlot,
                           linkPtr->pcmReassign);
    }
    else
    {
        CsrBtHfAudioAcceptConnectInd *msg;
        msg                 = (CsrBtHfAudioAcceptConnectInd *)CsrPmemAlloc(sizeof(CsrBtHfAudioAcceptConnectInd));
        msg->type           = CSR_BT_HF_AUDIO_ACCEPT_CONNECT_IND;
        msg->connectionId   = linkPtr->hfConnId;
        msg->linkType       = prim->linkType;
        CsrBtHfMessagePut(instData->appHandle, msg);
        linkPtr->audioAcceptPending = TRUE;
    }
}

/***********************************************************************************
    result of sco connect request. Inform app layer.
************************************************************************************/
void CsrBtHfConnectedStateCmScoConnectCfmHandler(HfMainInstanceData_t *instData)
{
    HfInstanceData_t *linkPtr = &instData->linkData[instData->index];

    CsrBtCmScoConnectCfm *prim;
    prim     = (CsrBtCmScoConnectCfm *) instData->recvMsgP;

    /* if sco already in use, ignore the confirm message */
    if (linkPtr->scoHandle == HF_SCO_UNUSED)
    {
        if ((linkPtr->lastAudioReq == CSR_BT_HF_AUDIO_ON) && (linkPtr->audioPending))
        { /* Application initiated outgoing SCO connection */
            linkPtr->audioPending = FALSE;

            if ((prim->resultCode     == CSR_BT_RESULT_CODE_CM_SUCCESS) &&
                (prim->resultSupplier == CSR_BT_SUPPLIER_CM))
            { /* Connection successful : Send success result to the application */
                linkPtr->scoHandle          = prim->eScoHandle;
                linkPtr->pcmSlot            = prim->pcmSlot;
                linkPtr->pcmMappingReceived = FALSE;

                CsrBtHfSendHfAudioConnectInd(instData,
                                             prim->pcmSlot,
                                             prim->linkType,
                                             prim->weSco,
                                             prim->rxPacketLength,
                                             prim->txPacketLength,
                                             prim->airMode,
                                             prim->txInterval,
                                             CSR_BT_RESULT_CODE_HF_SUCCESS,
                                             0xDEAD,
                                             CSR_BT_SUPPLIER_HF,
                                             CSR_BT_HF_AUDIO_CONNECT_CFM);
            }
            else
            { /* Connection failed : Inform application & allow CM to accept further
               * incoming SCO connection                                                */
                CsrBtHfSendHfAudioConnectInd(instData,
                                             0,
                                             prim->linkType,
                                             prim->weSco,
                                             prim->rxPacketLength,
                                             prim->txPacketLength,
                                             prim->airMode,
                                             prim->txInterval,
                                             prim->resultCode,
                                             0xDEAD,
                                             prim->resultSupplier,
                                             CSR_BT_HF_AUDIO_CONNECT_CFM);
                /* No more trials; make sure to accept eSCO/SCO connections after this! */
                /* In this case no sco connect accept can be pending... */
                CsrBtHfAcceptIncomingSco(linkPtr);
            }
        }
        else
        {
            if (linkPtr->audioPending)
            { /* linkPtr->lastAudioReq == CSR_BT_HF_AUDIO_OFF : App requested audio OFF
               * while waiting for connection confirmation                              */
                if ((prim->resultCode     == CSR_BT_RESULT_CODE_CM_SUCCESS) &&
                    (prim->resultSupplier == CSR_BT_SUPPLIER_CM))
                { /* SCO connected successfully but App decided to disconnect it */
                    linkPtr->pcmMappingReceived = FALSE;
                    CsrBtCmScoDisconnectReqSend(CSR_BT_HF_IFACEQUEUE, linkPtr->hfConnId);
                }
                else
                { /* SCO connection is failed so nothing to disconnect, just inform App
                   * about failure & allow CM to accept further incoming connection     */
                    linkPtr->audioPending = FALSE;
                    CsrBtHfSendHfAudioConnectInd(instData,
                                                 0,
                                                 0,
                                                 prim->weSco,
                                                 prim->rxPacketLength,
                                                 prim->txPacketLength,
                                                 prim->airMode,
                                                 prim->txInterval,
                                                 prim->resultCode,
                                                 0xDEAD,
                                                 prim->resultSupplier,
                                                 CSR_BT_HF_AUDIO_CONNECT_CFM);
                    /* No more trials; make sure to accept further eSCO/SCO connections */
                    CsrBtHfAcceptIncomingSco(linkPtr);
                }
            }
        }
    }
}

/***********************************************************************************
    Handle CSR_BT_CM_SCO_DISCONNECT_IND in Connected state
************************************************************************************/
void CsrBtHfConnectedStateCmScoDisconnectIndHandler(HfMainInstanceData_t *instData)
{
    CsrBtCmScoDisconnectInd    *prim;
    HfInstanceData_t *linkPtr = &instData->linkData[instData->index];

    prim    = (CsrBtCmScoDisconnectInd *) instData->recvMsgP;

    /* if sco already disconnected a race condition may have occurred so ignore the message */
    if (linkPtr->scoHandle != HF_SCO_UNUSED)
    {
        if (linkPtr->linkType == CSR_BT_HF_CONNECTION_HS)
        {
            linkPtr->pcmMappingReceived = FALSE;
        }

        if (linkPtr->audioPending == FALSE)
        { /* Remote device initiated disconnection */
            linkPtr->scoHandle = HF_SCO_UNUSED;

            CsrBtHfSendHfAudioDisconnectInd(instData, prim->eScoHandle,
                                            prim->reasonCode, prim->reasonSupplier);
            CsrBtHfAcceptIncomingSco(linkPtr);
        }
        else
        { /* linkPtr->audioPending == TRUE : HF initiated either audio OFF or ON */
            if (linkPtr->lastAudioReq == CSR_BT_HF_AUDIO_OFF)
            { /* App initiated audio OFF */
                if (prim->status)
                {
                    linkPtr->audioPending = FALSE;
                    linkPtr->scoHandle    = HF_SCO_UNUSED;

                    CsrBtHfSendHfAudioDisconnectCfm(instData, prim->eScoHandle,
                                                    CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
                    CsrBtHfAcceptIncomingSco(linkPtr);
                }
                else
                {
                    /* sco disconnect failed but it must be disconnected so we try again */
                    CsrBtCmScoDisconnectReqSend(CSR_BT_HF_IFACEQUEUE, linkPtr->hfConnId);
                }
            }
            else
            { /* linkPtr->audioPending == TRUE  && linkPtr->lastAudioReq == CSR_BT_HF_AUDIO_ON :
               * App requested audio ON while waiting for OFF, connect SCO again */
                if (prim->status)
                {
                    if (linkPtr->linkType == CSR_BT_HF_CONNECTION_HF)
                    {
                        if (((linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_CODEC_NEGOTIATION) &&
                              (instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_CODEC_NEGOTIATION))
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
                              || (linkPtr->hfQceCodecId != CSR_BT_HF_QCE_UNSUPPORTED)
#endif
                           )
                        {
                            CsrBtHfSendAtBcc(instData);
                        }
                        else
                        {
                            if (linkPtr->remoteVersion >= CSR_BT_FIRST_HFP_ESCO)
                            {
                                CsrBtHfSendCmScoConnectReq(linkPtr, CSR_BT_ESCO_DEFAULT_CONNECT);
                            }
                            else
                            {
                                CsrBtHfSendCmScoConnectReq(linkPtr, CSR_BT_SCO_DEFAULT_1P1);
                            }
                        }
                    }
                    else
                    { /* linkPtr->linkType is CSR_BT_HF_CONNECTION_HS */
                        CsrBtHfAcceptIncomingSco(linkPtr);
                        sendCkpd(instData);
                    }

                    linkPtr->scoHandle = HF_SCO_UNUSED;
                }
                else
                { /* Disconnection failed, it means we are still connected, just inform App about connection */
                    linkPtr->audioPending = FALSE;
                    CsrBtHfSendHfAudioConnectInd(instData, linkPtr->pcmSlot,
                                                 0,0,0,0,0,0, CSR_BT_RESULT_CODE_HF_SUCCESS,
                                                 0xDEAD, CSR_BT_SUPPLIER_HF, CSR_BT_HF_AUDIO_CONNECT_CFM);
                }
            }
        }
    }
}

void CsrBtHfConnectedStateCmScoAcceptConnectCfmHandler(HfMainInstanceData_t *instData)
{
    CsrBtCmScoAcceptConnectCfm    *prim;
    HfInstanceData_t *linkPtr = &instData->linkData[instData->index];

    prim = (CsrBtCmScoAcceptConnectCfm *) instData->recvMsgP;

    linkPtr->scoConnectAcceptPending = FALSE;

    /* if sco already connected ignore the message */
    if (linkPtr->scoHandle == HF_SCO_UNUSED)
    {
        if (linkPtr->audioPending == FALSE)
        { /* Handle incoming audio connection in below scenarios, when
           * Case 1 : linkPtr->lastAudioReq is CSR_BT_HF_AUDIO_ON : App had requested
           *          audio ON but audio connection was not successful.
           * Case 2 : linkPtr->lastAudioReq is CSR_BT_HF_AUDIO_OFF : App had requested
           *          audio OFF and audio disconnected successfully.                  */
            linkPtr->pcmMappingReceived = FALSE;

            if (prim->resultCode      == CSR_BT_RESULT_CODE_CM_SUCCESS &&
                     prim->resultSupplier  == CSR_BT_SUPPLIER_CM)
            {
               linkPtr->scoHandle          = prim->eScoHandle;
               linkPtr->pcmSlot            = prim->pcmSlot;
               CsrBtHfSendHfAudioConnectInd(instData,
                                            prim->pcmSlot,
                                            prim->linkType,
                                            prim->weSco,
                                            prim->rxPacketLength,
                                            prim->txPacketLength,
                                            prim->airMode,
                                            prim->txInterval,
                                            CSR_BT_RESULT_CODE_HF_SUCCESS,
                                            0xDEAD,
                                            CSR_BT_SUPPLIER_HF,
                                            CSR_BT_HF_AUDIO_CONNECT_IND);
            }
            else
            {
               /* Let the app know.about failure and allow CM to accept incoming connection. */
               CsrBtHfSendHfAudioConnectInd(instData,
                                            prim->pcmSlot,
                                            prim->linkType,
                                            prim->weSco,
                                            prim->rxPacketLength,
                                            prim->txPacketLength,
                                            prim->airMode,
                                            prim->txInterval,
                                            prim->resultCode,
                                            0xDEAD,
                                            prim->resultSupplier,
                                            CSR_BT_HF_AUDIO_CONNECT_IND);
               CsrBtHfAcceptIncomingSco(linkPtr);
            }
        }
        else
        { /* linkPtr->audioPending == TRUE, app requested either ON or OFF */
            if (linkPtr->lastAudioReq == CSR_BT_HF_AUDIO_ON)
            { /* Case 3 : App requested audio ON and received incoming audio connection,
               *          so just handle it.                                          */
               if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier  == CSR_BT_SUPPLIER_CM)
               {
                   linkPtr->scoHandle          = prim->eScoHandle;
                   linkPtr->pcmSlot            = prim->pcmSlot;
                   linkPtr->pcmMappingReceived = FALSE;
                   linkPtr->audioPending       = FALSE;

                   /* Since App also initiated audio connection, lets send audio confirmation msg */
                   CsrBtHfSendHfAudioConnectInd(instData,
                                                prim->pcmSlot,
                                                prim->linkType,
                                                prim->weSco,
                                                prim->rxPacketLength,
                                                prim->txPacketLength,
                                                prim->airMode,
                                                prim->txInterval,
                                                CSR_BT_RESULT_CODE_HF_SUCCESS,
                                                0xDEAD,
                                                CSR_BT_SUPPLIER_HF,
                                                CSR_BT_HF_AUDIO_CONNECT_CFM);
               }
            }
            else
            { /* Case 4 : App requested audio OFF while waiting for audio connection  */
                if ((prim->resultCode      == CSR_BT_RESULT_CODE_CM_SUCCESS) &&
                    (prim->resultSupplier  == CSR_BT_SUPPLIER_CM))
                {
                    linkPtr->scoHandle          = prim->eScoHandle;
                    linkPtr->pcmSlot            = prim->pcmSlot;
                    linkPtr->pcmMappingReceived = FALSE;

                    if (linkPtr->linkType == CSR_BT_HF_CONNECTION_HF)
                    {
                        CsrBtCmScoDisconnectReqSend(CSR_BT_HF_IFACEQUEUE, linkPtr->hfConnId);
                    }
                    else
                    {
                        sendCkpd(instData);
                    }
                }
            }
        }
    }
}


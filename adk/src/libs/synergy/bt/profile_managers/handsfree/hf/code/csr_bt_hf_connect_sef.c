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
#include "csr_bt_hf_at_inter.h"
#include "csr_bt_hf_connect_sef.h"

#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_hf_streams.h"
#endif

static void csrBtHfCmConnectAcceptCfmHandler(HfMainInstanceData_t *instData)
{
    CsrBtCmConnectAcceptCfm *prim = (CsrBtCmConnectAcceptCfm *) instData->recvMsgP;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    instData->currentDeviceAddress = linkPtr->currentDeviceAddress = prim->deviceAddr;

    if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
        prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        linkPtr->hfConnId = prim->btConnId;
        linkPtr->data->maxRfcFrameSize    = prim->profileMaxFrameSize;
        linkPtr->oldState = Activate_s;
        linkPtr->linkState = CSR_BT_LINK_STATUS_CONNECTED;
        linkPtr->incomingSlc = TRUE;
        CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL, prim->deviceAddr, prim->btConnId);

        if (linkPtr->linkType == CSR_BT_HF_CONNECTION_HF)
        {/* HFP */
            linkPtr->state = ServiceSearch_s;
            linkPtr->serviceState = sdcState_s;
            startSdcFeatureSearch(instData, FALSE);
        }
        else
        {/* HSP */
            CsrBtHfAcceptIncomingSco(linkPtr);
            CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
            linkPtr->state = Connected_s;
        }

#ifdef CSR_STREAMS_ENABLE
        CsrBtHfStreamsRegister(instData, prim->btConnId);
#endif
        /* Now make sure not to accept more connections if the maximum allowed is reached */
        CsrBtHfCancelAcceptCheck(instData);
    }
    else
    {/* connect to peer failed during par neg or other. Activate the service again  to let others connect */
        if (prim->resultCode != CSR_BT_RESULT_CODE_CM_ALREADY_CONNECTING)
        {
           CsrBtHfAllowConnectCheck(instData);
        }
       /* If already connecting, just wait for the incoming connection....*/
    }
}

/*************************************************************************************
    Connection attempt from peer complete, if success start service search.
************************************************************************************/
void CsrBtHfActivateStateCmConnectAcceptCfmHandler(HfMainInstanceData_t *instData)
{
    CsrBtCmConnectAcceptCfm *prim;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);
    CsrUint8 nrActiveHf = 0;
    CsrUint8 nrActiveHs = 0;
    CsrUint8 nrOfActiveConnections = CsrBtHfGetNumberOfRecordsInUse(instData,&nrActiveHf,&nrActiveHs);
    CsrBool connExist = FALSE;

    prim = (CsrBtCmConnectAcceptCfm *) instData->recvMsgP;

    if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
        prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {/* Check if connection already exists with the device */
        HfInstanceData_t *connInst = CsrBtHfGetInstFromBdAddr(instData, &prim->deviceAddr);

        if (connInst)
        {
            if (connInst->hfConnId != CSR_BT_CONN_ID_INVALID)
            {
                /* This means we are already connected and are receiving another incoming
                 * connection from the same device, we need to disconnect in this case. */
                connExist = TRUE;
            }
            else
            {
                /* This means outgoing connection was initiated to the same remote device,
                 * we need to drop the connection in order for this incoming connection to go through.*/
                if (connInst->sdpSearchData)
                {
                    /* This means the service search was already going on, we need to cancel it
                     * as we are dropping the outgoing connection. */
                    CsrBtUtilRfcConCancel((void *)connInst, connInst->sdpSearchData);

                    if (linkPtr == connInst)
                    {
                        /* The instance used by the CM is already getting used for a possible
                         * outgoing connection, mark this to be reused. */
                        connInst->instReused = TRUE;

                        /* We need to wait for the outgoing connection to get dropped before processing this
                         * incoming connection, save this connection and process after outgoing connection finishes.
                         */
                        CsrBtHfSaveCmMessage(instData);

                        /* returning from here, as we don't want to process this message immediately. */
                        return;
                    }

                    /* Note that if connInst and linkPtr doesn't match then we are using a different
                     * instance for this incoming connection and hence we do not need to wait for 
                     * the CsrBtUtilRfcConCancel to complete, continue with processing the
                     * current incoming connection. */
                }
            }
        }
    }

    /* Check if connection does not exist already for this device and allowed to establish more connections. */
    if (!connExist && 
        (instData->maxTotalSimultaneousConnections > nrOfActiveConnections))
    {
        csrBtHfCmConnectAcceptCfmHandler(instData);
    }
    else
    {
        if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
            prim->resultSupplier == CSR_BT_SUPPLIER_CM)
        {/* Disconnect */
            /* Set 'linkPtr->hfConnId' to handle CSR_BT_CM_CM_DISCONNECT_IND message */
            linkPtr->hfConnId = prim->btConnId;
            CsrBtCmDisconnectReqSend(prim->btConnId);
        }
    }
}

/*************************************************************************************
    Link state has changed. Record new link state.
************************************************************************************/
void CsrBtHfXStateCmConnectAcceptCfmHandler(HfMainInstanceData_t *instData)
{
    CsrBtCmConnectAcceptCfm *prim = (CsrBtCmConnectAcceptCfm *) instData->recvMsgP;

    if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
        prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        HfInstanceData_t *linkPtr = &instData->linkData[instData->index];

        if (linkPtr->state == ServiceSearch_s || linkPtr->state == Connect_s)
        {
            /* There is a crossover where outgoing slc request was through and we have received
             * incoming SLC connection. Being inside this callback indicates that both incoming
             * and outgoing is pointing to the same hf instance, hence one of them
             * needs to be given up. We give priority to incoming connection and hence we will be
             * letting our outgoing connection drop. */
            if (linkPtr->sdpSearchData)
            {
                /* This means the service search was already going on, we need to cancel it
                 * as we are dropping the outgoing connection. */
                CsrBtUtilRfcConCancel((void *)linkPtr, linkPtr->sdpSearchData);

                /* Mark this instance to be reused for incoming connection, so that when
                 * the outgoing request fails we won't be clearing the states. */
                linkPtr->instReused = TRUE;

                /* save this message for later processing when the SDC search of existing outgoing
                 * connection gets cancelled successfully. */
                CsrBtHfSaveCmMessage(instData);
            }
        }
        else
        {
            /* For states Connected_s the design is unchanged. */
            CsrBtCmDisconnectReqSend(prim->btConnId);
        }
    }
}

/*************************************************************************************
    Handle the Disconnect req in LpEnabled or connected state, send disconnect.
************************************************************************************/
void CsrBtHfXStateHfDisconnectReqHandler(HfMainInstanceData_t *instData)
{
    HfInstanceData_t *linkPtr = (HfInstanceData_t *) &(instData->linkData[instData->index]);

    linkPtr->disconnectReqReceived = TRUE;
    if (linkPtr->scoHandle != HF_SCO_UNUSED)
    {
        CsrBtCmScoDisconnectReqSend(CSR_BT_HF_IFACEQUEUE, linkPtr->hfConnId);
    }
    CsrBtCmDisconnectReqSend(linkPtr->hfConnId);
}

/*************************************************************************************
    Inform higher layer and clean up.
************************************************************************************/
void CsrBtHfXStateCmDisconnectIndHandler(HfMainInstanceData_t *instData)
{
    CsrBtCmDisconnectInd *prim;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    prim = (CsrBtCmDisconnectInd *) instData->recvMsgP;

    if (linkPtr->state != Activate_s)
    {
        if (!prim->status)
        {
            CsrBtCmDisconnectReqSend(prim->btConnId);
        }
        else if (linkPtr->linkType == CSR_BT_HF_CONNECTION_HF)
        {
            if (linkPtr->state != ServiceSearch_s)
            {
                linkPtr->pcmMappingReceived = FALSE;

                if (linkPtr->disconnectReqReceived == TRUE)
                {
                    CsrBtHfSaveQueueCleanUp(instData);
                    if (linkPtr->scoHandle != HF_SCO_UNUSED)
                    {
                        CsrBtHfSendHfAudioDisconnectInd(instData, linkPtr->scoHandle,
                                                            prim->reasonCode, prim->reasonSupplier);
                        linkPtr->scoHandle = HF_SCO_UNUSED;
                    }
                    CsrBtHfSendHfDisconnectInd(instData, prim->reasonCode, prim->reasonSupplier);
                    CsrBtHfInitInstanceData(linkPtr);
                    linkPtr->disconnectReqReceived = FALSE;
                }
                else if (linkPtr->state == Connected_s)
                {   /* no cancel received so disconnect must be result of lp fail */
                    CsrBtHfSaveQueueCleanUp(instData);

                    if (linkPtr->atSequenceState >= serviceLevel)
                    {
                        /* IOP Fix: If HFP is disconnected prior to SCO disconnection,
                           CM would not send CSR_BT_CM_SCO_DISCONNECT_IND though it receives
                           DM_SYNC_DISCONNECT_IND from DM. So just inform app about Audio
                           Disconnection now. */
                        if (linkPtr->scoHandle != HF_SCO_UNUSED)
                        {
                            CsrBtHfSendHfAudioDisconnectInd(instData, linkPtr->scoHandle,
                                                            prim->reasonCode, prim->reasonSupplier);
                            linkPtr->scoHandle = HF_SCO_UNUSED;
                        }
                        else if (linkPtr->audioAcceptPending)
                        {
                            /* SLC got disconnected while HF is waiting for application to respond
                             * Audio Connect Accept Indication. Respond to CM now as HF ignores
                             * sending application's response in Disconnected state */
                            CsrBtCmMapScoPcmResSend(prim->btConnId,
                                                    HCI_ERROR_REJ_BY_REMOTE_PERS,
                                                    NULL,
                                                    CSR_BT_PCM_DONT_CARE,
                                                    FALSE);
                            linkPtr->audioAcceptPending = FALSE;
                        }

                        CsrBtHfSendHfDisconnectInd(instData, prim->reasonCode, prim->reasonSupplier);
                    }
                    else
                    {
                        /* Application has not been informed of connection yet.
                         * Notify application of failed connection attempt on HF */
                        CsrBtHfSendHfServiceConnectInd(instData,
                                                       CSR_BT_RESULT_CODE_HF_CONNECT_ATTEMPT_FAILED,
                                                       CSR_BT_SUPPLIER_HF);
                    }
                    CsrBtHfInitInstanceData(linkPtr);
                }
                linkPtr->audioPending = FALSE;
                linkPtr->linkState = CSR_BT_LINK_STATUS_DISCONNECTED;
            }
            else /* Service search state */
            {
                if (linkPtr->disconnectReqReceived)
                {
                    /* a cancel has been received so send a cancel cfm to app layer */
                    CsrBtHfSendHfDisconnectInd(instData, prim->reasonCode, prim->reasonSupplier);
                    CsrBtHfInitInstanceData(linkPtr);
                    linkPtr->state = Activate_s;
                }
                else /* check the state of the SLC establishment*/
                {
                    if (linkPtr->serviceState == sdcState_s)
                    {
                        /* this means that the other device disconnected during SDC search*/
                        linkPtr->disconnectPeerReceived = TRUE;
                    }
                    else
                    {/* serviceState should be in either of the 2 remaining states
                      * btConnect_s - Service search is ongoing/completed and before start of AT sequence
                      * serviceConnect_s - AT sequence is ongoing but SLC has not been established */

                        /* this is when connection has been established, but before serviceConnectInd has been sent to app */
                        if (linkPtr->oldState == Connect_s)
                        {
                            /* app tried to connect. inform that connect failed */
                            if (linkPtr->pendingCancel)
                            {
                                CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_CANCELLED_CONNECT_ATTEMPT, CSR_BT_SUPPLIER_HF);
                            }
                            else
                            {
                                CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_CONNECT_ATTEMPT_FAILED, CSR_BT_SUPPLIER_HF);
                            }
                        }
                        CsrBtHfInitInstanceData(linkPtr);
                        linkPtr->state = Activate_s;
                    }
                }
                linkPtr->pcmMappingReceived = FALSE;
                linkPtr->linkState = CSR_BT_LINK_STATUS_DISCONNECTED;
            }
        }
        else
        {/* HSP */
            if (linkPtr->disconnectReqReceived)
            {
                CsrBtHfSaveQueueCleanUp(instData);
                linkPtr->state = Activate_s;
                linkPtr->pcmMappingReceived = FALSE;

                /* App requested Disconnect */
                CsrBtHfSendHfDisconnectInd(instData, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
                CsrBtHfInitInstanceData(linkPtr);
                linkPtr->disconnectReqReceived = FALSE;
            }
            else
            {
                /* this means other side disconnected or abnormal disconnect. send result to app */
                CsrBtHfSaveQueueCleanUp(instData);
                /* save result for DISCONNECT_IND */
                linkPtr->pcmMappingReceived = FALSE;

                CsrBtHfSendHfDisconnectInd(instData,prim->reasonCode, prim->reasonSupplier);
                linkPtr->disconnectReqReceived = FALSE;
                CsrBtHfInitInstanceData(linkPtr);
            }
        }
    }
    else
    {
        /* Disconnect request was sent for incoming connection when maximum allowed 
           simultaneous connection reached, reset the CONN ID */
        CsrBtHfInitInstanceData(linkPtr);
    }
}


/*************************************************************************************
    function called in connect and service search states to set the cancel req
    received flag. When the flag is set the confirm message on either connect or
    sdp search close will check the flag and disconnect.
************************************************************************************/
void CsrBtHfXStateHfCancelReqHandler(HfMainInstanceData_t *instData)
{
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    if ((!linkPtr->pendingCancel) && (linkPtr->searchOngoing))
    {
        if (linkPtr->searchAndCon)
        {
            linkPtr->pendingCancel = TRUE;
            CsrBtUtilRfcConCancel((void*) (linkPtr), linkPtr->sdpSearchData);
        }
        else
        {
            CsrBtUtilSdcSearchCancel((void*) (linkPtr), linkPtr->sdpSearchData);
        }
    }
    else
    {
        if (!linkPtr->searchOngoing)
        {
            linkPtr->pendingCancel = TRUE;
            CsrBtCmDisconnectReqSend(linkPtr->hfConnId);
        }
    }
    if (*((CsrBtHfPrim *)(instData->recvMsgP)) == CSR_BT_HF_DISCONNECT_REQ)
    {
        linkPtr->disconnectReqReceived = TRUE;
    }
    else
    {/* HF_CANCEL_REQ */
        linkPtr->pendingCancel = TRUE;
    }
}

/*************************************************************************************
    Handle incoming connection indication from remote device. Accept/reject the
    connection based on the current circumstances.
************************************************************************************/
void HfActivateStateCmRfcConnectAcceptIndHandler(HfMainInstanceData_t *instData)
{
    CsrBtCmRfcConnectAcceptInd *prim = (CsrBtCmRfcConnectAcceptInd *) instData->recvMsgP;
    HfInstanceData_t *connInst       = CsrBtHfGetInstFromBdAddr(instData, &prim->deviceAddr);

    /* Check if connection does already exist with this device */
    if (connInst && connInst->hfConnId != CSR_BT_CONN_ID_INVALID)
    { /* Crossover scenario : Already connected with the same device.
       * Reject the duplicate connection */
        CsrBtCmRfcConnectAcceptRspSend(CSR_BT_HF_IFACEQUEUE,
                                       prim->btConnId,
                                       prim->deviceAddr,
                                       FALSE,
                                       prim->localServerChannel,
                                       CSR_BT_MODEM_SEND_CTRL_DTE_DEFAULT,
                                       CSR_BT_DEFAULT_BREAK_SIGNAL,
                                       CSR_BT_DEFAULT_MSC_TIMEOUT);
    }
    else
    { /* Device is un-known, check if connection can be accepted */
        CsrBtCmRfcConnectAcceptRspSend(CSR_BT_HF_IFACEQUEUE,
                                       prim->btConnId,
                                       prim->deviceAddr,
                                       HfIsNewConnectionAllowed(instData, prim->localServerChannel),
                                       prim->localServerChannel,
                                       CSR_BT_MODEM_SEND_CTRL_DTE_DEFAULT,
                                       CSR_BT_DEFAULT_BREAK_SIGNAL,
                                       CSR_BT_DEFAULT_MSC_TIMEOUT);
    }
}


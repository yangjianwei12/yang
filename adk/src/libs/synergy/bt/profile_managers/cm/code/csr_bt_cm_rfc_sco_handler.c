/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#include "csr_synergy.h"

#if !defined(EXCLUDE_CSR_BT_RFC_MODULE) && !defined(EXCLUDE_CSR_BT_SCO_MODULE)

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"

#ifndef CSR_TARGET_PRODUCT_VM
#include "csr_hci_sco.h"
#endif

#include "csr_bt_cm_events_handler.h"

#ifndef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
#include "csr_bt_cm_bccmd.h"
#endif

void CsrBtCmDmScoConnectCfmMsgSend(CsrSchedQid               phandle,
                                   CsrBtConnId            btConnId,
                                   DM_SYNC_CONNECT_CFM_T  *dmPrim,
                                   CsrUint8                pcmSlot,
                                   CsrBtResultCode        resultCode,
                                   CsrBtSupplier    resultSupplier)
{
    /* Send a CSR_BT_CM_SCO_CONNECT_CFM signal to the application */
    CsrBtCmScoConnectCfm    *cmPrim;

    cmPrim                      = (CsrBtCmScoConnectCfm *)CsrPmemAlloc(sizeof(CsrBtCmScoConnectCfm));
    cmPrim->type                = CSR_BT_CM_SCO_CONNECT_CFM;
    cmPrim->btConnId            = btConnId;
    cmPrim->resultCode          = resultCode;
    cmPrim->pcmSlot             = pcmSlot;
    cmPrim->resultSupplier      = resultSupplier;

    if (dmPrim)
    {
        cmPrim->airMode         = dmPrim->air_mode;
        cmPrim->linkType        = dmPrim->link_type;
        cmPrim->rxPacketLength  = dmPrim->rx_packet_length;
        cmPrim->txInterval      = dmPrim->tx_interval;
        cmPrim->txPacketLength  = dmPrim->tx_packet_length;
        cmPrim->weSco           = dmPrim->wesco;
    }
    else
    {
        cmPrim->airMode         = 0;
        cmPrim->linkType        = 0;
        cmPrim->rxPacketLength  = 0;
        cmPrim->txInterval      = 0;
        cmPrim->txPacketLength  = 0;
        cmPrim->weSco           = 0;
    }

    if (resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
        resultSupplier == CSR_BT_SUPPLIER_CM &&
        dmPrim != NULL)
    {
        cmPrim->eScoHandle      = dmPrim->handle;
    }
    else
    {
        cmPrim->eScoHandle      = NO_SCO;
    }
    CsrBtCmPutMessage(phandle, cmPrim);
}

static void csrBtCmDmScoConnectCfmFailSend(CsrSchedQid phandle, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    /* Send a CSR_BT_CM_SCO_CONNECT_CFM signal to the application */
    CsrBtCmScoConnectCfm    *cmPrim;

    cmPrim                    = (CsrBtCmScoConnectCfm *)CsrPmemAlloc(sizeof(CsrBtCmScoConnectCfm));
    cmPrim->type              = CSR_BT_CM_SCO_CONNECT_CFM;
    cmPrim->btConnId          = 0;
    cmPrim->pcmSlot           = 0;
    cmPrim->airMode           = 0;
    cmPrim->linkType          = 0;
    cmPrim->rxPacketLength    = 0;
    cmPrim->txInterval        = 0;
    cmPrim->txPacketLength    = 0;
    cmPrim->weSco             = 0;
    cmPrim->resultCode        = resultCode;
    cmPrim->resultSupplier    = resultSupplier;
    cmPrim->eScoHandle        = NO_SCO;
    CsrBtCmPutMessage(phandle, cmPrim);
}

static void csrBtCmDmScoAcceptConnectCfmMsgSend(cmRfcConnInstType               *theLogicalLink,
                                                DM_SYNC_CONNECT_COMPLETE_IND_T  *dmPrim)
{
    /* Send a CM_ACCEPT_CONNECT_CFM signal to the application */

    CsrBtCmScoAcceptConnectCfm    *cmPrim;

    cmPrim                  = (CsrBtCmScoAcceptConnectCfm *)CsrPmemAlloc(sizeof(CsrBtCmScoAcceptConnectCfm));
    cmPrim->type            = CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM;
    cmPrim->btConnId        = theLogicalLink->btConnId;
    cmPrim->pcmSlot         = theLogicalLink->eScoParms ?
                    theLogicalLink->eScoParms->pcmSlot : 0;

    if (dmPrim->status == HCI_SUCCESS)
    {
        cmPrim->airMode         = dmPrim->air_mode;
        cmPrim->linkType        = dmPrim->link_type;
        cmPrim->rxPacketLength  = dmPrim->rx_packet_length;
        cmPrim->txInterval      = dmPrim->tx_interval;
        cmPrim->txPacketLength  = dmPrim->tx_packet_length;
        cmPrim->weSco           = dmPrim->wesco;
        cmPrim->resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
        cmPrim->resultSupplier  = CSR_BT_SUPPLIER_CM;
        cmPrim->eScoHandle      = dmPrim->handle;
    }
    else
    {
        cmPrim->eScoHandle      = NO_SCO;
        cmPrim->airMode         = 0;
        cmPrim->linkType        = 0;
        cmPrim->rxPacketLength  = 0;
        cmPrim->txInterval      = 0;
        cmPrim->txPacketLength  = 0;
        cmPrim->weSco           = 0;
        cmPrim->resultCode      = (CsrBtResultCode) dmPrim->status;
        cmPrim->resultSupplier  = CSR_BT_SUPPLIER_HCI;
    }

    if (theLogicalLink->eScoParms)
    {
        theLogicalLink->eScoParms->handle  = cmPrim->eScoHandle;
    }

    CsrBtCmPutMessage(theLogicalLink->appHandle, cmPrim);
}

static void csrBtCmDmScoAcceptConnectCfmFailSend(CsrSchedQid                    appHandle,
                                                 CsrBtResultCode        resultCode,
                                                 CsrBtSupplier    resultSupplier)
{
    /* Send a CM_ACCEPT_CONNECT_CFM fail signal to the application */

    CsrBtCmScoAcceptConnectCfm    *cmPrim;

    cmPrim                  = (CsrBtCmScoAcceptConnectCfm *)CsrPmemAlloc(sizeof(CsrBtCmScoAcceptConnectCfm));
    cmPrim->type            = CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM;
    cmPrim->btConnId        = 0;
    cmPrim->resultCode      = resultCode;
    cmPrim->resultSupplier  = resultSupplier;
    cmPrim->pcmSlot         = 0;

    cmPrim->airMode         = 0;
    cmPrim->linkType        = 0;
    cmPrim->rxPacketLength  = 0;
    cmPrim->txInterval      = 0;
    cmPrim->txPacketLength  = 0;
    cmPrim->weSco           = 0;
    cmPrim->eScoHandle      = NO_SCO;
    CsrBtCmPutMessage(appHandle, cmPrim);
}

static void csrBtCmDmScoDisconnectIndMsgSend(CsrSchedQid                     appHandle,
                                             CsrBtConnId                btConnId,
                                             hci_connection_handle_t handle,
                                             CsrBool                  status,
                                             CsrBtReasonCode         reasonCode)
{
    /* Send CSR_BT_CM_SCO_DISCONNECT_IND to the application */
    CsrBtCmScoDisconnectInd *cmPrim;

    cmPrim = (CsrBtCmScoDisconnectInd *)CsrPmemAlloc(sizeof(CsrBtCmScoDisconnectInd));

    cmPrim->type            = CSR_BT_CM_SCO_DISCONNECT_IND;
    cmPrim->btConnId        = btConnId;
    cmPrim->eScoHandle      = handle;
    cmPrim->status          = status;
    cmPrim->reasonCode      = reasonCode;
    cmPrim->reasonSupplier  = CSR_BT_SUPPLIER_HCI;
    CsrBtCmPutMessage(appHandle, cmPrim);
}

void CsrBtCmDmScoRenegotiateIndMsgSend(cmRfcConnInstType * theLogicalLink,
                                       CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtCmScoRenegotiateInd    * cmPrim;

    cmPrim = (CsrBtCmScoRenegotiateInd *)CsrPmemAlloc(sizeof(CsrBtCmScoRenegotiateInd));

    cmPrim->type            = CSR_BT_CM_SCO_RENEGOTIATE_IND;
    cmPrim->btConnId        = theLogicalLink->btConnId;
    cmPrim->resultCode      = resultCode;
    cmPrim->resultSupplier  = resultSupplier;
    cmPrim->eScoHandle      = theLogicalLink->eScoParms ?
                    theLogicalLink->eScoParms->handle : NO_SCO;

    CsrBtCmPutMessage(theLogicalLink->appHandle, cmPrim);
}

#ifdef CSR_BT_INSTALL_CM_PRI_SCO_RENEGOTIATE
void CsrBtCmDmScoRenegotiateCfmMsgSend(cmRfcConnInstType * theLogicalLink,
                                       CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtCmScoRenegotiateCfm    * cmPrim;

    cmPrim = (CsrBtCmScoRenegotiateCfm *)CsrPmemAlloc(sizeof(CsrBtCmScoRenegotiateCfm));

    cmPrim->type            = CSR_BT_CM_SCO_RENEGOTIATE_CFM;
    cmPrim->btConnId        = theLogicalLink->btConnId;
    cmPrim->resultCode      = resultCode;
    cmPrim->resultSupplier  = resultSupplier;
    cmPrim->eScoHandle      = theLogicalLink->eScoParms ?
                    theLogicalLink->eScoParms->handle : NO_SCO;
    CsrBtCmPutMessage(theLogicalLink->appHandle, cmPrim);
}

static void csrBtCmDmScoRenegotiateCfmFailSend(CsrSchedQid appHandle, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtCmScoRenegotiateCfm    * cmPrim;

    cmPrim = (CsrBtCmScoRenegotiateCfm *)CsrPmemAlloc(sizeof(CsrBtCmScoRenegotiateCfm));

    cmPrim->type            = CSR_BT_CM_SCO_RENEGOTIATE_CFM;
    cmPrim->btConnId        = 0;
    cmPrim->resultCode      = resultCode;
    cmPrim->resultSupplier  = resultSupplier;
    cmPrim->eScoHandle      = 0;
    CsrBtCmPutMessage(appHandle, cmPrim);
}
#endif
void CsrBtCmDmSyncConnectRfcCfmHandler(cmRfcConnInstType *theLogicalLink, cmInstanceData_t *cmData)
{
    DM_SYNC_CONNECT_CFM_T *dmPrim = (DM_SYNC_CONNECT_CFM_T *) cmData->recvMsgP;

    if (theLogicalLink->eScoParms)
    {
        /* This event is the confirmation of the opening (or otherwise) of a SCO
           connection following a SCO connect request from one of the profiles using
           L2cap. */
        CsrBtCmScoCommonParms parms;

        if (dmPrim->status == HCI_SUCCESS)
        {
            CsrBtCmScoGetCurrentNegotiateParms(theLogicalLink->eScoParms->negotiateCnt,
                                               &parms);
            CsrBtCmScoSetEScoParms(theLogicalLink->eScoParms,
                                   &parms,
                                   dmPrim->handle);
            CsrBtCmScoFreePacketTypeArray(&theLogicalLink->eScoParms->negotiateCnt);
            theLogicalLink->eScoParms->handle = dmPrim->handle;

            if (theLogicalLink->eScoParms->pcmSlot > 0 &&
                theLogicalLink->eScoParms->pcmSlot <= MAX_PCM_SLOTS)
            {
                cmData->dmVar.pcmAllocationTable[theLogicalLink->eScoParms->pcmSlot - 1] = dmPrim->handle;
            }

            CsrBtCmStoreDownstreamEScoParms(theLogicalLink->eScoParms,
                                            FALSE,
                                            dmPrim->link_type,
                                            dmPrim->tx_interval,
                                            dmPrim->wesco,
                                            dmPrim->rx_packet_length,
                                            dmPrim->tx_packet_length,
                                            dmPrim->air_mode,
                                            dmPrim->status);

            CsrBtCmDmScoConnectCfmMsgSend(theLogicalLink->appHandle,
                                          theLogicalLink->btConnId,
                                          dmPrim,
                                          theLogicalLink->eScoParms->pcmSlot,
                                          CSR_BT_RESULT_CODE_CM_SUCCESS,
                                          CSR_BT_SUPPLIER_CM);
            cmData->dmVar.rfcConnIndex = CM_ERROR;
            CsrBtCmDmLocalQueueHandler();
        }
        else
        {
            if (CsrBtCmScoGetNextNegotiateParms(theLogicalLink->eScoParms->negotiateCnt,
                                                &parms))
            {
                if ((dmPrim->status != HCI_ERROR_NO_CONNECTION)
                    && (dmPrim->status != HCI_ERROR_PAGE_TIMEOUT)
                    && (dmPrim->status != HCI_ERROR_CONN_TIMEOUT)
                    && (dmPrim->status != HCI_ERROR_MAX_NR_OF_SCO)
                    && (dmPrim->status != HCI_ERROR_HOST_TIMEOUT)
                    && (dmPrim->status != HCI_ERROR_LMP_RESPONSE_TIMEOUT))
                {
                    cmData->dmVar.activePlayer   = RFC_PLAYER;
                    cmData->dmVar.scoConnectAddr = theLogicalLink->deviceAddr;
                    CsrBtCmRfcScoConnectReqHandler(cmData, cmData->dmVar.scoConnectAddr);
                }
                else if (dmPrim->status == HCI_ERROR_HOST_TIMEOUT &&
                         (CsrBtCmScoCurrentNegotiateParmsIsSco(&parms) ||
                          CsrBtCmScoSeekToNextScoOnlyNegotiateParms(theLogicalLink->eScoParms->negotiateCnt)))
                {
                    cmData->dmVar.activePlayer   = RFC_PLAYER;
                    cmData->dmVar.scoConnectAddr = theLogicalLink->deviceAddr;
                    CsrBtCmRfcScoConnectReqHandler(cmData, cmData->dmVar.scoConnectAddr);
                }
                else
                {
                    CsrBtCmScoFreePacketTypeArray(&theLogicalLink->eScoParms->negotiateCnt);
                    CsrBtCmDmSyncClearPcmSlotFromTable(cmData,
                                                       theLogicalLink->eScoParms);
                    CsrBtCmDmScoConnectCfmMsgSend(theLogicalLink->appHandle,
                                                  theLogicalLink->btConnId,
                                                  dmPrim,
                                                  0,
                                                  (CsrBtResultCode) dmPrim->status,
                                                  CSR_BT_SUPPLIER_HCI);

                    cmData->dmVar.rfcConnIndex = CM_ERROR;
                    CsrBtCmDmLocalQueueHandler();
                }
            }
            else
            { /* Nore more packets to try */
                CsrBtCmScoFreePacketTypeArray(&theLogicalLink->eScoParms->negotiateCnt);
                CsrBtCmDmSyncClearPcmSlotFromTable(cmData,
                                                   theLogicalLink->eScoParms);
                CsrBtCmDmScoConnectCfmMsgSend(theLogicalLink->appHandle,
                                              theLogicalLink->btConnId,
                                              dmPrim,
                                              0,
                                              (CsrBtResultCode) dmPrim->status,
                                              CSR_BT_SUPPLIER_HCI);

                cmData->dmVar.rfcConnIndex = CM_ERROR;
                CsrBtCmDmLocalQueueHandler();
            }
        }
    }
    else
    {
        CsrBtCmDmScoConnectCfmMsgSend(theLogicalLink->appHandle,
                                      theLogicalLink->btConnId,
                                      dmPrim,
                                      0,
                                      CSR_BT_RESULT_CODE_CM_SYNCHRONOUS_CONNECTION_ATTEMPT_FAILED,
                                      CSR_BT_SUPPLIER_CM);

        cmData->dmVar.rfcConnIndex = CM_ERROR;
        dm_sync_disconnect_req(dmPrim->handle,
                               HCI_ERROR_OETC_USER);

        CsrBtCmDmLocalQueueHandler();
    }
}

void CsrBtCmDmSyncConnectCompleteRfcHandler(cmRfcConnInstType *theLogicalLink, DM_SYNC_CONNECT_COMPLETE_IND_T *dmPrim)
{
    /* This event is a SCO connect indication from remote bluetooth device. */
    CsrBtCmStoreDownstreamEScoParms(theLogicalLink->eScoParms,
                                    TRUE,
                                    dmPrim->link_type,
                                    dmPrim->tx_interval,
                                    dmPrim->wesco,
                                    dmPrim->rx_packet_length,
                                    dmPrim->tx_packet_length,
                                    dmPrim->air_mode,
                                    dmPrim->status);

    csrBtCmDmScoAcceptConnectCfmMsgSend(theLogicalLink, dmPrim);
}

void CsrBtCmDmSyncDisconnectRfcHandler(cmInstanceData_t *cmData,
                                       cmRfcConnInstType *theLogicalLink,
                                       CsrUint8 status,
                                       CsrBtReasonCode reason)
{
    CsrBool sendConfirmation = FALSE;

    switch (theLogicalLink->state)
    {
        case CSR_BT_CM_RFC_STATE_RELEASE:
            /* Need to release the RFCOMM connection                              */
            RfcDisconnectReqSend((CsrUint16) theLogicalLink->btConnId);
            break;

        default:
            sendConfirmation = TRUE;
            break;
    }

    if (cmData->dmVar.lockMsg == CSR_BT_CM_SCO_DISCONNECT_REQ)
    {
        sendConfirmation = TRUE;
        CsrBtCmDmLocalQueueHandler();
    }

    if (theLogicalLink->eScoParms)
    {
        if (sendConfirmation)
        {
            csrBtCmDmScoDisconnectIndMsgSend(theLogicalLink->appHandle,
                                             theLogicalLink->btConnId,
                                             theLogicalLink->eScoParms->handle,
                                             (status == HCI_SUCCESS) ? TRUE : FALSE,
                                             reason);
        }

        /* Clear the SCO fields in any case if RFC release was requested otherwise
         * clear these only if SCO disconnection is successful */
        if (theLogicalLink->state == CSR_BT_CM_RFC_STATE_RELEASE || status == HCI_SUCCESS)
        {
            CsrBtCmDmSyncClearPcmSlotFromTable(cmData, theLogicalLink->eScoParms);
            CsrBtCmRfcEscoParmsFree(theLogicalLink);
        }
    }
}

void CsrBtCmDmScoConnectReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmScoConnectReq *cmPrim  = (CsrBtCmScoConnectReq *) cmData->recvMsgP;
    cmRfcConnElement * theElement    = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromBtConnId, &(cmPrim->btConnId));

    if(theElement)
    { /* Request to create SCO connection to remote Bluetooth device */
        aclTable *aclConnectionElement;
        cmSyncNegotiationCntType_t *negotiateCnt = NULL;
        cmRfcConnInstType *theLogicalLink = theElement->cmRfcConnInst;

        returnAclConnectionElement(cmData, theLogicalLink->deviceAddr, &aclConnectionElement);

        if (theLogicalLink->eScoParms)
        {
            negotiateCnt = theLogicalLink->eScoParms->negotiateCnt;
        }

        if (aclConnectionElement &&
            CsrBtCmScoCreatePacketTypeArray(cmData,
                                            &cmPrim->parms,
                                            cmPrim->parmsLen,
                                            theLogicalLink->deviceAddr,
                                            &negotiateCnt))
        {
            if (!theLogicalLink->eScoParms)
            {
                theLogicalLink->eScoParms = CsrPmemZalloc(sizeof(*theLogicalLink->eScoParms));
                theLogicalLink->eScoParms->handle = NO_SCO;
            }

            theLogicalLink->eScoParms->negotiateCnt = negotiateCnt;

            if (theLogicalLink->eScoParms->handle == NO_SCO &&
                theLogicalLink->state == CSR_BT_CM_RFC_STATE_CONNECTED)
            {
                /* Request the DM to setup a SCO connection */
                CsrUint8 numOfSco = returnNumberOfScoConnection(cmData);

                if (numOfSco < CSR_BT_MAX_NUM_SCO_CONNS)
                {
                    if ((cmPrim->pcmSlot != CSR_BT_PCM_DONT_CARE  && cmPrim->pcmSlot != CSR_BT_PCM_DONT_MAP) &&
                        ((cmPrim->pcmSlot == 0) ||
                         ((cmPrim->pcmSlot <= MAX_PCM_SLOTS) &&
                          (cmData->dmVar.pcmAllocationTable[cmPrim->pcmSlot - 1] == NO_SCO))))
                    {
                        theLogicalLink->eScoParms->pcmSlot = cmPrim->pcmSlot;
                    }
                    else
                    {
                        if (cmPrim->pcmSlot == CSR_BT_PCM_DONT_MAP)
                        {
                            theLogicalLink->eScoParms->pcmSlot = 0;
                        }
                        else if ((cmPrim->pcmReassign) || (cmPrim->pcmSlot == CSR_BT_PCM_DONT_CARE))
                        {
                            theLogicalLink->eScoParms->pcmSlot = CsrBtCmDmFindFreePcmSlot(cmData);
                            if (theLogicalLink->eScoParms->pcmSlot > MAX_PCM_SLOTS)
                            {
                                CsrBtCmDmScoConnectCfmMsgSend(theLogicalLink->appHandle,
                                                              theLogicalLink->btConnId,
                                                              NULL,
                                                              0,
                                                              CSR_BT_RESULT_CODE_CM_INVALID_PCM_SLOT,
                                                              CSR_BT_SUPPLIER_CM);
                                CsrBtCmDmLocalQueueHandler();
                                return;
                            }
                        }
                        else
                        {
                            CsrBtCmDmScoConnectCfmMsgSend(theLogicalLink->appHandle,
                                                          theLogicalLink->btConnId,
                                                          NULL,
                                                          0,
                                                          CSR_BT_RESULT_CODE_CM_PCM_SLOT_BLOCKED,
                                                          CSR_BT_SUPPLIER_CM);
                            CsrBtCmDmLocalQueueHandler();
                            return;
                        }
                    }

                    cmData->dmVar.rfcConnIndex               = theElement->elementId;
                    if (theLogicalLink->eScoParms->pcmSlot != 0)
                    {
                        cmData->dmVar.pcmAllocationTable[theLogicalLink->eScoParms->pcmSlot - 1] = SCOBUSY_ACCEPT;
                    }

                    /* Inform SCO connect to device utility, automatic procedures like role switch can be sent from there.*/
                    (void)CmDuHandleAutomaticProcedure(cmData,
                                                       CM_DU_AUTO_EVENT_SCO_CONNECT,
                                                       (void *)theLogicalLink,
                                                       &aclConnectionElement->deviceAddr);
                    cmData->dmVar.scoConnectAddr = theLogicalLink->deviceAddr;
#ifndef EXCLUDE_CSR_BT_CHIP_SUPPORT_MAP_SCO_PCM
                    CsrBtCmBccmdMapScoPcmReqSend(theLogicalLink->eScoParms->pcmSlot,
                                                 RFC_SCO_CONNECT);
#else
                     /* Skip map SCO over PCM and go directly for the SCO setup procedure */
                    CsrBtCmRfcScoConnectReqHandler(cmData, cmData->dmVar.scoConnectAddr);
#endif
                }
                else
                {
                    CsrBtCmDmScoConnectCfmMsgSend(theLogicalLink->appHandle,
                                                  theLogicalLink->btConnId,
                                                  NULL,
                                                  0,
                                                  CSR_BT_RESULT_CODE_CM_SYNCHRONOUS_CONNECTION_LIMIT_EXCEEDED,
                                                  CSR_BT_SUPPLIER_CM);
                    CsrBtCmDmLocalQueueHandler();
                }

            }
            else
            { /* The Sco handle is already reserve */
                CsrBtResultCode resultCode;

                if (theLogicalLink->state != CSR_BT_CM_RFC_STATE_CONNECTED)
                {
                    resultCode = CSR_BT_RESULT_CODE_CM_UNKNOWN_CONNECTION_IDENTIFIER;
                }
                else
                {
                    resultCode = CSR_BT_RESULT_CODE_CM_SYNCHRONOUS_CONNECTION_ALREADY_EXISTS;
                }

                CsrBtCmDmScoConnectCfmMsgSend(theLogicalLink->appHandle,
                                              theLogicalLink->btConnId,
                                              NULL,
                                              0,
                                              resultCode,
                                              CSR_BT_SUPPLIER_CM);
                CsrBtCmDmLocalQueueHandler();
            }
        }
        else
        {
            CsrBtResultCode resultCode;

            if (!aclConnectionElement)
            {
                resultCode = CSR_BT_RESULT_CODE_CM_UNKNOWN_CONNECTION_IDENTIFIER;
            }
            else
            {
                resultCode = CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE_OR_PARAMETER_VALUE;
            }

            if (theLogicalLink->eScoParms)
            {
                CsrBtCmScoFreePacketTypeArray(&theLogicalLink->eScoParms->negotiateCnt);
            }

            CsrBtCmDmScoConnectCfmMsgSend(theLogicalLink->appHandle,
                                          theLogicalLink->btConnId,
                                          NULL,
                                          0,
                                          resultCode,
                                          CSR_BT_SUPPLIER_CM);
            CsrBtCmDmLocalQueueHandler();
        }
    }
    else
    { /* The search criterion did not match. */
        CsrBtCmDmLocalQueueHandler();
        csrBtCmDmScoConnectCfmFailSend(cmPrim->appHandle,
                                       CSR_BT_RESULT_CODE_CM_UNKNOWN_CONNECTION_IDENTIFIER, CSR_BT_SUPPLIER_CM);
    }
}

void CsrBtCmDmScoAcceptConnectReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmScoAcceptConnectReq *cmPrim = (CsrBtCmScoAcceptConnectReq *) cmData->recvMsgP;
    cmRfcConnElement * theElement    = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromBtConnId, &(cmPrim->btConnId));

    if(theElement)
    { /* The CM has registered the the application is able to accept a SCO connection */
        cmRfcConnInstType *theLogicalLink   = theElement->cmRfcConnInst;

        if (!theLogicalLink->eScoParms ||
            theLogicalLink->eScoParms->handle == NO_SCO)
        {
            if (!theLogicalLink->eScoParms)
            {
                theLogicalLink->eScoParms = CsrPmemZalloc(sizeof(*theLogicalLink->eScoParms));
            }
            theLogicalLink->eScoParms->handle          = SCOBUSY_ACCEPT;
            theLogicalLink->eScoParms->maxLatency      = cmPrim->maxLatency;
            theLogicalLink->eScoParms->packetType      = SCO_PACKET_RESERVED_BITS | cmPrim->audioQuality;
            theLogicalLink->eScoParms->reTxEffort      = cmPrim->reTxEffort;
            theLogicalLink->eScoParms->rxBdw           = cmPrim->rxBandwidth;
            theLogicalLink->eScoParms->txBdw           = cmPrim->txBandwidth;
            theLogicalLink->eScoParms->voiceSettings   = cmPrim->voiceSettings;
            theLogicalLink->eScoParms->pcmSlot         = 0;
        }
        else
        {
            if (theLogicalLink->eScoParms->handle == SCOBUSY_ACCEPT)
            {
                csrBtCmDmScoAcceptConnectCfmFailSend(cmPrim->appHandle,
                                                     CSR_BT_RESULT_CODE_CM_SYNCHRONOUS_CONNECTION_ALREADY_ACCEPTABLE, CSR_BT_SUPPLIER_CM);
            }
            else
            {
                csrBtCmDmScoAcceptConnectCfmFailSend(cmPrim->appHandle,
                                                     CSR_BT_RESULT_CODE_CM_SYNCHRONOUS_CONNECTION_ALREADY_EXISTS, CSR_BT_SUPPLIER_CM);
            }
        }
    }
    else
    {
        csrBtCmDmScoAcceptConnectCfmFailSend(cmPrim->appHandle,
                                             CSR_BT_RESULT_CODE_CM_UNKNOWN_CONNECTION_IDENTIFIER, CSR_BT_SUPPLIER_CM);
    }
}

void CsrBtCmDmScoCancelAcceptConnectReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmScoCancelAcceptConnectReq *cmPrim = (CsrBtCmScoCancelAcceptConnectReq *) cmData->recvMsgP;
    cmRfcConnElement * theElement    = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromBtConnId, &(cmPrim->btConnId));

    if(theElement)
    {
        cmRfcConnInstType *theLogicalLink   = theElement->cmRfcConnInst;

        if (theLogicalLink->eScoParms &&
            theLogicalLink->eScoParms->handle == SCOBUSY_ACCEPT)
        {
            /* The CM has registered the the application will not accept a SCO connection */
            theLogicalLink->eScoParms->handle = NO_SCO;
        }
    }
}

#ifdef CSR_BT_INSTALL_CM_PRI_SCO_RENEGOTIATE
void CsrBtCmDmScoRenegotiateReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmScoRenegotiateReq *cmPrim = (CsrBtCmScoRenegotiateReq *) cmData->recvMsgP;
    cmRfcConnElement * theElement    = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromBtConnId, &(cmPrim->btConnId));

    if(theElement)
    { /* Send a SCO disconnect request for a particular SCO connection handle */
        cmRfcConnInstType *theLogicalLink   = theElement->cmRfcConnInst;

        if (theLogicalLink->eScoParms &&
            theLogicalLink->eScoParms->handle < SCOBUSY_ACCEPT)
        { /* A SCO handle is present. Try to renegotiate the SCO connection */
            dm_sync_renegotiate_req(theLogicalLink->eScoParms->handle,
                                    cmPrim->maxLatency,
                                    cmPrim->reTxEffort,
                                    cmPrim->audioQuality);
        }
        else
        { /* A SCO handle is not present. Restore the local DM queue */
            CsrBtCmDmScoRenegotiateCfmMsgSend(theLogicalLink,
                                              CSR_BT_RESULT_CODE_CM_COMMAND_DISALLOWED, CSR_BT_SUPPLIER_CM);
            CsrBtCmDmLocalQueueHandler();
        }
    }
    else
    { /* Error, the search criterion did not match, restore local DM queue */
        csrBtCmDmScoRenegotiateCfmFailSend(cmPrim->appHandle,
                                           CSR_BT_RESULT_CODE_CM_UNKNOWN_CONNECTION_IDENTIFIER, CSR_BT_SUPPLIER_CM);
        CsrBtCmDmLocalQueueHandler();
    }
}
#endif

void CsrBtCmDmScoDisconnectReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmScoDisconnectReq *cmPrim = (CsrBtCmScoDisconnectReq *) cmData->recvMsgP;
    cmRfcConnElement * theElement    = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromBtConnId, &(cmPrim->btConnId));

    if(theElement)
    { /* Send a SCO disconnect request for a particular SCO connection handle */
        cmRfcConnInstType *theLogicalLink   = theElement->cmRfcConnInst;

        if (theLogicalLink->eScoParms &&
            theLogicalLink->eScoParms->handle < SCOBUSY_ACCEPT)
        {
            if (theElement->cmRfcConnInst->eScoParms->closeSco)
            {
                /* SCO Disconnect in progress. Only possible when it started from CM on RFC disconnect.
                   Confirmation to application and DM queue unlock would happen on handling DM_SYNC_DISCONNECT_CFM. */
                return;
            }
            /* A SCO handle is present. Try to release the SCO connection */
            cmData->dmVar.rfcConnIndex = theElement->elementId;
            theElement->cmRfcConnInst->eScoParms->closeSco = TRUE;
            dm_sync_disconnect_req(theLogicalLink->eScoParms->handle,
                                   HCI_ERROR_OETC_USER);
        }
        else
        {
            /* A SCO handle is not present. Restore the local DM queue */
            CsrBtCmDmLocalQueueHandler();
            csrBtCmDmScoDisconnectIndMsgSend(cmPrim->appHandle,
                                             cmPrim->btConnId,
                                             NO_SCO,
                                             TRUE,
                                             HCI_ERROR_NO_CONNECTION);
        }
    }
    else
    {
        /* Error, the search criterion did not match, restore local DM queue */
        CsrBtCmDmLocalQueueHandler();
        csrBtCmDmScoDisconnectIndMsgSend(cmPrim->appHandle,
                                         cmPrim->btConnId,
                                         NO_SCO,
                                         TRUE,
                                         HCI_ERROR_NO_CONNECTION);
    }
}

#endif /* !EXCLUDE_CSR_BT_RFC_MODULE && !EXCLUDE_CSR_BT_SCO_MODULE */


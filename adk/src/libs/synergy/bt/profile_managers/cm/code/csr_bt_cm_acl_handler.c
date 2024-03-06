/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
 ******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_le.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_sdc.h"

static void csrBtCmAclCloseCfmSend(cmInstanceData_t *cmData,
                                   CsrBtTypedAddr deviceAddr,
                                   CsrUint8 reason,
                                   CsrUint16 flags)
{
    CsrBtCmAclCloseCfm *prim;

    prim = CsrPmemZalloc(sizeof(*prim));
    prim->type = CSR_BT_CM_ACL_CLOSE_CFM;
    prim->deviceAddr = deviceAddr;
    prim->reason = reason;
    prim->flags = flags;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
}

static void csrBtCmAclOpenCfmSend(CsrSchedQid appHandle,
                                  CsrBtTypedAddr deviceAddr,
                                  CsrBool success)
{
    CsrBtCmAclOpenCfm *cfm = CsrPmemZalloc(sizeof(*cfm));

    cfm->type = CSR_BT_CM_ACL_OPEN_CFM;
    cfm->deviceAddr = deviceAddr;
    cfm->status = success;
    CsrBtCmPutMessage(appHandle, cfm);
}

void csrBtCmAclElemInit(cmInstanceData_t *cmData,
                        aclTable *aclConnectionElement,
                        const CsrBtDeviceAddr *deviceAddr)
{
    CsrMemSet(aclConnectionElement->remoteFeatures,
              0xFF,
              sizeof(aclConnectionElement->remoteFeatures));

    CsrBtBdAddrCopy(&aclConnectionElement->deviceAddr, deviceAddr);
#ifdef CSR_BT_INSTALL_CM_QHS_PHY_SUPPORT
    aclConnectionElement->qhsPhyConnected = FALSE;
#endif
#ifdef CSR_BT_INSTALL_CM_SWB_DISABLE_STATE
    aclConnectionElement->swbDisabled = FALSE;
#endif
    aclConnectionElement->remoteFeaturesValid = FALSE;
    aclConnectionElement->roleChecked = FALSE;
    aclConnectionElement->lmpVersion = CSR_BT_CM_INVALID_LMP_VERSION;
    aclConnectionElement->linkPolicySettings = cmData->dmVar.defaultLinkPolicySettings;
    /* Zero initialize (not valid) */
    CsrMemSet(&aclConnectionElement->curSsrSettings,
              0,
              sizeof(aclConnectionElement->curSsrSettings));
    aclConnectionElement->encryptType = DM_SM_ENCR_NONE;
    aclConnectionElement->flushTo = L2CA_FLUSH_TO_DEFAULT;
    aclConnectionElement->lsto = CSR_BT_HCI_DEFAULT_LSTO;
    aclConnectionElement->mode = CSR_BT_ACTIVE_MODE;
    aclConnectionElement->interval = 0;

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
    aclConnectionElement->logicalChannelTypeMask = CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL;
    aclConnectionElement->noOfGuaranteedLogicalChannels = 0;
#endif
    aclConnectionElement->l2capExtendedFeatures = CM_INVALID_L2CAP_EXT_FEAT;
    aclConnectionElement->unsolicitedQosSetup = FALSE;
    aclConnectionElement->serviceType = HCI_QOS_NO_TRAFFIC;
    aclConnectionElement->tokenRate = 0x00000000; /* don't care */
    aclConnectionElement->peakBandwidth = 0x00000000; /* don't care */
    aclConnectionElement->latency = CSR_BT_CM_DEFAULT_QOS_LATENCY; /* default latency */
    aclConnectionElement->delayVariation = 0xFFFFFFFFu; /* don't care */
    aclConnectionElement->aclQosSetupAppHandle = CSR_SCHED_QID_INVALID;
#ifdef EXCLUDE_CSR_BT_SC_MODULE
    aclConnectionElement->bondRequired = TRUE;
#endif
}

static void csrBtCmDmAclCloseSuccessIndHandler(cmInstanceData_t *cmData,
                                               CsrBtDeviceAddr deviceAddr,
                                               hci_error_t reason)
{
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    cmRfcConnElement *currentRfcElem, *nextRfcElem;
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    cmL2caConnElement *currentL2caElem, *nextL2caElem;
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

    aclTable *aclConnectionElement;

    CsrBtCmRemoveSavedIncomingConnectMessages(cmData, deviceAddr);
    returnAclConnectionElement(cmData, deviceAddr, &aclConnectionElement);

    if (aclConnectionElement)
    {
        CsrBtBdAddrZero(&(aclConnectionElement->deviceAddr));
        CsrBtCmDmAclRoleVarsClear(CsrBtCmDmGetAclRoleVars(aclConnectionElement));
#ifdef CSR_BT_INSTALL_CM_QHS_PHY_SUPPORT
        aclConnectionElement->qhsPhyConnected = FALSE;
#endif
#ifdef CSR_BT_INSTALL_CM_SWB_DISABLE_STATE
        aclConnectionElement->swbDisabled = FALSE;
#endif
        aclConnectionElement->aclRequestedByApp = FALSE;
        aclConnectionElement->l2capExtendedFeatures = CM_INVALID_L2CAP_EXT_FEAT;
#ifndef CSR_BT_EXCLUDE_HCI_QOS_SETUP
        aclConnectionElement->serviceType = HCI_QOS_NO_TRAFFIC;
        aclConnectionElement->tokenRate = 0x00000000; /* don't care */
        aclConnectionElement->peakBandwidth = 0x00000000; /* don't care */
        aclConnectionElement->latency = CSR_BT_CM_DEFAULT_QOS_LATENCY; /* default latency */
        aclConnectionElement->delayVariation = 0xFFFFFFFFu; /* don't care */
        aclConnectionElement->aclQosSetupAppHandle = CSR_SCHED_QID_INVALID;
#endif
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
        if ((aclConnectionElement->noOfGuaranteedLogicalChannels != 0) ||
            (aclConnectionElement->logicalChannelTypeMask != CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL))
        {
            aclConnectionElement->noOfGuaranteedLogicalChannels = 0;
            aclConnectionElement->logicalChannelTypeMask = CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL;
            /* Issue event CSR_BT_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE */
        }
#endif
        if (aclConnectionElement->rfcCloseTimerId != 0)
        {
            CsrSchedTimerCancel(aclConnectionElement->rfcCloseTimerId,
                                NULL,
                                NULL);
            aclConnectionElement->rfcCloseTimerId = 0;
        }

        CsrBtCmAclCloseLegacyHandler(cmData, aclConnectionElement,
                                     deviceAddr,
                                     reason);
    }

    /* Inform this to device utility in order to complete any automatic procedures.*/
    (void)CmDuHandleAutomaticProcedure(cmData,
                                       CM_DU_AUTO_EVENT_ACL_CLOSED,
                                       NULL,
                                       &deviceAddr);

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    for (currentRfcElem = CM_RFC_GET_FIRST(cmData->rfcVar.connList);
         currentRfcElem;
         currentRfcElem = nextRfcElem)
    {
        nextRfcElem = currentRfcElem->next; /* Make a copy of the next pointer, since CsrBtCmConnectCfmMsgSend can free
         currentRfcElem. */
        if (currentRfcElem->cmRfcConnInst)
        {
            if (CsrBtBdAddrEq(&(deviceAddr),
                              &(currentRfcElem->cmRfcConnInst->deviceAddr)))
            { /* If the device address match, the ACL connection is release without
                 receiving a RFC_RELEASE_IND */
                cmRfcConnInstType *theLogicalLink = currentRfcElem->cmRfcConnInst;
                CsrBtCmDmSyncClearPcmSlotFromTable(cmData,
                                                   theLogicalLink->eScoParms);

                if (theLogicalLink->state == CSR_BT_CM_RFC_STATE_CONNECT_INIT)
                {
                    if (cmData->rfcVar.connectState == CM_RFC_IDLE)
                    {
                        CsrBtCmConnectCfmMsgSend(cmData,
                                                 (CsrBtResultCode) reason,
                                                 CSR_BT_SUPPLIER_HCI);
                        CsrBtCmServiceManagerLocalQueueHandler(cmData);
                    }
                }
            }
        }
    }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    for (currentL2caElem = CM_L2CA_GET_FIRST(cmData->l2caVar.connList);
         currentL2caElem;
         currentL2caElem = nextL2caElem)
    { /* Search through the L2CAP connection list */
        nextL2caElem = currentL2caElem->next;
        if (currentL2caElem->cmL2caConnInst)
        {
            if (CsrBtBdAddrEq(&(deviceAddr),
                              &(currentL2caElem->cmL2caConnInst->deviceAddr)))
            { /* If the device address match, the ACL connection is release without
                 receiving a L2CAP DISCONNECT_IND */
                cmL2caConnInstType *l2CaConnection = currentL2caElem->cmL2caConnInst;

                if (l2CaConnection->state == CSR_BT_CM_L2CAP_STATE_CONNECT_INIT)
                {
                    if (cmData->l2caVar.connectState == CM_L2CA_IDLE)
                    {
                        CsrBtCmL2caConnectCfmMsgHandler(cmData,
                                                        currentL2caElem,
                                                        (CsrBtResultCode) reason,
                                                        CSR_BT_SUPPLIER_HCI);
                        CsrBtCmServiceManagerLocalQueueHandler(cmData);
                    }
                }
            }
        }
    }
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

    if (cmData->sdcVar.state == CSR_BT_CM_SDC_STATE_SEARCH &&
        cmData->sdcVar.lockMsg == CM_SDC_SERVICE_SEARCH_ATTR_REQ)
    {
        /* Reset the SDP state and send a failed cfm to the common utility */
        cmData->smVar.arg.result.code     = (CsrBtResultCode) reason;
        cmData->smVar.arg.result.supplier = CSR_BT_SUPPLIER_HCI;
        cmSdcServiceSearchAttrFailureHandler(cmData);
    }

    CsrBtCmSdcDecAclRefCountTo(cmData, deviceAddr);
}

static void csrBtCmDmAclOpenedSuccessIndHandler(cmInstanceData_t *cmData,
                                                CsrBtDeviceAddr deviceAddr,
                                                CsrBool incoming,
                                                CsrUint24 cod)
{
    aclTable *aclConnectionElement;
    CsrBtDeviceAddr devAddr;

    returnAclConnectionElement(cmData, deviceAddr, &aclConnectionElement);

    CsrBtCmGeneralExceptionOn(aclConnectionElement,
                              CSR_BT_CM_PRIM,
                              DM_ACL_CONN_HANDLE_IND,
                              cmData->globalState);

    CsrBtBdAddrZero(&devAddr);
    returnAclConnectionElement(cmData, devAddr, &aclConnectionElement);

    if (aclConnectionElement)
    {
        csrBtCmAclElemInit(cmData, aclConnectionElement, &deviceAddr);

        aclConnectionElement->cod = cod;
        aclConnectionElement->incoming = incoming;

        if (incoming)
        {
            aclConnectionElement->role = CSR_BT_SLAVE_ROLE;
        }
        else
        {
            aclConnectionElement->role = CSR_BT_MASTER_ROLE;
        }

        CsrBtCmDmAclRoleVarsClear(CsrBtCmDmGetAclRoleVars(aclConnectionElement));

        CsrBtCmReadRemoteFeaturesReqSend(CSR_BT_CM_IFACEQUEUE, deviceAddr);
        CsrBtCmReadRemoteVersionReqSend(CSR_BT_CM_IFACEQUEUE,
                                        deviceAddr,
                                        CSR_BT_ADDR_PUBLIC,
                                        CSR_BT_TRANSPORT_BREDR);
        CsrBtCmRoleDiscoveryReqSend(CSR_BT_CM_IFACEQUEUE, deviceAddr);
    }
    else
    {
        CsrBtTypedAddr addrt;

        addrt.addr = deviceAddr;
        addrt.type = CSR_BT_ADDR_TYPE_PUBLIC;
        CsrBtCmAclCloseReqSend(CSR_BT_CM_IFACEQUEUE, addrt, DM_ACL_FLAG_FORCE, HCI_ERROR_OETC_USER);
    }
}

CsrBool CmAclOpenPendingMsgCompareAddr(CsrCmnListElm_t *elem, void *data)
{
    cmPendingMsg_t *pendingMsg = (cmPendingMsg_t*) elem;
    CsrBtTypedAddr *addrt = (CsrBtTypedAddr*) data;

    if (pendingMsg->type == CSR_BT_CM_PENDING_MSG_ACL_OPEN_PARAMS)
    {
        cmAclOpenParam *aclOpenParams = &pendingMsg->arg.aclOpenParams;

        if (CsrBtAddrEq(&aclOpenParams->addrt, addrt))
        {
            return TRUE;
        }
    }

    return FALSE;
}

static CsrUint8 cmAclGetTransport(CsrUint16 flags)
{
    return ((flags & DM_ACL_FLAG_ULP) ? CSR_BT_TRANSPORT_LE : CSR_BT_TRANSPORT_BREDR);
}

static CsrBool csrBtCmAclOpenPendingMsgCompare(CsrCmnListElm_t *elem,
                                               void *data)
{
    cmPendingMsg_t *pendingMsg = (cmPendingMsg_t*) elem;
    cmAclOpenParam *params = (cmAclOpenParam*) data;

    if (pendingMsg->type == CSR_BT_CM_PENDING_MSG_ACL_OPEN_PARAMS)
    {
        cmAclOpenParam *aclOpenParams = &pendingMsg->arg.aclOpenParams;

        if (aclOpenParams->appHandle == params->appHandle &&
            CsrBtAddrEq(&aclOpenParams->addrt, &params->addrt) &&
            cmAclGetTransport(aclOpenParams->flags) == cmAclGetTransport(params->flags))
        {
            return TRUE;
        }
    }

    return FALSE;
}

void CmAclOpenPendingMsgAdd(cmInstanceData_t *cmData,
                            CsrSchedQid appHandle,
                            CsrBtTypedAddr addrt,
                            CsrUint16 flags)
{
    /* This check is added in order to stop application from flooding our
     * pending queue by issuing back to back ACL open for the same bd address and transport. */
    if (!CsrBtCmAclOpenPendingMsgGet(cmData, appHandle, addrt, flags))
    {
        cmPendingMsg_t *pendingMsg;

        CsrPCmnListAddLast(cmData->pendingMsgs, sizeof(cmPendingMsg_t), pendingMsg);
        pendingMsg->type                        = CSR_BT_CM_PENDING_MSG_ACL_OPEN_PARAMS;
        pendingMsg->arg.aclOpenParams.appHandle = appHandle;
        pendingMsg->arg.aclOpenParams.addrt     = addrt;
        pendingMsg->arg.aclOpenParams.flags     = flags;
    }
    else
    {
        CSR_LOG_TEXT_WARNING((CsrBtCmLto, 0, "ACL Open already pending for address %04x,%02x,%06lx transport enum:TRANSPORT_T:%d appHandle %04x",
                             addrt.addr.nap, addrt.addr.uap, addrt.addr.lap,
                             cmAclGetTransport(flags),
                             appHandle));
    }
}

cmPendingMsg_t *CsrBtCmAclOpenPendingMsgGet(cmInstanceData_t *cmData,
                                            CsrSchedQid appHandle,
                                            CsrBtTypedAddr addrt,
                                            CsrUint16 flags)
{
    if (cmData->pendingMsgs)
    {
        cmAclOpenParam params;

        params.appHandle = appHandle;
        params.addrt = addrt;
        params.flags = flags;

        return ((cmPendingMsg_t *) CsrCmnListSearch((CsrCmnList_t *) cmData->pendingMsgs,
                                                    csrBtCmAclOpenPendingMsgCompare,
                                                    &params));
    }

    return NULL;
}

CsrUint8 returnAclConnectionElement(cmInstanceData_t *cmData,
                                    CsrBtDeviceAddr devAddr,
                                    aclTable **aclConnectionElement)
{
    CsrUintFast8 i;

    for (i = 0; i < NUM_OF_ACL_CONNECTION; i++)
    {
        if (CsrBtBdAddrEq(&devAddr, &(cmData->roleVar.aclVar[i].deviceAddr)))
        {
            *aclConnectionElement = &(cmData->roleVar.aclVar[i]);
            return (CsrUint8) i;
        }
    }

    *aclConnectionElement = NULL;
    return CM_ERROR;
}

void returnAclConnectionFromIndex(cmInstanceData_t *cmData,
                                  CsrUint8 index,
                                  aclTable **aclConnectionElement)
{
    *aclConnectionElement = &(cmData->roleVar.aclVar[index]);
}

void returnNextAvailableAclConnectionElement(cmInstanceData_t *cmData,
                                             aclTable **aclConnectionElement)
{
    CsrUintFast8 i;

    for (i = 0; i < NUM_OF_ACL_CONNECTION; i++)
    {
        if (!CsrBtBdAddrEqZero(&cmData->roleVar.aclVar[i].deviceAddr))
        {
            *aclConnectionElement = &(cmData->roleVar.aclVar[i]);
            return;
        }
    }

    *aclConnectionElement = NULL;
}

void returnNextAclConnectionElement(cmInstanceData_t *cmData,
                                    aclTable **aclConnectionElement)
{
    CsrUintFast8 i;

    for (i = 0; i < NUM_OF_ACL_CONNECTION; i++)
    {
        if (!CsrBtBdAddrEqZero(&(cmData->roleVar.aclVar[i].deviceAddr)) &&
            !cmData->roleVar.aclVar[i].roleChecked)
        {
            *aclConnectionElement = &(cmData->roleVar.aclVar[i]);
            return;
        }
    }
    *aclConnectionElement = NULL;
}

CsrUint8 returnNumOfAclConnection(cmInstanceData_t *cmData)
{
    CsrUintFast8 i, t = 0;

    for (i = 0; i < NUM_OF_ACL_CONNECTION; i++)
    {
        if (!CsrBtBdAddrEqZero(&(cmData->roleVar.aclVar[i].deviceAddr)))
        {
            t++;
        }
    }
    return ((CsrUint8) t);
}

/* The DM reports that the ACL has been closed, either via a
 * DM_ACL_CLOSED_IND or through a failed DM_ACL_CONN_HANDLE_IND */
void CsrBtCmAclCloseLegacyHandler(cmInstanceData_t *cmData,
                                  aclTable *aclConnectionElement,
                                  CsrBtDeviceAddr deviceAddr,
                                  CsrUint8 reason)
{
#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    cmL2caConnElement *connElement = CM_L2CA_ELEMENT_ACTIVE(cmData);

    CSR_UNUSED(aclConnectionElement);
    CSR_UNUSED(deviceAddr);
    /* Is the pending L2CAP connection waiting for the detach (legacy
     * pairing work around)? */
    if (connElement != NULL &&
        connElement->cmL2caConnInst != NULL &&
        connElement->cmL2caConnInst->state == CSR_BT_CM_L2CAP_STATE_LEGACY_DETACH &&
        connElement->cmL2caConnInst &&
        CsrBtBdAddrEq(&cmData->dmVar.detachAddr,
                      &connElement->cmL2caConnInst->deviceAddr))
    {
        if (!cmData->l2caVar.cancelConnect)
        {
            /* Connection was waiting for channel to get detached
             * - start the L2CAP connection */
            CsrBtCml2caAutoConnectSetup(cmData, connElement);
        }
        else
        {
            CsrBtCmL2caConnectCancelCleanup(cmData, connElement);
            CsrBtCmServiceManagerLocalQueueHandler(cmData);
        }
    }
#endif
    CSR_UNUSED(reason);
}

CsrBool CsrBtCmDmCancelPageOrDetach(cmInstanceData_t *cmData,
                                    CsrBtDeviceAddr deviceAddr)
{
    /* Return TRUE if the ACL connection is being detach otherwise FALSE */
    aclTable *aclConnectionElement;

    returnAclConnectionElement(cmData, deviceAddr, &aclConnectionElement);

    if (aclConnectionElement)
    {
        /* An ACL is present  */
        if (CsrBtCmReturnNumOfConnectionsToPeerDevice(cmData, deviceAddr) == 0 &&
            !CsrBtCmCheckSavedIncomingConnectMessages(cmData, deviceAddr))
        {
            /* The ACL is only used by the outgoing connection which
             * is being setup. Called detach in order to cancel the
             * outgoing connection */
            TYPED_BD_ADDR_T ad;

            ad.addr = deviceAddr;
            ad.type = CSR_BT_ADDR_PUBLIC;

            dm_acl_close_req(&ad,
                             DM_ACL_FLAG_FORCE,
                             HCI_ERROR_OETC_USER,
                             NULL);
            return (TRUE);
        }
    }
    else
    { /* No knowledge about any ACL connections. Try to cancel page scan */
        dm_hci_create_connection_cancel(&(deviceAddr), NULL);
    }

    return (FALSE);
}

void CsrBtCmDmAclConnStartIndHandler(cmInstanceData_t *cmData)
{
    DM_ACL_CONN_START_IND_T *prim = (DM_ACL_CONN_START_IND_T*) cmData->recvMsgP;
#ifdef CSR_BT_LE_ENABLE
    if (!(prim->flags & DM_ACL_FLAG_ULP))
#endif
    {
#ifndef CSR_BT_EXCLUDE_HCI_QOS_SETUP
        CsrBtCmDmHciQosSetupDirect(cmData,
                                   CSR_SCHED_QID_INVALID,
                                   &(prim->addrt.addr),
                                   FALSE);
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_INQUIRY_PAGE_STATE
        cmData->dmVar.pagingInProgress = TRUE;
        CsrBtCmPropgateEvent(cmData,
                             CsrBtCmPropagateInquiryPageEvents,
                             CSR_BT_CM_EVENT_MASK_SUBSCRIBE_INQUIRY_PAGE_STATE,
                             HCI_SUCCESS,
                             NULL,
                             NULL);

        CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "Paging Started"));
#endif
    }
}

#ifdef INSTALL_CONTEXT_TRANSFER
void CsrBtCmDmAclOpenedSuccessIndHandlerExt(cmInstanceData_t *cmData,
                                                   CsrBtDeviceAddr deviceAddr,
                                                   CsrBool incoming,
                                                   CsrUint24 cod)
{
    aclTable *aclConnectionElement;
    CsrBtDeviceAddr devAddr;

    CsrBtBdAddrZero(&devAddr);
    returnAclConnectionElement(cmData, devAddr, &aclConnectionElement);

    if (aclConnectionElement)
    {
        csrBtCmAclElemInit(cmData, aclConnectionElement, &deviceAddr);

        aclConnectionElement->cod = cod;
        aclConnectionElement->incoming = incoming;

        if (incoming)
        {
            aclConnectionElement->role = CSR_BT_SLAVE_ROLE;
        }
        else
        {
            aclConnectionElement->role = CSR_BT_MASTER_ROLE;
        }

#if defined(INSTALL_CM_DEVICE_UTILITY) && defined(INSTALL_CM_INTERNAL_LPM)
        aclConnectionElement->sniffSettings = cmData->dmVar.defaultSniffSettings;
#endif /* INSTALL_CM_DEVICE_UTILITY && INSTALL_CM_INTERNAL_LPM */

        CsrBtCmDmAclRoleVarsClear(CsrBtCmDmGetAclRoleVars(aclConnectionElement));
    }
}
#endif /* ifdef INSTALL_CONTEXT_TRANSFER */

void CsrBtCmDmAclOpenedIndHandler(cmInstanceData_t *cmData)
{
    DM_ACL_OPENED_IND_T *dmPrim = (DM_ACL_OPENED_IND_T*) cmData->recvMsgP;

#ifdef CSR_BT_INSTALL_CHANGE_ACL_PACKET_TYPE
    if (dmPrim->status == HCI_SUCCESS)
    {
        dm_hci_change_packet_type_acl(&(dmPrim->addrt.addr),
                                      CSR_BT_ACL_PACKET_TYPE);
    }
#endif /* CSR_BT_INSTALL_CHANGE_ACL_PACKET_TYPE */

#ifdef CSR_BT_LE_ENABLE
    if (dmPrim->flags & DM_ACL_FLAG_ULP)
    {
        CsrBtCmLeAclOpenedIndHandler(cmData);
    }
    else
#endif /* CSR_BT_LE_ENABLE */
    {
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_INQUIRY_PAGE_STATE
        if (cmData->dmVar.pagingInProgress)
        {
            cmData->dmVar.pagingInProgress = FALSE;
            CsrBtCmPropgateEvent(cmData,
                                 CsrBtCmPropagateInquiryPageEvents,
                                 CSR_BT_CM_EVENT_MASK_SUBSCRIBE_INQUIRY_PAGE_STATE,
                                 HCI_SUCCESS,
                                 NULL,
                                 NULL);

            CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "Paging Stopped"));
        }
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_INQUIRY_PAGE_STATE */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ACL_CONNECTION
        CsrBtCmPropgateEvent(cmData,
                             CsrBtCmPropgateAclConnectEvents,
                             CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ACL_CONNECTION,
                             dmPrim->status,
                             dmPrim,
                             &dmPrim->status);
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ACL_CONNECTION*/

        if (dmPrim->status == HCI_SUCCESS)
        {
            csrBtCmDmAclOpenedSuccessIndHandler(cmData,
                                                dmPrim->addrt.addr,
                                                (CsrBool) (dmPrim->flags & DM_ACL_FLAG_INCOMING ?
                                                                TRUE : FALSE),
                                                dmPrim->dev_class);
            /* Notify successfull ACL connection to device utility.*/
            (void)CmDuHandleAutomaticProcedure(cmData,
                                               CM_DU_AUTO_EVENT_ACL_CONNECTED,
                                               NULL,
                                               &dmPrim->addrt.addr);
        }
        else
        {
            if (CsrBtBdAddrEq(&dmPrim->addrt.addr, &cmData->dmVar.readNameDeviceAddr))
            {
                /* If DM queue is locked by remote name request, free it here.*/
                if (cmData->dmVar.lockMsg == CSR_BT_CM_READ_REMOTE_NAME_REQ)
                {
                    /* Notify failed ACL connection to device utility.*/
                    if (!CmDuHandleAutomaticProcedure(cmData,
                                                      CM_DU_AUTO_EVENT_CLEAR_CACHE,
                                                      NULL,
                                                      &dmPrim->addrt.addr))
                    {
                        CsrBtCmDmLocalQueueHandler();
                    }

                    CsrBtCmReadRemoteNameCfmSend(cmData,
                                                 cmData->dmVar.appHandle,
                                                 dmPrim->addrt.addr,
                                                 (CsrBtResultCode) HCI_ERROR_PAGE_TIMEOUT,
                                                 CSR_BT_SUPPLIER_HCI);
                }
            }
        }

#ifndef CSR_BT_EXCLUDE_HCI_QOS_SETUP
        if (!(dmPrim->flags & DM_ACL_FLAG_INCOMING))
        {
            /* this indication is as a result of outgoing connection initiation */
            CsrBtCmDmHciQosSetupDirect(cmData,
                                       CSR_SCHED_QID_INVALID,
                                       &(dmPrim->addrt.addr),
                                       TRUE);
        }
#endif /* !CSR_BT_EXCLUDE_HCI_QOS_SETUP */
    }
}

void CsrBtCmDmAclOpenCfmHandler(cmInstanceData_t *cmData)
{
    cmPendingMsg_t *pendingMsg = NULL;
    DM_ACL_OPEN_CFM_T *prim = (DM_ACL_OPEN_CFM_T*) cmData->recvMsgP;

    if(cmData->pendingMsgs)
    {
        pendingMsg = (cmPendingMsg_t *) CsrCmnListSearch((CsrCmnList_t *) cmData->pendingMsgs,
                                                         CmAclOpenPendingMsgCompareAddr,
                                                         &prim->addrt);

        if (pendingMsg)
        {
            CsrSchedQid appHandle = pendingMsg->arg.aclOpenParams.appHandle;

            if (appHandle == CSR_BT_CM_IFACEQUEUE)
            { /* Requested by CM */
                CsrBtCmDmAclOpenInSdcCloseStateHandler(cmData, &prim->addrt.addr, prim->success);
            }
            else
            { /* Requested by application */
                aclTable *aclConnectionElement;

                returnAclConnectionElement(cmData, prim->addrt.addr, &aclConnectionElement);

                if (aclConnectionElement)
                { /* Just add the stakeholder for apps's requested ACL connection */
                    aclConnectionElement->aclRequestedByApp = TRUE;
                }
                /* Inform application about ACL confirmation */
                csrBtCmAclOpenCfmSend(appHandle, prim->addrt, prim->success);
            }

            CsrPCmnListRemove(cmData->pendingMsgs, (CsrCmnListElm_t* ) pendingMsg);
        }
    }
}

void CsrBtCmDmAclCloseIndHandler(cmInstanceData_t *cmData)
{
    DM_ACL_CLOSED_IND_T *dmPrim = (DM_ACL_CLOSED_IND_T*) cmData->recvMsgP;

#ifdef CSR_BT_LE_ENABLE
    if (dmPrim->flags & DM_ACL_FLAG_ULP)
    {
        CsrBtCmLeAclClosedIndHandler(cmData);
    }
    else
#endif
    {
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ACL_CONNECTION
        CsrBtCmPropgateEvent(cmData,
                             CsrBtCmPropgateAclDisconnectEvents,
                             CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ACL_CONNECTION,
                             HCI_SUCCESS,
                             dmPrim,
                             &dmPrim->reason);
#endif

        csrBtCmDmAclCloseSuccessIndHandler(cmData,
                                           dmPrim->addrt.addr,
                                           dmPrim->reason);
    }
}

void CsrBtCmDmAclCloseCfmHandler(cmInstanceData_t *cmData)
{
    DM_ACL_CLOSE_CFM_T *cfm = (DM_ACL_CLOSE_CFM_T*) cmData->recvMsgP;

    /* Handle only if disconnection was forced */
    if (cfm->flags & DM_ACL_FLAG_FORCE)
    {
        /* Application will get acl close cfm only when it has set force in acl close req flags */
        if (cmData->dmVar.appHandle != CSR_BT_CM_IFACEQUEUE)
        {
            aclTable *aclConnectionElement;

            returnAclConnectionElement(cmData, cfm->addrt.addr, &aclConnectionElement);

            if (aclConnectionElement)
            {
                aclConnectionElement->aclRequestedByApp = FALSE;
            }

            /* First condition is to make sure that the request has come from application. Then only we will send the ACL close confirm to application. 
             * Second condition is for a scenario where there is a race between upstream ACL confirm for an application initated ACL close
             * and ACL close initiated by synergy. After sending ACL confirm for an ACL close requested by application, we might still have 
             * CSR_BT_CM_ACL_CLOSE_REQ in our cmData->dmVar.lockMsg variable but corresponding app handle would be Invalid.
             */
            if((cmData->dmVar.lockMsg == CSR_BT_CM_ACL_CLOSE_REQ) &&
               (cmData->dmVar.appHandle != CSR_SCHED_QID_INVALID))
            {
                csrBtCmAclCloseCfmSend(cmData, cfm->addrt, cfm->status, cfm->flags);
            }
        }

        CSR_BT_CM_STATE_CHANGE(cmData->dmVar.state, CSR_BT_CM_DM_STATE_NULL);

        if (cmData->dmVar.lockMsg == CSR_BT_CM_ACL_CLOSE_REQ)
        {
            /* if CM or application has initiated a acl close request the  
               handle needs to be reset now that the close is processed. */
            cmData->dmVar.appHandle = CSR_SCHED_QID_INVALID;
            CsrBtCmDmLocalQueueHandler();
        }
    }
}

void CsrBtCmAclOpenReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmAclOpenReq *cmPrim = (CsrBtCmAclOpenReq*) cmData->recvMsgP;

    /* If ACL already exists, this request adds additional stakeholder for the
     * ACL to prevent disconnection in absence of other stakeholders (SLCs). */
    if (CsrBtCmReturnNumOfConnectionsToPeerDevice(cmData,
                                                  cmPrim->address.addr))
    {
        if (cmPrim->appHandle == CSR_BT_CM_IFACEQUEUE)
        {
            CsrBtCmDmAclOpenInSdcCloseStateHandler(cmData, &cmPrim->address.addr, TRUE);
            return;
        }
    }
    else
    {
        /* There are no ACLs present, check if cache parameter automatic procedure needs to run.*/
        if (CmDuHandleAutomaticProcedure(cmData,
                                          CM_DU_AUTO_EVENT_ACL_OPEN,
                                          (void *)cmPrim->address.type,
                                          &cmPrim->address.addr))
        {
            /* Handling will be taken care by device utility.*/
            return;
        }
    }

    /* Store ACL open parameters in order to use it when the respective confirmation comes.
     * If the message is added it will be removed on either of the following places based on the
     * given condition:
     * A. Condition: if DM_ACL_OPEN_CFM is received, remove it on CsrBtCmDmAclOpenCfmHandler.
     * B. Condition: if CSR_BT_CM_ACL_CLOSE_REQ is received before DM_ACL_OPEN_CFM, remove it on
     *               CsrBtCmAclCloseReqHandler based on the flags provided from application.
     */
    CmAclOpenPendingMsgAdd(cmData,
                           cmPrim->appHandle,
                           cmPrim->address,
                           cmPrim->flags);
    dm_acl_open_req(&cmPrim->address, cmPrim->flags, NULL);
}

static CsrBool cmAclOpenPendingMsgCompareTypeAclOpen(CsrCmnListElm_t *elem, void *data)
{
    cmPendingMsg_t *pendingMsg = (cmPendingMsg_t *)elem;

    CSR_UNUSED(data);
    return pendingMsg->type == CSR_BT_CM_PENDING_MSG_ACL_OPEN_PARAMS;
}

static CsrBool cmAclOpenPendingMsgCompareAddrAndTransport(CsrCmnListElm_t *elem, void *data)
{
    cmPendingMsg_t *pendingMsg = (cmPendingMsg_t *)elem;
    cmInstanceData_t *cmData = (cmInstanceData_t *)data;
    CsrBtCmAclCloseReq *req = (CsrBtCmAclCloseReq*) cmData->recvMsgP;

    if (pendingMsg->type == CSR_BT_CM_PENDING_MSG_ACL_OPEN_PARAMS)
    {
        cmAclOpenParam *aclOpenParams = &pendingMsg->arg.aclOpenParams;

        if (CsrBtAddrEq(&aclOpenParams->addrt, &req->address) &&
            cmAclGetTransport(aclOpenParams->flags) == cmAclGetTransport(req->flags))
        {
            return TRUE;
        }
    }

    return FALSE;
}

static void cmAclOpenRemovePendingAclOpenRequests(cmInstanceData_t *cmData)
{
    CsrBtCmAclCloseReq *req = (CsrBtCmAclCloseReq*) cmData->recvMsgP;

    if (cmData->pendingMsgs)
    {
        if ((req->flags & DM_ACL_FLAG_ALL) && CsrBtBdAddrEqZero(&req->address.addr))
        {
            /* Any pending ACL open request in the queue shall be cleared as
             * application wants to release all ACL connections. */
            CsrPCmnListIterateAllowRemove(cmData->pendingMsgs,
                                          cmAclOpenPendingMsgCompareTypeAclOpen,
                                          NULL);
        }
        else if (req->flags & DM_ACL_FLAG_FORCE)
        {
            /* Any pending ACL open request for the given transport shall be cleared,
             * as application wants to remove this ACL connection irrespective of the
             * current stakeholders. */
            CsrPCmnListIterateAllowRemove(cmData->pendingMsgs,
                                          cmAclOpenPendingMsgCompareAddrAndTransport,
                                          cmData);
        }
        else
        {
            /* Application wants to release its lock on this transport, remove any pending open request
             * for this bd address and transport by this application. */
            cmPendingMsg_t *pendingMsg = CsrBtCmAclOpenPendingMsgGet(cmData,
                                                                     req->appHandle,
                                                                     req->address,
                                                                     req->flags);
            if (pendingMsg)
            {
                CsrPCmnListRemove(cmData->pendingMsgs, (CsrCmnListElm_t* ) pendingMsg);
            }
        }
    }
}

void CsrBtCmAclCloseReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmAclCloseReq *prim = (CsrBtCmAclCloseReq *) cmData->recvMsgP;

    cmData->dmVar.appHandle = prim->appHandle;

    /* As per bluestack behavior, if for a given DM_ACL_OPEN_REQ, DM_ACL_CLOSE_REQ is 
     * sent without receiving DM_ACL_OPEN_CFM then we don't get DM_ACL_OPEN_CFM 
     * for our DM_ACL_OPEN_REQ.
     *
     * Following sequence shows normal and early close sequence:
     *
     * Normal sequence:
     * CM ---- DM_ACL_OPEN_REQ ---> Bluestack
     * CM <--- DM_ACL_OPENED_IND -- Bluestack
     * CM <--- DM_ACL_OPEN_CFM ---- Bluestack
     * CM ---- DM_ACL_CLOSE_REQ --> Bluestack
     * CM <--- DM_ACL_CLOSE_CFM --- Bluestack
     *
     * Early close sequence:
     * CM --- DM_ACL_OPEN_REQ ----> Bluestack
     * CM --- DM_ACL_CLOSE_REQ ---> Bluestack
     * CM <-- DM_ACL_OPENED_IND --- Bluestack
     * CM --- DM_ACL_CLOSE_CFM ---> Bluestack
     *
     * DM_ACL_OPEN_REQ contents are cached inside pending message list which gets cleared
     * while handling DM_ACL_OPEN_CFM. If DM_ACL_OPEN_CFM is not received then this
     * needs to be cleared here.
    */
    cmAclOpenRemovePendingAclOpenRequests(cmData);

    dm_acl_close_req(&prim->address,
                     prim->flags,
                     prim->reason,
                     NULL);

    /* Unblock the DM queue immediately if application has not set force in acl flags */
    if ((prim->flags & DM_ACL_FLAG_FORCE) == 0x0000)
        CsrBtCmDmLocalQueueHandler();
}


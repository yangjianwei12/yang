/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE

#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_l2cap_conftab.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_private_lib.h"
#endif
#include "bkeyval.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_events_handler.h"

#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_cm_streams_handler.h"
#endif

#ifndef EXCLUDE_CSR_BT_CM_LEGACY_PAIRING_DETACH
/* Called by the 'cache parameter update' or 'lp settings update'
 * handlers immediately before the L2CAP auto-connect-req gets
 * sent. The DM queue is already held at this point.  We return TRUE
 * if we detached, otherwise FALSE. */
CsrBool CsrBtCmL2caCheckLegacyDetach(cmInstanceData_t* cmData, cmL2caConnInstType* l2caConn)
{
    CsrUint8 numSlc = 0;
    aclTable* aclElement = NULL;

    numSlc = CsrBtCmReturnNumOfConnectionsToPeerDevice(cmData, l2caConn->deviceAddr);
    returnAclConnectionElement(cmData, l2caConn->deviceAddr, &aclElement);

    if (aclElement && !numSlc)
    { /* ACL exists but no SLC estabilished */
        if ((l2caConn->secLevel & SECL_OUT_AUTHENTICATION) ||
            (l2caConn->secLevel & SECL_OUT_ENCRYPTION) ||
            (l2caConn->secLevel & SECL_OUT_AUTHORISATION))
        { /* Security requested by profile for this connection */
            if (!CsrBtCmCheckSavedIncomingConnectMessages(cmData,
                l2caConn->deviceAddr))
            { /* No pending SLC connect indication for this address */
              /* Send ACL close - DM queue already locked, so send to DM*/
              /* directly */
                TYPED_BD_ADDR_T ad;

                CSR_BT_CM_STATE_CHANGE(l2caConn->state,
                    CSR_BT_CM_L2CAP_STATE_LEGACY_DETACH);
                cmData->dmVar.appHandle = CSR_BT_CM_IFACEQUEUE;
                CsrBtBdAddrCopy(&cmData->dmVar.detachAddr,
                    &l2caConn->deviceAddr);

                ad.addr = l2caConn->deviceAddr;
                ad.type = CSR_BT_ADDR_PUBLIC;
                dm_acl_close_req(&ad,
                    DM_ACL_FLAG_FORCE,
                    HCI_ERROR_OETC_USER, NULL);
                return TRUE;
            }
        }
    }
    return FALSE;
}
#endif /* !EXCLUDE_CSR_BT_CM_LEGACY_PAIRING_DETACH */

static cmL2caConnElement * returnCancelL2caConnectIndex(cmInstanceData_t *cmData,
                                                        psm_t localPsm, CsrSchedQid phandle,
                                                        CsrBtDeviceAddr deviceAddr)
{
    if (cmData->smVar.smInProgress &&
        ((cmData->smVar.smMsgTypeInProgress == CSR_BT_CM_L2CA_CONNECT_REQ) ||
         (cmData->smVar.smMsgTypeInProgress == CM_L2CA_TP_CONNECT_REQ))
        )
    {
        cmL2caConnElement *currentElem;

        for (currentElem = CM_L2CA_GET_FIRST(cmData->l2caVar.connList);
             currentElem;
             currentElem = currentElem->next)
        {
            /* Search through the l2ca list */
            if (currentElem->cmL2caConnInst)
            {
                if(currentElem->cmL2caConnInst->psm == localPsm)
                {
                    if (currentElem->cmL2caConnInst->appHandle == phandle)
                    {
                        if (CsrBtBdAddrEq(&(currentElem->cmL2caConnInst->deviceAddr), &(deviceAddr)))
                        {
                            if (currentElem->cmL2caConnInst->state == CSR_BT_CM_L2CAP_STATE_CONNECT ||
                                currentElem->cmL2caConnInst->state == CSR_BT_CM_L2CAP_STATE_CONNECT_INIT)
                            {
                                return (currentElem);
                            }
                        }
                    }
                }
            }
        }
    }
    return NULL;
}

static void csrBtCmL2caConnectCfmMsgSend(CsrSchedQid phandle,
                                         CsrBtConnId btConnId,
                                         psm_t psm,
                                         l2ca_mtu_t mtu,
                                         l2ca_mtu_t localMtu,
                                         CsrBtDeviceAddr deviceAddr,
                                         CsrBtResultCode resultCode,
                                         CsrBtSupplier resultSupplier,
                                         CsrUint16     context)
{
    CsrBtCmL2caConnectCfm * cmPrim = (CsrBtCmL2caConnectCfm *) CsrPmemAlloc(sizeof(CsrBtCmL2caConnectCfm));
    cmPrim->type                   = CSR_BT_CM_L2CA_CONNECT_CFM;
    cmPrim->btConnId               = btConnId;
    cmPrim->localPsm               = psm;
    cmPrim->mtu                    = mtu;
    cmPrim->localMtu               = localMtu;
    cmPrim->deviceAddr             = deviceAddr;
    cmPrim->resultCode             = resultCode;
    cmPrim->resultSupplier         = resultSupplier;
    cmPrim->context                = context; 
    CsrBtCmPutMessage(phandle, cmPrim);
}

static void cmEncodeTpAddr(CsrBtDeviceAddr deviceAddr,
                           CsrBtAddressType addressType,
                           CsrBtTransportType transportType,
                           CsrBtTpdAddrT *tpAddr)
{
    tpAddr->addrt.addr = deviceAddr;
    tpAddr->addrt.type = addressType;
    tpAddr->tp_type    = transportType;
}

static void cmDecodeTpAddr(cmL2caConnInstType *l2caConnInst, CsrBtTpdAddrT *tpAddr)
{
    l2caConnInst->deviceAddr    = tpAddr->addrt.addr;
    l2caConnInst->addressType   = tpAddr->addrt.type;
    l2caConnInst->transportType = tpAddr->tp_type;
}

void CmL2caConvertConnectCfmToTpPrim(cmInstanceData_t *cmData)
{
    L2CA_AUTO_CONNECT_CFM_T *prim = (L2CA_AUTO_CONNECT_CFM_T*)cmData->recvMsgP;
    L2CA_AUTO_TP_CONNECT_CFM_T *tpPrim = (L2CA_AUTO_TP_CONNECT_CFM_T *)CsrPmemAlloc(sizeof(L2CA_AUTO_TP_CONNECT_CFM_T));
    tpPrim->type = prim->type;
    tpPrim->phandle = prim->phandle;
    tpPrim->cid = prim->cid;
    tpPrim->reg_ctx = prim->reg_ctx;
    tpPrim->con_ctx = prim->con_ctx;
    tpPrim->tp_addrt.addrt.addr = prim->bd_addr;
    tpPrim->tp_addrt.addrt.type = TBDADDR_PUBLIC;
    tpPrim->tp_addrt.tp_type = BREDR_ACL;
    tpPrim->psm_local = prim->psm_local;
    tpPrim->result = prim->result;
    SynMemCpyS(&tpPrim->config,sizeof(L2CA_TP_CONFIG_T),&prim->config,sizeof(L2CA_CONFIG_T));
    tpPrim->ext_feats = prim->ext_feats;
    tpPrim->flags = 0;
    tpPrim->config_ext_length = 0;
    tpPrim->config_ext = NULL;

    cmData->recvMsgP = tpPrim;
}

static CsrBool cmL2caConvertConnectIndToTpPrim(cmInstanceData_t *cmData)
{
    L2CA_AUTO_CONNECT_IND_T *prim = (L2CA_AUTO_CONNECT_IND_T*)cmData->recvMsgP;

    if (prim->type == L2CA_AUTO_CONNECT_IND)
    {
        L2CA_AUTO_TP_CONNECT_IND_T *tpPrim = (L2CA_AUTO_TP_CONNECT_IND_T *)CsrPmemAlloc(sizeof(L2CA_AUTO_TP_CONNECT_IND_T));

        tpPrim->type                = prim->type;
        tpPrim->phandle             = prim->phandle;
        tpPrim->cid                 = prim->cid;
        tpPrim->reg_ctx             = prim->reg_ctx;
        tpPrim->identifier          = prim->identifier;
        tpPrim->tp_addrt.addrt.addr = prim->bd_addr;
        tpPrim->tp_addrt.addrt.type = TBDADDR_PUBLIC;
        tpPrim->tp_addrt.tp_type    = BREDR_ACL;
        tpPrim->psm_local           = prim->psm_local;
        tpPrim->local_control       = prim->local_control;
        tpPrim->flags               = 0;
        tpPrim->conftab_length      = 0;
        tpPrim->conftab             = NULL;

        cmData->recvMsgP = tpPrim;

        return (TRUE);
    }

    return (FALSE);
}

static cmL2caConnElement * createL2caConnElement(cmInstanceData_t *cmData,
                                          CsrSchedQid phandle,
                                          CsrUint16 context,
                                          CsrBtTpdAddrT tpdAddrT,
                                          psm_t localPsm,
                                          psm_t remotePsm,
                                          CsrBtConnId btConnId,
                                          dm_security_level_t secLevel,
                                          CsrUint16 conftabCount,
                                          CsrUint16 *conftab)
{
    cmL2caConnElement *theElement = NULL;
    cmL2caConnInstType *l2caConnection = NULL;

    theElement = (cmL2caConnElement *) CsrCmnListElementAddLast(&(cmData->l2caVar.connList), sizeof(cmL2caConnElement));
    l2caConnection = theElement->cmL2caConnInst;

    theElement->elementId           = cmData->elementCounter;

    l2caConnection->appHandle       = phandle;
    l2caConnection->context         = context;
    l2caConnection->psm             = localPsm;
    l2caConnection->remotePsm       = remotePsm;

    cmDecodeTpAddr(l2caConnection, &tpdAddrT);

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
    l2caConnection->logicalChannelControl = TRUE;
#endif

#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 && defined(CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS)
    l2caConnection->ssrAccepted     = FALSE;
#endif

    l2caConnection->btConnId        = btConnId;
#ifndef EXCLUDE_CSR_BT_CM_LEGACY_PAIRING_DETACH
    l2caConnection->secLevel        = secLevel;
#endif
    l2caConnection->classOfDevice   = 0;

    l2caConnection->incomingMtu     = 0;
    l2caConnection->conftabIter.block    = conftab;
    l2caConnection->conftabIter.size     = conftabCount;
    l2caConnection->conftabIter.iterator = 0;
#ifdef CSR_AMP_ENABLE
    l2caConnection->controller      = CSR_BT_AMP_CONTROLLER_BREDR;
#endif /* CSR_AMP_ENABLE */

    return theElement;
}

static void cmL2caTpConnectCfmMsgSend(CsrSchedQid phandle,
                                      CsrBtConnId btConnId,
                                      psm_t psm,
                                      l2ca_mtu_t mtu,
                                      l2ca_mtu_t localMtu,
                                      CsrBtDeviceAddr deviceAddr,
                                      CsrBtAddressType addressType,
                                      CsrBtTransportType transportType,
                                      CsrBtResultCode resultCode,
                                      CsrBtSupplier resultSupplier,
                                      CsrUint16     context)
{
    CsrBtCmL2caTpConnectCfm * cmPrim = (CsrBtCmL2caTpConnectCfm *) CsrPmemAlloc(sizeof(CsrBtCmL2caTpConnectCfm));
    cmPrim->type                   = CM_L2CA_TP_CONNECT_CFM;
    cmPrim->btConnId               = btConnId;
    cmPrim->localPsm               = psm;
    cmPrim->mtu                    = mtu;
    cmPrim->localMtu               = localMtu;
    cmPrim->resultCode             = resultCode;
    cmPrim->resultSupplier         = resultSupplier;
    cmPrim->context                = context;

    cmEncodeTpAddr(deviceAddr,
                   addressType,
                   transportType,
                   &cmPrim->tpdAddrT);

    CsrBtCmPutMessage(phandle, cmPrim);
}


void CsrBtCmL2caConnectCfmMsgHandler(cmInstanceData_t *cmData, cmL2caConnElement *theElement,
                                     CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    /* Send a CSR_BT_CM_L2CA_CONNECT_CFM/CM_L2CA_TP_CONNECT_CFM signal to the 
     * application, and restore the local service manager queue */
    cmL2caConnInstType *l2CaConnection = theElement->cmL2caConnInst;

    CsrBtCmDmSmClearRebondData(cmData);
    CsrBtCmDmUpdateAndClearCachedParamReqSend(l2CaConnection->deviceAddr);

    if (theElement->useTpPrim)
    {
        cmL2caTpConnectCfmMsgSend(l2CaConnection->appHandle,
                              l2CaConnection->btConnId,
                              l2CaConnection->psm,
                              l2CaConnection->outgoingMtu,
                              l2CaConnection->incomingMtu,
                              l2CaConnection->deviceAddr,
                              l2CaConnection->addressType,
                              l2CaConnection->transportType,
                              resultCode,
                              resultSupplier,
                              l2CaConnection->context);
    }
    else
    {
        csrBtCmL2caConnectCfmMsgSend(l2CaConnection->appHandle,
                                     l2CaConnection->btConnId,
                                     l2CaConnection->psm,
                                     l2CaConnection->outgoingMtu,
                                     l2CaConnection->incomingMtu,
                                     l2CaConnection->deviceAddr,
                                     resultCode, resultSupplier,
                                     l2CaConnection->context);
    }

    if (resultCode != CSR_BT_RESULT_CODE_CM_SUCCESS || resultSupplier != CSR_BT_SUPPLIER_CM)
    {
        CsrUint8 numOutgoing;
        CsrUint8 numIncoming;

        numberOfSecurityRegister(cmData,
                                 l2CaConnection->psm,
                                 l2CaConnection->deviceAddr,
                                 &numOutgoing,
                                 &numIncoming);
        if (numOutgoing == 1)
        {
            /* Unregister The Outgoing security setting */
            dm_sm_unregister_outgoing_req(CSR_BT_CM_IFACEQUEUE,
                                          0, /* context */
                                          &l2CaConnection->deviceAddr,
                                          (l2CaConnection->transportType == BREDR_ACL) ? SEC_PROTOCOL_L2CAP : SEC_PROTOCOL_LE_L2CAP,
                                          l2CaConnection->psm,
                                          NULL);
        }
        else
        {
            /* There is more that need this security setting */
            ;
        }
        CsrBtCmL2capClearL2capTableIndex(cmData, &(theElement->cmL2caConnInst));
    }
}

static void csrBtCmL2caConnectAcceptCfmMsgSend(CsrSchedQid appHandle, CsrBtConnId  btConnId,
                                               psm_t localPsm, psm_t remotePsm, l2ca_mtu_t mtu,
                                               l2ca_mtu_t localMtu, CsrBtDeviceAddr deviceAddr,
                                               CsrBtResultCode resultCode, CsrBtSupplier resultSupplier,
                                               CsrUint16 context)
{
    CsrBtCmL2caConnectAcceptCfm * cmPrim;

    cmPrim    = (CsrBtCmL2caConnectAcceptCfm *)CsrPmemAlloc(sizeof(CsrBtCmL2caConnectAcceptCfm));
    cmPrim->type            = CSR_BT_CM_L2CA_CONNECT_ACCEPT_CFM;
    cmPrim->btConnId        = btConnId;
    cmPrim->deviceAddr      = deviceAddr;
    cmPrim->localPsm        = localPsm;
    cmPrim->remotePsm       = remotePsm;
    cmPrim->mtu             = mtu;
    cmPrim->localMtu    = localMtu;
    cmPrim->resultCode      = resultCode;
    cmPrim->resultSupplier  = resultSupplier;
    cmPrim->context         = context;
    CsrBtCmPutMessage(appHandle, cmPrim);
}

static void cmL2caTpConnectAcceptCfmMsgSend(CsrSchedQid appHandle,
                                            CsrBtConnId  btConnId,
                                            psm_t localPsm,
                                            psm_t remotePsm,
                                            l2ca_mtu_t mtu,
                                            l2ca_mtu_t localMtu,
                                            CsrBtDeviceAddr deviceAddr,
                                            CsrBtAddressType addressType,
                                            CsrBtTransportType transportType,
                                            CsrBtResultCode resultCode,
                                            CsrBtSupplier resultSupplier,
                                            CsrUint16 context)
{
    CmL2caTpConnectAcceptCfm * cmPrim;

    cmPrim    = (CmL2caTpConnectAcceptCfm *)CsrPmemAlloc(sizeof(CmL2caTpConnectAcceptCfm));
    cmPrim->type                    = CM_L2CA_TP_CONNECT_ACCEPT_CFM;
    cmPrim->btConnId                = btConnId;
    cmPrim->localPsm                = localPsm;
    cmPrim->remotePsm               = remotePsm;
    cmPrim->mtu                     = mtu;
    cmPrim->localMtu                = localMtu;
    cmPrim->resultCode              = resultCode;
    cmPrim->resultSupplier          = resultSupplier;
    cmPrim->context                 = context;

    cmEncodeTpAddr(deviceAddr,
                   addressType,
                   transportType,
                   &cmPrim->tpdAddrT);

    CsrBtCmPutMessage(appHandle, cmPrim);
}

void CsrBtCmL2caConnectAcceptCfmHandler(cmInstanceData_t *cmData,
                                        cmL2caConnElement * theElement,
                                        CsrBtResultCode resultCode,
                                        CsrBtSupplier resultSupplier)
{
    /* Send a CM_L2CA_TP_CONNECT_ACCEPT_CFM or CSR_BT_CM_L2CA_ACCEPT_CONNECT_CFM signal 
     * to the application and restore the local service manager queue */
    cmL2caConnInstType *theLink = theElement->cmL2caConnInst;

    if (theElement->useTpPrim)
    {
        cmL2caTpConnectAcceptCfmMsgSend(theLink->appHandle,
                                        theLink->btConnId,
                                        theLink->psm,
                                        theLink->remotePsm,
                                        theLink->outgoingMtu,
                                        theLink->incomingMtu,
                                        theLink->deviceAddr,
                                        theLink->addressType,
                                        theLink->transportType,
                                        resultCode,
                                        resultSupplier,
                                        theLink->context);
    }
    else
    {
        csrBtCmL2caConnectAcceptCfmMsgSend(theLink->appHandle,
                                           theLink->btConnId,
                                           theLink->psm,
                                           theLink->remotePsm,
                                           theLink->outgoingMtu,
                                           theLink->incomingMtu,
                                           theLink->deviceAddr,
                                           resultCode,
                                           resultSupplier,
                                           theLink->context);
    }

    if (resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && resultSupplier == CSR_BT_SUPPLIER_CM)
    {
#ifndef CSR_STREAMS_ENABLE
        /* Start processing */
        CsrBtCmL2caDataStart(cmData, theLink);
#endif
    }
    else
    {
        /* No need to deregister security if the connection element was created because of
         * response from application, as in those cases the security was never registered. */
        if (!theElement->app_controlled)
        {
            CsrUint8 numOutgoing;
            CsrUint8 numIncoming;

            numberOfSecurityRegister(cmData, theLink->psm, theLink->deviceAddr, &numOutgoing, &numIncoming);
            if (numIncoming == 1)
            {
                /* Unregister The Incoming security setting */
                CsrBtScDeregisterReqSend((theLink->transportType == BREDR_ACL) ? SEC_PROTOCOL_L2CAP : SEC_PROTOCOL_LE_L2CAP,
                                         theLink->psm);
            }
            else
            {
                /* There is more that need this security setting */
                ;
            }
        }

        CsrBtCmL2capClearL2capTableIndex(cmData, &(theElement->cmL2caConnInst));
    }
}

void CsrBtCmL2caCancelConnectAcceptCfmMsgSend(cmInstanceData_t *cmData, CsrSchedQid appHandle, psm_t psm,
                                              CsrBtResultCode resultCode, CsrBtSupplier resultSupplier,
                                              CsrUint16 context)
{
    /* Send a CSR_BT_CM_L2CA_CANCEL_CONNECT_ACCEPT_CFM signal to the
     * application, and restore the local service manager queue */
    CsrBtCmL2caCancelConnectAcceptCfm    * cmPrim;

    cmPrim    = (CsrBtCmL2caCancelConnectAcceptCfm *)CsrPmemAlloc(sizeof(CsrBtCmL2caCancelConnectAcceptCfm));

    cmPrim->type            = CSR_BT_CM_L2CA_CANCEL_CONNECT_ACCEPT_CFM;
    cmPrim->resultCode      = resultCode;
    cmPrim->resultSupplier  = resultSupplier;
    cmPrim->localPsm        = psm;
    cmPrim->context         = context;
    CsrBtCmPutMessage(appHandle, cmPrim);
    CSR_UNUSED(cmData);
}

void CsrBtCml2caAutoConnectSetup(cmInstanceData_t *cmData, cmL2caConnElement *l2caConnElement)
{
    cmL2caConnInstType *l2CaConnection = l2caConnElement->cmL2caConnInst;

    CSR_BT_CM_STATE_CHANGE(cmData->l2caVar.connectState, CM_L2CA_CONNECT);
    CSR_BT_CM_STATE_CHANGE(l2CaConnection->state, CSR_BT_CM_L2CAP_STATE_CONNECT);

    if (l2caConnElement->useTpPrim)
    {
        CsrBtTpdAddrT tpAddr;

        cmEncodeTpAddr(l2CaConnection->deviceAddr,
                       l2CaConnection->addressType,
                       l2CaConnection->transportType,
                       &tpAddr);

        L2CA_AutoTpConnectReq(L2CA_CID_INVALID, /* L2CA_CID_INVALID =create new*/
                              l2CaConnection->psm,
                              &tpAddr,
                              l2CaConnection->remotePsm,
                              CM_L2CA_CONNECT_INPROGRESS_CTX,
                              CSR_BT_AMP_CONTROLLER_BREDR,
                              CSR_BT_AMP_CONTROLLER_BREDR,
                              l2CaConnection->conftabIter.size,
                              CsrMemDup(l2CaConnection->conftabIter.block,
                                        l2CaConnection->conftabIter.size*sizeof(CsrUint16)));
    }
    else
    {
        L2CA_AutoConnectReq(L2CA_CID_INVALID, /* L2CA_CID_INVALID =create new*/
                            l2CaConnection->psm,
                            &l2CaConnection->deviceAddr,
                            l2CaConnection->remotePsm,
                            CM_L2CA_CONNECT_INPROGRESS_CTX,
                            CSR_BT_AMP_CONTROLLER_BREDR,
                            CSR_BT_AMP_CONTROLLER_BREDR,
                            l2CaConnection->conftabIter.size,
                            CsrMemDup(l2CaConnection->conftabIter.block,
                                      l2CaConnection->conftabIter.size*sizeof(CsrUint16)));
    }
}

static void csrBtCmL2caOptimalMtuMps(cmInstanceData_t *cmData,
                                     cmL2caConnInstType *l2capConn,
                                     CsrUint16 *mtu,
                                     CsrUint16 *mps,
                                     CsrUint16 *outMtu)
{
    CsrUint16 mode = 0;
    CsrUint16 bredr = 0;
#ifdef CSR_AMP_ENABLE
    CsrBool align = FALSE;
#endif
    aclTable *aclConnectionElement = NULL;

    /* Rules for determining incoming MTU:
     * RFCOMM build          -> use RFCOMM optimal value
     * basic mode            -> use BR/EDR optimal value
     * flow mode with AMP    -> use AMP max, maximise MTU
     * flow mode without AMP -> use BR/EDR optimal, maximise MTU
     */
#ifndef CSR_TARGET_PRODUCT_VM
    if (cmData->rfcBuild)
    {
        *mtu = CSR_BT_RFC_BUILD_L2CAP_MAX_FRAME_SIZE;
        *mps = CSR_BT_RFC_BUILD_L2CAP_MAX_FRAME_SIZE;
    }
    else
#endif
    {
        /* Figure out whether we're running BR or EDR */
        if (HCI_FEATURE_IS_SUPPORTED(LMP_FEATURES_5SLOT_MR_BIT, cmData->dmVar.lmpSuppFeatures))
        {/* The local device supports EDR */
            /* At this point the remote BD Address is not 0. This function is called either at outgoing connection request, or
               when the ACL link is already established at incoming connection, so just try to find the ACL instance */
            returnAclConnectionElement(cmData, l2capConn->deviceAddr, &aclConnectionElement);

            if (aclConnectionElement && 
               !HCI_FEATURE_IS_SUPPORTED(LMP_FEATURES_5SLOT_MR_BIT, aclConnectionElement->remoteFeatures))
            {/* There is an ACL link and the remote device does not support EDR; use BR */
                bredr = CSR_BT_HCI_BUILD_L2CAP_NON_EDR_MAX_FRAME_SIZE;
            }
            else
            {/* The remote supports EDR too */
                bredr = CSR_BT_HCI_BUILD_L2CAP_EDR_MAX_FRAME_SIZE;
            }
        }
        else
        {/* The local device does not support EDR */
            bredr = CSR_BT_HCI_BUILD_L2CAP_NON_EDR_MAX_FRAME_SIZE;
        }

        if(!BKV_Scan16Single(&l2capConn->conftabIter,
                             L2CA_AUTOPT_FLOW_MODE, &mode) || (mode == 0))
        {
            /* Basic mode */
            *mtu = bredr;
            *mps = bredr;
        }
        else
        {
#ifdef CSR_AMP_ENABLE
            /* Flow mode with AMP. L2CAPs flow control mode will
             * automatically adjust the MPS to whatever controller we
             * are using, so optimising for 802.11/UWB should not
             * matter -- except that we need to set this to the
             * highest possible packet size we will ever encounter */
            align = TRUE;
            *mtu = CSR_BT_MAXIMUM_AUTOMATIC_MTU;
            if (CSR_BT_MAXIMUM_AMP_PACKET_SIZE > bredr)
            {
                *mps = CSR_BT_MAXIMUM_AMP_PACKET_SIZE;
            }
            else
            {
               *mps = bredr;
            }
#else
            /* Flow mode over BR/EDR */
            *mtu = CSR_BT_MAXIMUM_AUTOMATIC_MTU;
            *mps = bredr;
#endif
        }

        /* Cap sizes */
        *mtu = CSRMIN(*mtu, CSR_BT_MAXIMUM_AUTOMATIC_MTU);

        /* MPS should never exceed MTU */
        if(*mps > *mtu)
        {
            *mps = *mtu;
        }
#ifdef CSR_AMP_ENABLE
        else if(align)
        {
            /* Align MTU to a multiple of MPS */
            *mtu = *mtu - (*mtu % *mps);
        }
#endif
    }

    if(BKV_Scan16Single(&l2capConn->conftabIter,
                        L2CA_AUTOPT_MTU_OUT,
                        &mode))
    {
        /* Select hard requirement although minimum the spec value */
        *outMtu = CSRMAX(L2CA_MTU_MINIMUM, mode);
    }
    else
    {
        /* No hard requirements from profile, so we may only request
         * the specification default */
        *outMtu = L2CA_MTU_DEFAULT;
    }

}

/* Fix outgoing and non-critical flow control settings */
static void csrBtCmL2caFixConftabFlowControl(cmInstanceData_t *cmData,
                                             cmL2caConnInstType *l2capConn,
                                             CsrUint16 *rxWindowSize)
{
    CsrUint16 rxWindowSizeTmp;
    CsrUint16 val1;
    CsrUint16 val2;

    /* Only set up flow params if using flow control mode */
    if(!BKV_Scan16Single(&l2capConn->conftabIter,
                         L2CA_AUTOPT_FLOW_MODE, &val1) || (val1 != 0))
    {       
        /* Fix outgoing window size */
        if(!BKV_Scan16Range(&l2capConn->conftabIter,
                            L2CA_AUTOPT_FLOW_WINDOW_OUT,
                            &val1, &val2))
        {
            CsrBtCmL2caConftabTxwinAllowAnyPeer(l2capConn->conftabIter.block,
                                                &l2capConn->conftabIter.size);
        }

        /* Read incoming window size */
        if(!BKV_Scan16Range(&l2capConn->conftabIter,
                            L2CA_AUTOPT_FLOW_WINDOW_IN,
                            &val1, &val2))
        {
            rxWindowSizeTmp = MAX_L2CAP_TX_WINDOW;
        }
        else
        {
            rxWindowSizeTmp = CSRMAX(val1, val2);
        }
    }
    else
    {
        rxWindowSizeTmp = MAX_L2CAP_DATA_QUEUE_LENGTH;
    }

    if (rxWindowSize)
    {
        *rxWindowSize = rxWindowSizeTmp;
    }
    CSR_UNUSED(cmData);
}

/* Read conftab data priority and set CM L2CA instance member */
static void csrBtCmL2caFixConftabDataPriority(cmInstanceData_t *cmData,
                                              cmL2caConnInstType *l2capConn)
{
    CsrUint16 val = 0;

    if(BKV_Scan16Single(&l2capConn->conftabIter,
                        L2CA_AUTOPT_CHANNEL_PRIORITY, &val))
    {
        /* CSR_BT_CM_PRIORITY_LOW(=2) is internal to the CM, so this
         * check is only here to ensure the range. Other values are
         * defined in cm_prim */
        l2capConn->dataPriority = (CsrUint8)((val >= CSR_BT_CM_PRIORITY_LOW)
                                              ? CSR_BT_CM_PRIORITY_NORMAL
                                              : val);
    }
    else
    {
        l2capConn->dataPriority = CSR_BT_CM_PRIORITY_NORMAL;
    }
    CSR_UNUSED(cmData);
}

/* Set or read conftab flush timeout values as appropriate */
static void csrBtCmL2caFixConftabFlush(cmInstanceData_t *cmData,
                                       cmL2caConnInstType *l2capConn)
{
    CsrUint32 val = 0;
    CsrUint32 val2 = 0;

    /* Has incoming flush NOT been set? */
    if(!BKV_Scan32Range(&l2capConn->conftabIter,
                        L2CA_AUTOPT_FLUSH_IN, (uint32_t *) &val, (uint32_t *) &val2))
    {
        CsrBtCmL2caConftabFlushToAllowAnyPeer(l2capConn->conftabIter.block,
                                              &l2capConn->conftabIter.size);
    }
    CSR_UNUSED(cmData);
}

/* Set or read conftab MTU/MPS values as appropriate */
static void csrBtCmL2caFixConftabMtu(cmInstanceData_t *cmData,
                                     cmL2caConnInstType *l2capConn,
                                     l2ca_mtu_t incomingMps)
{
    CsrUint16 val = 0;
    CsrUint16 val2 = 0;

    /* Has incoming MTU been set? */
    if(BKV_Scan16Single(&l2capConn->conftabIter,
                        L2CA_AUTOPT_MTU_IN, &val))
    {
        l2capConn->incomingMtu = val;
    }
    else
    {
        CsrBtCmL2caConftabMtu(l2capConn->conftabIter.block,
                              &l2capConn->conftabIter.size,
                              TRUE, /* incoming */
                              l2capConn->incomingMtu);
    }

    /* Has outgoing MTU been set? */
    if(BKV_Scan16Single(&l2capConn->conftabIter,
                        L2CA_AUTOPT_MTU_OUT, &val))
    {
        l2capConn->outgoingMtu = val;
    }
    else
    {
        CsrBtCmL2caConftabMtu(l2capConn->conftabIter.block,
                              &l2capConn->conftabIter.size,
                              FALSE, /* outgoing */
                              l2capConn->outgoingMtu);
    }


    if(BKV_Scan16Single(&l2capConn->conftabIter,
                        L2CA_AUTOPT_FLOW_MODE, &val))
    {
        /* Has incoming MPS been set? */
        if(!BKV_Scan16Range(&l2capConn->conftabIter,
                           L2CA_AUTOPT_FLOW_MAX_PDU_IN, &val, &val2))
        {
            CsrBtCmL2caConftabMps(l2capConn->conftabIter.block,
                                  &l2capConn->conftabIter.size,
                                  TRUE, /* incoming */
                                  incomingMps,
                                  incomingMps);
        }

        /* Has outgoing MPS been set? */
        if(!BKV_Scan16Range(&l2capConn->conftabIter,
                            L2CA_AUTOPT_FLOW_MAX_PDU_OUT, &val, &val2))
        {
            CsrBtCmL2caConftabMps(l2capConn->conftabIter.block,
                                  &l2capConn->conftabIter.size,
                                  FALSE, /* outgoing */
                                  1, l2capConn->outgoingMtu); /* min, max */
        }        
    }
    CSR_UNUSED(cmData);
}

/* Add the cached remote L2CAP extended features to the table, if not
 * there already */
static void csrBtCmL2caFixConftabExtendedFeatures(cmInstanceData_t *cmData,
                                                  cmL2caConnInstType *l2capConn,
                                                  CsrBool fakeAll)
{
    CsrUint32 val = 0;
    aclTable *aclConn;
    CsrUint32 feats;

    /* We have a cached value */
    if(fakeAll ||
       ((returnAclConnectionElement(cmData, l2capConn->deviceAddr, &aclConn) != CM_ERROR) &&
        (aclConn->l2capExtendedFeatures != CM_INVALID_L2CAP_EXT_FEAT) &&
        (aclConn->l2capExtendedFeatures != 0)))
    {
        feats = (fakeAll ? 0xFFFFFFFFu : aclConn->l2capExtendedFeatures);

        if(!BKV_Scan32Single(&l2capConn->conftabIter,
                             L2CA_AUTOPT_EXT_FEATS, (uint32_t *) &val))
        {
            /* Add new entry */
            CsrBtCmL2caConftabExtendedFeatures(l2capConn->conftabIter.block,
                                               &l2capConn->conftabIter.size,
                                               feats);
        }
        else
        {
            /* Replace old entry, as we're sure that our cached value
             * is newer */
            if(BKV_JumpToKey(&l2capConn->conftabIter,
                             L2CA_AUTOPT_EXT_FEATS, FALSE))
            {
                BKVD_32_EXACT(l2capConn->conftabIter.block, l2capConn->conftabIter.iterator,
                              L2CA_AUTOPT_EXT_FEATS, feats);
            }
        }
    }
}

static void csrBtCmL2caConftabProcess(cmInstanceData_t *cmData,
                                      cmL2caConnInstType *l2caConnection)
{
    l2ca_mtu_t                    incomingMps;

    /* Calculate default, optimal MTU and MPS values */
    csrBtCmL2caOptimalMtuMps(cmData,
                             l2caConnection,
                             &l2caConnection->incomingMtu,
                             &incomingMps,
                             &l2caConnection->outgoingMtu);

    /* Make room for more conftab entries and adjust missing entries */
    CsrBtCmL2caConftabEnlarge(&l2caConnection->conftabIter.block,
                              &l2caConnection->conftabIter.size);

#ifdef CSR_STREAMS_ENABLE
    csrBtCmL2caFixConftabFlowControl(cmData, l2caConnection, NULL);
#else
    csrBtCmL2caFixConftabFlowControl(cmData,
                                     l2caConnection,
                                     &l2caConnection->rxQueueMax);
#endif

    csrBtCmL2caFixConftabMtu(cmData, l2caConnection, incomingMps);
    csrBtCmL2caFixConftabFlush(cmData, l2caConnection);
    csrBtCmL2caFixConftabExtendedFeatures(cmData, l2caConnection, FALSE);
    csrBtCmL2caFixConftabDataPriority(cmData, l2caConnection);
    CsrBtCmL2caConftabCull(&l2caConnection->conftabIter.block,
                           &l2caConnection->conftabIter.size);
}

#ifdef CSR_TARGET_PRODUCT_VM
static void cmL2caConnectReqUnLockAclForPeer(cmInstanceData_t *cmData, cmL2caConnElement *theElement)
{
    aclTable *aclConnectionElement;
    returnAclConnectionElement(cmData, theElement->cmL2caConnInst->deviceAddr, &aclConnectionElement);
    if (aclConnectionElement)
    {
        if ((theElement->cmL2caConnInst->remotePsm == 0xfeff /*Peer Signalling Profile*/ ||
            theElement->cmL2caConnInst->remotePsm == 0xfefd /*Handover Profile*/ ||
            theElement->cmL2caConnInst->remotePsm == 0xfefb /*Mirror Profile*/) &&
            aclConnectionElement->aclLockedForPeer == TRUE)
        {
            CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "cmL2caConnectReqUnLockAclForPeer, REMOTEPSM:%02x, LOCK STATE:%d",
                               theElement->cmL2caConnInst->remotePsm, aclConnectionElement->aclLockedForPeer));
            TYPED_BD_ADDR_T ad;
            ad.addr = theElement->cmL2caConnInst->deviceAddr;
            ad.type = CSR_BT_ADDR_PUBLIC;
            dm_acl_close_req(&ad, 0, 0, NULL);
            aclConnectionElement->aclLockedForPeer = FALSE;
        }
    }
}
#endif

static void cmProcessL2caConnectRequest(cmInstanceData_t *cmData,
                                      cmL2caConnElement *theElement, 
                                      CsrUint8 minEncKeySize)
{
    CsrUint8 numOutgoing;
    CsrUint8 numIncoming;
    cmL2caConnInstType *l2caConnection = theElement->cmL2caConnInst;
    csrBtCmL2caConftabProcess(cmData, l2caConnection);

    /* Deal with security */
    numberOfSecurityRegister(cmData, l2caConnection->psm, l2caConnection->deviceAddr,
                             &numOutgoing, &numIncoming);

    CsrBtCmDmSmClearRebondData(cmData);

    if (numOutgoing == 1)
    {
        /* Register Outgoing Security on this. The PSM has been
         * changed to localPsm as a workaround. The core stack
         * expects the remote PSM value in this signal, but when
         * it is used, it is compared to the local PSM */
        dm_sm_service_register_outgoing_req(CSR_BT_CM_IFACEQUEUE,
                                            0, /* context */
                                            &l2caConnection->deviceAddr,
                                            (l2caConnection->transportType == BREDR_ACL) ? SEC_PROTOCOL_L2CAP : SEC_PROTOCOL_LE_L2CAP,
                                            l2caConnection->psm,
                                            l2caConnection->secLevel,
                                            minEncKeySize,
                                            NULL);
    }
    else
    {
        /* The security on this PSM has already been set */
        ;
    }

    if (CsrBtCmDmWriteKnownCacheParams(cmData, l2caConnection->deviceAddr, L2CAP_PLAYER))
    {
        CSR_BT_CM_STATE_CHANGE(l2caConnection->state,
                               CSR_BT_CM_L2CAP_STATE_CONNECT_INIT);
        /* Wait for application of cached parameters */
    }
#ifndef CSR_TARGET_PRODUCT_VM
    else if (CsrBtCmL2caCheckLegacyDetach(cmData, l2caConnection)){
        /*Nothing to do here*/
        return;
    }
#endif
    else
    {
        /* Everything seems OK - initiate the L2CAP connection
         * sequence */
        CsrBtCml2caAutoConnectSetup(cmData, theElement);
    }
}


/* The application request to create a new l2cap connection. */
void CsrBtCml2caConnectReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmL2caConnectReq *cmPrim;
    cmL2caConnElement *theElement;
    CsrBtTpdAddrT tpdAddrT;

    cmPrim = (CsrBtCmL2caConnectReq *) cmData->recvMsgP;

    /* Sanity check: Validate the conftab. Bail out and restore the queue */
    if(!BKV_Validate(cmPrim->conftab, cmPrim->conftabCount))
    {
        /* Invalid data - tell client and abort now */
        csrBtCmL2caConnectCfmMsgSend(cmPrim->phandle, L2CA_CID_INVALID, cmPrim->localPsm,
                                     0, 0,
                                     cmPrim->addr,
                                     CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE_OR_PARAMETER_VALUE,
                                     CSR_BT_SUPPLIER_CM,
                                     cmPrim->context);

        CsrBtCmServiceManagerLocalQueueHandler(cmData);
        return;
    }

    /* Sanity check: Connection table full. Bail out and restore the queue */
    if (!CsrBtCmElementCounterIncrement(cmData))
    {
        csrBtCmL2caConnectCfmMsgSend(cmPrim->phandle, L2CA_CID_INVALID, cmPrim->localPsm, 0, 0, cmPrim->addr,
                                     CSR_BT_RESULT_CODE_CM_REJECTED_DUE_TO_LIMITED_RESOURCES,
                                     CSR_BT_SUPPLIER_CM,
                                     cmPrim->context);

        CsrBtCmServiceManagerLocalQueueHandler(cmData);
        return;
    }

    cmEncodeTpAddr(cmPrim->addr, TBDADDR_PUBLIC, BREDR_ACL, &tpdAddrT);

    theElement = createL2caConnElement(cmData, cmPrim->phandle, cmPrim->context,
                                       tpdAddrT, cmPrim->localPsm, cmPrim->remotePsm,
                                       BTCONN_ID_EMPTY, cmPrim->secLevel, cmPrim->conftabCount,
                                       cmPrim->conftab);

    cmData->l2caVar.activeElemId  = theElement->elementId;
    cmData->l2caVar.cancelConnect = FALSE;
    CSR_BT_CM_STATE_CHANGE(cmData->l2caVar.connectState, CM_L2CA_IDLE);

    /* Set Conftab to NULL, so no one should free it */
    cmPrim->conftab = NULL;
    cmPrim->conftabCount = 0;

    cmProcessL2caConnectRequest(cmData, theElement, cmPrim->minEncKeySize);
}

void CsrBtCml2caCancelConnectReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmCancelL2caConnectReq *prim = (CsrBtCmCancelL2caConnectReq *) cmData->recvMsgP;
    cmL2caConnElement *theElement     = returnCancelL2caConnectIndex(cmData, prim->localPsm, prim->phandle, prim->deviceAddr);

    if (theElement == NULL)
    {
        /* The CSR_BT_CM_L2CA_CONNECT_REQ msg is not placed in the l2cap
         * connection table */
        CsrUint8    dummy;
        CsrUint16   context;
        if(cancelServiceManagerMsg(cmData, CSR_BT_CM_L2CA_CONNECT_REQ, prim->phandle, prim->deviceAddr, CSR_BT_NO_SERVER, prim->localPsm, &dummy, &context))
        {
            /* The CSR_BT_CM_L2CA_CONNECT_REQ msg is found and removed from
             * the local SM queue */
            csrBtCmL2caConnectCfmMsgSend(prim->phandle, L2CA_CID_INVALID, prim->localPsm, 0, 0, prim->deviceAddr,
                                         CSR_BT_RESULT_CODE_CM_CANCELLED, CSR_BT_SUPPLIER_CM, context);
        }
        else
        {
            /* Nothing to cancel just ignore */
            ;
        }
    }
    else
    {
        cmData->l2caVar.cancelConnect   = TRUE;
        if(theElement->cmL2caConnInst && (theElement->cmL2caConnInst->transportType == LE_ACL))
        {
            /* We want to delay calling L2CA_DisconnectReq in case of LECOC. 
             * L2CA_DisconnectReq will be called when synergy gets either success
             * or failure from the bluestack.
             */
            return;
        }


        CsrBtCmScRejectedForSecurityReasonMsgSend(cmData,
                                                  prim->deviceAddr, TRUE);

        /* This device is a LMP version 1.2 or higher */
        if (cmData->l2caVar.connectState == CM_L2CA_SSP_REPAIR)
        {
            CsrBtCmSmCancelSppRepairInd(cmData);
            CsrBtCmL2CaConnectCfmErrorHandler(cmData, theElement,
                    CSR_BT_RESULT_CODE_CM_CANCELLED, CSR_BT_SUPPLIER_CM);
        }
        else
        {
            if (CsrBtCmDmCancelPageOrDetach(cmData, prim->deviceAddr))
            { /* The ACL is being detach change state to ensure that
                 it not cancel twice                                            */
                CSR_BT_CM_STATE_CHANGE(cmData->l2caVar.connectState, CM_L2CA_CANCELING);
            }
            else 
            {
                cmL2caConnInstType *l2caConnection = theElement->cmL2caConnInst;

                if ((l2caConnection != NULL) && (cmData->l2caVar.connectState == CM_L2CA_CONNECT_PENDING))
                { /* Request l2cap to cancel the outgoing connection            */
                    L2CA_DisconnectReq(CM_GET_UINT16ID_FROM_BTCONN_ID(l2caConnection->btConnId));
                    CSR_BT_CM_STATE_CHANGE(cmData->l2caVar.connectState, CM_L2CA_CANCELING);
                }
                else
                { /* Must Wait until CM receives the right Confirm Msg          */
                    ;
                }
            }
        }
    }
}

void CsrBtCml2caConnectAcceptReqHandler(cmInstanceData_t *cmData)
{
    /* The application request to accept a new l2cap connection,
     * create from a peer device */
    CsrBtCmL2caConnectAcceptReq  *cmPrim = (CsrBtCmL2caConnectAcceptReq *) cmData->recvMsgP;

    if (CsrBtCmElementCounterIncrement(cmData))
    {
        /* If it is possible to accept a new l2cap connection */
        CsrUint8    numOutgoing;
        CsrUint8    numIncoming;
        cmL2caConnElement  *theElement;
        cmL2caConnInstType *l2caConnection;
        CsrBtTpdAddrT tpdAddrT;

        cmEncodeTpAddr(cmPrim->deviceAddr, TBDADDR_PUBLIC, BREDR_ACL, &tpdAddrT);

        theElement = createL2caConnElement(cmData, cmPrim->phandle, cmPrim->context, tpdAddrT,
                      cmPrim->localPsm, NO_REMOTE_PSM, BTCONN_ID_RESERVED,
                      cmPrim->secLevel, cmPrim->conftabCount, cmPrim->conftab);

        theElement->app_controlled = FALSE;

        l2caConnection = theElement->cmL2caConnInst;
        l2caConnection->classOfDevice   = cmPrim->classOfDevice;
        cmPrim->conftab = NULL;
        cmPrim->conftabCount = 0;

        if (cmPrim->primaryAcceptor)
        {
            /* Start the connectable routine.*/
            cmData->l2caVar.activeElemId = theElement->elementId;
            CSR_BT_CM_STATE_CHANGE(l2caConnection->state,
                                   CSR_BT_CM_L2CAP_STATE_CONNECTABLE);
            CmL2caRemoteConnectionStateHandler(cmData, theElement);
        }
        else
        {
            /* Do not make the device connectable (Do not write a new
             * class of device value and do not set the device
             * connectable) because the profile already have a
             * primary l2cap connection */
            CSR_BT_CM_STATE_CHANGE(l2caConnection->state,
                                   CSR_BT_CM_L2CAP_STATE_IDLE);
        }

        numberOfSecurityRegister(cmData, cmPrim->localPsm, cmPrim->deviceAddr, &numOutgoing, &numIncoming);
        if (numIncoming == 1)
        {
            /* Register Incoming Security on this PSM, Don't apply for
             * outgoing connections*/
            CsrBtScRegisterReqSend(cmPrim->profileUuid,
                                   cmPrim->localPsm,
                                   FALSE,
                                   SEC_PROTOCOL_L2CAP,
                                   cmPrim->secLevel,
                                   cmPrim->minEncKeySize);
        }
        else
        {
            /* The security on this psm has already been set */
            ;
        }
    }
    else
    {
        /* It is not possible to accept a L2CAP connection. Inform the
         * application and restore the local L2cap queue */
        csrBtCmL2caConnectAcceptCfmMsgSend(cmPrim->phandle, L2CA_CID_INVALID,
                                           cmPrim->localPsm, NO_REMOTE_PSM,
                                           0, 0,
                                           cmPrim->deviceAddr,
                                           CSR_BT_RESULT_CODE_CM_REJECTED_DUE_TO_LIMITED_RESOURCES,
                                           CSR_BT_SUPPLIER_CM,
                                           cmPrim->context);

        if (cmPrim->primaryAcceptor)
        {
            /* Need to called this function because if primaryAcceptor
             * is TRUE then it uses the SM queue */
            CsrBtCmServiceManagerLocalQueueHandler(cmData);
        }
    }
}

void CsrBtCml2caCancelAcceptConnectReqHandler(cmInstanceData_t *cmData)
{
    /* The application request to cancel the previous connect accept
     * request signal */
    CsrBtCmL2caCancelConnectAcceptReq *cmPrim = (CsrBtCmL2caCancelConnectAcceptReq *) cmData->recvMsgP;
    cmL2caConnElement * theElement  = CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromCancelledBtConnIdPsm,
                                                           &(cmPrim->localPsm));

    if (theElement)
    {
        /* Remove COD and cancel the service */
        cmL2caConnInstType *l2caConnection  = theElement->cmL2caConnInst;
        cmData->l2caVar.activeElemId        = theElement->elementId;
        l2caConnection->btConnId            = BTCONN_ID_EMPTY;
        CSR_BT_CM_STATE_CHANGE((l2caConnection->state),
                               CSR_BT_CM_L2CAP_STATE_CANCEL_CONNECTABLE);
        CmL2caRemoteConnectionStateHandler(cmData, theElement);
    }
    else
    {
        /* The service can't be cancelled. Restore the local service manager queue.
         * A confirm has already been sent to application */
        CsrBtCmServiceManagerLocalQueueHandler(cmData);
    }
}

#ifndef CSR_TARGET_PRODUCT_VM
static CsrBool CsrBtCmL2CaReconnect(cmInstanceData_t *cmData, l2ca_conn_result_t response, CsrBtDeviceAddr deviceAddr)
{
    if (response == L2CA_CONNECT_REJ_SECURITY ||
        response == L2CA_CONNECT_KEY_MISSING)
    {
        if (!cmData->l2caVar.cancelConnect && CsrBtCmDmSmRebondNeeded(cmData))
        {
            return TRUE;
        }
        else
        {
            CsrBtCmScRejectedForSecurityReasonMsgSend(cmData,
                                                      deviceAddr, FALSE);
            return FALSE;
        }
    }
    return FALSE;
}
#endif

void CsrBtCmL2CaConnectCfmErrorHandler(cmInstanceData_t     *cmData,
                                       cmL2caConnElement    *theElement,
                                       CsrBtResultCode      resultCode,
                                       CsrBtSupplier  resultSupplier)
{
    CSR_BT_CM_STATE_CHANGE(cmData->l2caVar.connectState, CM_L2CA_IDLE);

    if (theElement->cmL2caConnInst->transportType == CSR_BT_TRANSPORT_BREDR)
    {
        /* Connection failed is as good as disconnected, notify this to device utility.*/
        (void)CmDuHandleAutomaticProcedure(cmData,
                                           CM_DU_AUTO_EVENT_SERVICE_DISCONNECTED,
                                           NULL,
                                           &theElement->cmL2caConnInst->deviceAddr);
    }

    CsrBtCmL2caConnectCfmMsgHandler(cmData, theElement, resultCode, resultSupplier);
    CsrBtCmServiceManagerLocalQueueHandler(cmData);
}

static void csrBtCmL2CaAutoConnectCfmErrorHandler(cmInstanceData_t     *cmData,
                                                  cmL2caConnElement    *theElement,
                                                  CsrBtResultCode      resultCode,
                                                  CsrBtSupplier  resultSupplier)
{
    cmL2caConnInstType *l2caConnection = theElement->cmL2caConnInst;

    switch (l2caConnection->state)
    {
        case CSR_BT_CM_L2CAP_STATE_CONNECT_ACCEPT:
        {
            /* Check if the application had accepted the connection. */
            if (theElement->app_controlled)
            {
                /* Send the failure to application and destroy the connection element and unlock sm queue. */
                CsrBtCmL2caConnectAcceptCfmHandler(cmData,
                                                   theElement,
                                                   resultCode,
                                                   resultSupplier);
            }
            else
            {
                CsrBtCmL2capAcceptFailClearing(cmData, l2caConnection);
            }

            CsrBtCmServiceManagerLocalQueueHandler(cmData);
        }
        break;

        case CSR_BT_CM_L2CAP_STATE_CONNECT:
            CsrBtCmL2CaConnectCfmErrorHandler(cmData, theElement, resultCode, resultSupplier);
            break;

        default:
            CsrBtCmGeneralException(L2CAP_PRIM,
                                    *(CsrPrim *)cmData->recvMsgP,
                                    cmData->globalState,
                                    "");
            break;
    }
}

/* The application request to create a new l2cap connection. */
void Cml2caTpConnectReqHandler(cmInstanceData_t *cmData)
{
    CmL2caTpConnectReq *cmPrim;
    cmL2caConnElement *theElement;

    cmPrim = (CmL2caTpConnectReq *) cmData->recvMsgP;

    /* Sanity check: Validate the conftab. Bail out and restore the queue */
    if(!BKV_Validate(cmPrim->conftab, cmPrim->conftabCount))
    {
        /* Invalid data - tell client and abort now */
        cmL2caTpConnectCfmMsgSend(cmPrim->phandle,
                                  L2CA_CID_INVALID, cmPrim->localPsm,
                                  0,
                                  0,
                                  cmPrim->tpdAddrT.addrt.addr,
                                  cmPrim->tpdAddrT.addrt.type,
                                  cmPrim->tpdAddrT.tp_type,
                                  CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE_OR_PARAMETER_VALUE,
                                  CSR_BT_SUPPLIER_CM,
                                  cmPrim->context);

        CsrBtCmServiceManagerLocalQueueHandler(cmData);
        return;
    }

    /* Sanity check: Connection table full. Bail out and restore the queue */
    if (!CsrBtCmElementCounterIncrement(cmData))
    {
        cmL2caTpConnectCfmMsgSend(cmPrim->phandle,
                                  L2CA_CID_INVALID,
                                  cmPrim->localPsm,
                                  0,
                                  0,
                                  cmPrim->tpdAddrT.addrt.addr,
                                  cmPrim->tpdAddrT.addrt.type,
                                  cmPrim->tpdAddrT.tp_type,
                                  CSR_BT_RESULT_CODE_CM_REJECTED_DUE_TO_LIMITED_RESOURCES,
                                  CSR_BT_SUPPLIER_CM,
                                  cmPrim->context);

        CsrBtCmServiceManagerLocalQueueHandler(cmData);
        return;
    }

    theElement = createL2caConnElement(cmData, cmPrim->phandle, cmPrim->context, cmPrim->tpdAddrT,
                          cmPrim->localPsm, cmPrim->remotePsm, BTCONN_ID_EMPTY,
                          cmPrim->secLevel, cmPrim->conftabCount, cmPrim->conftab);
    /* Set Conftab to NULL, so no one should free it */
    cmPrim->conftab = NULL;
    cmPrim->conftabCount = 0;
    /* Mark this element is created for sending TP Prim based connect request */
    theElement->useTpPrim = TRUE;

    cmData->l2caVar.activeElemId  = theElement->elementId;
    cmData->l2caVar.cancelConnect = FALSE;
    CSR_BT_CM_STATE_CHANGE(cmData->l2caVar.connectState, CM_L2CA_IDLE);

    cmProcessL2caConnectRequest(cmData, theElement, cmPrim->minEncKeySize);
}

static void csrBtCmL2caRemoveAutoConnectIndFromSaveQueue(cmInstanceData_t *cmData,
                                                         l2ca_cid_t       cid)
{
    CsrUint16               eventClass;
    void                    *msg;
    CsrMessageQueueType     *tempQueue  = NULL;

    while(CsrMessageQueuePop(&cmData->smVar.saveQueue, &eventClass, &msg))
    {
        if (eventClass == L2CAP_PRIM)
        {
            l2ca_cid_t saved_prim_cid = L2CA_CID_INVALID;
            if (((L2CA_UPRIM_T *) msg)->type == L2CA_AUTO_TP_CONNECT_IND)
            {
                saved_prim_cid = ((L2CA_AUTO_TP_CONNECT_IND_T *) msg)->cid;
            }
            else if (((L2CA_UPRIM_T *) msg)->type == L2CA_AUTO_CONNECT_IND)
            {
                saved_prim_cid = ((L2CA_AUTO_CONNECT_IND_T *) msg)->cid;
            }

            if (cid == saved_prim_cid)
            {
                L2CA_FreePrimitive(msg);
            }
            else
            {
                CsrMessageQueuePush(&tempQueue, eventClass, msg);
            }
        }
        else
        {
            CsrMessageQueuePush(&tempQueue, eventClass, msg);
        }
    }
    cmData->smVar.saveQueue = tempQueue;
}

static void cmL2capAddCreditCfmMsgSend(CsrSchedQid appHandle, CsrBtConnId  btConnId,
                                               CsrUint16 context, CsrUint16 credits,
                                               CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CmL2caAddCreditCfm * cmPrim;

    cmPrim    = (CmL2caAddCreditCfm *)CsrPmemAlloc(sizeof(CmL2caAddCreditCfm));
    cmPrim->type            = CM_L2CA_ADD_CREDIT_CFM;
    cmPrim->btConnId        = btConnId;
    cmPrim->context         = context;
    cmPrim->credits         = credits;
    cmPrim->resultCode      = resultCode;
    cmPrim->resultSupplier  = resultSupplier;
    CsrBtCmPutMessage(appHandle, cmPrim);
}

/* Handles outgoing (local) connection related states. */
static void cmL2caLocalConnectionStateHandler(cmInstanceData_t *cmData, cmL2caConnElement *connElement)
{
    cmL2caConnInstType *l2capLink = connElement->cmL2caConnInst;

    CSR_BT_CM_STATE_CHANGE(l2capLink->state, CSR_BT_CM_L2CAP_STATE_CONNECTED);
    CsrBtCmL2caConnectCfmMsgHandler(cmData,
                                    connElement,
                                    CSR_BT_RESULT_CODE_CM_SUCCESS,
                                    CSR_BT_SUPPLIER_CM);
    CsrBtCmServiceManagerLocalQueueHandler(cmData);

    if (l2capLink->transportType == CSR_BT_TRANSPORT_BREDR)
    {
        /* For BREDR transport, inform to device utility in order to set the correct link policy for this link.*/
        (void)CmDuHandleAutomaticProcedure(cmData,
                                           CM_DU_AUTO_EVENT_SERVICE_CONNECTED,
                                           NULL,
                                           &l2capLink->deviceAddr);
    }

    CsrBtCmWriteAutoFlushTimeout(cmData, &l2capLink->deviceAddr);

#ifndef CSR_STREAMS_ENABLE
    CsrBtCmL2caDataStart(cmData, l2capLink);
#endif
}

/* Handles remote (incoming) connection related states. */
void CmL2caRemoteConnectionStateHandler(cmInstanceData_t *cmData,
                                        cmL2caConnElement *connElement)
{
    CsrBool notifyConnectable = TRUE;

    if (connElement && connElement->cmL2caConnInst)
    {
        cmL2caConnInstType *l2capLink = connElement->cmL2caConnInst;

        switch (l2capLink->state)
        {
            case CSR_BT_CM_L2CAP_STATE_CONNECTABLE:
            {
                CSR_BT_CM_STATE_CHANGE(connElement->cmL2caConnInst->state,
                                       CSR_BT_CM_L2CAP_STATE_IDLE);
                break;
            }

            case CSR_BT_CM_L2CAP_STATE_CONNECT_ACCEPT_FINAL:
            {
                /* The l2cap connection is now establish with with success. Inform the
                 * application, and restore the  local service manager and DM queue.
                 *
                 * Currently there is at least one more connection attach to the
                 * device address. The l2cap connection has been accepted with with success.
                 * Inform the application, and restore the local service manager and
                 * DM queue */
                CSR_BT_CM_STATE_CHANGE(connElement->cmL2caConnInst->state, CSR_BT_CM_L2CAP_STATE_CONNECTED);
                CsrBtCmL2caConnectAcceptCfmHandler(cmData,
                                                   connElement,
                                                   CSR_BT_RESULT_CODE_CM_SUCCESS,
                                                   CSR_BT_SUPPLIER_CM);

                if (connElement->cmL2caConnInst->transportType == CSR_BT_TRANSPORT_BREDR)
                {
                    /* Inform this to Device utility if its getting used.*/
                    (void)CmDuHandleAutomaticProcedure(cmData,
                                                       CM_DU_AUTO_EVENT_SERVICE_CONNECTED,
                                                       NULL,
                                                       &connElement->cmL2caConnInst->deviceAddr);
                    /* Since we have already notified to device utility, no need to do it towards the end of this function. */
                    notifyConnectable = FALSE;

                    /* Writing auto flush timeout this is a part of the procedure. */
                    CsrBtCmWriteAutoFlushTimeout(cmData, &connElement->cmL2caConnInst->deviceAddr);
                }
                break;
            }

            case CSR_BT_CM_L2CAP_STATE_CANCEL_CONNECTABLE:
            { /* The connect accept service is cancel. Send CM_L2CA_CANCEL_ACCEPT_CONNECT_CFM
                 to the application and restore the local service manager and the local DM
                 queue. */
                CsrUint8    numOfOutgoing;
                CsrUint8    numOfIncoming;

                numberOfSecurityRegister(cmData,
                                         l2capLink->psm,
                                         l2capLink->deviceAddr,
                                         &numOfOutgoing,
                                         &numOfIncoming);

                if (numOfIncoming == 1)
                { /* Unregister The Incoming security setting */
                    CsrBtScDeregisterReqSend((l2capLink->transportType == BREDR_ACL) ? SEC_PROTOCOL_L2CAP : SEC_PROTOCOL_LE_L2CAP,
                                              l2capLink->psm);
                }
                CsrBtCmL2capClearL2capTableIndex(cmData, &(connElement->cmL2caConnInst));
                break;
            }

            default:
                break;
        }
    }

    if (notifyConnectable)
    {
        /* Notify device utility related to current L2CA handling, in order to take decision 
         * on whether to enable page scan or not. Since we are just informing state change
         * the result code is ignored here. */
        (void)CmDuHandleAutomaticProcedure(cmData, CM_DU_AUTO_EVENT_CONNECTABLE, NULL, NULL);
    }

    /* For remote connections, this is the place where SM queue needs to be unlocked. */
    if (connElement)
    {
        /* If connElement is NULL then the unlock handler has already been called. */
        CsrBtCmServiceManagerLocalQueueHandler(cmData);
    }
}

/* The application request to add credits to l2cap connection. */
void CmL2caAddCreditReqHandler(cmInstanceData_t *cmData)
{ /* Request to add credits to L2CAP connection */
    CmL2caAddCreditReq    * cmPrim = (CmL2caAddCreditReq *) cmData->recvMsgP;
    cmL2caConnElement * theElement = CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromBtConnId, &(cmPrim->btConnId));

    if (theElement)
    {
        cmL2caConnInstType *l2CaConnection = theElement->cmL2caConnInst;

#ifdef INSTALL_L2CAP_LECOC_CB
        if (l2CaConnection->state == CSR_BT_CM_L2CAP_STATE_CONNECTED)
        {
            L2CA_AddCreditReq(CM_GET_UINT16ID_FROM_BTCONN_ID(l2CaConnection->btConnId), cmPrim->context, cmPrim->credits);
        }
        else
#endif /* INSTALL_L2CAP_LECOC_CB */
        { /* Build and send CSR_BT_CM_L2CA_ADD_CREDIT_CFM with ERROR */
            cmL2capAddCreditCfmMsgSend(l2CaConnection->appHandle, cmPrim->btConnId,
                                             cmPrim->context, cmPrim->credits,
                                             (CsrBtReasonCode) CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE_OR_PARAMETER_VALUE,
                                             CSR_BT_SUPPLIER_CM);
        }
    }
    else
    { 
        cmL2capAddCreditCfmMsgSend(cmPrim->phandle,cmPrim->btConnId,
                                 cmPrim->context, cmPrim->credits,
                                 (CsrBtReasonCode) CSR_BT_RESULT_CODE_CM_UNKNOWN_CONNECTION_IDENTIFIER,
                                 CSR_BT_SUPPLIER_CM);
    }
}

static void csrBtCmL2caAutoConnectCmpHandler(cmInstanceData_t *cmData)
{ 
    L2CA_AUTO_TP_CONNECT_CFM_T *prim = (L2CA_AUTO_TP_CONNECT_CFM_T*)cmData->recvMsgP;
    cmL2caConnElement *l2caConnElement = CM_L2CA_ELEMENT_ACTIVE(cmData);

    /* Handling for L2CA_AUTO_CONNECT_CFM/L2CA_AUTO_TP_CONNECT_CFM prims */
    if (l2caConnElement && l2caConnElement->cmL2caConnInst)
    { /* The attempt to setup a new incoming or outgoing l2cap connection
         is finish */
        cmL2caConnInstType *l2caConnection = l2caConnElement->cmL2caConnInst;

        switch(prim->result)
        {
            case L2CA_CONNECT_SUCCESS:
            { /* A L2CAP connection is establish with success */
                CSR_BT_CM_STATE_CHANGE(cmData->l2caVar.connectState, CM_L2CA_IDLE);
                l2caConnection->btConnId = CM_CREATE_L2CA_CONN_ID(prim->cid);

                CsrBtCmDmSmClearRebondData(cmData);

                if (!cmData->l2caVar.cancelConnect)
                {
                    aclTable *aclConn;

                    CsrBtCmL2capStoreConfigOptions(&prim->config, l2caConnection);

                    /* Incoming connection? */
                    if (l2caConnection->state == CSR_BT_CM_L2CAP_STATE_CONNECT)
                    {
                        cmL2caLocalConnectionStateHandler(cmData, l2caConnElement);
                    }
                    else
                    {
                        CSR_BT_CM_STATE_CHANGE(l2caConnection->state,
                                               CSR_BT_CM_L2CAP_STATE_CONNECT_ACCEPT_FINAL);
                        CmL2caRemoteConnectionStateHandler(cmData, l2caConnElement);
                    }

                    /* Cache the remove supported features */
                    if(returnAclConnectionElement(cmData, l2caConnection->deviceAddr, &aclConn) != CM_ERROR)
                    {
                        aclConn->l2capExtendedFeatures = prim->ext_feats;
                    }
                }
                else
                {
                    cmData->smVar.arg.result.code        = CSR_BT_RESULT_CODE_CM_CANCELLED;
                    cmData->smVar.arg.result.supplier    = CSR_BT_SUPPLIER_CM;
                    L2CA_DisconnectReq(prim->cid);
                }
                break;
            }
            case L2CA_CONNECT_RETRYING :
            { /* Retry L2CAP connection (handled internally) */
                break;
            }

            case L2CA_CONNECT_INITIATING:
#ifdef CSR_TARGET_PRODUCT_VM
                cmL2caConnectReqUnLockAclForPeer(cmData, l2caConnElement);
                /*fall through*/
#endif
            case L2CA_CONNECT_PENDING :
            { /* The L2CAP connection is pending, wait for another CFM signal */
                l2caConnection->btConnId = CM_CREATE_L2CA_CONN_ID(prim->cid);

                /* Don't call L2CA_DisconnectReq for LECOC here, wait for either success or failure from the
                 * Bluestack. 
                 */
                if (cmData->l2caVar.cancelConnect &&
                    (l2caConnElement->cmL2caConnInst->transportType != LE_ACL))
                { /* The application has requested to cancel the outgoing 
                     l2cap connection */
                    if (cmData->l2caVar.connectState == CM_L2CA_CANCELING)
                    { /* The connection is being cancel ignore this message */
                        ;
                    }
                    else
                    { /* Request l2cap to cancel the outgoing connection
                         E.g. CsrBtCmCancelL2caConnectReq were received before 
                         CM receives this message */
                        CSR_BT_CM_STATE_CHANGE(cmData->l2caVar.connectState, CM_L2CA_CANCELING);
                        L2CA_DisconnectReq(prim->cid);
                    }
                }
                else
                {
                    CSR_BT_CM_STATE_CHANGE(cmData->l2caVar.connectState, CM_L2CA_CONNECT_PENDING);
                }
                break;
            }
            default:
            { /* Establishment failure */
#ifndef CSR_TARGET_PRODUCT_VM
                if (CsrBtCmL2CaReconnect(cmData, prim->result, l2caConnection->deviceAddr))
                {
                    /* The connect attempt fail due to security
                     * reason, e.g the local device had a SSP link key
                     * and the remote device did not */
                    CSR_BT_CM_STATE_CHANGE(cmData->l2caVar.connectState, CM_L2CA_SSP_REPAIR);
                    CsrBtCmSmSppRepairIndSend(cmData, l2caConnection->deviceAddr);
                }
                else
#endif
                {
                    if (!cmData->l2caVar.cancelConnect)
                    {
                        csrBtCmL2CaAutoConnectCfmErrorHandler(cmData,
                                                              l2caConnElement,
                                                              (CsrBtResultCode) prim->result,
                                                              CSR_BT_SUPPLIER_L2CAP_CONNECT);
                    }
                    else
                    {
                        csrBtCmL2CaAutoConnectCfmErrorHandler(cmData,
                                                              l2caConnElement,
                                                              CSR_BT_RESULT_CODE_CM_CANCELLED,
                                                              CSR_BT_SUPPLIER_CM);
                    }
                }
                break;
            }
        }
    }
    else
    { /* An incoming L2CAP connection must have failed, 
         remove it from save queue */
        csrBtCmL2caRemoveAutoConnectIndFromSaveQueue(cmData, prim->cid);
    }
}

static CsrBool CsrBtCmL2caReConfigIndHandler(cmInstanceData_t *cmData)
{
    L2CA_AUTO_TP_CONNECT_IND_T *prim = (L2CA_AUTO_TP_CONNECT_IND_T*)cmData->recvMsgP;
    CsrBtConnId btConnId          = CM_CREATE_L2CA_CONN_ID(prim->cid);
    cmL2caConnElement *theElement = CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromBtConnId, &(btConnId));

    if (theElement)
    { /*The peer request to reconfigure an established l2cap connection */
#ifdef CSR_AMP_ENABLE
        cmL2caConnInstType *l2caConnection  = theElement->cmL2caConnInst;
        /* No reconfiguraion for LE_ACL transport  */
        if (l2caConnection->transportType == LE_ACL)
        {
            return (FALSE);
        }

        if((CsrBtCmL2caFcsReConfigEnable(l2caConnection->conftabIter)) &&
            (l2caConnection->state == CSR_BT_CM_L2CAP_STATE_CONNECTED  ||
             l2caConnection->state == CSR_BT_CM_L2CAP_STATE_CONNECT    ||
             l2caConnection->state == CSR_BT_CM_L2CAP_STATE_CONNECT_ACCEPT_FINAL))
        {
            /* Tweak FCS based on AMP controller */
            CsrUint16 conftabLength;
            CsrUint16 *conftab = CsrBtCmL2caBuildFcsConftab(l2caConnection->controller, &conftabLength);
            if (theElement->useTpPrim)
            {
                L2CA_AutoTpConnectRsp(prim->identifier,
                                    prim->cid,
                                    L2CA_CONNECT_SUCCESS,
                                    CM_L2CA_RECONFIG_INPROGRESS_CTX, 
                                    conftabLength,
                                    conftab);
            }
            else
            {
                L2CA_AutoConnectRsp(prim->identifier,
                                    prim->cid,
                                    L2CA_CONNECT_SUCCESS,
                                    CM_L2CA_RECONFIG_INPROGRESS_CTX, 
                                    conftabLength,
                                    conftab);
            }
            /* Check if this connection is autorised */
            if (CsrBtBdAddrEq(&cmData->scVar.deviceAddr,&prim->tp_addrt.addrt.addr) && (cmData->scVar.psm == prim->psm_local))
            {
                l2caConnection->authorised = TRUE;
                CsrBtCmScCleanupVar(cmData);
            }
        }
        else
#endif
        { /* Reject reconfig */
            if (theElement->useTpPrim)
            {
                L2CA_AutoTpConnectRsp(prim->identifier,
                                    prim->cid,
                                    (l2ca_conn_result_t)L2CA_CONFIG_UNACCEPTABLE,
                                    CM_L2CA_INCOMING_CONNECT_REJECTED_CTX,
                                    0, /*conftab_length*/
                                    NULL); /*conftab*/
            }
            else
            {
                L2CA_AutoConnectRsp(prim->identifier,
                                    prim->cid,
                                    (l2ca_conn_result_t)L2CA_CONFIG_UNACCEPTABLE,
                                    CM_L2CA_INCOMING_CONNECT_REJECTED_CTX,
                                    0, /*conftab_length*/
                                    NULL); /*conftab*/
            }
        }
        return (TRUE);
    }
    return (FALSE);
}

static void csrBtCmL2caAutoConnectIndRejectHandler(cmInstanceData_t  *cmData,
                                                   l2cap_prim_t type,
                                                   l2ca_cid_t        cid,
                                                   l2ca_identifier_t identifier)
{ /* Reject the incoming l2cap connection */
    if (type == L2CA_AUTO_TP_CONNECT_IND)
    {
        L2CA_AutoTpConnectRsp(identifier,
                        cid,
                        L2CA_CONNECT_REJ_RESOURCES,
                        CM_L2CA_INCOMING_CONNECT_REJECTED_CTX,
                        0, /*conftab_length*/
                        NULL); /*conftab*/
    }
    else
    {
        L2CA_AutoConnectRsp(identifier,
                            cid,
                            L2CA_CONNECT_REJ_RESOURCES,
                            CM_L2CA_INCOMING_CONNECT_REJECTED_CTX,
                            0, /*conftab_length*/
                            NULL); /*conftab*/
    }

    /* We are not active after rejecting this request */
    cmData->l2caVar.activeElemId = CM_ERROR;

    if (cmData->smVar.popFromSaveQueue)
    { /* The L2CA_AUTO_CONNECT_IND/L2CA_AUTO_TP_CONNECT_IND message has been 
         restore from the SM queue. Restore it and lock it again
         in order to make sure that no message can use
         the SM before the CSR_BT_CM_SM_HOUSE_CLEANING
         is received.                                   */
        cmData->smVar.smInProgress = TRUE;
        CsrBtCmServiceManagerLocalQueueHandler(cmData);
    }
}

static void csrBtCmL2caAutoConnectIndAcceptHandler(cmInstanceData_t  *cmData,
                                                   cmL2caConnElement *l2caConnElement,
                                                   CsrBtDeviceAddr    deviceAddr,
                                                   psm_t              localPsm,
                                                   l2ca_cid_t         cid,
                                                   l2ca_identifier_t  identifier)
{
    cmL2caConnInstType *l2caConnInst  = l2caConnElement->cmL2caConnInst;

    cmData->l2caVar.activeElemId      = l2caConnElement->elementId;
    cmData->l2caVar.cancelConnect     = FALSE;
    l2caConnInst->btConnId            = CM_CREATE_L2CA_CONN_ID(cid);
    l2caConnInst->deviceAddr          = deviceAddr;

    CSR_BT_CM_STATE_CHANGE(l2caConnInst->state, CSR_BT_CM_L2CAP_STATE_CONNECT_ACCEPT);

    csrBtCmL2caConftabProcess(cmData, l2caConnInst);

    /* Conftab already adjusted (at connectacceptreq time), so
     * simply send our answer to L2CAP */
    if (l2caConnElement->useTpPrim)
    {
        L2CA_AutoTpConnectRsp(identifier,
                        cid,
                        L2CA_CONNECT_SUCCESS,
                        CM_L2CA_CONNECT_INPROGRESS_CTX,
                        l2caConnInst->conftabIter.size,
                        CsrMemDup(l2caConnInst->conftabIter.block,
                                  l2caConnInst->conftabIter.size*sizeof(CsrUint16)));
    }
    else
    {
        L2CA_AutoConnectRsp(identifier,
                            cid,
                            L2CA_CONNECT_SUCCESS,
                            CM_L2CA_CONNECT_INPROGRESS_CTX,
                            l2caConnInst->conftabIter.size,
                            CsrMemDup(l2caConnInst->conftabIter.block,
                                      l2caConnInst->conftabIter.size*sizeof(CsrUint16)));
    }
    /* Check if this connection is autorised */
    if (CsrBtBdAddrEq(&cmData->scVar.deviceAddr,&deviceAddr) && (cmData->scVar.psm == localPsm))
    {
        l2caConnInst->authorised = TRUE;
        CsrBtCmScCleanupVar(cmData);
    }
}

void CsrBtCmL2caAutoConnectCfmHandler(cmInstanceData_t *cmData)
{
    L2CA_AUTO_TP_CONNECT_CFM_T *prim = (L2CA_AUTO_TP_CONNECT_CFM_T*)cmData->recvMsgP;

    switch (prim->con_ctx)
    {
        case CM_L2CA_CONNECT_INPROGRESS_CTX:
        { /* Confirmation to earlier L2CA_AUTO_CONNECT_REQ/L2CA_AUTO_TP_CONNECT_REQ or
             L2CA_AUTO_CONNECT_RSP/L2CA_AUTO_TP_CONNECT_RSP where a new connection is
             being setup */
            csrBtCmL2caAutoConnectCmpHandler(cmData);
            break;
        }
        case CM_L2CA_RECONFIG_INPROGRESS_CTX:
        { /* Confirmation to earlier L2CA_AUTO_CONNECT_REQ/L2CA_AUTO_TP_CONNECT_REQ or
             L2CA_AUTO_CONNECT_RSP/L2CA_AUTO_TP_CONNECT_RSP where a connected l2cap 
             connection has been reconfigure. Note always 
             completed or failure at this point. Can not do
             anything about it either way */
            break;
        }
        default:
        { /* An incoming L2CAP connection must have failed, 
             remove it from save queue */
            csrBtCmL2caRemoveAutoConnectIndFromSaveQueue(cmData, prim->cid);
            break;
        }
    }
}

void CmL2caAddCreditCfmHandler(cmInstanceData_t *cmData)
{
    L2CA_ADD_CREDIT_CFM_T *cfm = (L2CA_ADD_CREDIT_CFM_T*)cmData->recvMsgP;
    CsrBtConnId btConnId          = CM_CREATE_L2CA_CONN_ID(cfm->cid);
    cmL2caConnElement *theElement = CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromBtConnId, &(btConnId));

    if (theElement)
    {
        cmL2caConnInstType *l2caConnection  = theElement->cmL2caConnInst;
        cmL2capAddCreditCfmMsgSend(l2caConnection->appHandle, btConnId, cfm->context, cfm->credits, cfm->result, ((cfm->result == L2CA_RESULT_SUCCESS) ? CSR_BT_SUPPLIER_CM : CSR_BT_SUPPLIER_L2CAP_MISC));
    }
}

static void cmL2caParseConftab(L2CA_AUTO_TP_CONNECT_IND_T *ind, 
                                      CmL2caTpConnectAcceptInd *prim)
{
    CsrUint16 val = 0;
    BKV_ITERATOR_T  conftabIter;
    conftabIter.block = ind->conftab;
    conftabIter.size = ind->conftab_length;
    conftabIter.iterator = 0;

    if (ind->conftab_length)
    { /* Parse the conftab and retrieve the respective the configuration values */
        if(BKV_Scan16Single(&conftabIter,L2CA_AUTOPT_MTU_OUT, &val))
        {
            prim->mtu = val;
        }
    }
}

static void cmL2caConnectAcceptIndSend(L2CA_AUTO_TP_CONNECT_IND_T *ind)
{
    if (ind->type == L2CA_AUTO_TP_CONNECT_IND)
    {
        CmL2caTpConnectAcceptInd *prim = (CmL2caTpConnectAcceptInd *)CsrPmemZalloc(sizeof(CmL2caTpConnectAcceptInd));

        prim->type          = CM_L2CA_TP_CONNECT_ACCEPT_IND;
        prim->btConnId      = CM_CREATE_L2CA_CONN_ID(ind->cid);
        prim->context       = ind->reg_ctx;
        prim->identifier    = ind->identifier;
        prim->tpdAddrT      = ind->tp_addrt;
        prim->localPsm      = ind->psm_local;
        prim->localControl  = ind->local_control;
        prim->flags         = ind->flags;
        /* Parse the conftab */
        cmL2caParseConftab(ind, prim);

        CsrBtCmPutMessage((CsrSchedQid)ind->reg_ctx, prim);
    }
    else
    {
        CsrBtCmL2caConnectAcceptInd *prim = (CsrBtCmL2caConnectAcceptInd *)CsrPmemAlloc(sizeof(*prim));

        prim->type          = CSR_BT_CM_L2CA_CONNECT_ACCEPT_IND;
        prim->btConnId      = CM_CREATE_L2CA_CONN_ID(ind->cid);
        prim->identifier    = ind->identifier;
        prim->localPsm      = ind->psm_local;
        prim->deviceAddr    = ind->tp_addrt.addrt.addr;

        CsrBtCmPutMessage((CsrSchedQid)ind->reg_ctx, prim);
    }
}

static CsrBool cmL2caIsCrossoverConnection(cmInstanceData_t *cmData, cmL2caConnEltType conElement)
{
    if (conElement.psm == CSR_BT_AVDTP_PSM || conElement.psm == CSR_BT_AVCTP_PSM)
    {
        /* Check whether A2DP signaling or AVRCP control channel is already connected */
        if (CM_FIND_L2CA_ELEMENT(CmL2caFindL2caConnElementWithConnectedPsmBdaddr, &conElement))
        {
            if (conElement.psm == CSR_BT_AVDTP_PSM)
            {
                /* Check whether connect ind is received for SIG or MEDIA channel */
                if (CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromReserveBtConnIdPsmBdaddr, &conElement) == NULL)
                {  
                    /* Connect ind received for AV SIG; this is crossover */
                    return (TRUE);
                }                
            }
            else
            {   /* AVRCP crossover */
                return (TRUE);
            }
        }
    }
    return (FALSE);
}

void CsrBtCmL2caAutoConnectIndHandler(cmInstanceData_t *cmData)
{
    L2CA_AUTO_CONNECT_IND_T *prim = (L2CA_AUTO_CONNECT_IND_T*)cmData->recvMsgP;
    CsrBool msgConverted = cmL2caConvertConnectIndToTpPrim(cmData);

    if (CsrBtCmL2caReConfigIndHandler(cmData))
    { /* The peer request to reconfigure an established l2cap connection */
        ;
    }
    else
    { /* The peer request to setup a new l2cap connection */
        if (cmData->smVar.smInProgress)
        { /* Another service is in progress, place this event on queue */
            /* Free the converted L2CA_AUTO_TP_CONNECT_IND prim */
            if (msgConverted)
            {
                CsrPmemFree(cmData->recvMsgP);
                /* Update the original received prim message */
                cmData->recvMsgP = prim;
            }

            CsrMessageQueuePush(&cmData->smVar.saveQueue, L2CAP_PRIM, cmData->recvMsgP);
            cmData->recvMsgP = NULL;

            return;
        }
        else
        { /* Ready to handle the incoming connection attempt */
            L2CA_AUTO_TP_CONNECT_IND_T *tpPrim = (L2CA_AUTO_TP_CONNECT_IND_T*)cmData->recvMsgP;
            CsrBool reject = TRUE;

            if (tpPrim->local_control == CSR_BT_AMP_CONTROLLER_BREDR)
            {
                cmL2caConnEltType connElt;

                connElt.psm = tpPrim->psm_local;
                connElt.devAddr = tpPrim->tp_addrt.addrt.addr;

                if (!cmL2caIsCrossoverConnection(cmData, connElt))
                {
                    cmL2caConnElement *theElement;
                    reject = FALSE;

                    theElement = CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromReserveBtConnIdPsmBdaddr, &connElt);

                    if (!theElement)
                    {
                        theElement = CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromReserveBtConnIdPsm, &(tpPrim->psm_local));
                    }

                    if (theElement)
                    {
                        /* lock the queue and accept the incoming connection. */
                        CsrBtCmSmLockQueue(cmData);
                        csrBtCmL2caAutoConnectIndAcceptHandler(cmData,
                                                               theElement,
                                                               tpPrim->tp_addrt.addrt.addr,
                                                               tpPrim->psm_local,
                                                               tpPrim->cid,
                                                               tpPrim->identifier);
                    }
                    else if (tpPrim->reg_ctx != CSR_BT_CM_CONTEXT_UNUSED &&
                             CsrBtCmElementCounterIncrement(cmData))
                    {
                        /* The context being used means the application has registered for handling connect indication messages.
                         * lock the sm queue and pass this to application and let it respond. */
                        CsrBtCmSmLockQueue(cmData);
                        cmL2caConnectAcceptIndSend(tpPrim);
                    }
                    else
                    { /* No applications are ready to accept a new L2CAP connection, 
                         it is therefore refused */
                        reject = TRUE;
                    } 
                }
            }

            if (reject)
            {
                csrBtCmL2caAutoConnectIndRejectHandler(cmData, tpPrim->type, tpPrim->cid, tpPrim->identifier);
            }
        }
    }

    if (msgConverted)
    {
        /* Free the converted L2CA_AUTO_TP_CONNECT_IND prim */
        CsrPmemFree(cmData->recvMsgP);
        /* Update the original received prim message */
        cmData->recvMsgP = prim;
    }
}

void CsrBtCmL2caConnectCancelCleanup(cmInstanceData_t *cmData, cmL2caConnElement * theElement)
{
    cmL2caConnInstType *l2caConnection  = theElement->cmL2caConnInst;

    if (l2caConnection->transportType == CSR_BT_TRANSPORT_BREDR &&
        CsrBtCmReturnNumOfConnectionsToPeerDevice(cmData, l2caConnection->deviceAddr))
    {
        /* Cancelling a connection is as good as a connection being disconnected, inform this to device utility. */
        (void)CmDuHandleAutomaticProcedure(cmData,
                                           CM_DU_AUTO_EVENT_SERVICE_DISCONNECTED,
                                           NULL,
                                           &l2caConnection->deviceAddr);
    }

    CsrBtCmL2caConnectCfmMsgHandler(cmData,
                                    theElement,
                                    CSR_BT_RESULT_CODE_CM_CANCELLED,
                                    CSR_BT_SUPPLIER_CM);
}

void CsrBtCmL2caConnectAcceptRspHandler(cmInstanceData_t *cmData)
{
    CsrBtCmL2caConnectAcceptRsp *prim = (CsrBtCmL2caConnectAcceptRsp *)cmData->recvMsgP;

    if (prim->accept)
    {
        CsrBtTpdAddrT tpdAddrT;
        cmL2caConnElement  *l2caConnElement;

        cmEncodeTpAddr(prim->deviceAddr, TBDADDR_PUBLIC, BREDR_ACL, &tpdAddrT);

        l2caConnElement = createL2caConnElement(cmData, prim->phandle, 0, tpdAddrT,
                              prim->localPsm, NO_REMOTE_PSM, BTCONN_ID_RESERVED,
                              0, prim->conftabCount, prim->conftab);
        /* Set Conftab to NULL, so no one should free it */
        prim->conftab = NULL;
        prim->conftabCount = 0;

        /* Mark this element as created because of application response. */
        l2caConnElement->app_controlled = TRUE;

        /* All good, accept the connection. */
        csrBtCmL2caAutoConnectIndAcceptHandler(cmData,
                                               l2caConnElement,
                                               prim->deviceAddr,
                                               prim->localPsm,
                                               CM_GET_UINT16ID_FROM_BTCONN_ID(prim->btConnId),
                                               prim->identifier);
    }
    else
    {
        /* Reject the connection. */
        csrBtCmL2caAutoConnectIndRejectHandler(cmData,
                                               L2CA_AUTO_CONNECT_IND,
                                               CM_GET_UINT16ID_FROM_BTCONN_ID(prim->btConnId),
                                               prim->identifier);

        /* Since we are rejecting the connection, unlock the service manager queue, which was locked
         * while sending CSR_BT_CM_L2CA_CONNECT_ACCEPT_IND */
        CsrBtCmServiceManagerLocalQueueHandler(cmData);
    }
}

void CmL2caTpConnectAcceptRspHandler(cmInstanceData_t *cmData)
{
    CmL2caTpConnectAcceptRsp *prim = (CmL2caTpConnectAcceptRsp *)cmData->recvMsgP;

    if (prim->accept)
    {
        cmL2caConnElement  *l2caConnElement;

        l2caConnElement = createL2caConnElement(cmData, prim->phandle, 0, prim->tpdAddrT,
                      prim->localPsm, NO_REMOTE_PSM, BTCONN_ID_RESERVED,
                      0, prim->conftabCount, prim->conftab);
        /* Set Conftab to NULL, so no one should free it */
        prim->conftab = NULL;
        prim->conftabCount = 0;

        /* Mark this element as created because of application response. */
        l2caConnElement->app_controlled = TRUE;
        /* Mark this element is created for sending TP Prim based connect response */
        l2caConnElement->useTpPrim = TRUE;

        /* All good, accept the connection. */
        csrBtCmL2caAutoConnectIndAcceptHandler(cmData,
                                            l2caConnElement,
                                            prim->tpdAddrT.addrt.addr,
                                            prim->localPsm,
                                            CM_GET_UINT16ID_FROM_BTCONN_ID(prim->btConnId),
                                            prim->identifier);
    }
    else
    {
        /* Reject the connection. */
        csrBtCmL2caAutoConnectIndRejectHandler(cmData,
                                               L2CA_AUTO_TP_CONNECT_IND,
                                               CM_GET_UINT16ID_FROM_BTCONN_ID(prim->btConnId),
                                               prim->identifier);

        /* Since we are rejecting the connection, unlock the service manager queue, which was locked
         * while sending CM_L2CA_TP_CONNECT_ACCEPT_IND */
        CsrBtCmServiceManagerLocalQueueHandler(cmData);
    }
}
#endif /* #ifndef EXCLUDE_CSR_BT_L2CA_MODULE */


/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_dm.h"

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
#include "csr_bt_cm_bnep.h"
#endif

#ifndef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
#include "csr_bt_cm_bccmd.h"
#endif

#include "csr_bt_cm_util.h"
#include "csr_bt_cm_events_handler.h"
#include "csr_bt_addr.h"

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
static void csrBtCmRfcExtractEScoParms(cmInstanceData_t *cmData, CsrBtCmSyncConnectInd *prim)
{
    cmRfcConnElement * theElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromScoHandle, &(prim->syncHandle));

    if (theElement)
    {
        cmRfcConnInstType *theLogicalLink   = theElement->cmRfcConnInst;

        if (theLogicalLink->eScoParms)
        {
            prim->txBdw           = theLogicalLink->eScoParms->txBdw;
            prim->rxBdw           = theLogicalLink->eScoParms->rxBdw;
            prim->maxLatency      = theLogicalLink->eScoParms->maxLatency;
            prim->voiceSettings   = theLogicalLink->eScoParms->voiceSettings;
            prim->reTxEffort      = theLogicalLink->eScoParms->reTxEffort;
            prim->packetType      = theLogicalLink->eScoParms->packetType;
        }
    }
}
#endif

static void csrBtCmSyncConnectIndEventMsgSend(cmInstanceData_t        *cmData,
                                              CsrSchedQid                   appHandle,
                                              CsrBool                  incoming,
                                              hci_connection_handle_t  syncHandle,
                                              CsrBtDeviceAddr          deviceAddr,
                                              CsrUint8                 linkType,
                                              CsrUint8                 txInterval,
                                              CsrUint8                 weSco,
                                              CsrUint16                rxPacketLength,
                                              CsrUint16                txPacketLength,
                                              CsrUint8                 airMode,
                                              CsrBtResultCode          resultCode,
                                              CsrBtSupplier            resultSupplier)
{
    CsrBtCmSyncConnectInd *prim;

    prim                    = (CsrBtCmSyncConnectInd *)CsrPmemZalloc(sizeof(CsrBtCmSyncConnectInd));
    prim->type              = CSR_BT_CM_SYNC_CONNECT_IND;
    prim->incoming          = incoming;
    prim->syncHandle        = syncHandle;
    prim->deviceAddr        = deviceAddr;
    prim->linkType          = linkType;
    prim->txInterval        = txInterval;
    prim->weSco             = weSco;
    prim->rxPacketLength    = rxPacketLength;
    prim->txPacketLength    = txPacketLength;
    prim->airMode           = airMode;
    prim->resultCode        = resultCode;
    prim->resultSupplier    = resultSupplier;

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    csrBtCmRfcExtractEScoParms(cmData, prim);
#endif

    CsrBtCmPutMessage(appHandle, prim);
}

static void csrBtCmSyncDisconnectIndEventMsgSend(cmInstanceData_t        *cmData,
                                                 CsrSchedQid                     appHandle,
                                                 CsrBtResultCode         resultCode,
                                                 CsrBtSupplier     resultSupplier,
                                                 hci_connection_handle_t syncHandle,
                                                 CsrBtDeviceAddr         deviceAddr,
                                                 hci_reason_t            reason)
{
    CsrBtCmSyncDisconnectInd *prim;

    prim                 = (CsrBtCmSyncDisconnectInd *)CsrPmemAlloc(sizeof(CsrBtCmSyncDisconnectInd));
    prim->type           = CSR_BT_CM_SYNC_DISCONNECT_IND;
    prim->syncHandle     = syncHandle;
    prim->deviceAddr     = deviceAddr;
    prim->reason         = reason;
    prim->resultCode     = resultCode;
    prim->resultSupplier = resultSupplier;
    CsrBtCmPutMessage(appHandle, prim);
}
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION */
#endif /* EXCLUDE_CSR_BT_SCO_MODULE */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_INITIALIZED
static void csrBtCmBlueCoreInitializedEventMsgSend(cmInstanceData_t        *cmData,
                                                   CsrSchedQid               appHandle)
{
    CsrBtCmBluecoreInitializedInd *prim;

    prim               = (CsrBtCmBluecoreInitializedInd *)CsrPmemAlloc(sizeof(CsrBtCmBluecoreInitializedInd));
    prim->type         = CSR_BT_CM_BLUECORE_INITIALIZED_IND;
    CsrBtCmPutMessage(appHandle, prim);
    CSR_UNUSED(cmData);
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_DEINITIALIZED
static void csrBtCmBlueCoreDeInitializedEventMsgSend(cmInstanceData_t       *cmData,
                                                   CsrSchedQid              appHandle,
                                                   CsrBtResultCode          resultCode,
                                                   CsrBtSupplier            resultSupplier)
{
    CsrBtCmBluecoreDeinitializedInd *prim;

    prim                    = (CsrBtCmBluecoreDeinitializedInd *)CsrPmemAlloc(sizeof(CsrBtCmBluecoreDeinitializedInd));
    prim->type              = CSR_BT_CM_BLUECORE_DEINITIALIZED_IND;
    prim->resultCode        = resultCode ;
    prim->resultSupplier    = resultSupplier ;

    CsrBtCmPutMessage(appHandle, prim);
}
#endif

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_EXT_SYNC_CONNECTION
static void csrBtCmExtSyncConnectIndEventMsgSend(cmInstanceData_t    *cmData,
                                            CsrSchedQid                   appHandle,
                                            CsrBool                  incoming,
                                            hci_connection_handle_t  syncHandle,
                                            CsrBtDeviceAddr          deviceAddr,
                                            CsrUint8                 linkType,
                                            CsrUint8                 txInterval,
                                            CsrUint8                 weSco,
                                            CsrUint8                 reservedSlots,
                                            CsrUint16                rxPacketLength,
                                            CsrUint16                txPacketLength,
                                            CsrUint8                 airMode,
                                            CsrBtResultCode          resultCode,
                                            CsrBtSupplier            resultSupplier)
{
    CsrBtCmExtSyncConnectInd *prim;

    prim                    = (CsrBtCmExtSyncConnectInd *)CsrPmemZalloc(sizeof(CsrBtCmExtSyncConnectInd));
    prim->type              = CSR_BT_CM_EXT_SYNC_CONNECT_IND;
    prim->incoming          = incoming;
    prim->syncHandle        = syncHandle;
    prim->deviceAddr        = deviceAddr;
    prim->linkType          = linkType;
    prim->txInterval        = txInterval;
    prim->weSco             = weSco;
    prim->reservedSlots     = reservedSlots;
    prim->rxPacketLength    = rxPacketLength;
    prim->txPacketLength    = txPacketLength;
    prim->airMode           = airMode;
    prim->resultCode        = resultCode;
    prim->resultSupplier    = resultSupplier;

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    {
        cmRfcConnElement * theElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromScoHandle, &(prim->syncHandle));

        if (theElement)
        {
            cmRfcConnInstType *theLogicalLink   = theElement->cmRfcConnInst;

            if (theLogicalLink->eScoParms)
            {
                prim->txBdw           = theLogicalLink->eScoParms->txBdw;
                prim->rxBdw           = theLogicalLink->eScoParms->rxBdw;
                prim->maxLatency      = theLogicalLink->eScoParms->maxLatency;
                prim->voiceSettings   = theLogicalLink->eScoParms->voiceSettings;
                prim->reTxEffort      = theLogicalLink->eScoParms->reTxEffort;
                prim->packetType      = theLogicalLink->eScoParms->packetType;
            }
        }
    }
#endif

    CsrBtCmPutMessage(appHandle, prim);
}
#endif
#endif /* EXCLUDE_CSR_BT_SCO_MODULE */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ACL_CONNECTION
static void csrBtCmAclConnectIndEventMsgSend(cmInstanceData_t *cmData,
                                             CsrSchedQid appHandle,
                                             CsrBtResultCode resultCode,
                                             CsrBtSupplier resultSupplier,
                                             CsrBtDeviceAddr deviceAddr,
                                             CsrBool incoming,
                                             CsrUint24 cod)
{
    CsrBtCmAclConnectInd *prim;

    prim                 = (CsrBtCmAclConnectInd *)CsrPmemAlloc(sizeof(CsrBtCmAclConnectInd));
    prim->type           = CSR_BT_CM_ACL_CONNECT_IND;
    prim->deviceAddr     = deviceAddr;
    prim->incoming       = incoming;
    prim->cod            = cod;
    prim->resultCode     = resultCode;
    prim->resultSupplier = resultSupplier;
    CsrBtCmPutMessage(appHandle, prim);
    CSR_UNUSED(cmData);
}

static void csrBtCmAclDisconnectIndEventMsgSend(cmInstanceData_t *cmData,
                                                CsrSchedQid appHandle,
                                                CsrBtReasonCode reasonCode,
                                                CsrBtDeviceAddr deviceAddr)
{
    CsrBtCmAclDisconnectInd *prim;

    prim                    = (CsrBtCmAclDisconnectInd *)CsrPmemAlloc(sizeof(CsrBtCmAclDisconnectInd));
    prim->type              = CSR_BT_CM_ACL_DISCONNECT_IND;
    prim->deviceAddr        = deviceAddr;
    prim->reasonCode        = reasonCode;
    prim->reasonSupplier    = CSR_BT_SUPPLIER_HCI;
    CsrBtCmPutMessage(appHandle, prim);
    CSR_UNUSED(cmData);
}
#endif

#if 0
static void csrBtCmSyncRenegotiateIndEventMsgSend(cmInstanceData_t        *cmData,
                                                  CsrSchedQid                   appHandle,
                                                  CsrBool                  incoming,
                                                  hci_connection_handle_t  syncHandle,
                                                  CsrBtDeviceAddr          deviceAddr)
{
    CsrBtCmSyncRenegotiateInd *prim;

    /* B-37386 */

    prim                = (CsrBtCmSyncRenegotiateInd *)CsrPmemAlloc(sizeof(CsrBtCmSyncRenegotiateInd));
    prim->type          = CSR_BT_CM_SYNC_RENEGOTIATE_IND;
    prim->incoming      = incoming;
    prim->syncHandle    = syncHandle;
    prim->deviceAddr    = deviceAddr;
    CsrBtCmPutMessage(appHandle, prim);
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LSTO_CHANGE
static void csrBtCmLstoChangeIndMsgSend(cmInstanceData_t        *cmData,
                                        CsrSchedQid             appHandle,
                                        CsrBtDeviceAddr          deviceAddr,
                                        CsrUint16              lsto)
{
    CsrBtCmLstoChangeInd    *cmPrim;

    cmPrim = (CsrBtCmLstoChangeInd *)CsrPmemAlloc(sizeof(CsrBtCmLstoChangeInd));

    cmPrim->type       = CSR_BT_CM_LSTO_CHANGE_IND;
    cmPrim->lsto       = lsto;
    cmPrim->deviceAddr = deviceAddr;
    CsrBtCmPutMessage(appHandle, cmPrim);
    CSR_UNUSED(cmData);
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_MODE_CHANGE
static void csrBtCmModeChangeIndMsgSend(cmInstanceData_t    *cmData,
                                        CsrSchedQid                   appHandle,
                                        CsrBtDeviceAddr          deviceAddr,
                                        CsrUint16              interval,
                                        CsrUint8               mode,
                                        CsrBtResultCode       resultCode,
                                        CsrBtSupplier   resultSupplier)
{
    CsrBtCmModeChangeInd    *cmPrim;

    cmPrim = (CsrBtCmModeChangeInd *)CsrPmemAlloc(sizeof(CsrBtCmModeChangeInd));

    cmPrim->type            = CSR_BT_CM_MODE_CHANGE_IND;
    cmPrim->interval        = interval;
    cmPrim->mode            = mode;
    cmPrim->resultCode      = resultCode;
    cmPrim->resultSupplier  = resultSupplier;
    cmPrim->deviceAddr      = deviceAddr;
    CsrBtCmPutMessage(appHandle, cmPrim);
    CSR_UNUSED(cmData);
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ROLE_CHANGE
static void csrBtCmSwitchRoleIndMsgSend(cmInstanceData_t    *cmData,
                                        CsrSchedQid                 appHandle,
                                        CsrBtDeviceAddr     deviceAddr,
                                        CsrUint8             role,
                                        CsrBtResultCode     resultCode,
                                        CsrBtSupplier resultSupplier)
{
    CsrBtCmRoleChangeInd    *cmPrim;

    cmPrim = (CsrBtCmRoleChangeInd *)CsrPmemAlloc(sizeof(CsrBtCmRoleChangeInd));

    cmPrim->type            = CSR_BT_CM_ROLE_CHANGE_IND;
    cmPrim->deviceAddr      = deviceAddr;
    cmPrim->role            = role;
    cmPrim->resultCode      = resultCode;
    cmPrim->resultSupplier  = resultSupplier;
    CsrBtCmPutMessage(appHandle, cmPrim);
    CSR_UNUSED(cmData);
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_MODE_CHANGE
static void csrBtCmSniffSubRatingIndMsgSend(cmInstanceData_t    *cmData,
                                            CsrSchedQid                   appHandle,
                                            CsrBtDeviceAddr       deviceAddr,
                                            CsrBtResultCode       resultCode,
                                            CsrBtSupplier   resultSupplier,
                                            CsrUint16              maxTxLatency,
                                            CsrUint16              maxRxLatency,
                                            CsrUint16              minRemoteTimeout,
                                            CsrUint16              minLocalTimeout)
{
    CsrBtCmSniffSubRatingInd *cmPrim;

    cmPrim = (CsrBtCmSniffSubRatingInd *)CsrPmemAlloc(sizeof(CsrBtCmSniffSubRatingInd));

    cmPrim->type                = CSR_BT_CM_SNIFF_SUB_RATING_IND;
    cmPrim->deviceAddr          = deviceAddr;
    cmPrim->resultCode          = resultCode;
    cmPrim->resultSupplier      = resultSupplier;
    cmPrim->maxTxLatency        = maxTxLatency;
    cmPrim->maxRxLatency        = maxRxLatency;
    cmPrim->minRemoteTimeout    = minRemoteTimeout;
    cmPrim->minLocalTimeout     = minLocalTimeout;
    CsrBtCmPutMessage(appHandle, cmPrim);
    CSR_UNUSED(cmData);
}
#endif

static void csrBtCmSetEventMaskCfmMsgSend(CsrSchedQid appHandle,
                                          CsrUint32 eventMask)
{
    CsrBtCmSetEventMaskCfm *cmPrim;

    cmPrim            = (CsrBtCmSetEventMaskCfm *)CsrPmemAlloc(sizeof(CsrBtCmSetEventMaskCfm));
    cmPrim->type      = CSR_BT_CM_SET_EVENT_MASK_CFM;
    cmPrim->eventMask = eventMask;
    CsrBtCmPutMessage(appHandle, cmPrim);
}


#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
static void csrBtCmLogicalChannelTypesIndSend(CsrSchedQid    appHandle,
                                    CsrBtDeviceAddr     deviceAddr,
                                    CsrBtLogicalChannelType logicalChannelTypeMask,
                                    CsrUint8            noOfGuaranteedLogicalChannels)
{
    CsrBtCmLogicalChannelTypesInd *cmPrim = (CsrBtCmLogicalChannelTypesInd *)CsrPmemAlloc(sizeof(CsrBtCmLogicalChannelTypesInd));

    cmPrim->type                                = CSR_BT_CM_LOGICAL_CHANNEL_TYPES_IND;
    cmPrim->deviceAddr                          = deviceAddr;
    cmPrim->logicalChannelTypeMask              = logicalChannelTypeMask;
    cmPrim->numberOfGuaranteedLogicalChannels   = noOfGuaranteedLogicalChannels;

    CsrBtCmPutMessage(appHandle, cmPrim);
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_REMOTE_FEATURES
static void csrBtCmRemoteFeaturesIndMsgSend(cmInstanceData_t    *cmData,
                                            CsrSchedQid              appHandle,
                                            CsrBtDeviceAddr     deviceAddr,
                                            CsrUint8            lmpFeatures[8],
                                            CsrBtResultCode     resultCode,
                                            CsrBtSupplier       resultSupplier)
{
    CsrBtCmRemoteFeaturesInd *cmPrim = (CsrBtCmRemoteFeaturesInd *)CsrPmemAlloc(sizeof(CsrBtCmRemoteFeaturesInd));

    cmPrim->type                 = CSR_BT_CM_REMOTE_FEATURES_IND;
    cmPrim->deviceAddr           = deviceAddr;
    cmPrim->resultCode           = resultCode;
    cmPrim->resultSupplier       = resultSupplier;
    SynMemCpyS(cmPrim->remoteLmpFeatures, sizeof(cmPrim->remoteLmpFeatures), lmpFeatures, sizeof(cmPrim->remoteLmpFeatures));
    CsrBtCmPutMessage(appHandle, cmPrim);
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_REMOTE_VERSION
static void csrBtCmRemoteVersionIndMsgSend(cmInstanceData_t *cmData,
                                           CsrSchedQid           appHandle,
                                           CsrBtDeviceAddr  deviceAddr,
                                           CsrUint8         lmpVersion,
                                           CsrUint16        manufacturerName,
                                           CsrUint16        lmpSubversion,
                                           CsrBtResultCode  resultCode,
                                           CsrBtSupplier    resultSupplier)
{
    CsrBtCmRemoteVersionInd *cmPrim = (CsrBtCmRemoteVersionInd *)CsrPmemAlloc(sizeof(CsrBtCmRemoteVersionInd));

    cmPrim->type                 = CSR_BT_CM_REMOTE_VERSION_IND;
    cmPrim->deviceAddr           = deviceAddr;
    cmPrim->lmpVersion           = lmpVersion;
    cmPrim->manufacturerName     = manufacturerName;
    cmPrim->lmpSubversion        = lmpSubversion;
    cmPrim->resultCode           = resultCode;
    cmPrim->resultSupplier       = resultSupplier;
    CsrBtCmPutMessage(appHandle, cmPrim);
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_A2DP_BIT_RATE
static void csrBtCmA2dpBitRateIndSend(cmInstanceData_t *cmData,
                                      CsrSchedQid            appHandle,
                                      CsrBtDeviceAddr   deviceAddr,
                                      CsrUint8          streamIdx,
                                      CsrUint32         bitRate)
{
    CsrBtCmA2dpBitRateInd *cmPrim = (CsrBtCmA2dpBitRateInd *)CsrPmemAlloc(sizeof(CsrBtCmA2dpBitRateInd));

    cmPrim->type                 = CSR_BT_CM_A2DP_BIT_RATE_IND;
    cmPrim->deviceAddr           = deviceAddr;
    cmPrim->streamIdx            = streamIdx;
    cmPrim->bitRate              = bitRate;
    CsrBtCmPutMessage(appHandle, cmPrim);
    CSR_UNUSED(cmData);
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_INQUIRY_PAGE_STATE
static void csrBtCmInquiryPageStateIndSend(cmInstanceData_t *cmData,
                                      CsrSchedQid            appHandle,
                                      CsrBtCmInquiryEventType       inquiryState,
                                      CsrBtCmPagingEventType        pageState)
{
    CsrBtCmInquiryPageEventInd *cmPrim = (CsrBtCmInquiryPageEventInd *)CsrPmemAlloc(sizeof(CsrBtCmInquiryPageEventInd));

    cmPrim->type        = CSR_BT_CM_INQUIRY_PAGE_EVENT_IND;
    cmPrim->inquiry     = inquiryState;
    cmPrim->paging      = pageState;

    CsrBtCmPutMessage(appHandle, cmPrim);
    CSR_UNUSED(cmData);
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE
static void csrBtCmEncryptChangeIndSend(cmInstanceData_t *cmData, 
                                        CsrSchedQid      appHandle,
                                        CsrBtDeviceAddr  deviceAddr,
                                        CsrBtAddressType deviceAddrType,
                                        CsrBtTransportType transportType,
                                        CsrUint16        encryptType,
                                        CsrBtResultCode  resultCode,
                                        CsrBtSupplier    resultSupplier)
{
    CsrBtCmEncryptChangeInd  *cmPrim = (CsrBtCmEncryptChangeInd *)CsrPmemAlloc(sizeof(CsrBtCmEncryptChangeInd));

    cmPrim->type            = CSR_BT_CM_ENCRYPT_CHANGE_IND;
    cmPrim->deviceAddr      = deviceAddr;
    cmPrim->deviceAddrType  = deviceAddrType;
    cmPrim->transportType   = transportType;
    cmPrim->encryptType     = encryptType;
    cmPrim->resultCode      = resultCode;  
    cmPrim->resultSupplier  = resultSupplier;   
    CsrBtCmPutMessage(appHandle, cmPrim);
    CSR_UNUSED(cmData);
}

static void csrBtCmSimplePairingCompleteIndSend(cmInstanceData_t *cmData, 
                                        CsrSchedQid      appHandle,
                                        TP_BD_ADDR_T    tp_addrt,
                                        CsrUint8        status,
                                        CsrUint16       flags)

{
    CsrBtCmSmSimplePairingCompleteInd  *cmPrim = (CsrBtCmSmSimplePairingCompleteInd *)CsrPmemAlloc(sizeof(CsrBtCmSmSimplePairingCompleteInd));

    cmPrim->type          = CSR_BT_CM_SM_SIMPLE_PAIRING_COMPLETE_IND;
    cmPrim->tp_addrt      = tp_addrt;
    cmPrim->status        = status;
    cmPrim->flags         = flags;
    CsrBtCmPutMessage(appHandle, cmPrim);
    CSR_UNUSED(cmData);
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_HIGH_PRIORITY_DATA
static void csrBtCmHighPriorityDataIndSend(cmInstanceData_t *cmData,
                                           CsrSchedQid       appHandle,
                                           CsrBtDeviceAddr  *deviceAddr,
                                           CsrBool           start)
{
    CsrBtCmHighPriorityDataInd *cmPrim = (CsrBtCmHighPriorityDataInd *)CsrPmemAlloc(sizeof(CsrBtCmHighPriorityDataInd));

    cmPrim->type                 = CSR_BT_CM_HIGH_PRIORITY_DATA_IND;
    cmPrim->deviceAddr           = *deviceAddr;
    cmPrim->start                = start;
    CsrBtCmPutMessage(appHandle, cmPrim);
    CSR_UNUSED(cmData);
}
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_HIGH_PRIORITY_DATA */


static CsrBool csrBtCmAppSubscribedToEvent(CsrCmnListElm_t *elem, void *data)
{
    subscribeParms *pSubscribeParms = (subscribeParms *) elem;
    CsrUint32 *event = (CsrUint32 *) data;

    if (pSubscribeParms->appHandle == CSR_SCHED_QID_INVALID)
    {
        return FALSE;
    }
    else
    {
        if ((*event & pSubscribeParms->eventMask) == *event)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

CsrBool CsrBtCmEventSubscribed(cmInstanceData_t *cmData,
                               CsrUint32 eventMask)
{
    if (CsrCmnListSearch((CsrCmnList_t *) &cmData->subscriptions,
                         csrBtCmAppSubscribedToEvent,
                         &eventMask))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static CsrBool csrBtCmAppCondSatisfied(CsrCmnListElm_t *elem, void *data)
{
    subscribeParms *pSubscribeParms = (subscribeParms *) elem;
    CsrBtCmEventMaskCond *condition = (CsrBtCmEventMaskCond *) data;

    if (pSubscribeParms->appHandle == CSR_SCHED_QID_INVALID)
    {
        return FALSE;
    }
    else
    {
        if ((*condition & pSubscribeParms->condition) == *condition)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}
#define CM_UINT8_PTR_TO_VAL(a) *((CsrUint8 *) (a))

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION
void CsrBtCmPropgateSyncConnectEvents(cmInstanceData_t  *cmData,
                                      CsrSchedQid             appHandle,
                                      void              *pContext1,
                                      void              *pContext2)
{
    DM_SYNC_CONNECT_COMPLETE_IND_T *dmPrim = (DM_SYNC_CONNECT_COMPLETE_IND_T *) pContext1;

    CsrUint8 incoming = CM_UINT8_PTR_TO_VAL(pContext2);

    if (dmPrim->status == HCI_SUCCESS)
    {
        csrBtCmSyncConnectIndEventMsgSend(cmData,
                                          appHandle,
                                          incoming,
                                          dmPrim->handle,
                                          dmPrim->bd_addr,
                                          dmPrim->link_type,
                                          dmPrim->tx_interval,
                                          dmPrim->wesco,
                                          dmPrim->rx_packet_length,
                                          dmPrim->tx_packet_length,
                                          dmPrim->air_mode,
                                          CSR_BT_RESULT_CODE_CM_SUCCESS,
                                          CSR_BT_SUPPLIER_CM);
    }
    else
    {
        csrBtCmSyncConnectIndEventMsgSend(cmData,
                                          appHandle,
                                          incoming,
                                          dmPrim->handle,
                                          dmPrim->bd_addr,
                                          dmPrim->link_type,
                                          dmPrim->tx_interval,
                                          dmPrim->wesco,
                                          dmPrim->rx_packet_length,
                                          dmPrim->tx_packet_length,
                                          dmPrim->air_mode,
                                          (CsrBtResultCode) dmPrim->status,
                                          CSR_BT_SUPPLIER_HCI);
    }
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_EXT_SYNC_CONNECTION
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
void CsrBtCmPropgateRfcExtSyncConnectEvents(cmInstanceData_t *cmData,
                                  CsrSchedQid             appHandle,
                                  void                  *pContext1,
                                  void                  *pContext2)
{
    cmRfcConnElement *currentElem = (cmRfcConnElement *)pContext1;

    if (currentElem->cmRfcConnInst->eScoParms)
    {
        csrBtCmExtSyncConnectIndEventMsgSend(cmData,
                                             appHandle,
                                             currentElem->cmRfcConnInst->eScoParms->incoming,
                                             currentElem->cmRfcConnInst->eScoParms->handle,
                                             currentElem->cmRfcConnInst->deviceAddr,
                                             currentElem->cmRfcConnInst->eScoParms->linkType,
                                             currentElem->cmRfcConnInst->eScoParms->txInterval,
                                             currentElem->cmRfcConnInst->eScoParms->weSco,
                                             currentElem->cmRfcConnInst->eScoParms->reservedSlots,
                                             currentElem->cmRfcConnInst->eScoParms->rxPacketLength,
                                             currentElem->cmRfcConnInst->eScoParms->txPacketLength,
                                             currentElem->cmRfcConnInst->eScoParms->airMode,
                                             CSR_BT_RESULT_CODE_CM_SUCCESS,
                                             CSR_BT_SUPPLIER_CM);
    }
}
#endif
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_EXT_SYNC_CONNECTION */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION
void CsrBtCmPropgateSyncDiscIndEvents(cmInstanceData_t *cmData,
                                      CsrSchedQid         appHandle,
                                      void              *pContext1,
                                      void              *pContext2)
{
    DM_SYNC_DISCONNECT_IND_T *dmPrim = (DM_SYNC_DISCONNECT_IND_T *) pContext1;

    CSR_UNUSED(pContext2);

    csrBtCmSyncDisconnectIndEventMsgSend(cmData,
                                         appHandle,
                                         CSR_BT_RESULT_CODE_CM_SUCCESS,
                                         CSR_BT_SUPPLIER_CM,
                                         dmPrim->handle,
                                         dmPrim->bd_addr,
                                         dmPrim->reason);
}

void CsrBtCmPropgateSyncDiscCfmEvents(cmInstanceData_t *cmData,
                                      CsrSchedQid         appHandle,
                                      void              *pContext1,
                                      void              *pContext2)
{
    DM_SYNC_DISCONNECT_CFM_T *dmPrim = (DM_SYNC_DISCONNECT_CFM_T *) pContext1;

    CSR_UNUSED(pContext2);

    if (dmPrim->status == HCI_SUCCESS)
    {
        csrBtCmSyncDisconnectIndEventMsgSend(cmData,
                                             appHandle,
                                             CSR_BT_RESULT_CODE_CM_SUCCESS,
                                             CSR_BT_SUPPLIER_CM,
                                             dmPrim->handle,
                                             dmPrim->bd_addr,
                                             HCI_ERROR_UNSPECIFIED);
    }
    else
    {
        csrBtCmSyncDisconnectIndEventMsgSend(cmData,
                                             appHandle,
                                             (CsrBtResultCode) dmPrim->status,
                                             CSR_BT_SUPPLIER_HCI,
                                             dmPrim->handle,
                                             dmPrim->bd_addr,
                                             HCI_ERROR_UNSPECIFIED);
    }
}
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION */
#endif /* EXCLUDE_CSR_BT_SCO_MODULE */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ACL_CONNECTION
void CsrBtCmPropgateAclConnectEvents(cmInstanceData_t *cmData,
                                     CsrSchedQid            appHandle,
                                     void             *pContext1,
                                     void             *pContext2)
{
    DM_ACL_OPENED_IND_T *dmPrim = (DM_ACL_OPENED_IND_T *) pContext1;

    if (dmPrim->status == HCI_SUCCESS)
    {
        csrBtCmAclConnectIndEventMsgSend(cmData,
                                         appHandle,
                                         CSR_BT_RESULT_CODE_CM_SUCCESS,
                                         CSR_BT_SUPPLIER_CM,
                                         dmPrim->addrt.addr,
                                         (CsrBool)(dmPrim->flags & DM_ACL_FLAG_INCOMING ? TRUE : FALSE),
                                         dmPrim->dev_class);
    }
    else
    {
        csrBtCmAclConnectIndEventMsgSend(cmData,
                                         appHandle,
                                         (CsrBtResultCode) dmPrim->status,
                                         CSR_BT_SUPPLIER_HCI,
                                         dmPrim->addrt.addr,
                                         (CsrBool)(dmPrim->flags & DM_ACL_FLAG_INCOMING ? TRUE : FALSE),
                                         dmPrim->dev_class);
    }
    CSR_UNUSED(pContext2);
}

void CsrBtCmPropgateAclDisconnectEvents(cmInstanceData_t *cmData,
                                        CsrSchedQid              appHandle,
                                        void             *pContext1,
                                        void             *pContext2)
{
    DM_ACL_CLOSED_IND_T *dmPrim = (DM_ACL_CLOSED_IND_T *) pContext1;

    csrBtCmAclDisconnectIndEventMsgSend(cmData,
                                        appHandle,
                                        (CsrBtReasonCode) dmPrim->reason,
                                        dmPrim->addrt.addr);
    CSR_UNUSED(pContext2);
}
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ACL_CONNECTION */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LSTO_CHANGE
void CsrBtCmPropgateLstoChangeEvents(cmInstanceData_t *cmData,
                                     CsrSchedQid             appHandle,
                                     void                  *pContext1,
                                     void                  *pContext2)
{
    DM_HCI_LINK_SUPERV_TIMEOUT_IND_T *dmPrim = (DM_HCI_LINK_SUPERV_TIMEOUT_IND_T *) pContext1;

    CSR_UNUSED(pContext2);

    csrBtCmLstoChangeIndMsgSend(cmData,
                                appHandle,
                                dmPrim->bd_addr,
                                dmPrim->timeout);
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_MODE_CHANGE
void CsrBtCmPropgateModeChangeEvents(cmInstanceData_t *cmData,
                                     CsrSchedQid                 appHandle,
                                     void                *pContext1,
                                     void                *pContext2)
{
    DM_HCI_MODE_CHANGE_EVENT_IND_T *dmPrim = (DM_HCI_MODE_CHANGE_EVENT_IND_T *) pContext1;

    CSR_UNUSED(pContext2);

    if (dmPrim->status == HCI_SUCCESS)
    {
        csrBtCmModeChangeIndMsgSend(cmData,
                                    appHandle,
                                    dmPrim->bd_addr,
                                    dmPrim->length,
                                    dmPrim->mode,
                                    CSR_BT_RESULT_CODE_CM_SUCCESS,
                                    CSR_BT_SUPPLIER_CM);
    }
    else
    {
        csrBtCmModeChangeIndMsgSend(cmData,
                                    appHandle,
                                    dmPrim->bd_addr,
                                    dmPrim->length,
                                    dmPrim->mode,
                                    (CsrBtResultCode) dmPrim->status,
                                    CSR_BT_SUPPLIER_HCI);
    }
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ROLE_CHANGE
void CsrBtCmPropgateRoleSwitchEvents(cmInstanceData_t *cmData,
                                     CsrSchedQid             appHandle,
                                     void                  *pContext1,
                                     void                  *pContext2)
{
    DM_HCI_SWITCH_ROLE_CFM_T *dmPrim = (DM_HCI_SWITCH_ROLE_CFM_T *) pContext1;

    CSR_UNUSED(pContext2);

    if (dmPrim->status == HCI_SUCCESS)
    {
        csrBtCmSwitchRoleIndMsgSend(cmData,
                                    appHandle,
                                    dmPrim->bd_addr,
                                    dmPrim->role,
                                    CSR_BT_RESULT_CODE_CM_SUCCESS,
                                    CSR_BT_SUPPLIER_CM);
    }
    else
    {
        csrBtCmSwitchRoleIndMsgSend(cmData,
                                    appHandle,
                                    dmPrim->bd_addr,
                                    dmPrim->role,
                                    (CsrBtResultCode) dmPrim->status,
                                    CSR_BT_SUPPLIER_HCI);
    }
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_MODE_CHANGE
void CsrBtCmPropgateSsrEvents(cmInstanceData_t *cmData,
                              CsrSchedQid             appHandle,
                              void                  *pContext1,
                              void                  *pContext2)
{
    DM_HCI_SNIFF_SUB_RATING_IND_T *dmPrim = (DM_HCI_SNIFF_SUB_RATING_IND_T *) pContext1;

    CSR_UNUSED(pContext2);

    if (dmPrim->status == HCI_SUCCESS)
    {
        csrBtCmSniffSubRatingIndMsgSend(cmData,
                                        appHandle,
                                        dmPrim->bd_addr,
                                        CSR_BT_RESULT_CODE_CM_SUCCESS,
                                        CSR_BT_SUPPLIER_CM,
                                        dmPrim->transmit_latency,
                                        dmPrim->receive_latency,
                                        dmPrim->remote_timeout,
                                        dmPrim->local_timeout);
    }
    else
    {
        csrBtCmSniffSubRatingIndMsgSend(cmData,
                                        appHandle,
                                        dmPrim->bd_addr,
                                        (CsrBtResultCode) dmPrim->status,
                                        CSR_BT_SUPPLIER_HCI,
                                        dmPrim->transmit_latency,
                                        dmPrim->receive_latency,
                                        dmPrim->remote_timeout,
                                        dmPrim->local_timeout);
    }
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_INITIALIZED
void CsrBtCmPropgateBlueCoreInitializedEvents(cmInstanceData_t *cmData,
                                              CsrSchedQid             appHandle,
                                              void                  *pContext1,
                                              void                  *pContext2)
{
    CSR_UNUSED(pContext1);
    CSR_UNUSED(pContext2);

    if (cmData->globalState != CSR_BT_CM_STATE_NOT_READY)
    {
        csrBtCmBlueCoreInitializedEventMsgSend(cmData, appHandle);
    }
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_DEINITIALIZED
void CsrBtCmPropgateBlueCoreDeInitializedEvents(cmInstanceData_t *cmData,
                                              CsrSchedQid        appHandle,
                                              void               *pContext1,
                                              void               *pContext2)
{
    CsrBtResultCode resultCode = *((CsrBtResultCode *) pContext1);
    CsrBtSupplier   resultSupplier = *((CsrBtSupplier *) pContext2);

    if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
    {
        csrBtCmBlueCoreDeInitializedEventMsgSend(cmData, appHandle, resultCode, resultSupplier);
    }
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
void CsrBtCmPropgateLogicalChannelTypeEvents(cmInstanceData_t *cmData,
                                  CsrSchedQid             appHandle,
                                  void               *pContext1,
                                  void               *pContext2)
{
    aclTable   *aclConElem = (aclTable *)pContext1;
    CSR_UNUSED(pContext2);

    csrBtCmLogicalChannelTypesIndSend(appHandle,aclConElem->deviceAddr,
                aclConElem->logicalChannelTypeMask,aclConElem->noOfGuaranteedLogicalChannels);
    CSR_UNUSED(cmData);
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOCAL_NAME_CHANGE
void CsrBtCmPropagateLocalNameChangeEvent(cmInstanceData_t *cmInst,
                                          CsrSchedQid appHandle,
                                          void *arg1, /* localName - copied */
                                          void *arg2) /* unused */
{
    CsrBtCmLocalNameChangeInd *ind;
    ind = CsrPmemAlloc(sizeof(CsrBtCmLocalNameChangeInd));
    ind->type = CSR_BT_CM_LOCAL_NAME_CHANGE_IND;
    ind->localName = (CsrUtf8String*)CsrStrDup((CsrCharString*)arg1);
    CsrBtCmPutMessage(appHandle, ind);
    CSR_UNUSED(cmInst);
    CSR_UNUSED(arg2);
}

static void csrBtEventsCmPropagateExistingLocalName(cmInstanceData_t *cmData, CsrSchedQid appHandle)
{
    if(cmData->dmVar.localName != NULL)
    {
        CsrBtCmPropagateLocalNameChangeEvent(cmData, appHandle,
                                             cmData->dmVar.localName,
                                             NULL);
    }
}
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOCAL_NAME_CHANGE */

#ifdef CSR_BT_LE_ENABLE
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY
void CsrBtCmPropagateLeAdvertisingEvent(cmInstanceData_t *cmInst,
                                        CsrSchedQid appHandle,
                                        void *arg1, /* ptr to uint8(event) */
                                        void *arg2) /* unused */
{
    CsrBtCmLeEventAdvertisingInd *ind = CsrPmemAlloc(sizeof(*ind));
    CsrUint32 var = *((CsrUint32*)arg1);

    ind->type = CSR_BT_CM_LE_EVENT_ADVERTISING_IND;
    ind->event = (CsrUint8)(var & 0xFF);
    ind->advType = cmInst->leVar.advType;
    ind->intervalMin = cmInst->leVar.params.adv.intervalMin;
    ind->intervalMax = cmInst->leVar.params.adv.intervalMax;
    ind->channelMap = cmInst->leVar.advChannelMap;

    CsrBtCmPutMessage(appHandle, ind);
    CSR_UNUSED(arg2);
}

void CsrBtCmPropagateLeScanEvent(cmInstanceData_t *cmInst,
                                 CsrSchedQid appHandle,
                                 void *arg1, /* CsrUint8 ptr (event) */
                                 void *arg2) /* unused */
{
    CsrBtCmLeEventScanInd *ind = CsrPmemAlloc(sizeof(*ind));
    CsrUint32 var = *((CsrUint32*)arg1);

    ind->type = CSR_BT_CM_LE_EVENT_SCAN_IND;
    ind->event = (CsrUint8)(var & 0xFF);
    ind->scanType = cmInst->leVar.scanType;
    ind->interval = cmInst->leVar.params.scan.interval;
    ind->window = cmInst->leVar.params.scan.window;

    CsrBtCmPutMessage(appHandle, ind);
    CSR_UNUSED(arg2);
}

void CsrBtCmPropagateLeConnectionEvent(cmInstanceData_t *cmInst,
                                       CsrSchedQid appHandle,
                                       void *arg1,
                                       void *arg2)
{
    CsrUint32 var = *((CsrUint32*)arg1);
    CsrUint8 hciReason = (CsrUint8)((var >> 8) & 0xFF);
    CsrUint8 type = (CsrUint8)(var & 0xFF);
    leConnVar *conn = (leConnVar *) arg2;
    CsrBtCmLeEventConnectionInd *ind = CsrPmemZalloc(sizeof(*ind));

    if (type == CSR_BT_LE_EVENT_CONNECT_SUCCESS)
    {
        ind->event = CSR_BT_CM_LE_MODE_ON;
    }
    else if (type == CSR_BT_LE_EVENT_CONNECTION_UPDATE_COMPLETE)
    { 
        ind->event = CSR_BT_CM_LE_MODE_MODIFY;
    }
    else
    { /* All other cases - CONNECT_FAIL, DISCONNECT, DISCONNECT_SYNC_TO */
        ind->event = CSR_BT_CM_LE_MODE_OFF;
    }

    ind->type           = CSR_BT_CM_LE_EVENT_CONNECTION_IND;
    ind->deviceAddr     = conn->addr;
    ind->role           = conn->master ? HCI_MASTER : HCI_SLAVE;
    ind->interval       = conn->connParams.conn_interval;
    ind->timeout        = conn->connParams.supervision_timeout;
    ind->latency        = conn->connParams.conn_latency;
    ind->accuracy       = conn->connParams.clock_accuracy;
    ind->connectStatus  = type;
    ind->reason         = hciReason;

    CsrBtCmPutMessage(appHandle, ind);
    CSR_UNUSED(cmInst);
}

static void csrBtEventsCmPropagateExistingLowEnergy(cmInstanceData_t *cmData,
                                                    CsrSchedQid appHandle)
{
    leConnVar *conn;
    CsrUint32 var = cmData->leVar.scanMode;
    
    CsrBtCmPropagateLeScanEvent(cmData,
                                appHandle,
                                &var,
                                NULL);
    var = cmData->leVar.advMode;
    CsrBtCmPropagateLeAdvertisingEvent(cmData,
                                       appHandle,
                                       &var,
                                       NULL);

    /* Traverse LE connections */
    for(conn = cmData->leVar.connCache;
        conn != NULL;
        conn = conn->next)
    {
        /* Entire DM_ACL_CONN_HANDLE_IND_T stored in cache */
        var = CSR_BT_LE_EVENT_CONNECT_SUCCESS | (HCI_SUCCESS << 8);
        CsrBtCmPropagateLeConnectionEvent(cmData,
                                          appHandle,
                                          &var,
                                          conn);
    }
}
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_SUBRATE_CHANGE
void CsrBtCmPropgateLeSubrateChangeEvent(cmInstanceData_t *cmData,
                                        CsrSchedQid              appHandle,
                                        void             *pContext1,
                                        void             *pContext2)
{
    DM_HCI_ULP_SUBRATE_CHANGE_IND_T *dmPrim = (DM_HCI_ULP_SUBRATE_CHANGE_IND_T *) pContext1;
    CsrBtCmLeSubrateChangeInd *ind = CsrPmemAlloc(sizeof(*ind));

    ind->type = CSR_BT_CM_LE_SUBRATE_CHANGE_IND;

    ind->status = (CsrBtResultCode)dmPrim->status;
    ind->addrt = dmPrim->addrt;
    ind->subrate_factor = dmPrim->subrate_factor;
    ind->peripheral_latency = dmPrim->peripheral_latency;
    ind->continuation_num = dmPrim->continuation_num;
    ind->supervision_timeout = dmPrim->supervision_timeout;

    CsrBtCmPutMessage(appHandle, ind);
    CSR_UNUSED(pContext2);
    CSR_UNUSED(cmData);
}
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_SUBRATE_CHANGE */

#ifdef INSTALL_CM_EVENT_MASK_SUBSCRIBE_ISOC_CIS_CONNECT_IND
void CmPropgateIsocCisNotifyConnectIndEvent(cmInstanceData_t *cmData,
                                        CsrSchedQid      appHandle,
                                        void             *pContext1,
                                        void             *pContext2)
{
    DM_ISOC_CIS_CONNECT_IND_T *dmPrim = (DM_ISOC_CIS_CONNECT_IND_T *) pContext1;;
    CmIsocNotifyCisConnectInd *notify = CsrPmemAlloc(sizeof(*notify));

    notify->type = CM_ISOC_NOTIFY_CIS_CONNECT_IND;
    notify->cig_id = dmPrim->cig_id;
    notify->cis_handle = dmPrim->cis_handle;
    notify->cis_id = dmPrim->cis_id;
    notify->tp_addrt.tp_type = dmPrim->tp_addrt.tp_type;
    CsrBtAddrCopy(&(notify->tp_addrt.addrt), &(dmPrim->tp_addrt.addrt));

    CsrBtCmPutMessage(appHandle, notify);
    CSR_UNUSED(pContext2);
    CSR_UNUSED(cmData);
}
#endif /* INSTALL_CM_EVENT_MASK_SUBSCRIBE_ISOC_CIS_CONNECT_IND */

#ifdef INSTALL_CM_EVENT_MASK_SUBSCRIBE_EXT_SCAN_TIMEOUT_IND
void CmPropgateExtScanTimeoutIndEvent(cmInstanceData_t *cmData,
                                        CsrSchedQid      appHandle,
                                        void             *pContext1,
                                        void             *pContext2)
{
    CmDmExtScanTimeoutInd *ind = CsrPmemAlloc(sizeof(*ind));

    ind->type = CM_DM_EXT_SCAN_TIMEOUT_IND;

    CsrBtCmPutMessage(appHandle, ind);
    CSR_UNUSED(pContext1);
    CSR_UNUSED(pContext2);
    CSR_UNUSED(cmData);
}
#endif /* INSTALL_CM_EVENT_MASK_SUBSCRIBE_EXT_SCAN_TIMEOUT_IND */
#endif /* CSR_BT_LE_ENABLE */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_OWN_ADDR_TYPE_CHANGE
void CsrBtCmPropagateLeOwnAddressTypeChangedEvent(cmInstanceData_t *cmInst,
                                                  CsrSchedQid appHandle,
                                                  void *arg1, /* unused */
                                                  void *arg2) /* unused */
{
    CsrBtCmLeOwnAddressTypeChangedInd *ind;

    ind = (CsrBtCmLeOwnAddressTypeChangedInd *) CsrPmemAlloc(sizeof(*ind));
    ind->type           = CSR_BT_CM_LE_OWN_ADDRESS_TYPE_CHANGED_IND;
    ind->addressType    = cmInst->leVar.ownAddressType;
    CsrBtCmPutMessage(appHandle, ind);
    CSR_UNUSED(arg1);
    CSR_UNUSED(arg2);
}
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_OWN_ADDR_TYPE_CHANGE */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_REMOTE_FEATURES
void CsrBtCmPropgateReadRemoteFeatureEvents(cmInstanceData_t *cmData,
                                            CsrSchedQid           appHandle,
                                            void             *pContext1,
                                            void             *pContext2)
{
    CsrUint8        lmpFeatures[8];
    CsrBtResultCode resultCode;
    CsrBtSupplier   resultSupplier;
    DM_HCI_READ_REMOTE_SUPP_FEATURES_CFM_T *dmPrim = (DM_HCI_READ_REMOTE_SUPP_FEATURES_CFM_T *) pContext1;

    CSR_UNUSED(pContext2);

    if (dmPrim->status == HCI_SUCCESS)
    {
        resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
        resultSupplier  = CSR_BT_SUPPLIER_CM;
        SynMemCpyS(lmpFeatures, sizeof(lmpFeatures), dmPrim->features, sizeof(lmpFeatures));

    }
    else
    {
        resultCode      = (CsrBtResultCode) dmPrim->status;
        resultSupplier  = CSR_BT_SUPPLIER_HCI;
        CsrMemSet(lmpFeatures, 0, sizeof(lmpFeatures));
    }
    csrBtCmRemoteFeaturesIndMsgSend(cmData,
                                    appHandle,
                                    dmPrim->bd_addr,
                                    lmpFeatures,
                                    resultCode,
                                    resultSupplier);
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_REMOTE_VERSION
void CsrBtCmPropgateReadRemoteVersionEvents(cmInstanceData_t *cmData,
                                            CsrSchedQid           appHandle,
                                            void             *pContext1,
                                            void             *pContext2)
{
    CsrBtResultCode                   resultCode;
    CsrBtSupplier                     resultSupplier;
    DM_HCI_READ_REMOTE_VER_INFO_CFM_T *dmPrim = (DM_HCI_READ_REMOTE_VER_INFO_CFM_T *) pContext1;

    CSR_UNUSED(pContext2);

    if (dmPrim->status == HCI_SUCCESS)
    {
        resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
        resultSupplier  = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        resultCode      = (CsrBtResultCode) dmPrim->status;
        resultSupplier  = CSR_BT_SUPPLIER_HCI;
    }

    csrBtCmRemoteVersionIndMsgSend(cmData,
                                   appHandle,
                                   dmPrim->tp_addrt.addrt.addr,
                                   dmPrim->LMP_version,
                                   dmPrim->manufacturer_name,
                                   dmPrim->LMP_subversion,
                                   resultCode,
                                   resultSupplier);   
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_INQUIRY_PAGE_STATE
void CsrBtCmPropagateInquiryPageEvents(cmInstanceData_t *cmData,
                                       CsrSchedQid      appHandle,
                                       void             *pContext1,
                                       void             *pContext2)
{
    CsrBtCmInquiryEventType       inquiryState = CSR_BT_CM_INQUIRY_TYPE_STOP;
    CsrBtCmPagingEventType        pageState = CSR_BT_CM_PAGE_TYPE_STOP;

    CSR_UNUSED(pContext1);
    CSR_UNUSED(pContext2);

    /* Inquiry operation is about to start or ongoing. In any case, do not indicate "STOP" */
    if (cmData->dmVar.inquiryAppState == CM_INQUIRY_APP_STATE_INQUIRING)
    {
        inquiryState = CSR_BT_CM_INQUIRY_TYPE_START;
    }

    /* check the paging state and update the status to the application */
    if (cmData->dmVar.pagingInProgress == TRUE)
    {
         pageState = CSR_BT_CM_PAGE_TYPE_START;
    }

    csrBtCmInquiryPageStateIndSend(cmData, appHandle, inquiryState, pageState);
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_A2DP_BIT_RATE
void CsrBtCmPropagateA2DPBitRateEvents(cmInstanceData_t *cmData,
                                            CsrSchedQid           appHandle,
                                            void             *pContext1,
                                            void             *pContext2)
{
    CsrBtCmA2dpBitRateReq * prim = (CsrBtCmA2dpBitRateReq *)pContext1;

    CSR_UNUSED(pContext2);

    csrBtCmA2dpBitRateIndSend(cmData,appHandle,prim->deviceAddr,prim->streamIdx,prim->bitRate);
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE
void CsrBtCmPropagateEncryptionRefreshIndStatusEvents(cmInstanceData_t *cmData,
                                                      CsrSchedQid appHandle,
                                                      void *pContext1,
                                                      void *pContext2)
{
    DM_HCI_REFRESH_ENCRYPTION_KEY_IND_T *dmPrim = (DM_HCI_REFRESH_ENCRYPTION_KEY_IND_T *) pContext1;
    CsrBtResultCode resultCode;
    CsrBtSupplier resultSupplier;
    CSR_UNUSED(pContext2);

    if (!dmPrim->status)
    {
        resultCode = CSR_BT_RESULT_CODE_CM_SUCCESS;
        resultSupplier = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        resultCode = (CsrBtResultCode) dmPrim->status;
        resultSupplier = CSR_BT_SUPPLIER_HCI;
    }

    csrBtCmEncryptChangeIndSend(cmData,
                                appHandle,
                                dmPrim->tp_addrt.addrt.addr,
                                dmPrim->tp_addrt.addrt.type,
                                dmPrim->tp_addrt.tp_type,
                                CSR_BT_CM_ENC_TYPE_KEY_REFRESH,
                                resultCode,
                                resultSupplier);
}

void CsrBtCmPropagateEncryptIndStatusEvents(cmInstanceData_t *cmData,
                                            CsrSchedQid           appHandle,
                                            void             *pContext1,
                                            void             *pContext2)
{
    DM_SM_ENCRYPTION_CHANGE_IND_T *dmPrim = (DM_SM_ENCRYPTION_CHANGE_IND_T *) pContext1;
    CSR_UNUSED(pContext2);

    csrBtCmEncryptChangeIndSend(cmData, 
                                appHandle,
                                dmPrim->tp_addrt.addrt.addr,
                                dmPrim->tp_addrt.addrt.type,
                                dmPrim->tp_addrt.tp_type,
                                dmPrim->encrypt_type,
                                CSR_BT_RESULT_CODE_CM_SUCCESS,
                                CSR_BT_SUPPLIER_CM);
}

void CsrBtCmPropagateEncryptCfmStatusEvents(cmInstanceData_t *cmData,
                                            CsrSchedQid           appHandle,
                                            void             *pContext1,
                                            void             *pContext2)
{
    CsrBtResultCode     resultCode;
    CsrBtSupplier       resultSupplier;
    DM_SM_ENCRYPT_CFM_T *dmPrim = (DM_SM_ENCRYPT_CFM_T *) pContext1;
    CSR_UNUSED(pContext2);

    if (dmPrim->success)
    {
        resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
        resultSupplier  = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        resultCode      = (CsrBtResultCode) HCI_ERROR_UNSPECIFIED;
        resultSupplier  = CSR_BT_SUPPLIER_HCI;
    }

    csrBtCmEncryptChangeIndSend(cmData, 
                                appHandle,
                                dmPrim->bd_addr,
                                TBDADDR_PUBLIC, /* Hardcoded to public as DM do not give this Info */
                                CSR_BT_TRANSPORT_LE,
                                dmPrim->encrypt_type,
                                resultCode,
                                resultSupplier);
}

void CsrBtCmPropagateSimplePairingIndStatusEvents(cmInstanceData_t *cmData,
                                            CsrSchedQid           appHandle,
                                            void             *pContext1,
                                            void             *pContext2)
{
    DM_SM_SIMPLE_PAIRING_COMPLETE_IND_T *dmPrim = (DM_SM_SIMPLE_PAIRING_COMPLETE_IND_T *) pContext1;
    CSR_UNUSED(pContext2);

    csrBtCmSimplePairingCompleteIndSend(cmData, 
                                appHandle,
                                dmPrim->tp_addrt,
                                dmPrim->status,
                                dmPrim->flags);
}
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_HIGH_PRIORITY_DATA
void CsrBtCmPropagateHighPriorityIndStatusEvents(cmInstanceData_t *cmData,
                                                 CsrSchedQid       appHandle,
                                                 void             *pContext1,
                                                 void             *pContext2)
{
    CsrBtDeviceAddr *deviceAddr = ((CsrBtDeviceAddr *) pContext1);
    CsrBool start = *((CsrBool *) pContext2);

    csrBtCmHighPriorityDataIndSend(cmData,
                                   appHandle,
                                   deviceAddr,
                                   start);
}
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_HIGH_PRIORITY_DATA */

static CsrBtCmEventMaskCond csrBtCmMapHciStatusToCondType(CsrUint8 status)
{
    if (status == HCI_SUCCESS)
    {
        return CSR_BT_CM_EVENT_MASK_COND_SUCCESS;
    }
    else
    {
        return CSR_BT_CM_EVENT_MASK_COND_UNKNOWN;
    }
}

typedef struct
{
    cmInstanceData_t *cmData;
    cmEventHandlerFuncType eventHandler;
    CsrUint32 eventMask;
    CsrBtCmEventMaskCond condition;
    void *pContext1;
    void *pContext2;
} csrBtCmEventParams;

static void propgateEvent(CsrCmnListElm_t *elem, void *data)
{
    csrBtCmEventParams *eventParams = (csrBtCmEventParams *) data;

    if (csrBtCmAppSubscribedToEvent(elem, &eventParams->eventMask))
    {
        if (csrBtCmAppCondSatisfied(elem, &eventParams->condition))
        {
            subscribeParms *pSubscribeParms = (subscribeParms *) elem;
            eventParams->eventHandler(eventParams->cmData,
                                      pSubscribeParms->appHandle,
                                      eventParams->pContext1,
                                      eventParams->pContext2);
        }
    }
}

void CsrBtCmPropgateEvent(cmInstanceData_t *cmData,
                          cmEventHandlerFuncType eventHandler,
                          CsrUint32 eventMask,
                          CsrUint8 status,
                          void *pContext1,
                          void *pContext2)
{
    csrBtCmEventParams eventParams;

    eventParams.cmData = cmData;
    eventParams.eventHandler = eventHandler;
    eventParams.eventMask = eventMask;
    eventParams.condition = csrBtCmMapHciStatusToCondType(status);
    eventParams.pContext1 = pContext1;
    eventParams.pContext2 = pContext2;

    CsrCmnListIterate((CsrCmnList_t *) &cmData->subscriptions,
                      propgateEvent,
                      &eventParams);
}

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_INITIALIZED
static void csrBtCmPropagateExistingBlueCoreInit(cmInstanceData_t *cmData, CsrSchedQid appHandle)
{
    CsrBtCmPropgateBlueCoreInitializedEvents(cmData,
                                             appHandle,
                                             NULL,
                                             NULL);
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_DEINITIALIZED
static void csrBtCmPropagateExistingBlueCoreDeInit(cmInstanceData_t     *cmData,
                                                    CsrSchedQid         appHandle,
                                                    CsrBtResultCode     resultCode,
                                                    CsrBtSupplier       resultSupplier)
{
    CsrBtCmPropgateBlueCoreDeInitializedEvents(cmData,
                                               appHandle,
                                               &resultCode,
                                               &resultSupplier
                                               );
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ACL_CONNECTION
static void csrBtCmPropagateExistingAclConns(cmInstanceData_t *cmData, CsrSchedQid appHandle)
{
    CsrUintFast8 i =0;

    for (i = 0; i < NUM_OF_ACL_CONNECTION; i++)
    {
        if(!CsrBtBdAddrEqZero(&cmData->roleVar.aclVar[i].deviceAddr))
        {
            csrBtCmAclConnectIndEventMsgSend(cmData,
                                             appHandle,
                                             CSR_BT_RESULT_CODE_CM_SUCCESS,
                                             CSR_BT_SUPPLIER_CM,
                                             cmData->roleVar.aclVar[i].deviceAddr,
                                             cmData->roleVar.aclVar[i].incoming,
                                             cmData->roleVar.aclVar[i].cod);
        }
    }
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_MODE_CHANGE
static void csrBtCmPropagateExistingSsr(cmInstanceData_t *cmData, CsrSchedQid appHandle)
{
    CsrUintFast8 i =0;

    for (i = 0; i < NUM_OF_ACL_CONNECTION; i++)
    {
        if(!CsrBtBdAddrEqZero(&cmData->roleVar.aclVar[i].deviceAddr) &&
           cmData->roleVar.aclVar[i].curSsrSettings.valid)
        {
            csrBtCmSniffSubRatingIndMsgSend(cmData,
                                            appHandle,
                                            cmData->roleVar.aclVar[i].deviceAddr,
                                            CSR_BT_RESULT_CODE_CM_SUCCESS,
                                            CSR_BT_SUPPLIER_CM,
                                            cmData->roleVar.aclVar[i].curSsrSettings.maxTxLatency,
                                            cmData->roleVar.aclVar[i].curSsrSettings.maxRxLatency,
                                            cmData->roleVar.aclVar[i].curSsrSettings.minRemoteTimeout,
                                            cmData->roleVar.aclVar[i].curSsrSettings.minLocalTimeout);
        }
    }
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ROLE_CHANGE
static void csrBtCmPropagateExistingRole(cmInstanceData_t *cmData, CsrSchedQid appHandle)
{
    CsrUintFast8 i =0;

    for (i = 0; i < NUM_OF_ACL_CONNECTION; i++)
    {
        if(!CsrBtBdAddrEqZero(&cmData->roleVar.aclVar[i].deviceAddr))
        {
            csrBtCmSwitchRoleIndMsgSend(cmData,
                                        appHandle,
                                        cmData->roleVar.aclVar[i].deviceAddr,
                                        cmData->roleVar.aclVar[i].role,
                                        CSR_BT_RESULT_CODE_CM_SUCCESS,
                                        CSR_BT_SUPPLIER_CM);
        }
    }
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LSTO_CHANGE
static void csrBtCmPropagateExistingLsto(cmInstanceData_t *cmData, CsrSchedQid appHandle)
{
    CsrUintFast8 i =0;

    for (i = 0; i < NUM_OF_ACL_CONNECTION; i++)
    {
        if(!CsrBtBdAddrEqZero(&cmData->roleVar.aclVar[i].deviceAddr))
        {
            csrBtCmLstoChangeIndMsgSend(cmData,
                                        appHandle,
                                        cmData->roleVar.aclVar[i].deviceAddr,
                                        cmData->roleVar.aclVar[i].lsto);
        }
    }
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_MODE_CHANGE
static void csrBtCmPropagateExistingMode(cmInstanceData_t *cmData, CsrSchedQid appHandle)
{
    CsrUintFast8 i =0;

    for (i = 0; i < NUM_OF_ACL_CONNECTION; i++)
    {
        if(!CsrBtBdAddrEqZero(&cmData->roleVar.aclVar[i].deviceAddr))
        {
            csrBtCmModeChangeIndMsgSend(cmData,
                                        appHandle,
                                        cmData->roleVar.aclVar[i].deviceAddr,
                                        cmData->roleVar.aclVar[i].interval,
                                        cmData->roleVar.aclVar[i].mode,
                                        CSR_BT_RESULT_CODE_CM_SUCCESS,
                                        CSR_BT_SUPPLIER_CM);
        }
    }
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_REMOTE_FEATURES
static void csrBtCmPropagateExistingRemoteFeatures(cmInstanceData_t *cmData, CsrSchedQid appHandle)
{
    CsrUintFast8 i =0;

    for (i = 0; i < NUM_OF_ACL_CONNECTION; i++)
    {
        if(!CsrBtBdAddrEqZero(&cmData->roleVar.aclVar[i].deviceAddr) &&
           cmData->roleVar.aclVar[i].remoteFeaturesValid)
        {
            csrBtCmRemoteFeaturesIndMsgSend(cmData,
                                            appHandle,
                                            cmData->roleVar.aclVar[i].deviceAddr,
                                            cmData->roleVar.aclVar[i].remoteFeatures,
                                            CSR_BT_RESULT_CODE_CM_SUCCESS,
                                            CSR_BT_SUPPLIER_CM);
        }
    }
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_REMOTE_VERSION
static void csrBtCmPropagateExistingRemoteVersion(cmInstanceData_t *cmData, CsrSchedQid appHandle)
{
    CsrUintFast8 i = 0;

    for (i = 0; i < NUM_OF_ACL_CONNECTION; i++)
    {
        if(!CsrBtBdAddrEqZero(&cmData->roleVar.aclVar[i].deviceAddr) &&
           cmData->roleVar.aclVar[i].lmpVersion != CSR_BT_CM_INVALID_LMP_VERSION)
        {
            csrBtCmRemoteVersionIndMsgSend(cmData,
                                           appHandle,
                                           cmData->roleVar.aclVar[i].deviceAddr,
                                           cmData->roleVar.aclVar[i].lmpVersion,
                                           cmData->roleVar.aclVar[i].manufacturerName,
                                           cmData->roleVar.aclVar[i].lmpSubversion,
                                           CSR_BT_RESULT_CODE_CM_SUCCESS,
                                           CSR_BT_SUPPLIER_CM);
        }
    }
}
#endif

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_A2DP_BIT_RATE
static void csrBtCmPropagateA2DPBitRate(cmInstanceData_t *cmData, CsrSchedQid appHandle)
{
    CsrUintFast8 i =0;
    
    for (i = 0; i < NUM_OF_ACL_CONNECTION; i++)
    {
        if(!CsrBtBdAddrEqZero(&cmData->roleVar.aclVar[i].deviceAddr))
        {
            /* Now check all the L2CAP connections to the given bd address */
            cmL2caConnElement *currentElem;
            CsrUint8 strIdx = 0;

            for (currentElem = CM_L2CA_GET_FIRST(cmData->l2caVar.connList); currentElem; currentElem = currentElem->next)
            { /* Search through the L2CAP connection list                     */
                if (currentElem->cmL2caConnInst)
                {
                    if ((currentElem->cmL2caConnInst->state == CSR_BT_CM_L2CAP_STATE_CONNECTED) &&
                        (CsrBtBdAddrEq(&(cmData->roleVar.aclVar[i].deviceAddr), &(currentElem->cmL2caConnInst->deviceAddr))) )
                    {
                        if (currentElem->cmL2caConnInst->logicalChannelStream)
                        {/* A2DP stream channels are only possible in L2CAP connections */
                            csrBtCmA2dpBitRateIndSend(cmData, appHandle,
                                        cmData->roleVar.aclVar[i].deviceAddr,
                                        strIdx,
                                        CSR_BT_A2DP_BIT_RATE_UNKNOWN);
                            strIdx++;
                        }
                    }
                }
            }            
        }
    }
}
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_A2DP_BIT_RATE */
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE
static void csrBtCmPropagateExistingEncryptStatus(cmInstanceData_t *cmData, CsrSchedQid appHandle)
{
    CsrUintFast8 i =0;

    for (i = 0; i < NUM_OF_ACL_CONNECTION; i++)
    {
        if(!CsrBtBdAddrEqZero(&cmData->roleVar.aclVar[i].deviceAddr))
        {
            csrBtCmEncryptChangeIndSend(cmData, 
                                        appHandle,
                                        cmData->roleVar.aclVar[i].deviceAddr,
                                        TBDADDR_PUBLIC, /* Hardcoded to public as CM only save info about BrEdr Conn */
                                        CSR_BT_TRANSPORT_BREDR, /* Hardcoded to BR/EDR as CM only save info about BrEdr Conn */
                                        cmData->roleVar.aclVar[i].encryptType,
                                        CSR_BT_RESULT_CODE_CM_SUCCESS,
                                        CSR_BT_SUPPLIER_CM);
        }
    }
}
#endif

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION
static void csrBtCmRfcPropagateExistingSyncConns(cmInstanceData_t *cmData, CsrSchedQid appHandle)
{
    cmRfcConnElement *currentElem;

    for (currentElem = CM_RFC_GET_FIRST(cmData->rfcVar.connList); currentElem; currentElem = currentElem->next)
    { /* Search through the RFC connection table                                */
        if (currentElem->cmRfcConnInst)
        {
            if (currentElem->cmRfcConnInst->state == CSR_BT_CM_RFC_STATE_CONNECTED)
            {
                if (currentElem->cmRfcConnInst->eScoParms &&
                    currentElem->cmRfcConnInst->eScoParms->handle != NO_SCO &&
                    currentElem->cmRfcConnInst->eScoParms->handle != SCOBUSY_ACCEPT)
                {
                    csrBtCmSyncConnectIndEventMsgSend(cmData,
                                                      appHandle,
                                                      currentElem->cmRfcConnInst->eScoParms->incoming,
                                                      currentElem->cmRfcConnInst->eScoParms->handle,
                                                      currentElem->cmRfcConnInst->deviceAddr,
                                                      currentElem->cmRfcConnInst->eScoParms->linkType,
                                                      currentElem->cmRfcConnInst->eScoParms->txInterval,
                                                      currentElem->cmRfcConnInst->eScoParms->weSco,
                                                      currentElem->cmRfcConnInst->eScoParms->rxPacketLength,
                                                      currentElem->cmRfcConnInst->eScoParms->txPacketLength,
                                                      currentElem->cmRfcConnInst->eScoParms->airMode,
                                                      CSR_BT_RESULT_CODE_CM_SUCCESS,
                                                      CSR_BT_SUPPLIER_CM);
                }
            }
        }
    }
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_EXT_SYNC_CONNECTION
static void csrBtCmRfcPropagateExistingExtSyncConns(cmInstanceData_t *cmData, CsrSchedQid appHandle)
{
    cmRfcConnElement *currentElem;

    for (currentElem = CM_RFC_GET_FIRST(cmData->rfcVar.connList); currentElem; currentElem = currentElem->next)
    { /* Search through the RFC connection table                                */
        if (currentElem->cmRfcConnInst)
        {
            if (currentElem->cmRfcConnInst->state == CSR_BT_CM_RFC_STATE_CONNECTED)
            {
                if (currentElem->cmRfcConnInst->eScoParms &&
                    currentElem->cmRfcConnInst->eScoParms->handle != NO_SCO &&
                    currentElem->cmRfcConnInst->eScoParms->handle != SCOBUSY_ACCEPT)
                {
                    csrBtCmExtSyncConnectIndEventMsgSend(cmData,
                                             appHandle,
                                             currentElem->cmRfcConnInst->eScoParms->incoming,
                                             currentElem->cmRfcConnInst->eScoParms->handle,
                                             currentElem->cmRfcConnInst->deviceAddr,
                                             currentElem->cmRfcConnInst->eScoParms->linkType,
                                             currentElem->cmRfcConnInst->eScoParms->txInterval,
                                             currentElem->cmRfcConnInst->eScoParms->weSco,
                                             currentElem->cmRfcConnInst->eScoParms->reservedSlots,
                                             currentElem->cmRfcConnInst->eScoParms->rxPacketLength,
                                             currentElem->cmRfcConnInst->eScoParms->txPacketLength,
                                             currentElem->cmRfcConnInst->eScoParms->airMode,
                                             CSR_BT_RESULT_CODE_CM_SUCCESS,
                                             CSR_BT_SUPPLIER_CM);
                }
            }
        }
    }
}
#endif
#endif
#endif /* EXCLUDE_CSR_BT_SCO_MODULE */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
static void csrBtCmPropagateLogicalChannelType(cmInstanceData_t *cmData, CsrSchedQid appHandle)
{
    CsrUintFast8 i =0;
    aclTable    *aclConnectionElement = NULL;

    for (i = 0; i < NUM_OF_ACL_CONNECTION; i++)
    {
        if(!CsrBtBdAddrEqZero(&cmData->roleVar.aclVar[i].deviceAddr))
        {
            aclConnectionElement = &(cmData->roleVar.aclVar[i]);
            csrBtCmLogicalChannelTypesIndSend(appHandle,
                                    cmData->roleVar.aclVar[i].deviceAddr,
                                    aclConnectionElement->logicalChannelTypeMask,
                                    aclConnectionElement->noOfGuaranteedLogicalChannels);
        }
    }
}
#endif

static void csrBtCmPropogateAllNewEvents(cmInstanceData_t *cmData, CsrSchedQid appHandle, CsrUint32 oldMask, CsrUint32 newMask)
{
    CsrUintFast8 i;
    CsrUint32 diffMask;

    if (oldMask == CSR_BT_CM_EVENT_MASK_SUBSCRIBE_NONE)
    {
        diffMask = newMask;
    }
    else
    {
        diffMask = ~oldMask & newMask;
    }

    for (i = 0; i < CSR_BT_CM_NUM_OF_CM_EVENTS; i++)
    {
        if (((diffMask>>i) & 0x000000001) != 0)
        {
            switch (diffMask & (1<<i))
            {
#ifndef EXCLUDE_CSR_BT_SCO_MODULE
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION:
                    {

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
                        csrBtCmRfcPropagateExistingSyncConns(cmData, appHandle);
#endif
                        break;
                    }
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION */
#endif /* EXCLUDE_CSR_BT_SCO_MODULE */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ACL_CONNECTION
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ACL_CONNECTION:
                    {
                        csrBtCmPropagateExistingAclConns(cmData, appHandle);
                        break;
                    }
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ROLE_CHANGE
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ROLE_CHANGE:
                {
                        csrBtCmPropagateExistingRole(cmData, appHandle);
                        break;
                    }
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_MODE_CHANGE
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_MODE_CHANGE:
                    {
                        csrBtCmPropagateExistingMode(cmData, appHandle);
                        csrBtCmPropagateExistingSsr(cmData, appHandle);
                        break;
                    }
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LSTO_CHANGE
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LSTO_CHANGE:
                    {
                        csrBtCmPropagateExistingLsto(cmData, appHandle);
                        break;
                    }
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_INITIALIZED
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_INITIALIZED:
                {
                    csrBtCmPropagateExistingBlueCoreInit(cmData, appHandle);
                    break;
                }
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE:
                {
                    csrBtCmPropagateLogicalChannelType(cmData, appHandle);
                    break;
                }
#endif

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_EXT_SYNC_CONNECTION
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_EXT_SYNC_CONNECTION:
                {
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
                    csrBtCmRfcPropagateExistingExtSyncConns(cmData, appHandle);
#endif
                    break;
                }
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_EXT_SYNC_CONNECTION */
#endif /* EXCLUDE_CSR_BT_SCO_MODULE */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_REMOTE_FEATURES
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_REMOTE_FEATURES:
                {
                    csrBtCmPropagateExistingRemoteFeatures(cmData, appHandle);
                    break;
                }
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_REMOTE_VERSION
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_REMOTE_VERSION:
                {
                    csrBtCmPropagateExistingRemoteVersion(cmData, appHandle);
                    break;
                }
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_A2DP_BIT_RATE
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_A2DP_BIT_RATE:
                {
#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
                    /* A2DP streams are only possible on L2CAP channels. */
                    csrBtCmPropagateA2DPBitRate(cmData, appHandle);
#endif
                    break;
                }
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_A2DP_BIT_RATE */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_INQUIRY_PAGE_STATE
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_INQUIRY_PAGE_STATE:
                {
                    CsrBtCmPropagateInquiryPageEvents(cmData, appHandle, NULL, NULL);
                    break;
                }
#endif

#ifdef CSR_BT_LE_ENABLE
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY:
                {
                    csrBtEventsCmPropagateExistingLowEnergy(cmData, appHandle);
                    break;
                }
#endif
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOCAL_NAME_CHANGE
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOCAL_NAME_CHANGE:
                {
                    csrBtEventsCmPropagateExistingLocalName(cmData, appHandle);
                    break;
                }
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE:
                {
                    csrBtCmPropagateExistingEncryptStatus(cmData, appHandle);
                    break;
                }
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_HIGH_PRIORITY_DATA
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_HIGH_PRIORITY_DATA:
                {
                    /* Do nothing */
                    break;
                }
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_DEINITIALIZED
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_DEINITIALIZED:
                {
                    csrBtCmPropagateExistingBlueCoreDeInit(cmData, appHandle,
                                CSR_BT_RESULT_CODE_CM_SUCCESS, CSR_BT_SUPPLIER_CM);
                    break;
                }
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_OWN_ADDR_TYPE_CHANGE
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LE_OWN_ADDR_TYPE_CHANGE:
                {
                    CsrBtCmPropagateLeOwnAddressTypeChangedEvent(cmData,
                                                                 appHandle,
                                                                 NULL,
                                                                 NULL);
                    break;
                }
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_OWN_ADDR_TYPE_CHANGE */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_SUBRATE_CHANGE
                case CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LE_SUBRATE_CHANGE:
                {
                    /* Do nothing */
                    break;
                }
#endif
                default:
                {
                    break;
                }
            }
        }
        else
        {
            ;
        }
    }
}

void CsrBtCmStoreDownstreamEScoParms(eScoParmVars *eScoParms,
                                     CsrBool     incoming,
                                     CsrUint8    linkType,
                                     CsrUint8    txInterval,
                                     CsrUint8    weSco,
                                     CsrUint16   rxPacketLength,
                                     CsrUint16   txPacketLength,
                                     CsrUint8    airMode,
                                     CsrUint8    status)
{
    if (eScoParms)
    {
        eScoParms->incoming = incoming;

        if (status == HCI_SUCCESS)
        {
            eScoParms->linkType       = linkType;
            eScoParms->txInterval     = txInterval;
            eScoParms->weSco          = weSco;
            eScoParms->rxPacketLength = rxPacketLength;
            eScoParms->txPacketLength = txPacketLength;
            eScoParms->airMode        = airMode;
        }
    }
}

void CsrBtCmSetEventMaskReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSetEventMaskReq *prim = (CsrBtCmSetEventMaskReq *) cmData->recvMsgP;
    subscribeParms *pSubscribeParms;
    CsrUint32 newEventMask;

    CsrUint32 eventMask = prim->eventMask & CSR_BT_CM_EVENT_MASK_RESERVER_VALUES_MASK;

    pSubscribeParms = (subscribeParms *) CsrCmnListSearchOffsetUint16((CsrCmnList_t *) &cmData->subscriptions,
                                                                      CsrOffsetOf(subscribeParms,
                                                                                  appHandle),
                                                                      prim->phandle);

    if (eventMask == CSR_BT_CM_EVENT_MASK_SUBSCRIBE_NONE)
    {
        /* The app has requested to unsubscribe from all current event */
        if (pSubscribeParms)
        {
            CsrCmnListElementRemove((CsrCmnList_t *) &cmData->subscriptions,
                                    (CsrCmnListElm_t *) pSubscribeParms);
        }
        csrBtCmSetEventMaskCfmMsgSend(prim->phandle, eventMask);
    }
    else
    {
        if (!pSubscribeParms)
        {
            /* Unknown app, so create a new instance to store the subscription */
            pSubscribeParms = (subscribeParms *) CsrCmnListElementAddLast((CsrCmnList_t *) &cmData->subscriptions,
                                                                          sizeof(subscribeParms));

            pSubscribeParms->eventMask = CSR_BT_CM_EVENT_MASK_SUBSCRIBE_NONE;
        }

        pSubscribeParms->condition = prim->conditionMask;
        pSubscribeParms->appHandle = prim->phandle;

        newEventMask = eventMask | pSubscribeParms->eventMask;

        csrBtCmSetEventMaskCfmMsgSend(prim->phandle, newEventMask);

        csrBtCmPropogateAllNewEvents(cmData,
                                     prim->phandle,
                                     pSubscribeParms->eventMask,
                                     eventMask);

        pSubscribeParms->eventMask = newEventMask;
    }
}

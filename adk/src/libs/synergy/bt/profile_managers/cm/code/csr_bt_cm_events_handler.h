#ifndef CM_EVENTS_HANDLER__
#define CM_EVENTS_HANDLER__
/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/


#include "csr_synergy.h"
#include "csr_bt_cm_main.h"
#include "hci_prim.h"
#include "bluetooth.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*cmEventHandlerFuncType)(cmInstanceData_t *cmData, CsrSchedQid    appHandle, void *pContext1, void *pContext2);

void CsrBtCmSetEventMaskReqHandler(cmInstanceData_t *cmData);
CsrBool CsrBtCmEventSubscribed(cmInstanceData_t        *cmData,
                               CsrUint32               eventMask);

void CsrBtCmPropgateEvent(cmInstanceData_t             *cmData,
                         cmEventHandlerFuncType    eventHandler,
                         CsrUint32                 eventMask,
                         CsrUint8                status,
                         void                     *pContext1,
                         void                    *pContext2);

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION
void CsrBtCmPropgateSyncConnectEvents(cmInstanceData_t *cmData,
                                          CsrSchedQid         appHandle,
                                          void              *pContext1,
                                          void              *pContext2);

void CsrBtCmPropgateSyncDiscIndEvents(cmInstanceData_t *cmData,
                                             CsrSchedQid         appHandle,
                                             void              *pContext1,
                                             void              *pContext2);

void CsrBtCmPropgateSyncDiscCfmEvents(cmInstanceData_t *cmData,
                                          CsrSchedQid         appHandle,
                                          void              *pContext1,
                                          void              *pContext2);
#endif
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ACL_CONNECTION
void CsrBtCmPropgateAclConnectEvents(cmInstanceData_t *cmData,
                                  CsrSchedQid             appHandle,
                                  void                  *pContext1,
                                  void                  *pContext2);

void CsrBtCmPropgateAclDisconnectEvents(cmInstanceData_t *cmData,
                                  CsrSchedQid             appHandle,
                                  void                  *pContext1,
                                  void                  *pContext2);
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LSTO_CHANGE
void CsrBtCmPropgateLstoChangeEvents(cmInstanceData_t *cmData,
                                  CsrSchedQid             appHandle,
                                  void                  *pContext1,
                                  void                  *pContext2);
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_MODE_CHANGE
void CsrBtCmPropgateModeChangeEvents(cmInstanceData_t *cmData,
                                  CsrSchedQid             appHandle,
                                  void                  *pContext1,
                                  void                  *pContext2);

void CsrBtCmPropgateSsrEvents(cmInstanceData_t *cmData,
                                  CsrSchedQid             appHandle,
                                  void                  *pContext1,
                                  void                  *pContext2);
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ROLE_CHANGE
void CsrBtCmPropgateRoleSwitchEvents(cmInstanceData_t *cmData,
                                  CsrSchedQid             appHandle,
                                  void                  *pContext1,
                                  void                  *pContext2);
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_INITIALIZED
void CsrBtCmPropgateBlueCoreInitializedEvents(cmInstanceData_t *cmData,
                                  CsrSchedQid             appHandle,
                                  void                  *pContext1,
                                  void                  *pContext2);
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_DEINITIALIZED
void CsrBtCmPropgateBlueCoreDeInitializedEvents(cmInstanceData_t *cmData,
                                              CsrSchedQid        appHandle,
                                              void               *pContext1,
                                              void               *pContext2);
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
void CsrBtCmPropgateLogicalChannelTypeEvents(cmInstanceData_t *cmData,
                                  CsrSchedQid             appHandle,
                                  void                  *pContext1,
                                  void                  *pContext2);
#endif

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_EXT_SYNC_CONNECTION
#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
void CsrBtCmPropgateL2CaExtSyncConnectEvents(cmInstanceData_t *cmData,
                                  CsrSchedQid             appHandle,
                                  void                  *pContext1,
                                  void                  *pContext2);
#endif

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
void CsrBtCmPropgateRfcExtSyncConnectEvents(cmInstanceData_t *cmData,
                                  CsrSchedQid             appHandle,
                                  void                  *pContext1,
                                  void                  *pContext2);
#endif
#endif
#endif /* EXCLUDE_CSR_BT_SCO_MODULE */

void CsrBtCmStoreDownstreamEScoParms(eScoParmVars *eScoParms,
                               CsrBool     incoming,
                               CsrUint8    linkType,
                               CsrUint8    txInterval,
                               CsrUint8    weSco,
                               CsrUint16   rxPacketLength,
                               CsrUint16   txPacketLength,
                               CsrUint8    airMode,
                               CsrUint8    status);

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_REMOTE_FEATURES
void CsrBtCmPropgateReadRemoteFeatureEvents(cmInstanceData_t *cmData,
                                            CsrSchedQid           appHandle,
                                            void             *pContext1,
                                            void             *pContext2);
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_REMOTE_VERSION
void CsrBtCmPropgateReadRemoteVersionEvents(cmInstanceData_t *cmData,
                                            CsrSchedQid           appHandle,
                                            void             *pContext1,
                                            void             *pContext2);
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_INQUIRY_PAGE_STATE
void CsrBtCmPropagateInquiryPageEvents(cmInstanceData_t *cmData,
                                            CsrSchedQid           appHandle,
                                            void             *pContext1,
                                            void             *pContext2);
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_A2DP_BIT_RATE
void CsrBtCmPropagateA2DPBitRateEvents(cmInstanceData_t *cmData,
                                            CsrSchedQid           appHandle,
                                            void             *pContext1,
                                            void             *pContext2);
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE
void CsrBtCmPropagateEncryptionRefreshIndStatusEvents(cmInstanceData_t *cmData,
                                                      CsrSchedQid appHandle,
                                                      void *pContext1,
                                                      void *pContext2);
void CsrBtCmPropagateEncryptIndStatusEvents(cmInstanceData_t *cmData,
                                            CsrSchedQid           appHandle,
                                            void             *pContext1,
                                            void             *pContext2);

void CsrBtCmPropagateEncryptCfmStatusEvents(cmInstanceData_t *cmData,
                                            CsrSchedQid           appHandle,
                                            void             *pContext1,
                                            void             *pContext2);

void CsrBtCmPropagateSimplePairingIndStatusEvents(cmInstanceData_t *cmData,
                                            CsrSchedQid           appHandle,
                                            void             *pContext1,
                                            void             *pContext2);
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOCAL_NAME_CHANGE
void CsrBtCmPropagateLocalNameChangeEvent(cmInstanceData_t *cmInst,
                                          CsrSchedQid appHandle,
                                          void *arg1,
                                          void *arg2);
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_HIGH_PRIORITY_DATA
void CsrBtCmPropagateHighPriorityIndStatusEvents(cmInstanceData_t *cmData,
                                                 CsrSchedQid       appHandle,
                                                 void             *pContext1,
                                                 void             *pContext2);
#endif

#ifdef CSR_BT_LE_ENABLE
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY
void CsrBtCmPropagateLeAdvertisingEvent(cmInstanceData_t *cmInst,
                                        CsrSchedQid appHandle,
                                        void *arg1, /* CsrUint8 ptr (event) */
                                        void *arg2); /* unused */

void CsrBtCmPropagateLeScanEvent(cmInstanceData_t *cmInst,
                                 CsrSchedQid appHandle,
                                 void *arg1, /* CsrUint8 ptr (event) */
                                 void *arg2); /* unused */

void CsrBtCmPropagateLeConnectionEvent(cmInstanceData_t *cmInst,
                                       CsrSchedQid appHandle,
                                       void *arg1,
                                       void *arg2); /* ptr to DM_ACL_OPENED/CLOSED_IND */
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_SUBRATE_CHANGE
void CsrBtCmPropgateLeSubrateChangeEvent(cmInstanceData_t *cmData,
                                        CsrSchedQid      appHandle,
                                        void             *pContext1,
                                        void             *pContext2);
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_SUBRATE_CHANGE */

#ifdef INSTALL_CM_EVENT_MASK_SUBSCRIBE_ISOC_CIS_CONNECT_IND
void CmPropgateIsocCisNotifyConnectIndEvent(cmInstanceData_t *cmData,
                                      CsrSchedQid      appHandle,
                                      void             *pContext1,
                                      void             *pContext2);
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ISOC_CIS_CONNECT_IND */

#ifdef INSTALL_CM_EVENT_MASK_SUBSCRIBE_EXT_SCAN_TIMEOUT_IND
void CmPropgateExtScanTimeoutIndEvent(cmInstanceData_t *cmData,
                                        CsrSchedQid      appHandle,
                                        void             *pContext1,
                                        void             *pContext2);
#endif /* INSTALL_CM_EVENT_MASK_SUBSCRIBE_EXT_SCAN_TIMEOUT_IND */
#endif /* CSR_BT_LE_ENABLE */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_OWN_ADDR_TYPE_CHANGE
void CsrBtCmPropagateLeOwnAddressTypeChangedEvent(cmInstanceData_t *cmInst,
                                                  CsrSchedQid appHandle,
                                                  void *arg1,
                                                  void *arg2);
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_OWN_ADDR_TYPE_CHANGE */

#ifdef __cplusplus
}
#endif

#endif


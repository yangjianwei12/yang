#ifndef CSR_BT_CM_DM_H__
#define CSR_BT_CM_DM_H__
/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "dmlib.h"
#include "csr_bt_cm_events_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NO_SCO                  (0xffff)
#define SCOBUSY_ACCEPT          (0xfffe)

#define RFC_SCO_CONNECT         (0x0000)
#define INCOMING_SCO_CON        (0x0200)

typedef struct {
    hci_pkt_type_t          packetType;
    CsrUint8                 featureBit;
} cmHciPacketFeatureType_t;

/*----------------------------------------------------------------------------*
 *
 *   Condition for Connection_Setup_Filter_Condition_Type
 *
 *---------------------------------------------------------------------------*/
#define HCI_COND_SETUP_AUTO_ACCEPT_FLAG_OFF          ((filter_condition_type_t)0x01)
#define HCI_COND_SETUP_AUTO_ACCEPT_FLAG_RS_OFF       ((filter_condition_type_t)0x02)
#define HCI_COND_SETUP_AUTO_ACCEPT_FLAG_RS_ON        ((filter_condition_type_t)0x03)

/* Time allowed to be discoverable during inquiry */
#ifndef SCAN_TIMER_DEFAULT
#define SCAN_TIMER_DEFAULT                                      (1000)
#endif /* SCAN_TIMER_DEFAULT */

/* Time to do inquiry when wanting discoverability also */
#ifndef HCI_INQUIRY_LENGTH_SHORTENED
#define HCI_INQUIRY_LENGTH_SHORTENED                            (5)
#endif /* HCI_INQUIRY_LENGTH_SHORTENED */

/*************************************************************************************
 These function are found under csr_bt_cm_dm_provider.c
************************************************************************************/
void CsrBtCmDmRestoreQueueHandler(cmInstanceData_t *cmData);
void CsrBtCmDmLocalQueueHandler(void);
void CsrBtCmDmProvider(cmInstanceData_t *cmData);
CsrBool cancelDmMsg(cmInstanceData_t *cmData, CsrBtCmPrim type, CsrSchedQid phandle, CsrBtDeviceAddr bd_addr);

/*************************************************************************************
 These function are found under cm_dm_handler.c
************************************************************************************/
void cmDmHandler(cmInstanceData_t *cmData);

/*************************************************************************************
 These function are found under csr_bt_cm_dm_arrival_handler.c
************************************************************************************/
void CsrBtCmDmArrivalHandler(cmInstanceData_t *cmData);

/*************************************************************************************
 These function are found under csr_bt_cm_maintenance_handler.c
************************************************************************************/
void CsrBtCmDmLockQueue(cmInstanceData_t *cmData);
void CsrBtCmDmUnlockQueue(cmInstanceData_t *cmData);
void CsrBtCmSmUnlockQueue(cmInstanceData_t *cmData);
void CsrBtCmSmLockQueue(cmInstanceData_t *cmData);
void CmSdcLockQueue(cmInstanceData_t *cmData);
void CmSdcUnlockQueue(cmInstanceData_t *cmData);

/*************************************************************************************
 These function are found under cm_dm_connect_able_handler.c
************************************************************************************/
void CsrBtCmDmHciWriteClassOfDeviceCompleteHandler(cmInstanceData_t *cmData);
#ifdef CSR_BT_INSTALL_CM_WRITE_COD
void CsrBtCmWriteCodReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmWriteCodReqHandler NULL
#endif
#ifdef CSR_BT_INSTALL_CM_READ_COD
void CsrBtCmReadCodReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciReadClassOfDeviceCompleteHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmReadCodReqHandler NULL
#endif
void CsrBtCmDmHciWriteScanEnableCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmWriteScanEnableCompleteSwitch(cmInstanceData_t *cmData, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);
void CsrBtCmWriteScanEnableReqHandler(cmInstanceData_t *cmData);
void CsrBtCmConnectAbleReqHandler(cmInstanceData_t *cmData);
#ifdef CSR_BT_INSTALL_CM_READ_SCAN_EANBLE
void CsrBtCmReadScanEnableReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciReadScanEnableCompleteHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmReadScanEnableReqHandler NULL
#endif
/*************************************************************************************
 These function are found under csr_bt_cm_dm_sco_handler.c
************************************************************************************/

#if !defined(EXCLUDE_CSR_BT_RFC_MODULE) && !defined(EXCLUDE_CSR_BT_SCO_MODULE)
void CsrBtCmDmScoCancelAcceptConnectReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmScoAcceptConnectReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmScoDisconnectReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmScoConnectReqHandler(cmInstanceData_t *cmData);
void CsrBtCmMapScoPcmResHandler(cmInstanceData_t *cmData);

#ifdef CSR_BT_INSTALL_CM_PRI_SCO_RENEGOTIATE
void CsrBtCmDmScoRenegotiateReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmDmScoRenegotiateReqHandler NULL
#endif /* CSR_BT_INSTALL_CM_PRI_SCO_RENEGOTIATE */

#else /* EXCLUDE_CSR_BT_RFC_MODULE || EXCLUDE_CSR_BT_SCO_MODULE */
#define CsrBtCmDmScoAcceptConnectReqHandler             NULL
#define CsrBtCmDmScoCancelAcceptConnectReqHandler       NULL
#define CsrBtCmDmScoDisconnectReqHandler                NULL
#define CsrBtCmDmScoConnectReqHandler                   NULL
#define CsrBtCmMapScoPcmResHandler                      NULL
#define CsrBtCmDmScoRenegotiateReqHandler               NULL
#endif /* !EXCLUDE_CSR_BT_RFC_MODULE && !EXCLUDE_CSR_BT_SCO_MODULE */


void CsrBtCmDmSyncRenegotiateIndHandler(cmInstanceData_t *cmData);
#ifdef CSR_BT_INSTALL_CM_PRI_SCO_RENEGOTIATE
void CsrBtCmDmSyncRenegotiateCfmHandler(cmInstanceData_t *cmData);
#endif
void CsrBtCmDmSyncDisconnectIndHandler(cmInstanceData_t *cmData);
void CsrBtCmDmSyncDisconnectCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmDmSyncConnectCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmDmSyncConnectIndHandler(cmInstanceData_t *cmData);
void CsrBtCmDmSyncConnectCompleteIndHandler(cmInstanceData_t *cmData);
void CsrBtCmDmSyncRegisterCfmHandler(cmInstanceData_t *cmData);
CsrUint8 returnNumberOfScoConnection(cmInstanceData_t *cmData);
void CsrBtCmBccmdMapScoPcmReqSend(CsrUint8 pcmSlot, CsrUint16 seqNo);
void CsrBtCmBccmdMapScoPcmCfmHandler(cmInstanceData_t *cmData);
CsrUint8 CsrBtCmDmFindFreePcmSlot(cmInstanceData_t *cmData);
#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
void CsrBtCmL2caMapScoPcmResHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmL2caMapScoPcmResHandler NULL
#endif
void CsrBtCmBccmdGetScoParametersCfmHandler(cmInstanceData_t *cmData);

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
void CsrBtCmIncomingScoReqHandler(cmInstanceData_t *cmData);
void CsrBtCmScoFreePacketTypeArray(cmSyncNegotiationCntType_t **negotiateCnt);
void CsrBtCmDmSyncClearPcmSlotFromTable(cmInstanceData_t *cmData, eScoParmVars *eScoParms);
#else
#define CsrBtCmIncomingScoReqHandler NULL
#define CsrBtCmScoFreePacketTypeArray(negotiateCnt)
#define CsrBtCmDmSyncClearPcmSlotFromTable(cmData, eScoParms)
#endif
CsrBool CsrBtCmDmIsEdrEsco(hci_pkt_type_t packetType);
CsrBool CsrBtCmDmIsBrEsco(hci_pkt_type_t packetType);
CsrBool CsrBtCmDmIsSco(hci_pkt_type_t packetType);

void CsrBtCmScoSetEScoParms(eScoParmVars *escoParms, CsrBtCmScoCommonParms *parms, hci_connection_handle_t handle);
CsrBool CsrBtCmScoGetNextNegotiateParms(cmSyncNegotiationCntType_t *negotiateCnt, CsrBtCmScoCommonParms *parms);
CsrBool CsrBtCmScoCurrentNegotiateParmsIsSco(CsrBtCmScoCommonParms *parms);
CsrBool CsrBtCmScoGetCurrentNegotiateParms(cmSyncNegotiationCntType_t *negotiateCnt, CsrBtCmScoCommonParms *parms);
CsrBool CsrBtCmScoSeekToNextScoOnlyNegotiateParms(cmSyncNegotiationCntType_t *negotiateCnt);
CsrUint16 CsrBtCmScoCreatePacketTypeArray(cmInstanceData_t                 *cmData,
                                          CsrBtCmScoCommonParms     **primParms,
                                          CsrUint16                   primParmsLen,
                                          CsrBtDeviceAddr               deviceAddr,
                                          cmSyncNegotiationCntType_t **negotiateCnt);

/*************************************************************************************
 These function are found under csr_bt_cm_acl_handler.c
************************************************************************************/
CsrUint8 returnAclConnectionElement(cmInstanceData_t *cmData,
                                    CsrBtDeviceAddr devAddr,
                                    aclTable **aclConnectionElement);
void returnAclConnectionFromIndex(cmInstanceData_t *cmData,
                                  CsrUint8 index,
                                  aclTable **aclConnectionElement);
void returnNextAclConnectionElement(cmInstanceData_t *cmData,
                                    aclTable **aclConnectionElement);
void returnNextAvailableAclConnectionElement(cmInstanceData_t *cmData,
                                             aclTable **aclConnectionElement);
CsrUint8 returnNumOfAclConnection(cmInstanceData_t *cmData);
cmPendingMsg_t *CsrBtCmAclOpenPendingMsgGet(cmInstanceData_t *cmData,
                                            CsrSchedQid appHandle,
                                            CsrBtTypedAddr addrt,
                                            CsrUint16 flags);
void CsrBtCmAclCloseLegacyHandler(cmInstanceData_t *cmData,
                                  aclTable *aclConnectionElement,
                                  CsrBtDeviceAddr deviceAddr,
                                  CsrUint8 reason);
CsrBool CsrBtCmDmCancelPageOrDetach(cmInstanceData_t *cmData,
                                    CsrBtDeviceAddr deviceAddr);
void CsrBtCmDmAclConnStartIndHandler(cmInstanceData_t *cmData);
void CsrBtCmDmAclOpenedIndHandler(cmInstanceData_t *cmData);
void CsrBtCmDmAclOpenCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmDmAclCloseIndHandler(cmInstanceData_t *cmData);
void CsrBtCmDmAclCloseCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmAclOpenReqHandler(cmInstanceData_t *cmData);
void CsrBtCmAclCloseReqHandler(cmInstanceData_t *cmData);
void CmAclOpenPendingMsgAdd(cmInstanceData_t *cmData,
                            CsrSchedQid appHandle,
                            CsrBtTypedAddr addrt,
                            CsrUint16 flags);
CsrBool CmAclOpenPendingMsgCompareAddr(CsrCmnListElm_t *elem, void *data);
#ifdef INSTALL_CONTEXT_TRANSFER
void CsrBtCmDmAclOpenedSuccessIndHandlerExt(cmInstanceData_t *cmData,
                                                   CsrBtDeviceAddr deviceAddr,
                                                   CsrBool incoming,
                                                   CsrUint24 cod);
#endif /* #ifdef INSTALL_CONTEXT_TRANSFER */

/*************************************************************************************
 These function are found under csr_bt_cm_rfc_sco_handler.c
************************************************************************************/
void CsrBtCmDmScoConnectCfmMsgSend(CsrSchedQid               phandle,
                              CsrBtConnId            btConnId,
                              DM_SYNC_CONNECT_CFM_T  *dmPrim,
                              CsrUint8                pcmSlot,
                              CsrBtResultCode        resultCode,
                              CsrBtSupplier    resultSupplier);

/*************************************************************************************
 These function are found under csr_bt_cm_l2cap_sco_handler.c
************************************************************************************/
void CsrBtCmDmScoL2capConnectCfmMsgSend(CsrSchedQid                phandle,
                                    CsrBtConnId             btConnId,
                                    DM_SYNC_CONNECT_CFM_T  *dmPrim,
                                    CsrUint8                pcmSlot,
                                    CsrBtResultCode        resultCode,
                                    CsrBtSupplier    resultSupplier);

/*************************************************************************************
 These function are found under csr_bt_cm_dm_read_local_bd_addr_handler.c
************************************************************************************/
void CsrBtCmDmHciReadBdAddrCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmReadBdAddrReqHandler(cmInstanceData_t *cmData);

/*************************************************************************************
 These function are found under csr_bt_cm_dm_write_link_super_visiontimeout.c
************************************************************************************/
#ifdef CSR_BT_INSTALL_CM_WRITE_LINK_SUPERVISION_TIMEOUT
void CsrBtCmWriteLinkSuperVTimeoutReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmWriteLinkSuperVTimeoutReqHandler NULL
#endif
void CsrBtCmDmHciWriteLinkSuperVisionTimeoutCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciLinkSupervisionTimeoutIndHandler(cmInstanceData_t *cmData);
void CsrBtCmWriteDmLinkSuperVisionTimeoutHandler(cmInstanceData_t   *cmData,
                                                 CsrSchedQid             phandle,
                                                 CsrUint16          timeout,
                                                 CsrBtDeviceAddr    deviceAddr);

/*************************************************************************************
 These function are found under csr_bt_cm_dm_write_page_to_handler.c
************************************************************************************/
#ifdef CSR_BT_INSTALL_CM_WRITE_PAGE_TO
void CsrBtCmDmWritePageToReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmDmWritePageToReqHandler NULL
#endif
void CsrBtCmDmHciWritePageToCompleteHandler(cmInstanceData_t *cmData);
/*************************************************************************************
 These function are found under csr_bt_cm_dm_read_local_bd_addr_handler.c
************************************************************************************/
void CsrBtCmRfcPortNegCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmRfcPortNegIndHandler(cmInstanceData_t *cmData);

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
void CsrBtCmRfcPortNegResHandler(cmInstanceData_t *cmData);
void CsrBtCmRfcPortnegReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmRfcPortNegResHandler NULL
#define CsrBtCmRfcPortnegReqHandler NULL
#endif

/*************************************************************************************
 These function are found under csr_bt_cm_dm_sc_handler.c
************************************************************************************/
void CsrBtCmGetSecurityConfIndSend(cmInstanceData_t *cmData, CsrUint8 lmpVersion);
void CsrBtCmDmSmInitCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmDmSmAccessIndHandler(cmInstanceData_t * cmData);
void CsrBtCmDmSmClearRebondData(cmInstanceData_t *cmData);
CsrBool CsrBtCmDmSmRebondNeeded(cmInstanceData_t *cmData);

#ifdef EXCLUDE_CSR_BT_SC_MODULE
void CsrBtCmPropgateSecurityEventIndEvent(cmInstanceData_t *cmData,
                                          CsrBtTransportMask transportMask,
                                          CsrBtAddressType addressType,
                                          const CsrBtDeviceAddr *deviceAddr,
                                          CsrBtCmSecurityEvent event);
void CsrBtCmLeUpdateLocalDbKeys(DM_SM_INIT_CFM_T *init);
void CsrBtCmDmSmAddDevice(CsrBtTypedAddr *deviceAddr,
                          DM_SM_TRUST_T trust,
                          DM_SM_KEYS_T *keys);
void CsrBtCmSmDbAddDeviceIndex(cmInstanceData_t *cmData, CsrUint8 deviceIndex);
void CsrBtCmDmSmInit(cmInstanceData_t *cmData);
void CsrBtCmLeKeysHandler(cmInstanceData_t *cmData);
void CsrBtCmBredrKeysHandler(cmInstanceData_t *cmData);
void CsrBtCmDatabaseReqHandler(cmInstanceData_t *cmData);
#ifdef CSR_BT_LE_SIGNING_ENABLE
void CmDmSmCsrkCounterChangeIndHandler(cmInstanceData_t *cmData);
#endif /* CSR_BT_LE_SIGNING_ENABLE */

#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
void CsrBtCmGattRpaOnlyCharRead(cmInstanceData_t *cmData,
                                const CsrBtTpdAddrT *tpAddrt,
                                CsrBool encrypted);
void CsrBtCmGattArrivalHandler(cmInstanceData_t *cmData);
#endif /* CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT */
void CmDmRemoveDeviceKeyReqHandler(cmInstanceData_t *cmData);
void CmDmRemoveDeviceKeyConfirmSend(cmInstanceData_t *cmData,
                                    CsrBtDeviceAddr *deviceAddr,
                                    CsrBtAddressType addressType,
                                    CsrBtResultCode resultCode,
                                    CsrBtSupplier resultSupplier);
void CmDmRemoveDeviceOptionsReqHandler(cmInstanceData_t *cmData);
void CmDmRemoveDeviceOptionsConfirmSend(cmInstanceData_t *cmData,
                                        CsrBtDeviceAddr *deviceAddr,
                                        CsrBtAddressType addressType,
                                        CsrBtResultCode resultCode,
                                        CsrBtSupplier resultSupplier);
void CmPropgateAddressMappedIndEvent(cmInstanceData_t *cmData,
                                     const CsrBtDeviceAddr *randomAddr,
                                     const CsrBtTypedAddr *idAddrt);

#define CsrBtCmSmDeleteStoreLinkKeyReqHandler                       NULL
#define CsrBtCmSmSetDefaultSecLevelReqHandler                       NULL
#define CsrBtCmSmAddDeviceReqHandler                                NULL

#else /* !EXCLUDE_CSR_BT_SC_MODULE */

#define CsrBtCmDatabaseReqHandler                                   NULL
#define CmDmRemoveDeviceKeyReqHandler                               NULL
#define CmDmRemoveDeviceOptionsReqHandler                           NULL

void CsrBtCmSmDeleteStoreLinkKeyReqHandler(cmInstanceData_t *cmData);
void CsrBtCmSmSetDefaultSecLevelReqHandler(cmInstanceData_t *cmData);
void CsrBtCmSmAddDeviceReqHandler(cmInstanceData_t *cmData);
#endif /* EXCLUDE_CSR_BT_SC_MODULE */

void CsrBtCmSmRemoveDeviceReqHandler(cmInstanceData_t *cmData);
#ifdef CSR_BT_INSTALL_SC_AUTHENTICATE
void CsrBtCmSmAuthenticateReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmSmAuthenticateReqHandler NULL
#endif

#ifdef CSR_BT_INSTALL_SC_ENCRYPTION
void CsrBtCmSmEncryptionReqHandler(cmInstanceData_t *cmData);
void CsrBtCmReadEncryptionStatusReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmSmEncryptionReqHandler NULL
#define CsrBtCmReadEncryptionStatusReqHandler NULL
#endif

#ifdef CSR_BT_INSTALL_SC_SECURITY_MODE
void CsrBtCmSmSetSecModeReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmSmSetSecModeReqHandler NULL
#endif
void CsrBtCmSmUnRegisterReqHandler(cmInstanceData_t *cmData);
void CsrBtCmSmPinRequestResHandler(cmInstanceData_t *cmData);
void cmSmAddDeviceReqHandler(cmInstanceData_t *cmData);
void CsrBtCmSmAuthoriseResHandler(cmInstanceData_t *cmData);
void CsrBtCmSmRegisterReqHandler(cmInstanceData_t *cmData);
void CsrBtCmScRejectedForSecurityReasonMsgSend(cmInstanceData_t *cmData,
                                               CsrBtDeviceAddr theAddr,
                                               CsrBool cancelInitiated);
void CsrBtCmSmCancelConnectReqHandler(cmInstanceData_t *cmData);
void cmEnEnableEnhancementsReqHandler(cmInstanceData_t *cmData);
void CsrBtCmSmInitReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmAuthoriseIndHandler(cmInstanceData_t *cmData);
void CsrBtCmScCleanupVar(cmInstanceData_t *cmData);

void CsrBtCmDmSmRemoveDeviceCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmDmSmAddDeviceCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmDmSmSecurityCfmHandler(cmInstanceData_t *cmData);
void CsrBtDmSmKeyRequestIndHandler(cmInstanceData_t *cmData);
void CsrBtCmDmSmKeysIndHandler(cmInstanceData_t *cmData);
void CsrBtCmScMessagePut(cmInstanceData_t *cmData, CsrBtCmPrim primId);
void CsrBtCmHciMessagePut(cmInstanceData_t *cmData, CsrBtCmPrim primId);

#ifdef INSTALL_CM_SM_CONFIG
void CmSmConfigReqHandler(cmInstanceData_t *cmData);
void CmSmConfigCompleteHandler(cmInstanceData_t *cmData);
#else
#define CmSmConfigReqHandler    NULL
#endif /* INSTALL_CM_SM_CONFIG */
/*************************************************************************************
 These function are found under csr_bt_cm_dm_set_local_name_handler.c
************************************************************************************/
void CsrBtCmDmHciChangeLocalNameCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmSetLocalNameReqHandler(cmInstanceData_t *cmData);

/*************************************************************************************
 These function are found under csr_bt_cm_dm_read_remote_name_handler.c
************************************************************************************/
void CsrBtCmReadRemoteNameCfmSend(cmInstanceData_t *cmData, CsrSchedQid phandle,
                      CsrBtDeviceAddr deviceAddr, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);
void CsrBtCmDmHciRemoteNameCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmReadRemoteNameReqHandler(cmInstanceData_t *cmData);
void CsrBtCmCancelReadRemoteNameReqHandler(cmInstanceData_t *cmData);

/*************************************************************************************
 These function are found under csr_bt_cm_dm_inquiry_handler.c
************************************************************************************/
void CsrBtCmStartInquiry(cmInstanceData_t *cmData);
#ifdef INSTALL_CM_INQUIRY
void CsrBtCmInquiryReqHandler(cmInstanceData_t *cmData);
void CsrBtCmCancelInquiryReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmInquiryReqHandler        NULL
#define CsrBtCmCancelInquiryReqHandler  NULL
#endif
void CsrBtCmDmHciInquiryCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciInquiryCancelCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciInquiryResultHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciInquiryResultWithRssiHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciExtendedInquiryResultIndHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciWriteInquiryTransmitPowerLevelCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmBccmdInquiryPriorityCfmHandler(cmInstanceData_t *cmData);
#ifdef INSTALL_CM_READ_INQUIRY_MODE
void CmDmReadInquiryModeReqHandler(cmInstanceData_t *cmData);
void CmDmReadInquiryModeCfmHandler(cmInstanceData_t *cmData);
#else
#define CmDmReadInquiryModeReqHandler  NULL
#endif /* INSTALL_CM_READ_INQUIRY_MODE */
#ifdef INSTALL_CM_READ_INQUIRY_TX
void CmDmReadInquiryTxReqHandler(cmInstanceData_t * cmData);
void CmDmReadInquiryTxCfmHandler(cmInstanceData_t * cmData);
#else
#define CmDmReadInquiryTxReqHandler  NULL
#endif /* INSTALL_CM_READ_INQUIRY_TX */
#ifdef INSTALL_CM_READ_EIR_DATA
void CmDmReadEIRDataReqHandler(cmInstanceData_t * cmData);
void CmDmReadEIRDataCfmHandler(cmInstanceData_t * cmData);
#else
#define CmDmReadEIRDataReqHandler   NULL
#endif /* INSTALL_CM_READ_EIR_DATA */

/*************************************************************************************
 These function are found under csr_bt_cm_dm_mode_change_handler.c
************************************************************************************/
void CsrBtCmDmHciModeChangeEventHandler(cmInstanceData_t *cmData);
CsrUint8 CmDmGetActualMode(cmInstanceData_t *cmData, CsrBtDeviceAddr *deviceAddr);

#ifdef CSR_BT_INSTALL_CM_LOW_POWER_CONFIG_PUBLIC
void CsrBtCmModeChangeConfigReqHandler(cmInstanceData_t *cmData);
void CsrBtCmModeChangeReqHandler(cmInstanceData_t *cmData);
void CmDmSniffSubRateReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmModeChangeReqHandler                             NULL
#define CsrBtCmModeChangeConfigReqHandler                       NULL
#define CmDmSniffSubRateReqHandler                              NULL
#endif /* CSR_BT_INSTALL_CM_LOW_POWER_CONFIG_PUBLIC */

/*************************************************************************************
 These function are found under csr_bt_cm_dm_write_lp_settings_handler.c
************************************************************************************/
#ifdef CSR_BT_INSTALL_CM_LINK_POLICY
void CmDmWriteLinkPolicyReqHandler(cmInstanceData_t *cmData);

#ifdef CSR_BT_INSTALL_CM_READ_LP
void CmDmReadLinkPolicyReqHandler(cmInstanceData_t *cmData);
#else
#define CmDmReadLinkPolicyReqHandler NULL
#endif /* CSR_BT_INSTALL_CM_READ_LP */

#else /* !CSR_BT_INSTALL_CM_LINK_POLICY */
#define CmDmWriteLinkPolicyReqHandler NULL
#define CmDmReadLinkPolicyReqHandler NULL
#endif /* CSR_BT_INSTALL_CM_LINK_POLICY */


void CsrBtCmDmHciWriteLpSettingsCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciWriteDefaultLinkPolicySettingsCompleteHandler(cmInstanceData_t *cmData);
#ifdef CSR_BT_INSTALL_CM_SWITCH_ROLE_PUBLIC
void CsrBtCmAlwaysMasterDevicesReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmAlwaysMasterDevicesReqHandler NULL
#endif
void CsrBtCmAlwaysMasterDevicesCfmHandler(cmInstanceData_t *cmData);

/*************************************************************************************
 These function are found under csr_bt_cm_dm_maintenance_handler.c
************************************************************************************/
void csrBtCmAclElemInit(cmInstanceData_t *cmData,
                        aclTable *aclConnectionElement,
                        const CsrBtDeviceAddr *deviceAddr);
cmRfcConnElement * returnReserveScoIndexToThisAddress(cmInstanceData_t *cmData, CsrBtDeviceAddr deviceAddr);
CsrUint8 returnConnectAbleParameters(cmInstanceData_t *cmData);
#ifdef CSR_BT_INSTALL_CM_WRITE_VOICE_SETTINGS
void CsrBtCmWriteVoiceSettingsReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmWriteVoiceSettingsReqHandler NULL
#endif
void CsrBtCmDmHciWriteVoiceSettingCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmDmSmEncryptionChangeHandler(cmInstanceData_t *cmData);
void CsrBtCmDmSmEncryptCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmSmSppRepairIndSend(cmInstanceData_t *cmData, CsrBtDeviceAddr deviceAddr);
void CsrBtCmSmCancelSppRepairInd(cmInstanceData_t *cmData);
void CsrBtCmLogicalChannelTypeHandler(cmInstanceData_t *cmData);
void CsrBtCmA2DPBitrateHandler(cmInstanceData_t *cmData);
void CsrBtCmDmSmSimplePairingCompleteHandler(cmInstanceData_t *cmData);

#ifdef INSTALL_CM_DM_CONFIGURE_DATA_PATH
void CmDmConfigureDataPathReqHandler(cmInstanceData_t *cmData);
void CmDmConfigureDataPathCfmHandler(cmInstanceData_t *cmData);
#else
#define CmDmConfigureDataPathReqHandler NULL
#endif /* End of INSTALL_CM_DM_CONFIGURE_DATA_PATH */

#ifdef INSTALL_CM_DM_LE_READ_CHANNEL_MAP
void CmDmLeReadChannelMapReqHandler(cmInstanceData_t *cmData);
void CmDmLeReadChannelMapCfmHandler(cmInstanceData_t *cmData);
#else
#define CmDmLeReadChannelMapReqHandler NULL
#endif /* End of INSTALL_CM_DM_LE_READ_CHANNEL_MAP */

#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
void CsrBtCmSetAvStreamInfoReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmSetAvStreamInfoReqHandler NULL
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

#ifdef CSR_TARGET_PRODUCT_VM
void CsrBtCmDmBadMessageIndHandler(cmInstanceData_t *cmData);
#endif

/*************************************************************************************
 These function are found under csr_bt_cm_dm_switch_role_handler.c
************************************************************************************/
#ifdef CSR_BT_INSTALL_CM_SWITCH_ROLE_PUBLIC
void CmDmSwitchRoleReqHandler(cmInstanceData_t *cmData);
#else
#define CmDmSwitchRoleReqHandler NULL
#endif
void CsrBtCmRoleSwitchCfmSend(CsrSchedQid appHandle, CsrBtResultCode resultCode,
                              CsrBtSupplier resultSupplier,
                              CsrBtDeviceAddr deviceAddr, CsrUint8 role, CsrBtCmRoleType roleType);
void CsrBtCmDmHciRoleDiscoveryCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciSwitchRoleCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmDmRoleDiscoveryReqHandler(cmInstanceData_t *cmData);

#if defined(INSTALL_CM_DEVICE_UTILITY) && defined(CSR_BT_INSTALL_CM_INTERNAL_ROLE_CONTROL)
aclRoleVars_t* CsrBtCmDmGetAclRoleVars(aclTable *aclConnectionElement);
void CsrBtCmDmAclRoleVarsClear(aclRoleVars_t *roleVars);

#ifdef CSR_BT_INSTALL_CM_PRI_ALWAYS_SUPPORT_MASTER_ROLE
void CsrBtCmAlwaysSupportMasterRoleReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmAlwaysSupportMasterRoleReqHandler NULL
#endif

#ifdef CSR_BT_INSTALL_CM_ROLE_SWITCH_CONFIG
void CsrBtCmRoleSwitchConfigReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmRoleSwitchConfigReqHandler        NULL
#endif

CsrBool CsrBtCmRoleSwitchAllowedByUpperLayer(aclTable *aclConnectionElement);

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
CsrBool CsrBtCmRoleSwitchBeforeScoSetupNeeded(cmInstanceData_t *cmData);
#endif

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
void CmDmBnepSwitchRoleReqHandler(cmInstanceData_t *cmData);
#else
#define CmDmBnepSwitchRoleReqHandler NULL
#endif /* EXCLUDE_CSR_BT_BNEP_MODULE */

void CmDmInformRoleSwitchStatusToRequester(cmInstanceData_t *cmData,
                                           CsrUint8 role,
                                           CsrBtDeviceAddr *deviceAddr,
                                           CsrBtResultCode resultCode,
                                           CsrBtSupplier resultSupplier);

#else /* !INSTALL_CM_DEVICE_UTILITY OR !CSR_BT_INSTALL_CM_INTERNAL_ROLE_CONTROL */
#define CsrBtCmDmAclRoleVarsClear(_roleVars)
#define CsrBtCmDmGetAclRoleVars(_aclConnectionElement)              NULL
#define CsrBtCmAlwaysSupportMasterRoleReqHandler                    NULL
#define CsrBtCmRoleSwitchConfigReqHandler                           NULL
#define CsrBtCmRoleSwitchAllowedByUpperLayer(_aclConnElement)       FALSE
#define CmDmBnepSwitchRoleReqHandler                                NULL
#endif /* INSTALL_CM_DEVICE_UTILITY AND CSR_BT_INSTALL_CM_INTERNAL_ROLE_CONTROL */

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
void CsrBtCmRfcScoConnectReqHandler(cmInstanceData_t *cmData, CsrBtDeviceAddr deviceAddr);
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
void CsrBtCmL2caScoConnectReqHandler(cmInstanceData_t *cmData, CsrBtDeviceAddr deviceAddr);
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

/*************************************************************************************
 These function are found under csr_bt_cm_dm_iac_handler.c
************************************************************************************/
#ifdef CSR_BT_INSTALL_CM_PRI_IAC

#ifdef CSR_BT_INSTALL_CM_PRI_IAC_READ
void CsrBtCmReadIacReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciReadIacCompleteHandler(cmInstanceData_t *cmData);
#else /* !CSR_BT_INSTALL_CM_PRI_IAC_READ */
#define CsrBtCmReadIacReqHandler                            NULL
#endif /* CSR_BT_INSTALL_CM_PRI_IAC_READ */

void CsrBtCmWriteIacReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciWriteIacCompleteHandler(cmInstanceData_t *cmData);
#else /* !CSR_BT_INSTALL_CM_PRI_IAC */
#define CsrBtCmReadIacReqHandler                            NULL
#define CsrBtCmWriteIacReqHandler                           NULL
#endif /* CSR_BT_INSTALL_CM_PRI_IAC */
/*************************************************************************************
 These function are found under csr_bt_cm_dm_read_local_name_handler.c
************************************************************************************/
#ifdef CSR_BT_INSTALL_CM_READ_LOCAL_NAME
void CsrBtCmReadLocalNameReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmReadLocalNameReqHandler NULL
#endif

void CsrBtCmDmHciReadLocalNameCompleteHandler(cmInstanceData_t *cmData);

/*************************************************************************************
 These function are found under csr_bt_cm_dm_dut_handler.c
************************************************************************************/
#ifdef CSR_BT_INSTALL_CM_DUT_MODE
void CsrBtCmDeviceUnderTestReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciDeviceUnderTestCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmSendDeviceUnderTestComplete(CsrSchedQid appHandle, CsrUint8 status, CsrUint8 step);
void CsrBtCmDeviceUnderTestDisableReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmDeviceUnderTestReqHandler            NULL
#define CsrBtCmDeviceUnderTestDisableReqHandler     NULL
#endif

/*************************************************************************************
 These function are found under csr_bt_cm_dm_read_local_version_handler.c
************************************************************************************/
void CsrBtCmDmHciReadLocalVersionCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmReadLocalVersionReqHandler(cmInstanceData_t *cmData);
#ifdef EXCLUDE_CSR_BT_SC_MODULE
#define CsrBtCmGetSecurityConfResHandler                    NULL
#else
void CsrBtCmGetSecurityConfResHandler(cmInstanceData_t *cmData);
#endif

/*************************************************************************************
 This function are found under csr_bt_cm_sdc_handler.c
************************************************************************************/
void CsrBtCmDmAclOpenInSdcCloseStateHandler(cmInstanceData_t * cmData,
                                            CsrBtDeviceAddr *deviceAddr,
                                            CsrBool success);
void CsrBtCmSdcDecAclRefCountTo(cmInstanceData_t * cmData, CsrBtDeviceAddr deviceAddr);

/*************************************************************************************
These functions are found under csr_bt_cm_dm_read_tx_power_level_handler.c
**************************************************************************************/
#ifdef CSR_BT_INSTALL_CM_READ_TX_POWER_LEVEL
void CsrBtCmDmHciReadTxPowerLevelCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmReadTxPowerLevelReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmReadTxPowerLevelReqHandler NULL
#endif
#ifdef CSR_BT_LE_ENABLE
void CsrBtCmReadAdvertisingChTxPowerReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmReadAdvertisingChTxPowerReqHandler NULL
#endif
/*************************************************************************************
These functions are found under csr_bt_cm_dm_get_link_quality_handler.c
**************************************************************************************/
#ifdef CSR_BT_INSTALL_CM_GET_LINK_QUALITY
void CsrBtCmDmHciGetLinkQualityCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmGetLinkQualityReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmGetLinkQualityReqHandler NULL
#endif
/*************************************************************************************
These functions are found under csr_bt_cm_dm_read_rssi_handler.c
**************************************************************************************/
#ifdef CSR_BT_INSTALL_CM_READ_RSSI
void CsrBtCmDmHciReadRssiCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmReadRssiReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmReadRssiReqHandler NULL
#endif
/*************************************************************************************
These functions are found under csr_bt_cm_dm_features_handler.c
**************************************************************************************/
void CsrBtCmDmHciReadSuppFeaturesCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciReadRemoteFeaturesCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciReadRemoteExtFeaturesCompleteHandler(cmInstanceData_t *cmData);
#ifdef INSTALL_CM_REMOTE_EXT_FEATURES
void CsrBtCmReadRemoteExtFeaturesReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmReadRemoteExtFeaturesReqHandler  NULL
#endif
#ifdef CSR_BT_INSTALL_CM_READ_LOCAL_EXT_FEATURES
void CsrBtCmDmHciReadLocalExtFeaturesCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmReadLocalExtFeaturesReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmReadLocalExtFeaturesReqHandler NULL
#endif
void CsrBtCmReadRemoteFeaturesReqHandler(cmInstanceData_t * cmData);

/*************************************************************************************
These functions are found under csr_bt_cm_dm_afh_handler.c
**************************************************************************************/
#ifdef CSR_BT_INSTALL_CM_AFH
void CsrBtCmDmHciSetAfhChannelClassCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmSetAfhChannelClassReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciReadAfhChannelAssesModeCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmReadAfhChannelAssesModeReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciWriteAfhChannelAssesModeCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmWriteAfhChannelAssesModeReqHandler(cmInstanceData_t *cmData);
void CsrBtCmReadAfhChannelMapCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmReadAfhChannelMapReqHandler(cmInstanceData_t *cmData);
#else /* !CSR_BT_INSTALL_CM_AFH */
#define CsrBtCmSetAfhChannelClassReqHandler             NULL
#define CsrBtCmReadAfhChannelAssesModeReqHandler        NULL
#define CsrBtCmWriteAfhChannelAssesModeReqHandler       NULL
#define CsrBtCmReadAfhChannelMapReqHandler              NULL
#endif /* CSR_BT_INSTALL_CM_AFH */
/*************************************************************************************
These functions are found under csr_bt_cm_dm_read_clock_handler.c
**************************************************************************************/
#ifdef CSR_BT_INSTALL_CM_READ_CLOCK
void CsrBtCmDmHciReadClockCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmReadClockReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmReadClockReqHandler NULL
#endif
/*************************************************************************************
These functions are found under csr_bt_cm_dm_read_remote_version_handler.c
**************************************************************************************/
void CsrBtCmDmHciReadRemoteVersionCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmReadRemoteVersionReqHandler(cmInstanceData_t *cmData);

/*************************************************************************************
These functions are found under csr_bt_cm_dm_set_event_filter_handler.c
**************************************************************************************/
#ifdef CSR_BT_INSTALL_CM_PRI_SET_EVENT_FILTER_BDADDR
void CsrBtCmSetEventFilterBdaddrReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmSetEventFilterBdaddrReqHandler NULL
#endif

#ifdef INSTALL_CM_SET_EVENT_FILTER_COD
void CsrBtCmSetEventFilterCodReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmSetEventFilterCodReqHandler  NULL
#endif

#ifdef INSTALL_CM_CLEAR_EVENT_FILTER
void CsrBtCmClearEventFilterReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmClearEventFilterReqHandler   NULL
#endif
void CsrBtCmSetEventFilterCommonCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmBccmdMaxEventFilterHandler(cmInstanceData_t *cmData);


/*************************************************************************************
These functions are found under csr_bt_cm_dm_cache_params_handler.c
**************************************************************************************/
#ifdef CSR_BT_INSTALL_CM_CACHE_PARAMS
void CsrBtCmDmWriteCachedPageModeCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmDmUpdateAndClearCachedParamReqSend(CsrBtDeviceAddr  devAddr);
void CmDmUpdateAndClearCachedParamDirect(cmInstanceData_t *cmData, CsrBtDeviceAddr  devAddr);
void CsrBtCmDmUpdateAndClearCachedParamReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmWriteCacheParamsReqHandler(cmInstanceData_t *cmData);
CsrBool CsrBtCmDmWriteKnownCacheParams(cmInstanceData_t *cmData,
                                       CsrBtDeviceAddr devAddr,
                                       CsrBtCmPlayer player);
CsrBool CmDmWriteKnownCacheParamsDirect(cmInstanceData_t *cmData, CsrBtDeviceAddr devAddr);
CsrBool CmDmWriteCacheParamsAll(cmInstanceData_t *cmData, CsrBtDeviceAddr *deviceAddr);
void CmDmStoreCacheParams(cmInstanceData_t *cmData,
                          CsrBtDeviceAddr devAddr,
                          CsrUint16 clockOffset,
                          page_scan_mode_t pageScanMode,
                          page_scan_rep_mode_t pageScanRepMode);
void CsrBtCmDmWriteCachedClockOffsetCfmHandler(cmInstanceData_t *cmData);
dmCacheParamEntry *CmInsertEntryInCacheParamTable(cmInstanceData_t *cmData, CsrBtDeviceAddr *devAddr);
#ifdef ENABLE_SHUTDOWN
void CsrBtCmRemoveCacheParamTable(cmInstanceData_t *cmData);
#endif
void CsrBtCmFlushCmCacheStopTimer(cmInstanceData_t *cmData);
void CsrBtCmFlushCmCacheStartTimer(cmInstanceData_t *cmData);
void CsrBtCmDmHciReadClockOffsetCompleteHandler(cmInstanceData_t *cmData);
#else /* CSR_BT_INSTALL_CM_CACHE_PARAMS */
#define CsrBtCmDmUpdateAndClearCachedParamReqSend(_devAddr)
#define CsrBtCmDmUpdateAndClearCachedParamDirect(_cmData, _devAddr)
#define CsrBtCmDmUpdateAndClearCachedParamReqHandler                 NULL
#define CsrBtCmDmWriteCacheParamsReqHandler                          NULL
#define CsrBtCmDmWriteKnownCacheParams(_cmData, _devAddr, _player)   (CSR_UNUSED(_player), FALSE)
#define CsrBtCmDmWriteKnownCacheParamsDirect(_cmData, _devAddr)      FALSE
#define CmDmWriteCacheParamsAll(cmData, deviceAddr)                  FALSE
#define CmDmStoreCacheParams(_cmData,_devAddr,_clockOffset,_pageScanMode,_pageScanRepMode)
#define CsrBtCmRemoveCacheParamTable(_cmData)
#define CsrBtCmFlushCmCacheStopTimer(_cmData)
#define CsrBtCmFlushCmCacheStartTimer(_cmData)
#endif /* !CSR_BT_INSTALL_CM_CACHE_PARAMS */


/*************************************************************************************
These functions are found under cm_dm_write_scan_handler.c
**************************************************************************************/
#ifdef CSR_BT_INSTALL_CM_WRITE_PAGE_SCAN
void CsrBtCmWritePageScanSettingsReqHandler(cmInstanceData_t *cmData);
void CsrBtCmWritePageScanTypeReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmWritePageScanSettingsReqHandler  NULL
#define CsrBtCmWritePageScanTypeReqHandler      NULL
#endif
void CsrBtCmDmHciWritePageScanActivityCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciWritePageScanTypeCompleteHandler(cmInstanceData_t *cmData);
/*************************************************************************************
These functions are found under csr_bt_cm_dm_write_inquiry_handler.c
**************************************************************************************/
void CsrBtCmWriteInquiryScanSettingsReqHandler(cmInstanceData_t *cmData);
#ifdef CSR_BT_INSTALL_CM_WRITE_INQUIRY_SCAN_TYPE
void CsrBtCmWriteInquiryScanTypeReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmWriteInquiryScanTypeReqHandler NULL
#endif
void CsrBtCmDmHciWriteInquiryScanActivityCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciWriteInquiryScanTypeCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciWriteInquiryModeCompleteHandler(cmInstanceData_t *cmData);
#ifdef INSTALL_CM_WRITE_INQUIRY_MODE
void CmDmWriteInquiryModeReqHandler(cmInstanceData_t * cmData);
#else
#define CmDmWriteInquiryModeReqHandler NULL
#endif /* INSTALL_CM_WRITE_INQUIRY_MODE */


/*************************************************************************************
These functions are found under csr_bt_cm_dm_sniff_sub_rate_handler.c
**************************************************************************************/
void CsrBtCmDmHciSniffSubRatingIndHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciSniffSubRateCompleteHandler(cmInstanceData_t *cmData);

#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1

#if defined(INSTALL_CM_DEVICE_UTILITY) && defined(INSTALL_CM_INTERNAL_LPM)
void CsrBtCmDmCheckSsrReqSend(CsrBtDeviceAddr deviceAddr);
void CsrBtCmDmCheckSsrReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmDmCheckSsrReqSend(deviceAddr)
#define CsrBtCmDmCheckSsrReqHandler NULL
#endif

#ifdef CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
void CsrBtCmDmModeSettingsReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmDmModeSettingsReqHandler                 NULL
#endif

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
void CsrBtCmDmL2caModeSettingsReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmDmL2caModeSettingsReqHandler             NULL
#endif

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
void CsrBtCmDmBnepModeSettingsReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmDmBnepModeSettingsReqHandler             NULL
#endif

#else /* CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS */
#define CsrBtCmDmModeSettingsReqHandler                 NULL
#define CsrBtCmDmL2caModeSettingsReqHandler             NULL
#define CsrBtCmDmBnepModeSettingsReqHandler             NULL
#endif /* !CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS */

#else /* CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 */
#define CsrBtCmDmModeSettingsReqHandler                 NULL
#define CsrBtCmDmL2caModeSettingsReqHandler             NULL
#define CsrBtCmDmBnepModeSettingsReqHandler             NULL
#define CsrBtCmDmCheckSsrReqHandler                     NULL
#endif /* !(CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1) */

void CsrBtCmDmPowerSettingsReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmPowerSettingsCfmHandler(cmInstanceData_t *cmData);
/*************************************************************************************
These functions are found under csr_bt_cm_dm_set_link_behavior_handler.c
**************************************************************************************/
#ifdef CSR_BT_INSTALL_CM_SET_LINK_BEHAVIOR
void CsrBtCmDmSetLinkBehaviorReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmSetLinkBehaviorCfmHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmDmSetLinkBehaviorReqHandler             NULL
#define CsrBtCmDmSetLinkBehaviorCfmHandler(_cmData)
#endif

/*************************************************************************************
These functions are found under csr_bt_cm_dm_extended_inquiry_response.c
**************************************************************************************/
#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1
/* LMP version is 2.1 or newer - init EIR-data and use other 2.1 features */

#ifdef CSR_BT_INSTALL_CM_AUTO_EIR
void CsrBtCmEirInitData(cmInstanceData_t *cmData);
void CsrBtCmEirExtractServicesFromRecord(cmInstanceData_t *cmData, CsrUint16 serviceRecordLen, CsrUint8 *serviceRecord);
void CsrBtCmEirAddServiceRecord(cmInstanceData_t *cmData, CsrUint32 serviceRecordHandle);
void CsrBtCmEirRemoveServiceRecord(cmInstanceData_t *cmData, CsrUint32 serviceRecordHandle);
void CsrBtCmEirUpdateName(cmInstanceData_t *cmData);
void CsrBtCmEirUpdateManufacturerReqHandler(cmInstanceData_t *cmData);

#ifdef CSR_BT_INSTALL_CM_EIR_FLAGS
void CsrBtCmEirFlagsReqHandler(cmInstanceData_t *cmData);
#else /* CSR_BT_INSTALL_CM_EIR_FLAGS */
#define CsrBtCmEirFlagsReqHandler                       NULL
#endif /* !CSR_BT_INSTALL_CM_EIR_FLAGS */

#define CsrBtCmSetEirDataReqHandler                     NULL

#else /* CSR_BT_INSTALL_CM_AUTO_EIR */

void CsrBtCmSetEirDataReqHandler(cmInstanceData_t *cmData);

#define CsrBtCmEirInitData(_cmData)
#define CsrBtCmEirUpdateManufacturerReqHandler          NULL
#define CsrBtCmEirFlagsReqHandler                       NULL

#endif /* !CSR_BT_INSTALL_CM_AUTO_EIR */

void CsrBtCmDmHciWriteExtendedInquiryResponseDataCompleteHandler(cmInstanceData_t *cmData);

#else /* CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 */

#define CsrBtCmEirInitData(_cmData)
#define CsrBtCmEirExtractServicesFromRecord(_cmData, _serviceRecordLen, _serviceRecord)
#define CsrBtCmEirAddServiceRecord(_cmData, _serviceRecordHandle)
#define CsrBtCmEirRemoveServiceRecord(_cmData, _serviceRecordHandle)
#define CsrBtCmEirUpdateManufacturerReqHandler          NULL
#define CsrBtCmEirFlagsReqHandler                       NULL
#define CsrBtCmDmHciWriteExtendedInquiryResponseDataCompleteHandler(_cmData)

#endif /* CSR_BT_BT_VERSION < CSR_BT_BLUETOOTH_VERSION_2P1 */

/*************************************************************************************
These functions are found under csr_bt_cm_dm_write_auto_flush_timeout.c
**************************************************************************************/
#ifdef CSR_BT_INSTALL_CM_READ_FAILED_CONTACT_COUNTER
void CsrBtCmReadFailedContactCounterReqHandler(cmInstanceData_t * cmData);
void CsrBtCmDmHciReadFailedContactCounterCompleteHandler(cmInstanceData_t * cmData);
#else
#define CsrBtCmReadFailedContactCounterReqHandler NULL
#endif
#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1
void CsrBtCmDmWriteAutoFlushTimeoutReqHandler(cmInstanceData_t * cmData);
#else
#define CsrBtCmDmWriteAutoFlushTimeoutReqHandler NULL
#endif
void CsrBtCmDmHciWriteAutoFlushTimeoutCompleteHandler(cmInstanceData_t *cmData);
void CsrBtCmWriteAutoFlushTimeout(cmInstanceData_t *cmData, CsrBtDeviceAddr *deviceAddr);
#else
#define CsrBtCmDmWriteAutoFlushTimeoutReqHandler    NULL
#define CsrBtCmDmHciWriteAutoFlushTimeoutCompleteHandler(cmData)
#define CsrBtCmWriteAutoFlushTimeout(cmData, deviceAddr)
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

/*************************************************************************************
These functions are found under csr_bt_cm_dm_encryption_key_size.c
**************************************************************************************/
void CsrBtCmReadEncryptionKeySizeReqHandler(cmInstanceData_t *cmData);
#ifdef CSR_BT_LE_ENABLE
void CsrBtCmSmSetEncryptionKeySizeReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmSmSetEncryptionKeySizeReqHandler NULL
#endif

/*************************************************************************************
 These function are found under csr_bt_cm_dm_qos_handler.c
************************************************************************************/
#ifndef CSR_BT_EXCLUDE_HCI_QOS_SETUP
CsrBool CsrBtCmDmHciQosSetupDirect(cmInstanceData_t *cmData,
                                     CsrSchedQid appHandle,
                                     CsrBtDeviceAddr* addr,
                                     CsrBool setDefaultQos);
void CsrBtCmDmHciQosSetupReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmHciQosSetupCompleteHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmDmHciQosSetupReqHandler NULL
#endif

void CsrBtCmDmWriteAuthPayloadTimeoutReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmWriteAuthPayloadTimeoutCompleteHandler(cmInstanceData_t *cmData);
void CmDmHciAuthPayloadTimeoutExpiredIndHandler(cmInstanceData_t *cmData);
#ifdef INSTALL_CM_READ_APT
void CmDmReadAuthPayloadTimeoutReqHandler(cmInstanceData_t * cmData);
void CmDmReadAuthPayloadTimeoutCfmHandler(cmInstanceData_t * cmData);
#else
#define CmDmReadAuthPayloadTimeoutReqHandler    NULL
#endif /* INSTALL_CM_READ_APT */

#ifdef EXCLUDE_CSR_BT_SC_MODULE
#define CsrBtScRegisterReqSend(_profileUuid,                                    \
                               _channel,                                        \
                               _outgoing,                                       \
                               _protocol,                                       \
                               _secLevel,                                       \
                               _minEncKeySize)                                  \
    dm_sm_service_register_req(CSR_BT_CM_IFACEQUEUE,                            \
                               0,                                               \
                               _protocol,                                       \
                               _channel,                                        \
                               _outgoing,                                       \
                               _secLevel,                                       \
                               _minEncKeySize,                                  \
                               NULL)

#define CsrBtScDeregisterReqSend(_protocol, _channel)                           \
    dm_sm_unregister_req(CSR_BT_CM_IFACEQUEUE, 0, _protocol, _channel, NULL)

#endif /* EXCLUDE_CSR_BT_SC_MODULE */


#ifdef CSR_BT_ISOC_ENABLE
/* Handler functions for ISOC related functionalities */
void CsrBtCmDmIsocRegisterReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmIsocConfigureCigReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmIsocRemoveCigReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmIsocCisConnectReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmIsocCisConnectRspHandler(cmInstanceData_t *cmData);
void CsrBtCmDmIsocCisDisconnectReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmIsocSetupIsoDataPathReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmIsocRemoveIsoDataPathReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmIsocCreateBigReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmIsocTerminateBigReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmIsocBigCreateSyncReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmIsocBigTerminateSyncReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmIsocConfigureCigTestReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmIsocCreateBigTestReqHandler(cmInstanceData_t *cmData);
void CmDmIsocReadIsoLinkQualityReqHandler(cmInstanceData_t *cmData);

/* Upstream message confirmations for ISOC related functionalities */
void CsrBtCmDmIsocRegisterCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmIsocConfigureCigCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmIsocRemoveCigCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmIsocCisConnectIndHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmIsocCisConnectCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmIsocCisDisconnectCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmIsocCisDisconnectIndHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmIsocSetupIsoDataPathCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmIsocRemoveIsoDataPathCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmIsocCreateBigCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmIsocTerminateBigCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmIsocBigCreateSyncCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmIsocBigTerminateSyncIndHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmBleBigInfoAdvReportIndHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmIsocConfigureCigTestCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmIsocCreateBigTestCfmHandler(cmInstanceData_t *cmData, void *msg);
void CmDmIsocReadIsoLinkQualityCfmHandler(cmInstanceData_t *cmData, void *msg);

#else /* !CSR_BT_ISOC_ENABLE */
#define CsrBtCmDmIsocRegisterReqHandler             NULL
#define CsrBtCmDmIsocConfigureCigReqHandler         NULL
#define CsrBtCmDmIsocRemoveCigReqHandler            NULL
#define CsrBtCmDmIsocCisConnectReqHandler           NULL
#define CsrBtCmDmIsocCisConnectRspHandler           NULL
#define CsrBtCmDmIsocCisDisconnectReqHandler        NULL
#define CsrBtCmDmIsocSetupIsoDataPathReqHandler     NULL
#define CsrBtCmDmIsocRemoveIsoDataPathReqHandler    NULL
#define CsrBtCmDmIsocCreateBigReqHandler            NULL
#define CsrBtCmDmIsocTerminateBigReqHandler         NULL
#define CsrBtCmDmIsocBigCreateSyncReqHandler        NULL
#define CsrBtCmDmIsocBigTerminateSyncReqHandler     NULL
#define CsrBtCmDmIsocConfigureCigTestReqHandler     NULL
#define CsrBtCmDmIsocCreateBigTestReqHandler        NULL
#define CmDmIsocReadIsoLinkQualityReqHandler        NULL
#endif /* End of CSR_BT_ISOC_ENABLE */

#if defined(CSR_BT_INSTALL_EXTENDED_SCANNING) || defined(CSR_BT_INSTALL_EXTENDED_ADVERTISING) \
    || defined(CSR_BT_INSTALL_PERIODIC_SCANNING) || defined(CSR_BT_INSTALL_PERIODIC_ADVERTISING)
#ifdef CSR_STREAMS_ENABLE
void CmExtScanFilteredAdvReportDoneIndHandler(cmInstanceData_t *cmData);
void CmPeriodicScanSyncAdvReportDoneIndHandler(cmInstanceData_t *cmData);
#else /* !CSR_STREAMS_ENABLE */
#define CmExtScanFilteredAdvReportDoneIndHandler NULL
#define CmPeriodicScanSyncAdvReportDoneIndHandler NULL
#endif /* End of CSR_STREAMS_ENABLE */
void CsrBtCmDmGetAdvScanCapabilitiesReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmGetAdvScanCapabilitiesCfmHandler(cmInstanceData_t *cmData, void *msg);
#else
#define CmExtScanFilteredAdvReportDoneIndHandler NULL
#define CmPeriodicScanSyncAdvReportDoneIndHandler NULL
#define CsrBtCmDmGetAdvScanCapabilitiesReqHandler NULL
#endif

#ifdef CSR_BT_INSTALL_EXTENDED_ADVERTISING

/* Handler functions for Extended Advertising related functionalities */
void CsrBtCmDmExtAdvRegisterAppAdvSetReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmExtAdvUnregisterAppAdvSetReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmExtAdvSetParamsReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmExtAdvSetDataReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmExtAdvSetScanRespDataReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmExtAdvEnableReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmExtAdvReadMaxAdvDataLenReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmExtAdvSetRandomAddrReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmExtAdvSetsInfoReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmExtAdvMultiEnableReqHandler(cmInstanceData_t *cmData);
void CmDmExtAdvGetAddressReqHandler(cmInstanceData_t *cmData);


/* Upstream message confirmations for Extended Advertising related functionalities */
void CsrBtCmDmExtAdvRegisterAppAdvSetCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtAdvUnregisterAppAdvSetCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtAdvSetParamsCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtAdvSetDataCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtAdvSetScanRespDataCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtAdvEnableCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtAdvReadMaxAdvDataLenCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtAdvTerminatedIndHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtAdvSetRandomAddrCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtAdvSetsInfoCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtAdvMultiEnableCfmHandler(cmInstanceData_t *cmData, void *msg);
void CmDmExtAdvGetAddressCfmHandler(cmInstanceData_t *cmData, void *msg);

#else /* !CSR_BT_INSTALL_EXTENDED_ADVERTISING */
#define CsrBtCmDmExtAdvRegisterAppAdvSetReqHandler      NULL
#define CsrBtCmDmExtAdvUnregisterAppAdvSetReqHandler    NULL
#define CsrBtCmDmExtAdvSetParamsReqHandler              NULL
#define CsrBtCmDmExtAdvSetDataReqHandler                NULL
#define CsrBtCmDmExtAdvSetScanRespDataReqHandler        NULL
#define CsrBtCmDmExtAdvEnableReqHandler                 NULL
#define CsrBtCmDmExtAdvReadMaxAdvDataLenReqHandler      NULL
#define CsrBtCmDmExtAdvSetRandomAddrReqHandler          NULL
#define CsrBtCmDmExtAdvSetsInfoReqHandler               NULL
#define CsrBtCmDmExtAdvMultiEnableReqHandler            NULL
#define CmDmExtAdvGetAddressReqHandler                  NULL
#endif /* End of CSR_BT_INSTALL_EXTENDED_ADVERTISING */

#if defined(CSR_BT_INSTALL_EXTENDED_ADVERTISING) && defined(INSTALL_CM_EXT_ADV_SET_PARAM_V2)
void CmDmExtAdvSetParamsV2ReqHandler(cmInstanceData_t *cmData);
void CmDmExtAdvSetParamsV2CfmHandler(cmInstanceData_t *cmData, void *msg);
#else
#define CmDmExtAdvSetParamsV2ReqHandler                 NULL
#endif

#if defined(CSR_BT_INSTALL_EXTENDED_SCANNING) && defined(INSTALL_CM_EXT_SET_CONN_PARAM)
void CmDmExtSetConnParamsReqHandler(cmInstanceData_t *cmData);
void CmDmExtSetConnParamsCfmHandler(cmInstanceData_t *cmData, void *msg);
#else
#define CmDmExtSetConnParamsReqHandler                  NULL
#endif /* if defined(CSR_BT_INSTALL_EXTENDED_ADVERTISING) && defined(INSTALL_CM_EXT_SET_CONN_PARAM) */

#ifdef CSR_BT_INSTALL_EXTENDED_SCANNING

/* Handler functions for Extended Scan related functionalities */
void CsrBtCmDmExtScanGetGlobalParamsReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmExtScanSetGlobalParamsReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmExtScanRegisterScannerReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmExtScanUnregisterScannerReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmExtScanConfigureScannerReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmExtScanEnableScannersReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmExtScanGetCtrlScanInfoReqHandler(cmInstanceData_t *cmData);

/* Upstream message confirmations for Extended Scan related functionalities */
void CsrBtCmDmExtScanGetGlobalParamsCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtScanSetGlobalParamsCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtScanRegisterScannerCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtScanUnregisterScannerCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtScanConfigureScannerCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtScanEnableScannersCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtScanGetCtrlScanInfoCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtScanCtrlScanInfoIndHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtScanFilteredAdvReportIndHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmExtScanDurationExpiredIndHandler(cmInstanceData_t *cmData, void *msg);

#else /* !CSR_BT_INSTALL_EXTENDED_SCANNING */
#define CsrBtCmDmExtScanGetGlobalParamsReqHandler       NULL
#define CsrBtCmDmExtScanSetGlobalParamsReqHandler       NULL
#define CsrBtCmDmExtScanRegisterScannerReqHandler       NULL
#define CsrBtCmDmExtScanUnregisterScannerReqHandler     NULL
#define CsrBtCmDmExtScanConfigureScannerReqHandler      NULL
#define CsrBtCmDmExtScanEnableScannersReqHandler        NULL
#define CsrBtCmDmExtScanGetCtrlScanInfoReqHandler       NULL
#endif /* End of CSR_BT_INSTALL_EXTENDED_SCANNING */

#ifdef CSR_BT_INSTALL_PERIODIC_ADVERTISING

/* Handler functions for Periodic Advertising related functionalities */
void CsrBtCmDmPeriodicAdvSetParamsReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmPeriodicAdvSetDataReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmPeriodicAdvReadMaxAdvDataLenReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmPeriodicAdvStartReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmPeriodicAdvStopReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmPeriodicAdvSetTransferReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmPeriodicAdvEnableReqHandler(cmInstanceData_t *cmData);

/* Upstream message confirmations for Periodic Advertising related functionalities */
void CsrBtCmDmPeriodicAdvSetParamsCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmPeriodicAdvSetDataCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmPeriodicAdvReadMaxAdvDataLenCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmPeriodicAdvStartCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmPeriodicAdvStopCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmPeriodicAdvSetTransferCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmPeriodicAdvEnableCfmHandler(cmInstanceData_t *cmData, void *msg);

#else /* !CSR_BT_INSTALL_PERIODIC_ADVERTISING */
#define CsrBtCmDmPeriodicAdvSetParamsReqHandler             NULL
#define CsrBtCmDmPeriodicAdvSetDataReqHandler               NULL
#define CsrBtCmDmPeriodicAdvReadMaxAdvDataLenReqHandler     NULL
#define CsrBtCmDmPeriodicAdvStartReqHandler                 NULL
#define CsrBtCmDmPeriodicAdvStopReqHandler                  NULL
#define CsrBtCmDmPeriodicAdvSetTransferReqHandler           NULL
#define CsrBtCmDmPeriodicAdvEnableReqHandler                NULL
#endif /* End of CSR_BT_INSTALL_PERIODIC_ADVERTISING */

#ifdef CSR_BT_INSTALL_PERIODIC_SCANNING

/* Handler functions for Periodic Scanning related functionalities */
void CsrBtCmDmPeriodicScanStartFindTrainsReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmPeriodicScanStopFindTrainsReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmPeriodicScanSyncToTrainReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmPeriodicScanSyncToTrainCancelReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmPeriodicScanSyncAdvReportEnableReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmPeriodicScanSyncTerminateReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmPeriodicScanSyncLostRspHandler(cmInstanceData_t *cmData);
void CsrBtCmDmPeriodicScanSyncTransferReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmPeriodicScanSyncTransferParamsReqHandler(cmInstanceData_t *cmData);

/* Upstream message confirmations for Periodic Scanning related functionalities */
void CsrBtCmDmPeriodicScanStartFindTrainsCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmPeriodicScanStopFindTrainsCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmPeriodicScanSyncToTrainCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmPeriodicScanSyncToTrainCancelCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmPeriodicScanSyncAdvReportEnableCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmPeriodicScanSyncTerminateCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmPeriodicScanSyncAdvReportIndHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmPeriodicScanSyncLostIndHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmPeriodicScanSyncTransferCfmHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmPeriodicScanSyncTransferIndHandler(cmInstanceData_t *cmData, void *msg);
void CsrBtCmDmPeriodicScanSyncTransferParamsCfmHandler(cmInstanceData_t *cmData, void *msg);
#else /* !CSR_BT_INSTALL_PERIODIC_SCANNING */
#define CsrBtCmDmPeriodicScanStartFindTrainsReqHandler      NULL
#define CsrBtCmDmPeriodicScanStopFindTrainsReqHandler       NULL
#define CsrBtCmDmPeriodicScanSyncToTrainReqHandler          NULL
#define CsrBtCmDmPeriodicScanSyncToTrainCancelReqHandler    NULL
#define CsrBtCmDmPeriodicScanSyncAdvReportEnableReqHandler  NULL
#define CsrBtCmDmPeriodicScanSyncTerminateReqHandler        NULL
#define CsrBtCmDmPeriodicScanSyncLostRspHandler             NULL
#define CsrBtCmDmPeriodicScanSyncTransferReqHandler         NULL
#define CsrBtCmDmPeriodicScanSyncTransferParamsReqHandler   NULL
#endif /* End of CSR_BT_INSTALL_PERIODIC_SCANNING */

#ifdef CSR_BT_INSTALL_CRYPTO_SUPPORT
/* Handler functions for Crypto related functionalities */
void CsrBtCmDmCryptoGeneratePublicPrivateKeyReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmCryptoGenerateSharedSecretKeyReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmCryptoEncryptReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmCryptoHashReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmCryptoHashReqContinueHandler(cmInstanceData_t *cmData);
void CsrBtCmDmCryptoDecryptReqHandler(cmInstanceData_t *cmData);
void CsrBtCmDmCryptoAesCtrReqHandler(cmInstanceData_t *cmData);

/* Upstream message confirmations for Crypto related functionalities */
void CsrBtCmDmCryptoGeneratePublicPrivateKeyCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmDmCryptoGenerateSharedSecretKeyCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmDmCryptoEncryptCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmDmCryptoHashCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmDmCryptoDecryptCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmDmCryptoAesCtrCfmHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmDmCryptoGeneratePublicPrivateKeyReqHandler NULL
#define CsrBtCmDmCryptoGenerateSharedSecretKeyReqHandler  NULL
#define CsrBtCmDmCryptoEncryptReqHandler                  NULL
#define CsrBtCmDmCryptoHashReqHandler                     NULL
#define CsrBtCmDmCryptoDecryptReqHandler                  NULL
#define CsrBtCmDmCryptoAesCtrReqHandler                   NULL
#endif /* End of CSR_BT_INSTALL_CRYPTO_SUPPORT */

void CmWriteScHostSupportOverrideReqHandler(cmInstanceData_t *cmData);
void CmReadScHostSupportOverrideMaxBdAddrReqHandler(cmInstanceData_t *cmData);
void CmDmWriteScHostSupportOverrideCfmHandler(cmInstanceData_t *cmData);
void CmDmReadScHostSupportOverrideMaxBdAddrCfmHandler(cmInstanceData_t *cmData);

void CmSmRefreshEncryptionKeyReqHandler(cmInstanceData_t *cmData);
#ifdef INSTALL_CM_LE_PHY_UPDATE_FEATURE
void CmDmLeReadPhyReqHandler(cmInstanceData_t *cmData);
void CmDmLeSetPhyReqHandler(cmInstanceData_t *cmData);
void CmDmLeSetDefaultPhyReqHandler(cmInstanceData_t *cmData);

void CmDmLeSetPhyCfmHandler(cmInstanceData_t *cmData);
void CmDmLeSetDefaultPhyCfmHandler(cmInstanceData_t *cmData);
void CmDmLeSetDefaultPhyIndHandler(cmInstanceData_t *cmData);
void CmDmLeReadPhyCfmHandler(cmInstanceData_t *cmData);
#else
#define  CmDmLeReadPhyReqHandler        NULL
#define CmDmLeSetPhyReqHandler          NULL
#define CmDmLeSetDefaultPhyReqHandler   NULL
#endif

void CmSmGenerateCrossTransKeyRequestRspHandler(cmInstanceData_t *cmData);
void CmDmChangeConnectionLinkKeyReqHandler(cmInstanceData_t *cmData);
void CmDmChangeConnectionLinkKeyCfmHandler(cmInstanceData_t *cmData);

void CmInitSequenceHandler(cmInstanceData_t *cmData,
                           CmInitSeqEvent    event,
                           CsrBtResultCode   resultCode,
                           CsrBtSupplier     resultSupplier);

#ifdef INSTALL_CM_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL
void CmDmLeReadRemoteTransmitPowerLevelReqHandler(cmInstanceData_t * cmData);
void CmDmLeReadRemoteTransmitPowerLevelCfmHandler(cmInstanceData_t * cmData);
#else
#define CmDmLeReadRemoteTransmitPowerLevelReqHandler    NULL
#endif /* INSTALL_CM_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL */

#ifdef INSTALL_CM_LE_SET_TRANSMIT_POWER_REPORTING
void CmDmLeSetTransmitPowerReportingEnableReqHandler(cmInstanceData_t *cmData);
void CmDmLeSetTransmitPowerReportingEnableCfmHandler(cmInstanceData_t *cmData);
void CmDmLeTransmitPowerReportingIndHandler(cmInstanceData_t *cmData);
#else
#define CmDmLeSetTransmitPowerReportingEnableReqHandler     NULL
#endif /* INSTALL_CM_LE_SET_TRANSMIT_POWER_REPORTING */

#ifdef INSTALL_CM_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL
void CmDmLeEnhancedReadTransmitPowerLevelReqHandler(cmInstanceData_t * cmData);
void CmDmLeEnhancedReadTransmitPowerLevelCfmHandler(cmInstanceData_t * cmData);
#else
#define CmDmLeEnhancedReadTransmitPowerLevelReqHandler  NULL
#endif /* INSTALL_CM_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL */

#ifndef CSR_BT_EXCLUDE_HCI_READ_CONN_ACCEPT_TIMEOUT
void CmDmReadConnAcceptTimeoutReqHandler(cmInstanceData_t * cmData);
void CmDmReadConnAcceptTimeoutCfmHandler(cmInstanceData_t * cmData);
#else
#define CmDmReadConnAcceptTimeoutReqHandler NULL
#endif /* CSR_BT_EXCLUDE_HCI_READ_CONN_ACCEPT_TIMEOUT */

#ifndef CSR_BT_EXCLUDE_HCI_WRITE_CONN_ACCEPT_TIMEOUT
void CmDmWriteConnAcceptTimeoutReqHandler(cmInstanceData_t * cmData);
void CmDmWriteConnAcceptTimeoutCfmHandler(cmInstanceData_t * cmData);
#else
#define CmDmWriteConnAcceptTimeoutReqHandler NULL
#endif /* CSR_BT_EXCLUDE_HCI_WRITE_CONN_ACCEPT_TIMEOUT */

#ifdef INSTALL_CM_LE_PATH_LOSS_REPORTING
void CmDmLeSetPathLossReportingParametersReqHandler(cmInstanceData_t * cmData);
void CmDmLeSetPathLossReportingParametersCfmHandler(cmInstanceData_t * cmData);
void CmDmLeSetPathLossReportingEnableReqHandler(cmInstanceData_t * cmData);
void CmDmLeSetPathLossReportingEnableCfmHandler(cmInstanceData_t * cmData);
void CmDmLePathLossThresholdIndHandler(cmInstanceData_t * cmData);
#else
#define CmDmLeSetPathLossReportingParametersReqHandler NULL
#define CmDmLeSetPathLossReportingEnableReqHandler NULL
#endif /* INSTALL_CM_LE_PATH_LOSS_REPORTING */

CsrBool CmSearchPendingListByType(CsrCmnListElm_t *elem, void *data);

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_CM_DM_H__ */

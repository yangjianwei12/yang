#ifndef CSR_BT_CM_RFC_H__
#define CSR_BT_CM_RFC_H__
/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_cm_main.h"
#include "rfcommlib.h"
#include "csr_bt_rfc_proxy_lib.h"
#include "csr_bt_cm_util.h"

#define RFC_PSM                        (0x0003)

#define RFC_LOCAL_RELEASE               (0)
#define RFC_REMOTE_RELEASE              (1)
#define RFC_RELEASE_FAIL                (2)

#define RFC_FORCE_CLOSE_TIME            (1000000)
#if defined(CSR_TARGET_PRODUCT_WEARABLE)
#define CSR_BT_CM_RFC_TX_WINDOW_SIZE    (5)
#else
#define CSR_BT_CM_RFC_TX_WINDOW_SIZE    (2)
#endif
#define RFC_FLAGS_UNUSED                0

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    CsrBtConnId   bt_conn_id;
    CsrUint8      server;
} cmConnIdServerType;

typedef struct
{
    CsrBtConnId   bt_conn_id;
    CsrUint16     context;
    CsrUint8      server;
} cmConnIdServerContextType;

typedef struct
{
    CsrBtConnId     bt_conn_id;
    CsrUint8        lServer;
    CsrUint8        rServer;
    CsrBtDeviceAddr devAddr;
} cmConnIdLocalRemoteServersType;

typedef struct
{
    CsrBtConnId     bt_conn_id;
    CsrUint8        lServer;
    CsrUint8        rServer;
    CsrBtDeviceAddr devAddr;
    CsrUint16       context;
} cmConnIdLocalRemoteServersContextType;

typedef struct
{
    CsrBtConnId    bt_conn_id;
    CsrUint32      state;
} cmConnIdStateType;

#define CM_RFC_GET_FIRST(list) \
    ((cmRfcConnElement *)CsrCmnListGetFirst(&(list)))
#define CM_FIND_RFC_ELEMENT(func, searchDataPtr) \
    ((cmRfcConnElement *)CsrCmnListSearch(&(cmData->rfcVar.connList), func, searchDataPtr))

#define CM_RFC_ELEMENT_ACTIVE(_cmData)                                                  \
    ((_cmData)->rfcVar.activeElemId != CM_ERROR ?                                       \
        (cmRfcConnElement *) CsrCmnListSearchOffsetUint8(&(_cmData)->rfcVar.connList,   \
                                                         CsrOffsetOf(cmRfcConnElement,  \
                                                                     elementId),        \
                                                         (_cmData)->rfcVar.activeElemId) : \
        NULL)

cmConnIdServerType CsrBtCmReturnConnIdServerStruct(CsrBtConnId bt_conn_id, CsrUint8 server);
cmConnIdServerContextType CsrBtCmReturnConnIdServerContextStruct(CsrBtConnId bt_conn_id, CsrUint8 server, CsrUint16 context);
cmConnIdLocalRemoteServersType CsrBtCmReturnConnIdLocalRemoteServersStruct(CsrBtConnId bt_conn_id, CsrUint8 localServer, CsrUint8 remoteServer, CsrBtDeviceAddr devAddr);
cmConnIdLocalRemoteServersContextType CsrBtCmReturnConnIdLocalRemoteServersContextStruct(CsrBtConnId       bt_conn_id,
                                                                                         CsrUint8          localServer,
                                                                                         CsrUint8          remoteServer,
                                                                                         CsrBtDeviceAddr   devAddr,
                                                                                         CsrUint16         context);

CsrBool CsrBtCmIncomingSecRegisterDeregisterRequired(cmInstanceData_t *cmData, CsrUint8 server);

/*************************************************************************************
 These function are found in CsrBtCmRfcArrivalHandler.c
************************************************************************************/
void CsrBtCmRfcArrivalHandler(cmInstanceData_t *cmData);

/*************************************************************************************
 These function are found in cmRfcRegisterHandler.c
************************************************************************************/
void CsrBtCmRfcRegisterCfmHandler(cmInstanceData_t *cmData);

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
void CsrBtCmRfcRegisterReqHandler(cmInstanceData_t *cmData);
void CsrBtCmRfcUnRegisterReqMsgSend(cmInstanceData_t *cmData);
#else
#define CsrBtCmRfcRegisterReqHandler NULL
#define CsrBtCmRfcUnRegisterReqMsgSend NULL
#endif

/*************************************************************************************
 These function are found in cmRfcConnectHandler.c
************************************************************************************/
void CsrBtCmRfcStartInitiateConnection(cmInstanceData_t *cmData,
                                       cmRfcConnInstType * theLogicalLink);
void CsrBtCmConnectCfmMsgSend(cmInstanceData_t *cmData,
                              CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
void CsrBtCmRfcConnectReqHandler(cmInstanceData_t *cmData);
void CsrBtCmSmAccessReqHandler(cmInstanceData_t *cmData);
void CsrBtCmCancelConnectReqHandler(cmInstanceData_t *cmData);

#ifdef CSR_BT_INSTALL_CM_PRI_CONNECT_EXT
void CsrBtCmRfcConnectReqExtHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmRfcConnectReqExtHandler NULL
#endif /* CSR_BT_INSTALL_CM_PRI_CONNECT_EXT */

#else /* EXCLUDE_CSR_BT_RFC_MODULE */
#define CsrBtCmRfcConnectReqHandler NULL
#define CsrBtCmSmAccessReqHandler   NULL
#define CsrBtCmCancelConnectReqHandler NULL
#endif /* !EXCLUDE_CSR_BT_RFC_MODULE */

void CsrBtCmDmSmAccessReqMsgSend(void);
void CsrBtCmDmSmAccessCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmRfcCommonErrorHandler(cmInstanceData_t *cmData,
                                  cmRfcConnInstType * theLogicalLink);
void CmRfcRemoteConnectionStateHandler(cmInstanceData_t    *cmData,
                                       cmRfcConnElement    *connElement);
void CsrBtCmRfcServerConnectCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmRfcServerConnectIndHandler(cmInstanceData_t *cmData);
void CsrBtCmRfcClientConnectCfmHandler(cmInstanceData_t *cmData);

/*************************************************************************************
 These function are found in cmRfcAcceptConnectHandler.c
************************************************************************************/
void CsrBtCmConnectAcceptCfmMsgSend(cmInstanceData_t *cmData, 
                                    cmRfcConnElement *theElement, 
                                    CsrBtResultCode resultCode, 
                                    CsrBtSupplier resultSupplier);

void csrBtCmAcceptConnectCfmMsgSend(CsrSchedQid                  appHandle,
                                           BD_ADDR_T            deviceAddr,
                                           CsrBtConnId           btConnId,
                                           CsrUint8              localServerChannel,
                                           CsrUint16             profileMaxFrameSize,
                                           CsrBtResultCode      resultCode,
                                           CsrBtSupplier  resultSupplier,
                                           CsrUint16     context);
                                    
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
void CsrBtCmRfcConnectAcceptTimeoutHandler(cmInstanceData_t *cmData);
void CsrBtCmRfcAcceptConnectReqHandler(cmInstanceData_t *cmData);
void CsrBtCmRfcCancelConnectAcceptReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmRfcAcceptConnectReqHandler       NULL
#define CsrBtCmRfcConnectAcceptTimeoutHandler   NULL
#define CsrBtCmRfcCancelConnectAcceptReqHandler NULL
#endif
/*************************************************************************************
 These function are found in cmRfcReleaseHandler.c
************************************************************************************/
CsrBool CsrBtCmRfcReleaseStatus(CsrBtReasonCode     reasonCode,
                               CsrBtSupplier reasonSupplier);

void CsrBtCmDisconnectIndMsgCleanupSend(cmInstanceData_t    *cmData,
                                        cmRfcConnElement    *theElement,
                                        CsrBtReasonCode      reasonCode,
                                        CsrBtSupplier        reasonSupplier,
                                        CsrBool              status,
                                        CsrBool              localTerminated);

void CsrBtCmRfcReleaseIndHandler(cmInstanceData_t *cmData);
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
void CsrBtCmRfcReleaseReqHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmRfcReleaseReqHandler NULL
#endif

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
void CsrBtCmRfcDisconnectRspHandler(cmInstanceData_t *cmData);
#else
#define CsrBtCmRfcDisconnectRspHandler NULL
#endif

void CsrBtCmRfcConnectAcceptRspHandler(cmInstanceData_t *cmData);

/*************************************************************************************
 These function are found in cmRfcControlHandler.c
************************************************************************************/
void CsrBtCmControlIndMsgSend(cmRfcConnInstType * theLogicalLink,
                              CsrUint8 modem_signal,
                              CsrUint8 break_signal);
void CsrBtCmRfcModemStatusIndHandler(cmInstanceData_t *cmData);
void CsrBtCmRfcModemStatusCfmHandler(cmInstanceData_t *cmData);

/*************************************************************************************
 These function are found in cmRfcDataHandler.c
************************************************************************************/
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
void CsrBtCmRfcControlReqMsgSend(cmInstanceData_t *cmData);
#ifdef CSR_STREAMS_ENABLE
#define CsrBtCmRfcDataReqHandler            csrBtCmCommonDataPrimHandler
#define CsrBtCmRfcDataResHandler            csrBtCmCommonDataPrimHandler
#else /* CSR_STREAMS_ENABLE */
void CsrBtCmRfcDataReqHandler(cmInstanceData_t *cmData);
void CsrBtCmRfcDataResHandler(cmInstanceData_t *cmData);
#endif
#else /* EXCLUDE_CSR_BT_RFC_MODULE */
#define CsrBtCmRfcDataReqHandler NULL
#define CsrBtCmRfcDataResHandler NULL
#define CsrBtCmRfcControlReqMsgSend NULL
#endif /* !EXCLUDE_CSR_BT_RFC_MODULE */

#ifndef CSR_STREAMS_ENABLE
void CsrBtCmRfcRestoreDataInReceiveBuffer(cmRfcConnInstType *theLogicalLink);
void CsrBtCmRfcDataIndHandler(cmInstanceData_t *cmData);
/*************************************************************************************
 These function are found in cmRfcDataNoCredit.c
************************************************************************************/
void CsrBtCmRfcDataWriteCfmHandler(cmInstanceData_t *cmData);
#endif /* !CSR_STREAMS_ENABLE */

/*************************************************************************************
 These function are found in cmRfcScoHandler.c
************************************************************************************/
void CsrBtCmDmSyncDisconnectRfcHandler(cmInstanceData_t *cmData, cmRfcConnInstType *theLogicalLink,
                                       CsrUint8 status, CsrBtReasonCode reason);
void CsrBtCmDmSyncConnectRfcCfmHandler(cmRfcConnInstType *theLogicalLink, cmInstanceData_t *cmData);
void CsrBtCmDmSyncConnectCompleteRfcHandler(cmRfcConnInstType *theLogicalLink,
                                            DM_SYNC_CONNECT_COMPLETE_IND_T * dmPrim);
void CsrBtCmDmScoRenegotiateCfmMsgSend(cmRfcConnInstType * theLogicalLink,
                    CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);
void CsrBtCmDmScoRenegotiateIndMsgSend(cmRfcConnInstType * theLogicalLink,
                    CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);

/*************************************************************************************
 These function are found in cmRfcMaintenanceHandler.c
************************************************************************************/
CsrUint16 CsrBtCmRfcDetermineMtu(cmInstanceData_t *cmData, CsrBtDeviceAddr deviceAddr, 
                                 CsrUint16 proposedMtu);
cmRfcConnElement *CsrBtCmRfcFindRfcConnElementFromDeviceAddrState(cmInstanceData_t *cmData,
                                                                  CsrBtDeviceAddr *deviceAddr, 
                                                                  CsrUint32 state);
CsrBool CsrBtCmRfcConnElementIndexCheck(CsrCmnListElm_t *elem, void *value);
void CsrBtCmRfcEscoParmsFree(cmRfcConnInstType *connInst);
void cleanUpConnectionTable(cmRfcConnInstType ** theLogicalLink);
CsrBool CsrBtCmRfcFindRfcConnElementFromIndex(CsrCmnListElm_t *elem, void *value);
CsrBool CsrBtCmRfcFindRfcConnElementFromBtPendingConnId(CsrCmnListElm_t * elem, void * value);
#ifndef EXCLUDE_CSR_BT_SCO_MODULE
CsrBool CsrBtCmRfcFindRfcConnElementFromScoHandle(CsrCmnListElm_t *elem, void *value);
#endif
CsrBool CsrBtCmRfcFindRfcConnElementFromBtConnId(CsrCmnListElm_t *elem, void *value);
CsrBool CsrBtCmRfcFindRfcConnElementFromConnIdServer(CsrCmnListElm_t *elem, void *value);
CsrBool CsrBtCmRfcFindRfcConnElementFromConnIdServerContext(CsrCmnListElm_t *elem, void *value);
CsrBool CsrBtCmRfcFindRfcConnElementFromConnIdLocalRemoteServers(CsrCmnListElm_t *elem, void *value);
CsrBool CsrBtCmRfcFindRfcConnElementFromConnIdLocalRemoteServersContext(CsrCmnListElm_t *elem, void *value);

CsrBool CsrBtCmRfcFindRfcConnElementFromConnIdRemoteServer(CsrCmnListElm_t *elem, void *value);
cmRfcConnElement *CsrBtCmRfcFindRfcConnElementFromServerState(cmInstanceData_t *cmData, 
                                                              CsrUint8 server, 
                                                              CsrUint32 state);
#ifndef EXCLUDE_CSR_BT_SCO_MODULE
cmRfcConnElement *CsrBtCmRfcFindRfcConnElementFromDeviceAddrScoHandle(cmInstanceData_t *cmData, 
                                                                      CsrBtDeviceAddr *deviceAddr, 
                                                                      hci_connection_handle_t scoHandle);
#endif
cmRfcConnElement *CsrBtCmRfcFindRfcConnElementFromDeviceAddrState1OrState2(cmInstanceData_t *cmData, 
                                                                           CsrBtDeviceAddr *deviceAddr, 
                                                                           CsrUint32 state1, 
                                                                           CsrUint32 state2);

#ifdef INSTALL_CONTEXT_TRANSFER
void CmRfcConnectCb(bool is_client, RFC_CONNECT_CFM_T *prim);
#endif /* ifdef INSTALL_CONTEXT_TRANSFER */


/*************************************************************************************
 These function are found in csr_bt_cm_rfc_amp_handler.c
************************************************************************************/
#ifdef CSR_AMP_ENABLE
void CsrBtCmRfcAmpMoveChannelCfmHandler(cmInstanceData_t *cmData);
void CsrBtCmRfcAmpMoveChannelCmpIndHandler(cmInstanceData_t *cmData);
void CsrBtCmRfcAmpMoveChannelIndHandler(cmInstanceData_t *cmData);
void CsrBtCmRfcAmpMoveChannelReqHandler(cmInstanceData_t *cmData);
void CsrBtCmRfcAmpMoveChannelResHandler(cmInstanceData_t *cmData);
void CsrBtCmRfcAmpLinkLossIndHandler(cmInstanceData_t *cmData);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_CM_RFC_H__ */

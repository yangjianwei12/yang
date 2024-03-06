#ifndef CSR_BT_PAC_HANDLER_H__
#define CSR_BT_PAC_HANDLER_H__
/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_pac_prim.h"
#include "csr_bt_obex_util.h"
#include "csr_log_text_2.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtPacLto);

#define PAC_INSTANCE_STATE_IDLE                     (0)
#define PAC_INSTANCE_STATE_CONNECTING               (1)
#define PAC_INSTANCE_STATE_CONNECTED                (2)

#define PAC_LTSO_STATE                              (1)

#define PAC_INSTANCE_STATE_CHANGE(_var, _state)      \
do                                                   \
{                                                    \
    CSR_LOG_TEXT_DEBUG((CsrBtPacLto,                 \
                         PAC_LTSO_STATE,             \
                         #_var" state: %d -> %d",    \
                         (_var),                     \
                         (_state)));                 \
    (_var) = (_state);                               \
} while (0)

void CsrBtPacInit(void **gash);
void CsrBtPacHandler(void **gash);

#ifdef INSTALL_PAC_MULTI_INSTANCE_SUPPORT
typedef struct PacInstancePool
{
    CsrUint8                numberInPool;
    CsrSchedQid             phandles[PAC_MAX_NUM_INSTANCES];
} PacInstancePool;
#endif

typedef struct
{
    CsrSchedQid             appHandle;      /* Application handle */
    void                   *obexInst;       /* OBEX instance */
    CsrSchedQid             pacInstanceId;  /* PAC Instance Id */
    CsrBtDeviceAddr         deviceAddr;     /* PSE device address */
    CsrUint8                state;          /* Current state of the instance */

#ifdef INSTALL_PAC_MULTI_INSTANCE_SUPPORT
    /* Instance specific info */
    PacInstancePool        *pacInstances;   /* Registered instances info */
#endif

    /* Connection specific information */
#ifdef INSTALL_PAC_CUSTOM_SECURITY_SETTINGS
    dm_security_level_t     secOutgoing;
#endif
    CsrUint32               supportedFeatures;      /* Aggregate supported features for a connection */
    CsrBtPacSrcType         supportedRepositories;  /* Repositories supported by remote server */

    /* Operation specific fields */
    CsrBtPacPrim            operation;      /* This is used to distinguish handling of OBEX GET responses */
    CsrBool                 isFinal;        /* Flag to signify last response packet and send operation confirmation */
    CsrUint8                srmp;           /* Holds current SRMP header status */

    CsrUint16               curFolderId;    /* Current folder */
    CsrUint16               targetFolderId; /* Target folder of current operation */

    /* Remote server application header parameters */
    CsrUint16               pbSize;         /* Phonebook size mentioned in GET response */
    CsrUint8                newMissedCall;  /* Number of new missed calls returned in GET response */
    CsrBtPbVersionInfo      versionInfo;    /* Folder version information returned in GET response */

    /* Local application header parameters */
    CsrUint8                (*propertySel)[8];  /* Alias of property selector array passed by application */
    CsrUint8                (*vcardSel)[8];     /* Alias of vCard selector array passed by application */
    CsrUint8                vcardSelOp;
    CsrUint8                order;
    CsrCharString           *searchVal;
    CsrUint8                searchAtt;
    CsrUint16               maxLstCnt;
    CsrUint16               lstStartOff;
    CsrBtPacFormatType      format;
    CsrUint8                rstMissedCall;
} PacInst;

void CsrBtPacMessagePut(CsrSchedQid phandle, void *msg);

typedef CsrUint8 (*PacStateHandlerType)(PacInst *pacInstanceData, void *msg);

void CsrBtPacResetLocalAppHeaderPar(PacInst *pInst);
void CsrBtPacResetRemoteAppHeaderPar(PacInst *pInst);

void CsrBtPacPullPbCfmSend(PacInst *pInst,
                           CsrBtObexResponseCode responseCode);
void CsrBtPacPullPbIndSend(PacInst *pInst,
                           CsrUint16 bodyOffset,
                           CsrUint16 bodyLength,
                           CsrUint8 *obexPacket,
                           CsrUint16 obexPacketLength);
void CsrBtPacPullvCardEntryCfmSend(PacInst *pInst,
                                   CsrBtObexResponseCode responseCode);
void CsrBtPacPullvCardEntryIndSend(PacInst *pInst,
                                   CsrUint16 bodyOffset,
                                   CsrUint16 bodyLength,
                                   CsrUint8 *obexPacket,
                                   CsrUint16 obexPacketLength);
void CsrBtPacPullvCardListIndSend(PacInst *pInst,
                                  CsrUint16 bodyOffset,
                                  CsrUint16 bodyLength,
                                  CsrUint8 *obexPacket,
                                  CsrUint16 obexPacketLength);
void CsrBtPacPullvCardListCfmSend(PacInst *pInst,
                                  CsrBtObexResponseCode responseCode);
void CsrBtPacSetFolderCfmSend(PacInst *pInst,
                              CsrBtObexResponseCode responseCode);
void CsrBtPacSetBackFolderCfmSend(PacInst *pInst,
                                  CsrBtObexResponseCode responseCode);
void CsrBtPacSetRootFolderCfmSend(PacInst *pInst,
                                  CsrBtObexResponseCode responseCode);
void CsrBtPacAbortCfmSend(PacInst *pInst);

#ifdef INSTALL_PAC_CUSTOM_SECURITY_SETTINGS
void CsrBtPacSecurityOutCfmSend(PacInst *pInst,
                                CsrBtPacSecurityOutReq *pMsg,
                                CsrBtResultCode result);
#endif

void CsrBtPacConnectCfmSend(PacInst *pInst,
                            CsrBtResultCode resultCode,
                            CsrBtSupplier resultSupplier,
                            CsrUint16 maxPeerObexPacketLength,
                            CsrBtConnId cid,
                            CsrSchedQid appHandle);
void CsrBtPacAuthenticateIndSend(PacInst *pInst,
                                 const CsrBtDeviceAddr *deviceAddr,
                                 CsrUint8 options,
                                 CsrUint16 realmLength,
                                 CsrUint8 *realm);
void CsrBtPacDisconnectIndSend(PacInst *pInst,
                               CsrBtReasonCode reasonCode,
                               CsrBtSupplier reasonSupplier);
void PacGetInstanceIdsCfmSend(PacInst* pInst,
                              PacGetInstanceIdsReq *req);

CsrUint8 CsrBtPacConnectReqHandler(PacInst *pInst, void *msg);
CsrUint8 CsrBtPacAuthenticateResHandler(PacInst *pInst, void *msg);
CsrUint8 CsrBtPacPullPbReqHandler(PacInst *pInst, void *msg);
CsrUint8 CsrBtPacPullPbResHandler(PacInst *pInst, void *msg);
CsrUint8 CsrBtPacPullvCardEntryReqHandler(PacInst *pInst, void *msg);
CsrUint8 CsrBtPacPullvCardEntryResHandler(PacInst *pInst, void *msg);
CsrUint8 CsrBtPacPullvCardListReqHandler(PacInst *pInst, void *msg);
CsrUint8 CsrBtPacPullvCardListResHandler(PacInst *pInst, void *msg);
CsrUint8 CsrBtPacSetFolderReqHandler(PacInst *pInst, void *msg);
CsrUint8 CsrBtPacSetBackFolderReqHandler(PacInst *pInst, void *msg);
CsrUint8 CsrBtPacSetRootFolderReqHandler(PacInst *pInst, void *msg);
CsrUint8 CsrBtPacAbortReqHandler(PacInst *pInst, void *msg);
CsrUint8 CsrBtPacDisconnectReqHandler(PacInst *pInst, void *msg);
CsrUint8 CsrBtPacCancelConnectReqHandler(PacInst *pInst, void *msg);

#ifdef INSTALL_PAC_MULTI_INSTANCE_SUPPORT
CsrUint8 PacRegisterQIDReqHandler(PacInst *pInst, void *msg);
#else
#define PacRegisterQIDReqHandler       NULL
#endif
CsrUint8 PacGetInstanceIdsReqHandler(PacInst *pInst, void *msg);
#ifdef CSR_TARGET_PRODUCT_VM
CsrBool PacSetAppHandle(CsrSchedQid pacInstanceId, CsrSchedQid appHandle);
#endif

#ifdef INSTALL_PAC_CUSTOM_SECURITY_SETTINGS
CsrUint8 CsrBtPacSecurityOutReqHandler(PacInst *pInst, void *msg);
#else
#define CsrBtPacSecurityOutReqHandler       NULL
#endif

void CsrBtPacCmSdsRegisterReqHandler(PacInst *pInst);
void CsrBtPacCmSdsRegistertCfmHandler(PacInst *pInst, void *msg);

/* Public functions which return the callback handlers statically defined in csr_bt_pac_sef.c */
ObexUtilAuthenticateIndFuncType PacDeliverAuthenticateIndCb(void);
ObexUtilDisconnectIndFuncType PacDeliverDisconnectIndCb(void);

/* Prototypes from pac_free_down.c */
void CsrBtPacFreeDownstreamMessageContents(CsrUint16 eventClass, void * message);

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_PAC_HANDLER_H__ */

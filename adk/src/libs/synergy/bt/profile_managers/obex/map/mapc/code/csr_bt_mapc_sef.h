#ifndef CSR_BT_MAPC_SEF_H__
#define CSR_BT_MAPC_SEF_H__
/******************************************************************************
 Copyright (c) 2009-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_sched.h"
#include "csr_bt_mapc_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAPC_TEXT_X_BT_NOTIFICATION_REGISTRATION    "x-bt/MAP-NotificationRegistration"
#define MAPC_TEXT_X_BT_MAP_MSG_LISTING              "x-bt/MAP-msg-listing" 
#define MAPC_TEXT_X_BT_MESSAGE                      "x-bt/message"
#define MAPC_TEXT_X_BT_MESSAGE_STATUS               "x-bt/messageStatus"
#define MAPC_TEXT_X_BT_MESSAGE_UPDATE               "x-bt/MAP-messageUpdate"
#define MAPC_TEXT_X_BT_MAS_INSTANCE_INFORMATION     "x-bt/MASInstanceInformation"

#define CSR_BT_OBEX_MAP_SUPP_FEATURES_TAG_ID        0x10
#define CSR_BT_OBEX_MAP_TAG_SIZE                    0x02

void CsrBtMapcMessagePut(CsrSchedQid phandle, void *msg);

CsrUint8 CsrBtMapcGetInstanceIdsReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcConnectReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcCancelConnectReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcDisconnectReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcSelectMasInstanceResHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcSetFolderReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcSetBackFolderReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcSetRootFolderReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcGetFolderListingReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcGetFolderListingResHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcGetMessageListingReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcGetMessageListingResHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcGetMessageReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcGetMessageResHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcSetMessageStatusReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcPushMessageReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcPushMessageResHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcUpdateInboxReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcAbortReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcNotificationRegistrationReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcEventNotificationResHandler(MapcInstanceData *pInst, void *msg);

#ifdef INSTALL_MAPC_CUSTOM_SECURITY_SETTINGS
CsrUint8 CsrBtMapcSecurityInReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcSecurityOutReqHandler(MapcInstanceData *pInst, void *msg);
#else
#define CsrBtMapcSecurityInReqHandler               NULL
#define CsrBtMapcSecurityOutReqHandler              NULL
#endif

CsrUint8 CsrBtMapcRegisterQIDReqHandler(MapcInstanceData *pInst, void *msg);
CsrBool CsrBtMapcObexServerCmHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcConnectExtReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcGetMasInstanceInformationReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcGetConversationListingReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcGetConversationListingResHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcGetOwnerStatusReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcSetOwnerStatusReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcSetNotificationFilterReqHandler(MapcInstanceData *pInst, void *msg);

void csrBtMapcRegisterMnsRecord(MapcInstanceData *pInst);

/* Private Downstream message handlers */
CsrUint8 CsrBtMapcAddNotiReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcRemoveNotiReqHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcServiceCleanupHandler(MapcInstanceData *pInst, void *msg);

/* Private upstream message handlers */
CsrUint8 CsrBtMapcAddNotiCfmHandler(MapcInstanceData *pInst, void *msg);
CsrUint8 CsrBtMapcRemoveNotiCfmHandler(MapcInstanceData *pInst, void *msg);

/* Prototypes from mapc_free_down.c */
void CsrBtMapcFreeDownstreamMessageContents(CsrUint16 eventClass, void * message);

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_MAPC_SEF_H__ */


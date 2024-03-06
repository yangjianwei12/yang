#ifndef CSR_BT_MAPC_HANDLER_H__
#define CSR_BT_MAPC_HANDLER_H__
/******************************************************************************
 Copyright (c) 2009-2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/


#include "csr_synergy.h"
#include "csr_log_text_2.h"
#include "csr_bt_tasks.h"
#include "csr_bt_mapc_prim.h"
#include "csr_bt_obex_util.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CSR_BT_MAPC_CLIENT_INST_ID     0x01
#define MAPC_NOTI_CHANNEL_REGISTER_CONTEXT_ID   0xFF

#define MAPC_INSTANCES_POOL_SIZE         (NUM_MAPC_INST)

#define CSR_BT_OBEX_MAPC_TAG_SIZE   (2)
#define CSR_BT_OBEX_MAPC_TAG_INSTANCE_ID        0x0f
#define CSR_BT_OBEX_MAPC_TAG_INSTANCE_ID_LEN    0x01

#define MAS_INVALID_INSTANCE 0xff

#ifdef CSR_LOG_ENABLE
/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtMapcLth);
#endif /*CSR_LOG_ENABLE */

typedef struct CsrBtMapcInstancePool
{
    CsrUint8                        numberInPool;
    CsrSchedQid                     phandles[MAPC_INSTANCES_POOL_SIZE];
    struct CsrBtMapcInstancePool   *next;
} CsrBtMapcInstancePool;

typedef struct
{
    CsrUint16               serviceIdx;
    CsrUint8                masInstanceId;
    CsrBtMapSupportedFeatures features;/* remote supported features */
} CsrBtMapcMasInstIdMap;

#define MAPC_MAS_STATE_IDLE                     0
#define MAPC_MAS_STATE_CONNECTED                1
#define MAPC_MAS_STATE_DISCONNECT_DEREGISTER    2

typedef struct
{
    CsrBtMapcInstancePool       *mapcInstances;
    CsrSchedQid                 mapcInstanceId;
    CsrUint8                    numberOfMapcInstances;
    CsrUint16                   maxFrameSize;
    CsrUint16                   windowSize;
    CsrBool                     localSrmEnable;

    /* OBEX Client part of MAPC Client */
    CsrSchedQid                 appHandle;
    CsrBtMapcMasInstIdMap       *masInstToServiceMap;
    CsrUint16                   masInstToServiceMapLength;
    CsrUint8                    masState;
    CsrBtDeviceAddr             deviceAddr;
    CsrUint8                    masInstanceId; /* MASInstanceID from SDP record */

    CsrBool                     notificationRegistrationOn;
#ifdef INSTALL_MAPC_CUSTOM_SECURITY_SETTINGS    
    dm_security_level_t         secIncoming;
    dm_security_level_t         secOutgoing;
#endif
    void                        *obexClientInst;
    CsrUint16                   fullSize;
    CsrUint16                   partialSize;
    CsrUint8                    newMessage;
    CsrBtMapFracDel             fractionDeliver;
    CsrUtf8String               *mseTime;
    CsrBtConnId                 masConnId;
    CsrUtf8String               *databaseId;
    CsrUtf8String               *folderVersionCounter;
    CsrBtMapPresence            presenceAvailability;
    CsrUtf8String               *presenceText;
    CsrUtf8String               *lastActivity;
    CsrBtMapChatState           chatState;
    CsrUtf8String               *convListingVersionCounter;
    CsrUint16                   numberOfConversations;
    CsrUint8                    srmp;           /* Holds current SRMP header status */

#ifdef CSR_AMP_ENABLE
    CsrBtConnId                 slaveBtConnId;
#endif /* CSR_AMP_ENABLE */
    CsrBool                     normalDisconnect;
    CsrCmnList_t                notiServiceList;/* [MapcNotiService_t] List for notification service */
    CsrUint8                    mnsServerChannel;
    psm_t                       mnsPsm;
} MapcInstanceData;

typedef struct notiInstance
{
    struct notiInstance    *next;
    struct notiInstance    *prev;
    CsrUint8                masInstanceId;/* MASInstanceID from SDP record */
    CsrSchedQid             mapHandle;
    CsrSchedQid             appHandle;
    CsrBool                 awaitingDisc;
} notiInstance_t;

typedef struct MapcNotiService
{
    struct MapcNotiService *next;
    struct MapcNotiService *prev;
    CsrBtDeviceAddr         deviceAddr;
    CsrCmnList_t            notiInstanceList;/* [notiInstance_t] List of instances associated to this device */
    CsrUint8                obexInstId;
    void                   *obexInst;
    dm_security_level_t     security;
    CsrUint16               maxFrameSize;
    CsrBool                 putFinalFlag;
    CsrBtConnId             connId;
    CsrBool                 putOp;
    CsrUint8                tempMasInstanceId; /* Temporary mas instance for which the PUT operation is ongoing */
    MapcInstanceData       *mapcInst;
} MapcNotiService_t;

typedef CsrUint8 (* MapcStateHandlerType)(MapcInstanceData * mapcInstanceData, void *msg);

/* Macros for Notification service list */
#define MAPC_NOTI_INSTANCE_ID_INVALID         (0xFF) /** Indicates that a Instance ID is invalid */

#define NOTI_SERVICE_LIST_ADD_FIRST(listPtr)         \
            ((MapcNotiService_t *)CsrCmnListElementAddFirst((listPtr),sizeof(MapcNotiService_t)))

#define NOTI_SERVICE_LIST_REMOVE(listPtr, elemPtr)   \
            (CsrCmnListElementRemove((listPtr), (CsrCmnListElm_t *)(elemPtr)))

#define NOTI_SERVICE_LIST_GET_FIRST(listPtr)         \
            ((MapcNotiService_t *)CsrCmnListGetFirst((listPtr)))

#define NOTI_SERVICE_LIST_GET_ADDR(listPtr, addPtr)  \
            ((MapcNotiService_t *)CsrCmnListSearchOffsetAddr((listPtr), offsetof(MapcNotiService_t, deviceAddr), addPtr))

#define NOTI_SERVICE_LIST_GET_INSTID(listPtr, id)    \
            ((MapcNotiService_t *)CsrCmnListSearchOffsetUint8((listPtr), offsetof(MapcNotiService_t, obexInstId), id))

#define NOTI_SERVICE_LIST_GET_CONNID(listPtr, id)    \
            ((MapcNotiService_t *)CsrCmnListSearchOffsetUint32((listPtr), offsetof(MapcNotiService_t, connId), id))


/* Macros for Notification instance list */
#define NOTI_INSTANCE_LIST_ADD_FIRST(listPtr)         \
            ((notiInstance_t *)CsrCmnListElementAddFirst((listPtr),sizeof(notiInstance_t)))

#define NOTI_INSTANCE_LIST_REMOVE(listPtr, elemPtr)   \
            (CsrCmnListElementRemove((listPtr), (CsrCmnListElm_t *)(elemPtr)))

#define NOTI_INSTANCE_LIST_GET_FIRST(listPtr)         \
            ((notiInstance_t *)CsrCmnListGetFirst((listPtr)))

#define NOTI_INSTANCE_LIST_GET_INSTID(listPtr, id)    \
            ((notiInstance_t *)CsrCmnListSearchOffsetUint8((listPtr), offsetof(notiInstance_t, masInstanceId), id))

#define NOTI_INSTANCE_LIST_GET_MAPHANDLE(listPtr, id)    \
            ((notiInstance_t *)CsrCmnListSearchOffsetUint16((listPtr), offsetof(notiInstance_t, mapHandle), id))

#define NOTI_INSTANCE_LIST_GET_APPHANDLE(listPtr, id)    \
            ((notiInstance_t *)CsrCmnListSearchOffsetUint16((listPtr), offsetof(notiInstance_t, appHandle), id))

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_MAPC_HANDLER_H__ */


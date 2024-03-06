/* Copyright (c) 2019 - 2022 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_CSIS_CLIENT_PRIVATE_H
#define GATT_CSIS_CLIENT_PRIVATE_H

#include <stdlib.h>

#include "gatt_csis_client.h"
#include "csr_list.h"
#include "service_handle.h"
#include "csr_bt_gatt_client_util_lib.h"

/* Defines for csis client library */

/* Invalid CSIS handle*/
#define INVALID_CSIS_HANDLE (0xffff)

/* CSIS Lock notification value */
#define CSIS_NOTIFICATION_VALUE   (0x01)
#define GATT_CSIS_WRITE_CCD_VALUE_LENGTH (0x02)

/* CSIS SIRK and SIZE notification supported */
#define CSIS_SIRK_SIZE_NOTIFICATION_SUPPORTED     (0x01)

 /* Length of PSRI */
#define GATT_CSIS_PSRI_DATA_LENGTH          (sizeof(uint8) * 6)

 /* Macros for creating messages */
#define MAKE_CSIS_CLIENT_MESSAGE(TYPE) TYPE *message = (TYPE *)CsrPmemZalloc(sizeof(TYPE));

#define CsisMessageSend(TASK, ID, MSG) {\
    MSG->id = ID; \
    CsrSchedMessagePut(TASK, CSIS_CLIENT_PRIM, MSG);\
    }

#define CsisMessageSendConditionally(TASK, ID, MSG, CMD) CsisMessageSend(TASK, ID, MSG)

/* Macro to check Size characteristics support */
#define CSIS_CHECK_SIZE_SUPPORT(csis_client) \
    (csis_client->handles.csisSizeHandle!=INVALID_CSIS_HANDLE)?(TRUE):(FALSE)

#define CSIS_CHECK_LOCK_SUPPORT(csis_client) \
    (csis_client->handles.csisLockHandle!=INVALID_CSIS_HANDLE)?(TRUE):(FALSE)

/* UUID values as per CSIPS_Assigned_Numbers_v1-BN-allocated-r02*/
#define GATT_CHARACTERISTIC_UUID_SET_IDENTITY_RESOLVING_KEY                 (0x2B84)
#define GATT_CHARACTERISTIC_UUID_COORDINATED_SET_SIZE                       (0x2B85)
#define GATT_CHARACTERISTIC_UUID_COORDINATED_SET_LOCK                       (0x2B86)
#define GATT_CHARACTERISTIC_UUID_COORDINATED_SET_RANK                       (0x2B87)

#define GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_UUID                       (0x2902)

typedef int32      GattCsisInternalMessageId;

#define CSIS_CLIENT_INTERNAL_MSG_DISCOVER         ((GattCsisMessageId) 0x01)
#define CSIS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ ((GattCsisMessageId) 0x02)
#define CSIS_CLIENT_INTERNAL_MSG_READ_CS_INFO_REQ ((GattCsisMessageId) 0x03)
#define CSIS_CLIENT_INTERNAL_MSG_WRITE_LOCK_REQ   ((GattCsisMessageId) 0x04)

typedef struct
{
    CsrCmnList_t serviceHandleList;
} GattCsisClient;

typedef struct CsisC
{
    AppTaskData lib_task;                   /*! Lib Task */
    AppTask app_task;                       /*! Application Registered Task */

    GattCsisClientDeviceData handles;

    uint16 csisLockEndHandle;           /*! Lock assocaited with Coorinated Set Member Handle characteristics End Handle*/
    uint16 csisSirkEndHandle;           /*! Set Identity Resolving Key Handle End Handle*/
    uint16 csisSizeEndHandle;           /*! Size of Coordinated Set Handle: End Handle*/

    uint16 pendingCmd;                  /*! Any read/write command pending? */
    uint16 pendingHandle;

    uint16 sirkNotificationSupported:1; /*! SIRK notification is optional property of server, if supported
                                                then server supports size characterisctics notification too */
    uint16 lockNotificationEnabled:1;   /*! Notification has been enabled/disabled */
    uint16 sirkNotificationEnabled:1;   /*! Notification has been enabled/disabled */
    uint16 sizeNotificationEnabled:1;   /*! Notification has been enabled/disabled */

    /* GattId, cid and service handle is a part of this structure */
    ServiceHandleListElm_t *srvcElem;
}CsisC;

typedef struct
{
    connection_id_t cid;
    uint16 startHandle;
    uint16 endHandle;
} GattCsisClientRegistrationParams;

/*! @brief GATT CSIS Client[GCsisC]Library Instance.
 */
typedef struct CsisC GCsisC;

bool csisSrvcHndlFindByGattId(CsrCmnListElm_t *elem, void *data);
CsrUint8 csisInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data);

#define CSIS_ADD_SERVICE_HANDLE(_List) \
    (ServiceHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ServiceHandleListElm_t))

#define CSIS_REMOVE_SERVICE_HANDLE(_List,_ServiceHandle) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        csisInstFindBySrvcHndl,(void *)(&(_ServiceHandle)))

#define CSIS_FIND_SERVICE_HANDLE_BY_GATTID(_List,_id) \
                              ((ServiceHandleListElm_t *)CsrCmnListSearch(&(_List), \
                                        csisSrvcHndlFindByGattId,(void *)(&(_id))))

#define ADD_CSIS_CLIENT_INST(_List) \
                         ServiceHandleNewInstance((void**)(&(_List)),sizeof(GVCSC))

#define FREE_CSIS_CLIENT_INST(_Handle) \
                           ServiceHandleFreeInstanceData(_Handle)

#define FIND_CSIS_INST_BY_SERVICE_HANDLE(_Handle) \
                              (GCsisC) *)ServiceHandleGetInstanceData(_Handle)


/* Defines for CSIS client message which is pending and yet to process completely  */
typedef uint16 CsisClientPendingReadType;

#define CSIS_CLIENT_PENDING_NONE                (CsisClientPendingReadType)0x0000
#define CSIS_CLIENT_INIT_PENDING                (CsisClientPendingReadType)0x0001
#define CSIS_CLIENT_WRITE_PENDING_NOTIFICATION  (CsisClientPendingReadType)0x0002
#define CSIS_CLIENT_WRITE_PENDING_LOCK          (CsisClientPendingReadType)0x0003
#define CSIS_CLIENT_READ_PENDING_CS_INFO_REQ    (CsisClientPendingReadType)0x0004

/*Set the pending FLAG so that only one request is processed at a TIME */
#define SET_PENDING_FLAG(type, pendingRequest) (pendingRequest = type)
/* Clear pending FLAG as new requests can be processed */
#define CLEAR_PENDING_FLAG(pendingRequest) (pendingRequest = CSIS_CLIENT_PENDING_NONE)

/* Internal message structure for Read request  */
typedef struct
{
    GattCsisInternalMessageId id;                          /*! service message id */
    ServiceHandle srvcHndl;
    uint16 handle;       /* Handle of the Characteristics */
} CsisClientInternalMsgReadCsInfoReq;

/* Internal message structure for Write request  */
typedef struct
{
    GattCsisInternalMessageId id;                          /*! service message id */
    ServiceHandle srvcHndl;
    uint16 handle;
    bool enableLock;   /* TRUE/FALSE - get_lock/release_lock*/
} CsisClientInternalMsgWriteLockReq;

/* Internal message structure for discover request  */
typedef struct
{
    GattCsisInternalMessageId id;                          /*! service message id */
    ServiceHandle srvc_hndl;
    uint16 cid;           /*! Connection Identifier for remote device */
    uint16 startHandle;   /*! Start handle of the service */
    uint16 endHandle;     /*! End handle of the service */
} CsisClientInternalMsgDiscover ;

/* Internal Message Structure to Initiate registering notification */
typedef struct
{
    GattCsisInternalMessageId id;  /*! service message id */
    ServiceHandle srvcHndl;
    uint16 handle; /*! CCD handle */
    bool enable;   /*! Enable/Disable notification */
}CsisClientInternalMsgNotificationReq;

#endif /* GATT_CSIS_CLIENT_PRIVATE_H */


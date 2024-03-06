/* Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_MCS_CLIENT_PRIVATE_H_
#define GATT_MCS_CLIENT_PRIVATE_H_


#include <stdlib.h>

#include "gatt_mcs_client.h"
#include "csr_bt_gatt_client_util_lib.h"

/* Size of the Client Characteristic Configuration in number of octects */
#define GATT_MCS_CLIENT_CHARACTERISTIC_CONFIG_SIZE          (2)

#define GATT_ATTR_HANDLE_INVALID                       ((uint16) 0x0000)

#define McsMessageSend(TASK, ID, MSG) {\
    MSG->id = ID; \
    CsrSchedMessagePut(TASK, MCS_CLIENT_PRIM, MSG);\
    }

/*
    @brief Client library private data.
*/
typedef struct
{
    AppTaskData libTask;
    AppTask appTask;

    /* Any read/write command pending */
    uint8 pendingCmd;

    /* This is used to store the handle of the characteristic when a descriptor discovery is pending */
    uint16 pendingHandle;

    GattMcsClientDeviceData handles;

    /* GattId, cid and service handle is a part of this structure */
    ServiceHandleListElm_t *srvcElem;

    uint8 writeCccCount;
} GMCSC;

typedef int32                                            McsClientInternalMsg;

#define MCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ         ((McsClientInternalMsg) 0x01)
#define MCS_CLIENT_INTERNAL_MSG_READ_REQ                     ((McsClientInternalMsg) 0x02)
#define MCS_CLIENT_INTERNAL_MSG_WRITE_REQ                    ((McsClientInternalMsg) 0x03)
#define MCS_CLIENT_INTERNAL_MSG_SET_MEDIA_CONTROL_POINT_REQ  ((McsClientInternalMsg) 0x04)

typedef uint8                                             McsClientPendingOp;
#define MCS_CLIENT_PENDING_OP_NONE                       ((McsClientPendingOp) 0x00)
#define MCS_CLIENT_PENDING_OP_WRITE_CCCD                 ((McsClientPendingOp) 0x01)

#define MCS_CLIENT_INTERNAL_MSG_BASE                     0x00
#define MCS_CLIENT_INTERNAL_MSG_TOP                      0x05

/* Internal Message Structure to Initiate registering notification */
typedef struct
{
    McsClientInternalMsg id;
    ServiceHandle srvcHndl;
    MediaPlayerAttributeMask characType;
    uint32 notifValue;
} McsClientInternalMsgNotificationReq;

/* Internal Message Structure to read a characteristic */
typedef struct
{
    McsClientInternalMsg id;
    ServiceHandle srvcHndl;
    MediaPlayerAttribute charac;
} McsClientInternalMsgRead;

/* Internal Message Structure to write a characteristic */
typedef struct
{
    McsClientInternalMsg id;
    ServiceHandle srvcHndl;
    MediaPlayerAttribute charac;
    uint16 sizeValue;
    uint8*  value;
} McsClientInternalMsgWrite;

typedef struct
{
    McsClientInternalMsg id;
    ServiceHandle srvcHndl;
    GattMcsOpcode op;
    int32 val;
} McsClientInternalMsgSetMediaControlPoint;

typedef struct
{
    CsrCmnList_t serviceHandleList;
} GattMcsClient;

#endif

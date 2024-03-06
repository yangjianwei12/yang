/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_TDS_CLIENT_PRIVATE_H_
#define GATT_TDS_CLIENT_PRIVATE_H_


#include <stdlib.h>

#include "gatt_tds_client.h"
#include "csr_bt_gatt_client_util_lib.h"

/* Size of the Client Characteristic Configuration in number of octects */
#define GATT_TDS_CLIENT_CHARACTERISTIC_CONFIG_SIZE          (2)

#define GATT_ATTR_HANDLE_INVALID                       ((uint16) 0x0000)


#define TdsMessageSend(TASK, ID, MSG) {\
    MSG->id = ID; \
    CsrSchedMessagePut(TASK, TDS_CLIENT_PRIM, MSG);\
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

    GattTdsClientDeviceData handles;

    /* GattId, cid and service handle is a part of this structure */
    ServiceHandleListElm_t *srvcElem;
} GTDSC;

typedef int32                                            TdsClientInternalMsg;

#define TDS_CLIENT_INTERNAL_MSG_INDICATION_REQ              ((TdsClientInternalMsg) 0x01)
#define TDS_CLIENT_INTERNAL_MSG_READ_REQ                    ((TdsClientInternalMsg) 0x02)
#define TDS_CLIENT_INTERNAL_MSG_SET_TDS_POINT_REQ           ((TdsClientInternalMsg) 0x03)

typedef uint8                                             TdsClientPendingOp;
#define TDS_CLIENT_PENDING_OP_NONE                       ((TdsClientPendingOp) 0x00)
#define TDS_CLIENT_PENDING_OP_WRITE_CCCD                 ((TdsClientPendingOp) 0x01)

#define TDS_CLIENT_INTERNAL_MSG_BASE                     0x00
#define TDS_CLIENT_INTERNAL_MSG_TOP                      0x05

/* Internal Message Structure to Initiate registering notification */
typedef struct
{
    TdsClientInternalMsg id;
    ServiceHandle srvcHndl;
    uint32 indicValue;
} TdsClientInternalMsgIndicationReq;

/* Internal Message Structure to read a characteristic */
typedef struct
{
    TdsClientInternalMsg id;
    ServiceHandle srvcHndl;
    TdsCharAttribute charac;
} TdsClientInternalMsgRead;

typedef struct
{
    TdsClientInternalMsg id;
    ServiceHandle srvcHndl;
    GattTdsOpcode op;
    uint16 sizeValue;
    uint8*  value;
} TdsClientInternalMsgSetDiscoveryControlPoint;

typedef struct
{
    CsrCmnList_t serviceHandleList;
} GattTdsClient;

#endif

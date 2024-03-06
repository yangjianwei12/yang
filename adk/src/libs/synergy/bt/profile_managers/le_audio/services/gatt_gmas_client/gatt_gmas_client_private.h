/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #2 $
******************************************************************************/

#ifndef GATT_GMAS_CLIENT_PRIVATE_H_
#define GATT_GMAS_CLIENT_PRIVATE_H_


#include <stdlib.h>

#include "gatt_gmas_client.h"
#include "csr_bt_gatt_client_util_lib.h"


#define GATT_ATTR_HANDLE_INVALID                       ((uint16) 0x0000)


#define GattGmasClientMessageSend(TASK, ID, MSG) {\
    MSG->id = ID; \
    CsrSchedMessagePut(TASK, GMAS_CLIENT_PRIM, MSG);\
    }

/*
    @brief Gatt Gmas Client library private data.
*/
typedef struct
{
    AppTaskData              libTask;
    AppTask                  appTask;
    GattGmasClientDeviceData handles;
    ServiceHandleListElm_t   *srvcElem;     /* GattId, cid and service handle is a part of this structure */
} GGMASC;

typedef int32                                            GattGmasClientInternalMsg;

#define GATT_GMAS_CLIENT_INTERNAL_MSG_READ_ROLE                      ((GattGmasClientInternalMsg) 0x01)
#define GATT_GMAS_CLIENT_INTERNAL_MSG_READ_UNICAST_FEATURES          ((GattGmasClientInternalMsg) 0x02)
#define GATT_GMAS_CLIENT_INTERNAL_MSG_READ_BROADCAST_FEATURES        ((GattGmasClientInternalMsg) 0x03)

/* Internal Message Structure to read a characteristic */
typedef struct
{
    GattGmasClientInternalMsg  id;
    ServiceHandle              srvcHndl;
    uint16                     handle;
} GattGmasClientInternalMsgCommonRead;

typedef GattGmasClientInternalMsgCommonRead GattGmasClientInternalMsgReadRole;
typedef GattGmasClientInternalMsgCommonRead GattGmasClientInternalMsgReadUnicastFeatures;
typedef GattGmasClientInternalMsgCommonRead GattGmasClientInternalMsgReadBroadcastFeatures;

typedef struct
{
    CsrCmnList_t serviceHandleList;
} GattGmasClient;


#endif

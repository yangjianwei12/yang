/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef GATT_TMAS_CLIENT_PRIVATE_H_
#define GATT_TMAS_CLIENT_PRIVATE_H_


#include <stdlib.h>

#include "gatt_tmas_client.h"
#include "csr_bt_gatt_client_util_lib.h"

/* Size of the Client Characteristic Configuration in number of octects */
#define GATT_TMAS_CLIENT_ROLE_NAME_SIZE          (2)

#define GATT_ATTR_HANDLE_INVALID                       ((uint16) 0x0000)


#define GattTmasClientMessageSend(TASK, ID, MSG) {\
    MSG->id = ID; \
    CsrSchedMessagePut(TASK, TMAS_CLIENT_PRIM, MSG);\
    }

/*
    @brief Gatt Tmas Client library private data.
*/
typedef struct
{
    AppTaskData              libTask;
    AppTask                  appTask;
    GattTmasClientDeviceData handles;
    ServiceHandleListElm_t   *srvcElem;     /* GattId, cid and service handle is a part of this structure */
} GTMASC;

typedef int32                                            GattTmasClientInternalMsg;

#define GATT_TMAS_CLIENT_INTERNAL_MSG_READ               ((GattTmasClientInternalMsg) 0x01)

#define GATT_TMAS_CLIENT_INTERNAL_MSG_BASE               ((GattTmasClientInternalMsg) 0x00)
#define GATT_TMAS_CLIENT_INTERNAL_MSG_TOP                ((GattTmasClientInternalMsg) 0x02)

/* Internal Message Structure to read a characteristic */
typedef struct
{
    GattTmasClientInternalMsg  id;
    ServiceHandle              srvcHndl;
    uint16                     handle;
} GattTmasClientInternalMsgRead;

typedef struct
{
    CsrCmnList_t serviceHandleList;
} GattTmasClient;


#endif

/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_QSS_SERVER_PRIVATE_H_
#define GATT_QSS_SERVER_PRIVATE_H_

#include "gatt_qss_server.h" 
#include "csr_bt_gatt_prim.h"


/* Internal QSS service structure for the server role */
typedef struct __GQSSS__
{
    AppTaskData     libTask;
    AppTask         appTask;

    ServiceHandle   srvHndl;
    uint16          startHandle;
    uint16          endHandle;

    CsrBtGattId     gattId;
} GQSSS;

/* Macros for creating messages */
#define MAKE_QSS_MESSAGE(TYPE) TYPE *message = (TYPE*)CsrPmemZalloc(sizeof(TYPE))

#define QssMessageSend(TASK, IND, MSG) {\
    MSG->ind = IND; \
    CsrSchedMessagePut(TASK, QSS_SERVER_PRIM, MSG);\
    }

/* Macro fuction's for intialisation of the MESSAGE pointer memory using the MSG pointer*/
#define GattQssPopulateReadAccessInd(MESSAGE, MSG) {\
    MESSAGE->cid = MSG->btConnId;\
    MESSAGE->handle = MSG->attrHandle;\
    MESSAGE->flags = ATT_ACCESS_READ;\
    MESSAGE->offset = MSG->offset;\
}

#define GattQssPopulateWriteAccessInd(MESSAGE, MSG) {\
    MESSAGE->cid = MSG->btConnId;\
    MESSAGE->handle = MSG->attrHandle;\
    MESSAGE->flags = ATT_ACCESS_WRITE;\
    MESSAGE->size_value = MSG->writeUnit[0].valueLength;\
    MESSAGE->value = MSG->writeUnit[0].value;\
}

#endif
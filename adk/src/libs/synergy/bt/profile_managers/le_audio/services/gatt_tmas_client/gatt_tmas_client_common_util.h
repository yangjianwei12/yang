/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef GATT_TMAS_CLIENT_COMMON_UTIL_H_
#define GATT_TMAS_CLIENT_COMMON_UTIL_H_

#include "gatt_tmas_client.h"
#include "gatt_tmas_client_private.h"

typedef struct
{
    TmapClientConnectionId  cid;
    uint16                  startHandle;
    uint16                  endHandle;
} GattTmasClientRegistrationParams;

/* Gets TMAS Client status from GATT result code */
GattTmasClientStatus getTmasClientStatusFromGattStatus(status_t status);

bool gattTmasClientRegister(GattTmasClientRegistrationParams *regParam, GTMASC *gattTmasClient);

void gattTmasClientInitServiceHandleList(CsrCmnListElm_t *elem);

ServiceHandle gattTmasClientGetServiceHandle(GTMASC **gattTmasClient, CsrCmnList_t *list);

GattTmasClient *gattTmasClientGetMainInstance(void);

#define GATT_TMAS_CLIENT_ADD_SERVICE_HANDLE(_List) \
    (ServiceHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ServiceHandleListElm_t))

#define GATT_TMAS_CLIENT_REMOVE_SERVICE_HANDLE(_List,_Element) \
    (CsrCmnListElementRemove(&(_List), \
                             (CsrCmnListElm_t *)(_Element)))

#endif

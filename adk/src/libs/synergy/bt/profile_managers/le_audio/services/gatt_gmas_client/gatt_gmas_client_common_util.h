/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_GMAS_CLIENT_COMMON_UTIL_H_
#define GATT_GMAS_CLIENT_COMMON_UTIL_H_

#include "gatt_gmas_client.h"
#include "gatt_gmas_client_private.h"

typedef struct
{
    GmapClientConnectionId  cid;
    uint16                  startHandle;
    uint16                  endHandle;
} GattGmasClientRegistrationParams;

/* Gets GMAS Client status from GATT result code */
GattGmasClientStatus getGmasClientStatusFromGattStatus(status_t status);

bool gattGmasClientRegister(GattGmasClientRegistrationParams *regParam, GGMASC *gattGmasClient);

void gattGmasClientInitServiceHandleList(CsrCmnListElm_t *elem);

ServiceHandle gattGmasClientGetServiceHandle(GGMASC **gattGmasClient, CsrCmnList_t *list);

GattGmasClient *gattGmasClientGetMainInstance(void);

#define GATT_GMAS_CLIENT_ADD_SERVICE_HANDLE(_List) \
    (ServiceHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ServiceHandleListElm_t))

#define GATT_GMAS_CLIENT_REMOVE_SERVICE_HANDLE(_List,_Element) \
    (CsrCmnListElementRemove(&(_List), \
                             (CsrCmnListElm_t *)(_Element)))

#endif

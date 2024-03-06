/* Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_MCS_CLIENT_COMMON_UTIL_H_
#define GATT_MCS_CLIENT_COMMON_UTIL_H_

#include "gatt_mcs_client.h"
#include "gatt_mcs_client_private.h"

typedef struct
{
    McpConnectionId cid;
    uint16 startHandle;
    uint16 endHandle;
} GattClientRegistrationParams;

/* Gets info type from characteristic handle */
MediaPlayerAttribute getMcsCharacFromHandle(GMCSC *mcsClient, uint16 handle);
MediaPlayerAttribute getMcsCharacFromCccHandle(GMCSC *mcsClient, uint16 handle);

/* Gets MCS status from GATT result code */
GattMcsClientStatus getMcsClientStatusFromGattStatus(status_t status);

bool gattRegisterMcsClient(GattClientRegistrationParams *regParam, GMCSC *gattMcsClient);

void initMcsServiceHandleList(CsrCmnListElm_t *elem);

ServiceHandle getMcsServiceHandle(GMCSC **gattMcsClient, CsrCmnList_t *list);

GattMcsClient *mcsClientGetMainInstance(void);

#define MCS_ADD_SERVICE_HANDLE(_List) \
    (ServiceHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ServiceHandleListElm_t))

#define MCS_REMOVE_SERVICE_HANDLE(_List,_Element) \
    (CsrCmnListElementRemove(&(_List), \
                             (CsrCmnListElm_t *)(_Element)))
#endif

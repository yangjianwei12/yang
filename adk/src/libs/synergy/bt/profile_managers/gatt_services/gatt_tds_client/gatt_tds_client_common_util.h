/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_TDS_CLIENT_COMMON_UTIL_H_
#define GATT_TDS_CLIENT_COMMON_UTIL_H_

#include "gatt_tds_client.h"
#include "gatt_tds_client_private.h"
#include "service_handle.h"

typedef struct
{
    uint32 cid;
    uint16 startHandle;
    uint16 endHandle;
#ifndef SYNERGY_CHP_ENABLE
    AppTask         clientTask;
#endif
} GattClientRegistrationParams;

/* Gets TDS status from GATT result code */
GattTdsClientStatus getTdsClientStatusFromGattStatus(status_t status);

bool gattRegisterTdsClient(GattClientRegistrationParams *regParam, GTDSC *gattTdsClient);

void initTdsServiceHandleList(CsrCmnListElm_t *elem);

ServiceHandle getTdsServiceHandle(GTDSC **gattTdsClient, CsrCmnList_t *list);

GattTdsClient *tdsClientGetMainInstance(void);

#define TDS_ADD_SERVICE_HANDLE(_List) \
    (ServiceHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ServiceHandleListElm_t))

#define TDS_REMOVE_SERVICE_HANDLE(_List,_Element) \
    (CsrCmnListElementRemove(&(_List), \
                             (CsrCmnListElm_t *)(_Element)))
#endif

/* Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_ASCS_CLIENT_UTIL_H_
#define GATT_ASCS_CLIENT_UTIL_H_

#include "gatt_ascs_client.h"
#include "gatt_ascs_client_private.h"


typedef struct
{
    ConnectionId cid;
    uint16 startHandle;
    uint16 endHandle;
} GattAscsClientRegistrationParams;

bool GattRegisterAscsClient(GattAscsClientRegistrationParams *regParam, GAscsC *gattAscsClient);

void InitAscsAseCharcList(CsrCmnListElm_t *elem);

void InitAscsServiceHandleList(CsrCmnListElm_t *elem);

CsrBool ascsInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data);

ServiceHandle getAscsServiceHandle(GAscsC **gattAscsClient, CsrCmnList_t *list);

#define ASCS_ADD_SERVICE_HANDLE(_List) \
    (ServiceHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ServiceHandleListElm_t))

#define ASCS_REMOVE_SERVICE_HANDLE(_List,_ServiceHandle) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        ascsInstFindBySrvcHndl,(void *)(&(_ServiceHandle)))

#define FREE_ASCS_CLIENT_INST(_Handle) \
                           ServiceHandleFreeInstanceData(_Handle)

#define FIND_ASCS_INST_BY_SERVICE_HANDLE(_Handle) \
                              (GAscsC *)ServiceHandleGetInstanceData(_Handle)


#endif   /* GATT_ASCS_CLIENT_UTIL_H_ */

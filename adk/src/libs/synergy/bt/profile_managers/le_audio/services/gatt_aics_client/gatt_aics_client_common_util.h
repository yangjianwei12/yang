/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_AICS_CLIENT_COMMON_UTIL_H_
#define GATT_AICS_CLIENT_COMMON_UTIL_H_

#include "gatt_aics_client.h"
#include "gatt_aics_client_private.h"

typedef struct
{
    connection_id_t cid;
    uint16 start_handle;
    uint16 end_handle;
} gatt_aics_client_registration_params_t;

bool GattRegisterAicsClient(gatt_aics_client_registration_params_t *reg_param, GAICS *gatt_aics_client);

void InitAicsServiceHandleList(CsrCmnListElm_t *elem);

CsrBool aicsInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data);

ServiceHandle getAicsServiceHandle(GAICS **gatt_aics_client, CsrCmnList_t *list);

#define AICS_ADD_SERVICE_HANDLE(_List) \
    (ServiceHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ServiceHandleListElm_t))

#define AICS_REMOVE_SERVICE_HANDLE(_List,_ServiceHandle) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        aicsInstFindBySrvcHndl,(void *)(&(_ServiceHandle)))

#define FREE_AICS_CLIENT_INST(_Handle) \
                           ServiceHandleFreeInstanceData(_Handle)

#define FIND_AICS_INST_BY_SERVICE_HANDLE(_Handle) \
                              (GAICS *)ServiceHandleGetInstanceData(_Handle)


#endif   /* GATT_AICS_CLIENT_COMMON_UTIL_H_ */

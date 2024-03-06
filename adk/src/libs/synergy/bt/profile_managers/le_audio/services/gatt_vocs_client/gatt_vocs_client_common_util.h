/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_VOCS_CLIENT_COMMON_UTIL_H_
#define GATT_VOCS_CLIENT_COMMON_UTIL_H_

#include "gatt_vocs_client.h"
#include "gatt_vocs_client_private.h"

typedef struct
{
    connection_id_t cid;
    uint16 start_handle;
    uint16 end_handle;
} gatt_vocs_client_registration_params_t;

bool GattRegisterVocsClient(gatt_vocs_client_registration_params_t *reg_param, GVOCS *gatt_vocs_client);

void InitVocsServiceHandleList(CsrCmnListElm_t *elem);

CsrBool vocsInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data);

ServiceHandle getVocsServiceHandle(GVOCS **gatt_vocs_client, CsrCmnList_t *list);

#define VOCS_ADD_SERVICE_HANDLE(_List) \
    (ServiceHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ServiceHandleListElm_t))

#define VOCS_REMOVE_SERVICE_HANDLE(_List,_ServiceHandle) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        vcsInstFindBySrvcHndl,(void *)(&(_ServiceHandle)))

#define FREE_VOCS_CLIENT_INST(_Handle) \
                           ServiceHandleFreeInstanceData(_Handle)

#define FIND_VOCS_INST_BY_SERVICE_HANDLE(_Handle) \
                              (GVOCS *)ServiceHandleGetInstanceData(_Handle)


#endif   /* GATT_VOCS_CLIENT_COMMON_UTIL_H_ */

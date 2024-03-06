/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_VCS_CLIENT_COMMON_UTIL_H_
#define GATT_VCS_CLIENT_COMMON_UTIL_H_

#include "gatt_vcs_client.h"
#include "gatt_vcs_client_private.h"

typedef struct
{
    connection_id_t cid;
    uint16 start_handle;
    uint16 end_handle;
} gatt_client_registration_params_t;

bool GattRegisterClient(gatt_client_registration_params_t *reg_param, GVCSC *gatt_vcs_client);

void InitServiceHandleList(CsrCmnListElm_t *elem);

CsrBool vcsInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data);

ServiceHandle getServiceHandle(GVCSC **gatt_vcs_client, CsrCmnList_t *list);

#define VCS_ADD_SERVICE_HANDLE(_List) \
    (ServiceHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ServiceHandleListElm_t))

#define VCS_REMOVE_SERVICE_HANDLE(_List,_ServiceHandle) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        vcsInstFindBySrvcHndl,(void *)(&(_ServiceHandle)))

#define FREE_VCS_CLIENT_INST(_Handle) \
                           ServiceHandleFreeInstanceData(_Handle)

#define FIND_VCS_INST_BY_SERVICE_HANDLE(_Handle) \
                              (GVCSC *)ServiceHandleGetInstanceData(_Handle)


#endif

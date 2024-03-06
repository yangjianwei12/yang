/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#ifndef GATT_MICS_CLIENT_COMMON_UTIL_H_
#define GATT_MICS_CLIENT_COMMON_UTIL_H_

#include "gatt_mics_client.h"
#include "gatt_mics_client_private.h"

typedef struct
{
    connection_id_t cid;
    uint16 start_handle;
    uint16 end_handle;
} gatt_client_registration_params_t;

bool GattMicsRegisterClient(gatt_client_registration_params_t *reg_param, GMICSC *gatt_mics_client);

CsrBool micsInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data);

ServiceHandle getMicsServiceHandle(GMICSC **gatt_mics_client, CsrCmnList_t *list);

#define MICS_ADD_SERVICE_HANDLE(_List) \
    (ServiceHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ServiceHandleListElm_t))

#define MICS_REMOVE_SERVICE_HANDLE(_List,_ServiceHandle) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        micsInstFindBySrvcHndl,(void *)(&(_ServiceHandle)))

#define FREE_MICS_CLIENT_INST(_Handle) \
                           ServiceHandleFreeInstanceData(_Handle)

#define FIND_MICS_INST_BY_SERVICE_HANDLE(_Handle) \
                              (GMICSC *)ServiceHandleGetInstanceData(_Handle)


#endif

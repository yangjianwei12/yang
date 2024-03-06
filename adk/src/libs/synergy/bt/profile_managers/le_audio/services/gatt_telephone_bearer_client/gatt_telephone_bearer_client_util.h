/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#ifndef GATT_TELEPHONE_BEARER_CLIENT_UTIL_H_
#define GATT_TELEPHONE_BEARER_CLIENT_UTIL_H_

#include "gatt_telephone_bearer_client.h"
#include "gatt_telephone_bearer_client_private.h"


typedef struct
{
    connection_id_t cid;
    uint16 start_handle;
    uint16 end_handle;
} gatt_tbs_client_registration_params_t;

bool GattRegisterTbsClient(gatt_tbs_client_registration_params_t *reg_param, GTBSC *gatt_tbs_client);

void InitTbsClientServiceHandleList(CsrCmnListElm_t *elem);

bool tbsClientInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data);

ServiceHandle getTbsClientServiceHandle(GTBSC **gatt_tbs_client, CsrCmnList_t *list);

#define TBS_ADD_SERVICE_HANDLE(_List) \
    (ServiceHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ServiceHandleListElm_t))

#define TBS_REMOVE_SERVICE_HANDLE(_List,_Element) \
                             (CsrCmnListElementRemove(&(_List), \
                             (CsrCmnListElm_t *)(_Element)))

#define FREE_TBS_CLIENT_INST(_Handle) \
                           ServiceHandleFreeInstanceData(_Handle)

#define FIND_TBS_INST_BY_SERVICE_HANDLE(_Handle) \
                              (GTBSC *)ServiceHandleGetInstanceData(_Handle)

#endif   /* GATT_TELEPHONE_BEARER_CLIENT_UTIL_H_ */

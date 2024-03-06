/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/


#include "gatt_telephone_bearer_client_private.h"
#include "gatt_telephone_bearer_client_util.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "csr_bt_gatt_lib.h"

extern gatt_tbs_client *tbs_client_main;

bool GattRegisterTbsClient(gatt_tbs_client_registration_params_t *reg_param, GTBSC *gatt_tbs_client)
{
    CsrBtTypedAddr addr;

    gatt_tbs_client->srvcElem->gattId = CsrBtGattRegister(CSR_BT_TBS_CLIENT_IFACEQUEUE);

    if (gatt_tbs_client->srvcElem->gattId)
    {
        if (CsrBtGattClientUtilFindAddrByConnId(reg_param->cid,
                                                &addr))
        {
            CsrBtGattClientRegisterServiceReqSend(gatt_tbs_client->srvcElem->gattId,
                                                  reg_param->start_handle,
                                                  reg_param->end_handle,
                                                  addr);
            return TRUE;
        }
        else
            return FALSE;
    }
    else
        return FALSE;
}

ServiceHandle getTbsClientServiceHandle(GTBSC **gatt_tbs_client, CsrCmnList_t *list)
{
    ServiceHandle newServiceHandle = ServiceHandleNewInstance((void **)gatt_tbs_client, sizeof(GTBSC));

    if(*gatt_tbs_client)
    {
        ServiceHandleListElm_t *elem = TBS_ADD_SERVICE_HANDLE(*list);
        elem->service_handle = newServiceHandle;

        /* Set memory contents to all zeros */
        CsrMemSet(*gatt_tbs_client, 0, sizeof(GTBSC));

        (*gatt_tbs_client)->srvcElem = elem;

        return elem->service_handle;
    }

    return 0;
}

bool tbsClientInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ServiceHandleListElm_t *clnt_hndl_elm = (ServiceHandleListElm_t *)elem;
    ServiceHandle client_handle = *(ServiceHandle *)data;

    return (clnt_hndl_elm->service_handle == client_handle);
}


void InitTbsClientServiceHandleList(CsrCmnListElm_t *elem)
{
    /* Initialize a ASCS Service Handle list element. This function is called every
     * time a new entry is made on the service handle list */
    ServiceHandleListElm_t *cElem = (ServiceHandleListElm_t *) elem;

    cElem->service_handle = 0;
}

void gatt_tbs_client_init(void **gash)
{
    tbs_client_main = CsrPmemZalloc(sizeof(*tbs_client_main));
    *gash = tbs_client_main;

    CsrCmnListInit(&tbs_client_main->service_handle_list, 0, InitTbsClientServiceHandleList, NULL);
}

/****************************************************************************/
#ifdef ENABLE_SHUTDOWN
void GattTbsClientDeInit(void **gash)
{
    CsrCmnListDeinit(&tbs_client_main->service_handle_list);
    CsrPmemFree(tbs_client_main);
}
#endif

GattTelephoneBearerClientDeviceData *GattTelephoneBearerClientGetHandlesReq(ServiceHandle clntHndl)
{
    GTBSC *tbsClient = ServiceHandleGetInstanceData(clntHndl);

    if (tbsClient)
    {
        GattTelephoneBearerClientDeviceData *tbsHandles = CsrPmemZalloc(sizeof(GattTelephoneBearerClientDeviceData));

        memcpy(tbsHandles, &(tbsClient->handles), sizeof(GattTelephoneBearerClientDeviceData));

        return tbsHandles;
    }

    return NULL;
}



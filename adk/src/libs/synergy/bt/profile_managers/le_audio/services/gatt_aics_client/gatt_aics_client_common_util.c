/* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_aics_client_private.h"
#include "gatt_aics_client_common_util.h"
#include "csr_bt_gatt_lib.h"

gatt_aics_client *aics_client_main;

bool GattRegisterAicsClient(gatt_aics_client_registration_params_t *reg_param, GAICS *gatt_aics_client)
{
    CsrBtTypedAddr addr;

#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
    gatt_aics_client->srvcElem->gattId = CsrBtGattRegister(CSR_BT_AICS_CLIENT_IFACEQUEUE);
#endif
    if (gatt_aics_client->srvcElem->gattId)
    {
        if (CsrBtGattClientUtilFindAddrByConnId(reg_param->cid,
                                                &addr))
        {
            CsrBtGattClientRegisterServiceReqSend(gatt_aics_client->srvcElem->gattId,
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

ServiceHandle getAicsServiceHandle(GAICS **gatt_aics_client, CsrCmnList_t *list)
{
    ServiceHandleListElm_t *elem = AICS_ADD_SERVICE_HANDLE(*list);

    elem->service_handle = ServiceHandleNewInstance((void **) gatt_aics_client, sizeof(GAICS));

    if((*gatt_aics_client))
        (*gatt_aics_client)->srvcElem = elem;

    return elem->service_handle;
}

CsrBool aicsInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ServiceHandleListElm_t *clnt_hndl_elm = (ServiceHandleListElm_t *)elem;
    ServiceHandle client_handle = *(ServiceHandle *)data;

    return (clnt_hndl_elm->service_handle == client_handle);
}

void InitAicsServiceHandleList(CsrCmnListElm_t *elem)
{
    /* Initialize a AICS Service Handle list element. This function is called every
     * time a new entry is made on the service handle list */
    ServiceHandleListElm_t *cElem = (ServiceHandleListElm_t *) elem;

    cElem->service_handle = 0;
}

void gatt_aics_client_init(void **gash)
{
    aics_client_main = CsrPmemZalloc(sizeof(*aics_client_main));
    *gash = aics_client_main;

    CsrCmnListInit(&aics_client_main->service_handle_list, 0, InitAicsServiceHandleList, NULL);
}

/****************************************************************************/
#ifdef ENABLE_SHUTDOWN
void GattAicsClientDeInit(void **gash)
{
    CsrCmnListDeinit(&aics_client_main->service_handle_list);
    CsrPmemFree(aics_client_main);
}
#endif

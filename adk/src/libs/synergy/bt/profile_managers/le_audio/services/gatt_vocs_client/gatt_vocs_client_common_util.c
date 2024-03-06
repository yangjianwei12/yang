/* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_vocs_client_private.h"
#include "gatt_vocs_client_common_util.h"
#include "csr_bt_gatt_lib.h"

gatt_vocs_client *vocs_client_main;

bool GattRegisterVocsClient(gatt_vocs_client_registration_params_t *reg_param, GVOCS *gatt_vocs_client)
{
    CsrBtTypedAddr addr;

#ifndef EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
    gatt_vocs_client->srvcElem->gattId = CsrBtGattRegister(CSR_BT_VOCS_CLIENT_IFACEQUEUE);
#endif

    if (gatt_vocs_client->srvcElem->gattId)
    {
        if (CsrBtGattClientUtilFindAddrByConnId(reg_param->cid,
                                                &addr))
        {
            CsrBtGattClientRegisterServiceReqSend(gatt_vocs_client->srvcElem->gattId,
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

ServiceHandle getVocsServiceHandle(GVOCS **gatt_vocs_client, CsrCmnList_t *list)
{
    ServiceHandleListElm_t *elem = VOCS_ADD_SERVICE_HANDLE(*list);

    elem->service_handle = ServiceHandleNewInstance((void **) gatt_vocs_client, sizeof(GVOCS));

    if((*gatt_vocs_client))
        (*gatt_vocs_client)->srvcElem = elem;

    return elem->service_handle;
}

CsrBool vocsInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ServiceHandleListElm_t *clnt_hndl_elm = (ServiceHandleListElm_t *)elem;
    ServiceHandle client_handle = *(ServiceHandle *)data;

    return (clnt_hndl_elm->service_handle == client_handle);
}

void InitVocsServiceHandleList(CsrCmnListElm_t *elem)
{
    /* Initialize a VOCS Service Handle list element. This function is called every
     * time a new entry is made on the service handle list */
    ServiceHandleListElm_t *cElem = (ServiceHandleListElm_t *) elem;

    cElem->service_handle = 0;
}

void gatt_vocs_client_init(void **gash)
{
    vocs_client_main = CsrPmemZalloc(sizeof(*vocs_client_main));
    *gash = vocs_client_main;

    CsrCmnListInit(&vocs_client_main->service_handle_list, 0, InitVocsServiceHandleList, NULL);
}

/****************************************************************************/
#ifdef ENABLE_SHUTDOWN
void GattVocsClientDeInit(void **gash)
{
    CsrCmnListDeinit(&vocs_client_main->service_handle_list);
    CsrPmemFree(vocs_client_main);
}
#endif

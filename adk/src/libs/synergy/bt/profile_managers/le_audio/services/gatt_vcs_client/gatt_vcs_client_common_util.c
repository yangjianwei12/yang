/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_vcs_client_private.h"
#include "gatt_vcs_client_common_util.h"
#include "csr_bt_gatt_lib.h"
#include "gatt_vcs_client_debug.h"

gatt_vcs_client *vcs_client_main;

bool GattRegisterClient(gatt_client_registration_params_t *reg_param, GVCSC *gatt_vcs_client)
{
    CsrBtTypedAddr addr;

    gatt_vcs_client->srvcElem->gattId = CsrBtGattRegister(CSR_BT_VCS_CLIENT_IFACEQUEUE);

    if (gatt_vcs_client->srvcElem->gattId)
    {
        if (CsrBtGattClientUtilFindAddrByConnId(reg_param->cid,
                                                &addr))
        {
            CsrBtGattClientRegisterServiceReqSend(gatt_vcs_client->srvcElem->gattId,
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

ServiceHandle getServiceHandle(GVCSC **gatt_vcs_client, CsrCmnList_t *list)
{
    ServiceHandleListElm_t *elem = VCS_ADD_SERVICE_HANDLE(*list);

    elem->service_handle = ServiceHandleNewInstance((void **) gatt_vcs_client, sizeof(GVCSC));

    if(*gatt_vcs_client)
        (*gatt_vcs_client)->srvcElem = elem;

    return elem->service_handle;
}

CsrBool vcsInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ServiceHandleListElm_t *clnt_hndl_elm = (ServiceHandleListElm_t *)elem;
    ServiceHandle client_handle = *(ServiceHandle *)data;

    return (clnt_hndl_elm->service_handle == client_handle);
}

void InitServiceHandleList(CsrCmnListElm_t *elem)
{
    /* Initialize a CsrBtAseCharacElement. This function is called every
     * time a new entry is made on the queue list */
    ServiceHandleListElm_t *cElem = (ServiceHandleListElm_t *) elem;

    cElem->service_handle = 0;
}

void gatt_vcs_client_init(void **gash)
{
    vcs_client_main = CsrPmemZalloc(sizeof(*vcs_client_main));
    *gash = vcs_client_main;

    CsrCmnListInit(&vcs_client_main->service_handle_list, 0, InitServiceHandleList, NULL);
}

/****************************************************************************/
#ifdef ENABLE_SHUTDOWN
void GattVcsClientDeInit(void **gash)
{
    CsrCmnListDeinit(&vcs_client_main->service_handle_list);
    CsrPmemFree(vcs_client_main);
}
#endif

GattVcsClientDeviceData *GattVcsClientGetHandlesReq(ServiceHandle clntHndl)
{
    GVCSC *vcsClient = ServiceHandleGetInstanceData(clntHndl);

    if (vcsClient)
    {
        GattVcsClientDeviceData *vcsHandles = CsrPmemZalloc(sizeof(GattVcsClientDeviceData));
        memcpy(vcsHandles, &(vcsClient->handles), sizeof(GattVcsClientDeviceData));
        return vcsHandles;
    }
    GATT_VCS_CLIENT_ERROR("GattVcsClientGetHandlesReq : vcsClient NULL Instance");
    return NULL;
}

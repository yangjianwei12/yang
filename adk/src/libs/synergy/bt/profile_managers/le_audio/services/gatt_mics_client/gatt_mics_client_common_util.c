/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#include "gatt_mics_client_private.h"
#include "gatt_mics_client_common_util.h"
#include "csr_bt_gatt_lib.h"
#include "gatt_mics_client_debug.h"

gatt_mics_client *mics_client_main;

bool GattMicsRegisterClient(gatt_client_registration_params_t *reg_param, GMICSC *gatt_mics_client)
{
    CsrBtTypedAddr addr;

    gatt_mics_client->srvcElem->gattId = CsrBtGattRegister(CSR_BT_MICS_CLIENT_IFACEQUEUE);

    if (gatt_mics_client->srvcElem->gattId)
    {
        if (CsrBtGattClientUtilFindAddrByConnId(reg_param->cid,
                                                &addr))
        {
            CsrBtGattClientRegisterServiceReqSend(gatt_mics_client->srvcElem->gattId,
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

ServiceHandle getMicsServiceHandle(GMICSC **gatt_mics_client, CsrCmnList_t *list)
{
    ServiceHandleListElm_t *elem = MICS_ADD_SERVICE_HANDLE(*list);

    elem->service_handle = ServiceHandleNewInstance((void **) gatt_mics_client, sizeof(GMICSC));

    if(*gatt_mics_client)
        (*gatt_mics_client)->srvcElem = elem;

    return elem->service_handle;
}

CsrBool micsInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ServiceHandleListElm_t *clnt_hndl_elm = (ServiceHandleListElm_t *)elem;
    ServiceHandle client_handle = *(ServiceHandle *)data;

    return (clnt_hndl_elm->service_handle == client_handle);
}

static void InitServiceHandleList(CsrCmnListElm_t *elem)
{
    /* Initialize a CsrBtAseCharacElement. This function is called every
     * time a new entry is made on the queue list */
    ServiceHandleListElm_t *cElem = (ServiceHandleListElm_t *) elem;

    cElem->service_handle = 0;
}

void GattMicsClientInit(void **gash)
{
    mics_client_main = CsrPmemZalloc(sizeof(*mics_client_main));
    *gash = mics_client_main;

    CsrCmnListInit(&mics_client_main->service_handle_list, 0, InitServiceHandleList, NULL);
}

/****************************************************************************/
#ifdef ENABLE_SHUTDOWN
void GattMicsClientDeInit(void **gash)
{
    CsrCmnListDeinit(&mics_client_main->service_handle_list);
    CsrPmemFree(mics_client_main);
}
#endif

GattMicsClientDeviceData *GattMicsClientGetHandles(ServiceHandle clntHndl)
{
    GMICSC *micsClient = ServiceHandleGetInstanceData(clntHndl);

    if (micsClient)
    {
        GattMicsClientDeviceData *micsHandles = CsrPmemZalloc(sizeof(GattMicsClientDeviceData));
        CsrMemCpy(micsHandles, &(micsClient->handles), sizeof(GattMicsClientDeviceData));
        return micsHandles;
    }
    GATT_MICS_CLIENT_ERROR("GattMicsClientGetHandles: micsClient NULL Instance");
    return NULL;
}

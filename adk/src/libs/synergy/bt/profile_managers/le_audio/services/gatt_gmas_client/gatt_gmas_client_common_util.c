/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/


#include "gatt_gmas_client_private.h"
#include "gatt_gmas_client_common_util.h"

GattGmasClient *gmasClientMain;

bool gattGmasClientRegister(GattGmasClientRegistrationParams *regParam, GGMASC *gattGmasClient)
{
    CsrBtTypedAddr addr;

    gattGmasClient->srvcElem->gattId = CsrBtGattRegister(CSR_BT_GMAS_CLIENT_IFACEQUEUE);

    if (gattGmasClient->srvcElem->gattId)
    {
        if (CsrBtGattClientUtilFindAddrByConnId(regParam->cid,
                                                &addr))
        {
            CsrBtGattClientRegisterServiceReqSend(gattGmasClient->srvcElem->gattId,
                                                  regParam->startHandle,
                                                  regParam->endHandle,
                                                  addr);
            return TRUE;
        }
        else
            return FALSE;
    }
    else
        return FALSE;
}

ServiceHandle gattGmasClientGetServiceHandle(GGMASC **gattGmasClient, CsrCmnList_t *list)
{
    ServiceHandleListElm_t *elem = GATT_GMAS_CLIENT_ADD_SERVICE_HANDLE(*list);

    elem->service_handle = ServiceHandleNewInstance((void **) gattGmasClient, sizeof(GGMASC));

    if(*gattGmasClient)
        (*gattGmasClient)->srvcElem = elem;

    return elem->service_handle;
}

void gattGmasClientInitServiceHandleList(CsrCmnListElm_t *elem)
{
    ServiceHandleListElm_t *cElem = (ServiceHandleListElm_t *) elem;

    cElem->service_handle = 0;
}

void gattGmasClientInit(void **gash)
{
    gmasClientMain = CsrPmemZalloc(sizeof(*gmasClientMain));
    *gash = gmasClientMain;

    if (gmasClientMain)
        CsrCmnListInit(&gmasClientMain->serviceHandleList, 0, gattGmasClientInitServiceHandleList, NULL);
}

GattGmasClient *gattGmasClientGetMainInstance(void)
{
    return gmasClientMain;
}

/****************************************************************************/
#ifdef ENABLE_SHUTDOWN
void gattGmasClientDeInit(void **gash)
{
    CsrCmnListDeinit(&gmasClientMain->serviceHandleList);
    CsrPmemFree(gmasClientMain);
}
#endif

GattGmasClientStatus getGmasClientStatusFromGattStatus(status_t status)
{
    GattGmasClientStatus gmasClientStatus;

    switch(status)
    {
        case CSR_BT_GATT_RESULT_SUCCESS:
            gmasClientStatus = GATT_GMAS_CLIENT_STATUS_SUCCESS;
            break;
        default:
            gmasClientStatus = GATT_GMAS_CLIENT_STATUS_FAILED;
            break;
    }
    return gmasClientStatus;
}

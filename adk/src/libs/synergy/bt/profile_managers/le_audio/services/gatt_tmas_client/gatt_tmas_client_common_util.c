/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #58 $
******************************************************************************/


#include "gatt_tmas_client_private.h"
#include "gatt_tmas_client_common_util.h"

GattTmasClient *tmasClientMain;

bool gattTmasClientRegister(GattTmasClientRegistrationParams *regParam, GTMASC *gattTmasClient)
{
    CsrBtTypedAddr addr;

    gattTmasClient->srvcElem->gattId = CsrBtGattRegister(CSR_BT_TMAS_CLIENT_IFACEQUEUE);

    if (gattTmasClient->srvcElem->gattId)
    {
        if (CsrBtGattClientUtilFindAddrByConnId(regParam->cid,
                                                &addr))
        {
            CsrBtGattClientRegisterServiceReqSend(gattTmasClient->srvcElem->gattId,
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

ServiceHandle gattTmasClientGetServiceHandle(GTMASC **gattTmasClient, CsrCmnList_t *list)
{
    ServiceHandleListElm_t *elem = GATT_TMAS_CLIENT_ADD_SERVICE_HANDLE(*list);

    elem->service_handle = ServiceHandleNewInstance((void **) gattTmasClient, sizeof(GTMASC));

    if(*gattTmasClient)
        (*gattTmasClient)->srvcElem = elem;

    return elem->service_handle;
}

void gattTmasClientInitServiceHandleList(CsrCmnListElm_t *elem)
{
    ServiceHandleListElm_t *cElem = (ServiceHandleListElm_t *) elem;

    cElem->service_handle = 0;
}

void gattTmasClientInit(void **gash)
{
    tmasClientMain = CsrPmemZalloc(sizeof(*tmasClientMain));
    *gash = tmasClientMain;

    if (tmasClientMain)
        CsrCmnListInit(&tmasClientMain->serviceHandleList, 0, gattTmasClientInitServiceHandleList, NULL);
}

GattTmasClient *gattTmasClientGetMainInstance(void)
{
    return tmasClientMain;
}

/****************************************************************************/
#ifdef ENABLE_SHUTDOWN
void gattTmasClientDeInit(void **gash)
{
    CsrCmnListDeinit(&tmasClientMain->serviceHandleList);
    CsrPmemFree(tmasClientMain);
}
#endif

GattTmasClientStatus getTmasClientStatusFromGattStatus(status_t status)
{
    GattTmasClientStatus tmasClientStatus;

    switch(status)
    {
        case CSR_BT_GATT_RESULT_SUCCESS:
            tmasClientStatus = GATT_TMAS_CLIENT_STATUS_SUCCESS;
            break;
        default:
            tmasClientStatus = GATT_TMAS_CLIENT_STATUS_FAILED;
            break;
    }
    return tmasClientStatus;
}

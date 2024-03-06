/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_tds_client_private.h"
#include "gatt_tds_client_common_util.h"

GattTdsClient *tdsClientMain;

bool gattRegisterTdsClient(GattClientRegistrationParams *regParam, GTDSC *gattTdsClient)
{
    CsrBtTypedAddr addr;

    gattTdsClient->srvcElem->gattId = CsrBtGattRegister(CSR_BT_TDS_CLIENT_IFACEQUEUE);

    if (gattTdsClient->srvcElem->gattId)
    {
        if (CsrBtGattClientUtilFindAddrByConnId(regParam->cid,
                                                &addr))
        {
            CsrBtGattClientRegisterServiceReqSend(gattTdsClient->srvcElem->gattId,
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

ServiceHandle getTdsServiceHandle(GTDSC **gattTdsClient, CsrCmnList_t *list)
{
    ServiceHandleListElm_t *elem = TDS_ADD_SERVICE_HANDLE(*list);

    elem->service_handle = ServiceHandleNewInstance((void **) gattTdsClient, sizeof(GTDSC));

    if(*gattTdsClient)
        (*gattTdsClient)->srvcElem = elem;

    return elem->service_handle;
}

void initTdsServiceHandleList(CsrCmnListElm_t *elem)
{
    ServiceHandleListElm_t *cElem = (ServiceHandleListElm_t *) elem;

    cElem->service_handle = 0;
}

void gattTdsClientInit(void **gash)
{
    tdsClientMain = CsrPmemAlloc(sizeof(*tdsClientMain));
    *gash = tdsClientMain;

    if (tdsClientMain)
        CsrCmnListInit(&tdsClientMain->serviceHandleList, 0, initTdsServiceHandleList, NULL);
}

GattTdsClient *tdsClientGetMainInstance(void)
{
    return tdsClientMain;
}

/****************************************************************************/
#ifdef ENABLE_SHUTDOWN
void gattTdsClientDeInit(void **gash)
{
    CsrCmnListDeinit(&tdsClientMain->serviceHandleList);
    CsrPmemFree(tdsClientMain);
}
#endif

GattTdsClientStatus getTdsClientStatusFromGattStatus(status_t status)
{
    GattTdsClientStatus tdsStatus;

    switch(status)
    {
        case CSR_BT_GATT_RESULT_SUCCESS:
            tdsStatus = GATT_TDS_CLIENT_STATUS_SUCCESS;
        break;

        case CSR_BT_GATT_RESULT_TRUNCATED_DATA:
            tdsStatus = GATT_TDS_CLIENT_STATUS_TRUNCATED_DATA;
        break;

        case CSR_BT_GATT_ACCESS_RES_INSUFFICIENT_AUTHENTICATION:
            tdsStatus = GATT_TDS_CLIENT_STATUS_ACCESS_INSUFFICIENT_AUTHENTICATION;
        break;

        case ATT_RESULT_TIMEOUT:
            tdsStatus = GATT_TDS_CLIENT_STATUS_ATT_TIMEOUT;
        break;

        default:
            tdsStatus = GATT_TDS_CLIENT_STATUS_FAILED;
        break;
    }
    return tdsStatus;
}


GattTdsClientDeviceData *GattTdsClientGetHandlesReq(ServiceHandle clntHndl)
{
    GTDSC *tdsClient = ServiceHandleGetInstanceData(clntHndl);

    if (tdsClient)
    {
        GattTdsClientDeviceData *tdsHandles = CsrPmemZalloc(sizeof(GattTdsClientDeviceData));

        memcpy(tdsHandles, &(tdsClient->handles), sizeof(GattTdsClientDeviceData));

        return tdsHandles;
    }

    return NULL;
}

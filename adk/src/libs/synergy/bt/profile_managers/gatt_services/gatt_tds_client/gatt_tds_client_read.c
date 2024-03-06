/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_tds_client.h"
#include "gatt_tds_client_debug.h"
#include "gatt_tds_client_private.h"
#include "gatt_tds_client_read.h"
#include "gatt_tds_client_common_util.h"

/***************************************************************************/
void tdsClientHandleInternalRead(const GTDSC * tdsClient, TdsCharAttribute charac)
{
    uint16 currHandle = GATT_ATTR_HANDLE_INVALID;

    switch (charac)
    {
        case BREDR_HANDOVER_DATA:
        {
            currHandle = tdsClient->handles.bredrHandoverDataHandle;
            break;
        }
        case COMPLETET_TRANSPORT_BLOCK:
        {
            currHandle = tdsClient->handles.CompleteTransportBlockHandle;
            break;
        }
        default :
        {
            break;
        }
    }

    if (currHandle != GATT_ATTR_HANDLE_INVALID)
    {
        CsrBtGattReadReqSend(tdsClient->srvcElem->gattId,
                             tdsClient->srvcElem->cid,
                             currHandle,
                             0);
    }
    else
    {
        GattTdsClientGetTdsAttributeCfm *message = CsrPmemAlloc(sizeof(*message));

        message->srvcHndl = tdsClient->srvcElem->service_handle;
        message->status = CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER;
        message->charac = charac;
        message->sizeValue = 0;
        message->value = NULL;

        TdsMessageSend(tdsClient->appTask, GATT_TDS_CLIENT_GET_TDS_ATTRIBUTE_CFM, message);
    }
}


/****************************************************************************/
void GattTdsClientGetTdsAttribute(ServiceHandle clntHndl, TdsCharAttribute charac)
{
    GTDSC *gattTdsClient = ServiceHandleGetInstanceData(clntHndl);

    if (gattTdsClient)
    {
        TdsClientInternalMsgRead *message = CsrPmemAlloc(sizeof(*message));

        message->srvcHndl = gattTdsClient->srvcElem->service_handle;
        message->charac = charac;

        TdsMessageSend(gattTdsClient->libTask,
                       TDS_CLIENT_INTERNAL_MSG_READ_REQ,
                       message);
    }
    else
    {
        GATT_TDS_CLIENT_ERROR("Invalid TDS Client instance!\n");
    }
}

/****************************************************************************/
void handleTdsReadValueResp(GTDSC *tdsClient, uint16 handle, status_t resultCode, uint16 valueLength, uint8 *value)
{
    uint8 *readVal;
    GattTdsClientGetTdsAttributeCfm *message = CsrPmemAlloc(sizeof(*message));

    if( handle == tdsClient->handles.bredrHandoverDataHandle)
        message->charac = BREDR_HANDOVER_DATA;
    else
        message->charac = COMPLETET_TRANSPORT_BLOCK;
    
    message->srvcHndl = tdsClient->srvcElem->service_handle;
    message->status = getTdsClientStatusFromGattStatus(resultCode);
    message->sizeValue = valueLength;

    if(value)
    {
        readVal = (uint8 *) CsrPmemAlloc(valueLength);
        CsrMemCpy(readVal, value, valueLength);

        message->value = readVal;
    }
    else
    {
        message->value = NULL;
    }

    TdsMessageSend(tdsClient->appTask, GATT_TDS_CLIENT_GET_TDS_ATTRIBUTE_CFM, message);
}

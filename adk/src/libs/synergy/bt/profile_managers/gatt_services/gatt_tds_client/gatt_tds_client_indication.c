/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_tds_client.h"
#include "gatt_tds_client_debug.h"
#include "gatt_tds_client_private.h"
#include "gatt_tds_client_indication.h"
#include "gatt_tds_client_common_util.h"


/*******************************************************************************/
void tdsClientIndicationCfm(GTDSC *const tdsClient,
                       GattTdsClientStatus status,
                       GattTdsClientMessageId id)
{
    GattTdsClientIndicationCfm *message = CsrPmemAlloc(sizeof(*message));

    /* Fill in client reference */
    message->srvcHndl = tdsClient->srvcElem->service_handle;

    /* Send the confirmation message to app task  */
    TdsMessageSend(tdsClient->appTask, id, message);

    CSR_UNUSED(status);
}


/***************************************************************************/
void GattTdsClientRegisterForIndicationReq(ServiceHandle clntHndl, uint32 indicValue)
{
    GTDSC *gattTdsClient = ServiceHandleGetInstanceData(clntHndl);

    if (gattTdsClient)
    {
        TdsClientInternalMsgIndicationReq *message = CsrPmemAlloc(sizeof(*message));

        message->srvcHndl = gattTdsClient->srvcElem->service_handle;
        message->indicValue = indicValue;

        gattTdsClient->pendingCmd = TDS_CLIENT_PENDING_OP_WRITE_CCCD;

        TdsMessageSend(gattTdsClient->libTask,
                       TDS_CLIENT_INTERNAL_MSG_INDICATION_REQ,
                       message);
    }
    else
    {
        GATT_TDS_CLIENT_ERROR("Invalid TDS Client instance!\n");
    }
}


/****************************************************************************/
void tdsClientHandleInternalRegisterForIndication(GTDSC *gatt_tds_client, uint32 indic_value)
{
    if (gatt_tds_client)
    {
        uint16 currHandle = gatt_tds_client->handles.tdsControlPointCCCDHandle;

        if (currHandle != GATT_ATTR_HANDLE_INVALID)
        {
            uint8* value = (uint8*)(CsrPmemAlloc(GATT_TDS_CLIENT_CHARACTERISTIC_CONFIG_SIZE));

            value[0] = indic_value ? TDS_INDICATION_VALUE : 0;
            value[1] = 0;

            CsrBtGattWriteReqSend(gatt_tds_client->srvcElem->gattId,
                                  gatt_tds_client->srvcElem->cid,
                                  currHandle,
                                  0,
                                  GATT_TDS_CLIENT_CHARACTERISTIC_CONFIG_SIZE,
                                  value);
        }
    }
    else
    {
        GATT_TDS_CLIENT_ERROR("Invalid TDS Client instance!\n");
    }
}

/****************************************************************************/
void handleTdsClientIndication(GTDSC *tdsClient, uint16 handle, uint16 valueLength, uint8 *value)
{
    /* Send the Indication response */
    CsrBtGattClientIndicationRspSend(tdsClient->srvcElem->gattId, tdsClient->srvcElem->cid);
    
    if(handle == 0)
    {
        /* Unlikely */
    }
    else if (handle == tdsClient->handles.tdsControlPointHandle)
    {
        GattTdsClientTdsCPAttributeInd *message = CsrPmemAlloc(sizeof(*message));

        message->srvcHndl = tdsClient->srvcElem->service_handle;
        message->sizeValue = valueLength;
        if(value)
        {
            message->value = (uint8 *) CsrPmemAlloc(valueLength);
            CsrMemCpy(message->value, value, valueLength);
        }
        TdsMessageSend(tdsClient->appTask,
                       GATT_TDS_CLIENT_CONTROL_POINT_IND,
                       message);
    }
}


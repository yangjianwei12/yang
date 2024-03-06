/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_tds_client_private.h"
#include "gatt_tds_client_write.h"
#include "gatt_tds_client_debug.h"
#include "gatt_tds_client.h"
#include "gatt_tds_client_common_util.h"


/***************************************************************************/

void tdsClientHandleInternalSetTdsControlPoint(GTDSC *const tdsClient,
                                                 GattTdsOpcode op,
                                                 uint16 len,
                                                 uint8* val)
{
    if (tdsClient->handles.tdsControlPointHandle)
    {
        uint8 opSize = len + 1/* opcode */;
        uint8 *value;

        value = (uint8*) (CsrPmemAlloc(opSize));
        value[0] = op;

        memcpy(&value[1], val, sizeof(uint8)*len);
        CsrBtGattWriteReqSend(tdsClient->srvcElem->gattId,
                              tdsClient->srvcElem->cid,
                              tdsClient->handles.tdsControlPointHandle,
                              0,
                              opSize,
                              value);
    }
    else
    {
        GattTdsClientSetTdsControlPointCfm *message = CsrPmemAlloc(sizeof(*message));

        message->srvcHndl = tdsClient->srvcElem->service_handle;
        message->status = GATT_TDS_CLIENT_STATUS_CHAR_NOT_SUPPORTED;

        TdsMessageSend(tdsClient->appTask, GATT_TDS_SET_TDS_CONTROL_POINT_CFM, message);
    }

}


void GattTdsClientSetTdsControlPoint(ServiceHandle clntHndl, GattTdsOpcode op,uint16 len, uint8* val)
{
    GTDSC *gattTdsClient = ServiceHandleGetInstanceData(clntHndl);

    if (gattTdsClient)
    {
        if (gattTdsClient->pendingCmd == TDS_CLIENT_PENDING_OP_WRITE_CCCD)
        {
            GattTdsClientSetTdsControlPointCfm *message = CsrPmemAlloc(sizeof(*message));

            message->srvcHndl = clntHndl;
            message->status = GATT_TDS_CLIENT_STATUS_BUSY;

            TdsMessageSend(gattTdsClient->appTask,
                           GATT_TDS_SET_TDS_CONTROL_POINT_CFM,
                           message);
        }
        else
        {
            TdsClientInternalMsgSetDiscoveryControlPoint *message = CsrPmemAlloc(sizeof(*message));

            message->srvcHndl = gattTdsClient->srvcElem->service_handle;
            message->op = op;
            message->sizeValue = len;
            message->value = val;

            TdsMessageSend(gattTdsClient->libTask,
                           TDS_CLIENT_INTERNAL_MSG_SET_TDS_POINT_REQ,
                           message);
        }
    }
    else
    {
        GATT_TDS_CLIENT_ERROR("Invalid TDS Client instance!\n");
    }
}

/****************************************************************************/
void handleTdsWriteValueResp(GTDSC *tdsClient, uint16 handle, status_t resultCode)
{
    if (handle == tdsClient->handles.tdsControlPointHandle )
    {
        GattTdsClientSetTdsControlPointCfm *message = CsrPmemAlloc(sizeof(*message));
        message->srvcHndl = tdsClient->srvcElem->service_handle;
        message->status = getTdsClientStatusFromGattStatus(resultCode);

        TdsMessageSend(tdsClient->appTask,
                       GATT_TDS_SET_TDS_CONTROL_POINT_CFM,
                       message);
    }
}



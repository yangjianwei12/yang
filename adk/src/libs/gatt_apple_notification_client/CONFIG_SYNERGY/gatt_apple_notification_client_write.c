/* Copyright (c) 2014 - 2022 Qualcomm Technologies International, Ltd. */

#include "gatt_apple_notification_client_write.h"
#include "gatt_apple_notification_client_external_msg_send.h"

static void handleWriteClientConfigResp(GANCS *ancs, const CsrBtGattWriteCfm *write_cfm)
{
    switch(ancs->pending_cmd)
    {
        case ancs_pending_write_ns_cconfig:
            gattAncsSendSetNotificationSourceNotificationResponse(ancs, write_cfm->resultCode);
            break;

        case ancs_pending_write_ds_cconfig:
            gattAncsSendSetDataSourceNotificationResponse(ancs, write_cfm->resultCode);
            break;

        default:
        break;
    }
}

static void writeClientValue(GANCS *ancs, uint16 handle, uint16 size_value, const uint8* value)
{
    uint8 *data = (uint8*)CsrPmemAlloc(size_value);
    memcpy(data, value, size_value);

    GattWriteReqSend(ancs->gattId, 
                     ancs->cid,
                     handle, 
                     0, 
                     size_value, 
                     data);
}

void writeClientConfigNotifyValue(GANCS *ancs, bool notifications_enable, uint16 handle)
{
    uint8 value[2];
    
    value[1] = 0;
    value[0] = notifications_enable ? CSR_BT_GATT_CLIENT_CHARAC_CONFIG_NOTIFICATION : 0;
    
    writeClientValue(ancs, handle, sizeof(value), value);
}

void ancsWriteCharValue(GANCS *ancs, const ANCS_INTERNAL_MSG_WRITE_CP_CHARACTERISTIC_T* req, uint16 handle)
{
    writeClientValue(ancs, handle, req->size_command_data, req->command_data);
}

void handleAncsWriteValueResp(GANCS *ancs, const CsrBtGattWriteCfm *write_cfm)
{
    switch (ancs->pending_cmd)
    {
        case ancs_pending_write_ns_cconfig:
        case ancs_pending_write_ds_cconfig:
        {
            handleWriteClientConfigResp(ancs, write_cfm);
        }
        break;

        case ancs_pending_write_cp_attr:
        case ancs_pending_write_cp_app_attr:
        case ancs_pending_write_cp:
            gattAncsSendWriteControlPointResponse(ancs, write_cfm->resultCode);
            break;

        case ancs_pending_none:
            DEBUG_LOG_WARN("handleAncsWriteValueResp: write value response not expected; pending_cmd=enum:ancs_pending_cmd_t:%d", ancs->pending_cmd);

            break;
            
        default:
        {
            /* No other pending write values expected */
            DEBUG_PANIC("handleAncsWriteValueResp: write value response not expected; pending_cmd=enum:ancs_pending_cmd_t:%d", ancs->pending_cmd);
        }
        break;
    }
}
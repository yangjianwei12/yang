/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>

#include <gatt.h>

#include "gatt_telephone_bearer_client_write.h"
#include "gatt_telephone_bearer_client_notification.h"


/*******************************************************************************/
void sendTbsClientWriteCfm(GTBSC *const tbs_client,
                                  const GattTelephoneBearerClientStatus status,
                                  GattTelephoneBearerClientMessageId id)
{
    /* Use GATT_TBS_CLIENT_SET_CFM to create the message
     * because the structure of all write confirmations is the same,
     * but we will send the right message using the id parameter */
    MAKE_TBSC_MESSAGE(GATT_TBS_CLIENT_SET_CFM);
    /* Fill in client reference */
    message->tbsHandle = tbs_client->srvcHandle;
    /* Fill in the status */
    message->status = status;

    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Write Cfm handle = 0x%x status = 0x%x msg = 0x%x\n",
                                tbs_client->srvcHandle, status, id));

    /* Send the confirmation message to app task  */
    MessageSend(tbs_client->appTask, id, message);
}


/****************************************************************************/
void handleTbsWriteCharacteristicValueResp(GTBSC *tbs_client,
                                                const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *write_cfm)
{
    if (tbs_client != NULL)
    {
        GattTelephoneBearerClientStatus status = write_cfm->status==gatt_status_success?
                    GATT_TBS_CLIENT_STATUS_SUCCESS:GATT_TBS_CLIENT_STATUS_FAILED;

        if(write_cfm->handle == tbs_client->bearer_name_ccc_handle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TBS_CLIENT_PROVIDER_NAME_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->bearer_tech_ccc_handle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TBS_CLIENT_TECHNOLOGY_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->signal_strength_ccc_handle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TBS_CLIENT_SIGNAL_STRENGTH_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->list_current_calls_ccc_handle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TBS_CLIENT_CURRENT_CALLS_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->status_flags_ccc_handle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TBS_CLIENT_FLAGS_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->incoming_target_bearer_uri_ccc_handle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TBS_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->call_state_ccc_handle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TBS_CLIENT_CALL_STATE_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->call_control_point_ccc_handle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TBS_CLIENT_CALL_CONTROL_POINT_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->termination_reason_ccc_handle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TBS_CLIENT_TERMINATION_REASON_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->incoming_call_ccc_handle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TBS_CLIENT_INCOMING_CALL_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->remote_friendly_name_ccc_handle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TBS_CLIENT_CALL_FRIENDLY_NAME_SET_NOTIFICATION_CFM);
        }
        else
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Write Cfm ERROR unknown handle = 0x%x status = 0x%x\n",
                                        write_cfm->handle, status));
        }
    }
}


/****************************************************************************/
void handleTbsWriteWithoutResponseResp(GTBSC *tbs_client,
                                           const GATT_MANAGER_WRITE_WITHOUT_RESPONSE_CFM_T *write_cfm)
{
    if (tbs_client != NULL)
    {
        GattTelephoneBearerClientStatus status = write_cfm->status==gatt_status_success?
                    GATT_TBS_CLIENT_STATUS_SUCCESS:GATT_TBS_CLIENT_STATUS_FAILED;

        /* Write CFMs */
        if(write_cfm->handle == tbs_client->signal_strength_interval_handle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TBS_CLIENT_WRITE_SIGNAL_STRENGTH_INTERVAL_CFM);
        }
        else if(write_cfm->handle == tbs_client->call_control_point_handle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TBS_CLIENT_WRITE_CALL_CONTROL_POINT_CFM);
        }

    }

}


void tbsWriteSignalStrengthIntervalRequest(GTBSC *tbs_client, const uint8 interval, bool writeworesponse)
{

    if (tbs_client != NULL)
    {
        uint8 value[TBS_CLIENT_SIGNAL_STRENGTH_INTERVAL_SIZE];
        value[0] = interval;
        if(writeworesponse)
        {
            GattManagerWriteWithoutResponse((Task)&tbs_client->lib_task,
                                                tbs_client->signal_strength_interval_handle,
                                                TBS_CLIENT_SIGNAL_STRENGTH_INTERVAL_SIZE,
                                                value);
        }
        else
        {
            GattManagerWriteCharacteristicValue((Task)&tbs_client->lib_task,
                                                tbs_client->signal_strength_interval_handle,
                                                TBS_CLIENT_SIGNAL_STRENGTH_INTERVAL_SIZE,
                                                value);
        }
    }

}

void tbsWriteCallControlPointRequest(GTBSC *tbs_client, const uint8 opcode, const uint8 size, const uint8* param)
{
    if (tbs_client != NULL)
    {
        uint8 writeLen = size+1;
        uint8* value = NULL;
        value = PanicUnlessMalloc(writeLen);
        value[0] = opcode;
        memmove(&value[1], param, size);
        GattManagerWriteWithoutResponse((Task)&tbs_client->lib_task,
                                            tbs_client->call_control_point_handle,
                                            writeLen,
                                            value);
    }
}


void GattTelephoneBearerClientWriteSignalStrengthIntervalRequest(const ServiceHandle tbsHandle,
                                                                 const uint8 interval,
                                                                 bool writeWithoutResponse)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid parameters - Write Signal Strength Internal Request\n"));
        return;
    }

    MAKE_TBSC_MESSAGE(TELEPHONE_BEARER_INTERNAL_MSG_WRITE_SIGNAL_STRENGTH_INTERVAL);
    message->interval = interval;
    message->writeWithoutResponse = writeWithoutResponse;
    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Write Signal Strength interval = %x\n", interval));
    MessageSend((Task)&tbs_client->lib_task,
                 TELEPHONE_BEARER_INTERNAL_MSG_WRITE_SIGNAL_STRENGTH_INTERVAL,
                 message);
}

void GattTelephoneBearerClientWriteCallControlPointSimpleRequest(const ServiceHandle tbsHandle,
                                                                        const GattTbsOpcode opcode,
                                                                        const uint8 callIndex)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid parameters - Write Call Control Point Request\n"));
        return;
    }

    MAKE_TBSC_MESSAGE(TELEPHONE_BEARER_INTERNAL_MSG_WRITE_CALL_CONTROL_POINT);
    message->opcode = opcode;
    message->size = 1;
    message->param[0] = callIndex;
    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Write Call Control Point = %x:%x\n", opcode, callIndex));
    MessageSend((Task)&tbs_client->lib_task,
                 TELEPHONE_BEARER_INTERNAL_MSG_WRITE_CALL_CONTROL_POINT,
                 message);

}


void GattTelephoneBearerClientWriteCallControlPointRequest(const ServiceHandle tbsHandle,
                                                                 const GattTbsOpcode opcode,
                                                                 const uint8 size,
                                                                 const uint8* param)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid parameters - Write Call Control Point Request\n"));
        return;
    }

    MAKE_TBSC_MESSAGE_WITH_LEN(TELEPHONE_BEARER_INTERNAL_MSG_WRITE_CALL_CONTROL_POINT, size);
    message->opcode = opcode;
    message->size = size;
    memmove(&message->param[0], param, size);
    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Write Call Control Point = %x\n", opcode));
    MessageSend((Task)&tbs_client->lib_task,
                 TELEPHONE_BEARER_INTERNAL_MSG_WRITE_CALL_CONTROL_POINT,
                 message);

}


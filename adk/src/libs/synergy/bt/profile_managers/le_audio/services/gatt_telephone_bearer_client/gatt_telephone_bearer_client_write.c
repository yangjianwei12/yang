/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include <string.h>
#include <stdio.h>

#include "gatt_telephone_bearer_client_write.h"
#include "gatt_telephone_bearer_client_notification.h"


/*******************************************************************************/
void sendTbsClientWriteCfm(GTBSC *const tbs_client,
                                  const status_t status,
                                  GattTelephoneBearerClientMessageId id)
{
    /* Use GATT_TBS_CLIENT_SET_CFM to create the message
     * because the structure of all write confirmations is the same,
     * but we will send the right message using the id parameter */
    MAKE_TBSC_MESSAGE(GattTelephoneBearerClientSetCfm);
    /* Fill in client reference */
    message->tbsHandle = tbs_client->srvcElem->service_handle;
    /* Fill in the status */
    message->status = status;

    GATT_TBS_CLIENT_INFO("GTBSC: Write Cfm handle = 0x%x status = 0x%x msg = 0x%x\n",
                          tbs_client->srvcHandle, status, id);

    /* Send the confirmation message to app task  */
    TbsClientMessageSend(tbs_client->appTask, id, message);
}


/****************************************************************************/
void handleTbsWriteCharacteristicValueResp(GTBSC *tbs_client,
                                                const CsrBtGattWriteCfm *write_cfm)
{
    if (tbs_client != NULL)
    {
        GattTelephoneBearerClientStatus status = write_cfm->resultCode==CSR_BT_GATT_RESULT_SUCCESS?
                    GATT_TELEPHONE_BEARER_CLIENT_STATUS_SUCCESS:GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED;

        if(write_cfm->handle == tbs_client->handles.bearerNameCccHandle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TELEPHONE_BEARER_CLIENT_PROVIDER_NAME_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->handles.bearerTechCccHandle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TELEPHONE_BEARER_CLIENT_TECHNOLOGY_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->handles.signalStrengthCccHandle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TELEPHONE_BEARER_CLIENT_SIGNAL_STRENGTH_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->handles.listCurrentCallsCccHandle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TELEPHONE_BEARER_CLIENT_CURRENT_CALLS_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->handles.statusFlagsCccHandle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TELEPHONE_BEARER_CLIENT_FLAGS_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->handles.incomingTargetBearerUriCccHandle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TELEPHONE_BEARER_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->handles.callStateCccHandle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TELEPHONE_BEARER_CLIENT_CALL_STATE_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->handles.callControlPointCccHandle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TELEPHONE_BEARER_CLIENT_CALL_CONTROL_POINT_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->handles.terminationReasonCccHandle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TELEPHONE_BEARER_CLIENT_TERMINATION_REASON_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->handles.incomingCallCccHandle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TELEPHONE_BEARER_CLIENT_INCOMING_CALL_SET_NOTIFICATION_CFM);
        }
        else if(write_cfm->handle == tbs_client->handles.remoteFriendlyNameCccHandle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TELEPHONE_BEARER_CLIENT_CALL_FRIENDLY_NAME_SET_NOTIFICATION_CFM);
        }
        else
        {
            GATT_TBS_CLIENT_ERROR("GTBSC: Write Cfm ERROR unknown handle = 0x%x status = 0x%x\n",
                                   write_cfm->handle, status);
        }
    }
}


/****************************************************************************/
void handleTbsWriteWithoutResponseResp(GTBSC *tbs_client,
                                           const CsrBtGattWriteCfm *write_cfm)
{
    if (tbs_client != NULL)
    {
        GattTelephoneBearerClientStatus status = write_cfm->resultCode==CSR_BT_GATT_RESULT_SUCCESS?
                    GATT_TELEPHONE_BEARER_CLIENT_STATUS_SUCCESS:GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED;

        /* Write CFMs */
        if(write_cfm->handle == tbs_client->handles.signalStrengthIntervalHandle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TELEPHONE_BEARER_CLIENT_WRITE_SIGNAL_STRENGTH_INTERVAL_CFM);
        }
        else if(write_cfm->handle == tbs_client->handles.callControlPointHandle)
        {
            sendTbsClientWriteCfm(tbs_client,
                                  status,
                                  GATT_TELEPHONE_BEARER_CLIENT_WRITE_CALL_CONTROL_POINT_CFM);
        }
    }
}


void tbsWriteSignalStrengthIntervalRequest(GTBSC *tbs_client,
                                               const uint8 interval,
                                               bool writeworesponse)
{
    if (tbs_client != NULL)
    {
        uint8 *value;
        value = (uint8*)CsrPmemZalloc(TBS_CLIENT_SIGNAL_STRENGTH_INTERVAL_SIZE* sizeof(uint8));
        value[0] = interval;
        if(writeworesponse)
        {
	        CsrBtGattWriteCmdReqSend(tbs_client->srvcElem->gattId,
	                                 tbs_client->srvcElem->cid,
                                     tbs_client->handles.signalStrengthIntervalHandle,
	                                 TBS_CLIENT_SIGNAL_STRENGTH_INTERVAL_SIZE,
	                                 value);
		}			 
		else
		{						 
            CsrBtGattWriteReqSend(tbs_client->srvcElem->gattId,
	                              tbs_client->srvcElem->cid,
                                  tbs_client->handles.signalStrengthIntervalHandle,
	                              0,
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
        value = (uint8*)CsrPmemZalloc(writeLen);
        value[0] = opcode;
        memmove(&value[1], param, size);

        CsrBtGattWriteCmdReqSend(tbs_client->srvcElem->gattId,
                                 tbs_client->srvcElem->cid,
                                 tbs_client->handles.callControlPointHandle,
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
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid parameters - Write Signal Strength Internal Request\n");
        return;
    }
    else
    {
        MAKE_TBSC_MESSAGE(TelephoneBearerInternalMsgWriteSignalStrengthInterval);

        message->interval = interval;
        message->srvcHndl = tbs_client->srvcElem->service_handle;
        message->writeWithoutResponse = writeWithoutResponse;
        GATT_TBS_CLIENT_INFO("GTBSC: Signal Strength interval = %x\n", interval);
        TbsClientMessageSend((AppTask)tbs_client->lib_task,
                     TELEPHONE_BEARER_INTERNAL_MSG_WRITE_SIGNAL_STRENGTH_INTERVAL,
                     message);
    }
}

void GattTelephoneBearerClientWriteCallControlPointSimpleRequest(const ServiceHandle tbsHandle,
                                                                        const GattTbsOpcode opcode,
                                                                        const uint8 callIndex)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid parameters - Write Call Control Point Request\n");
        return;
    }
    else
    {
        MAKE_TBSC_MESSAGE(TelephoneBearerInternalMsgWriteCallControlPoint);
        message->param = (uint8*)CsrPmemZalloc(sizeof(callIndex));
        message->srvcHndl = tbs_client->srvcElem->service_handle;
        message->opcode = opcode;
        message->size = 1;
        message->param[0] = callIndex;
        GATT_TBS_CLIENT_INFO("GTBSC: Write Call Control Point = %x:%x\n", opcode,callIndex);
        TbsClientMessageSend((AppTask)tbs_client->lib_task,
                     TELEPHONE_BEARER_INTERNAL_MSG_WRITE_CALL_CONTROL_POINT,
                     message);
    }
}


void GattTelephoneBearerClientWriteCallControlPointRequest(const ServiceHandle tbsHandle,
                                                                 const GattTbsOpcode opcode,
                                                                 const uint8 size,
                                                                 const uint8* param)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid parameters - Write Call Control Point Request\n");
        return;
    }
    else
    {
        MAKE_TBSC_MESSAGE_WITH_LEN(TelephoneBearerInternalMsgWriteCallControlPoint, size);
        message->param = (uint8*)CsrPmemZalloc(size);
        message->srvcHndl = tbs_client->srvcElem->service_handle;
        message->opcode = opcode;
        message->size = size;
        memmove(message->param, param, size);
        GATT_TBS_CLIENT_INFO("GTBSC: Call Control Point = %x\n", opcode);
        TbsClientMessageSend((AppTask)tbs_client->lib_task,
                     TELEPHONE_BEARER_INTERNAL_MSG_WRITE_CALL_CONTROL_POINT,
                     message);
    }
}


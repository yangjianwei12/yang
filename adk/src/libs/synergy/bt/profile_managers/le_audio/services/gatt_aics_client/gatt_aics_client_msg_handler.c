/* Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_aics_client_private.h"
#include "gatt_aics_client_debug.h"
#include "gatt_aics_client.h"
#include "gatt_aics_client_discovery.h"
#include "gatt_aics_client_common.h"
#include "gatt_aics_client_read.h"
#include "gatt_aics_client_write.h"
#include "gatt_aics_client_notification.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "gatt_aics_client_init.h"

#include "csr_bt_gatt_lib.h"

/***************************************************************************/
static GattAicsClientMessageId aicsClientSetMessageIdWriteCfm(uint16 command)
{
    GattAicsClientMessageId id = GATT_AICS_CLIENT_MESSAGE_TOP;

    switch(command)
    {
        case aics_client_write_set_gain_setting_pending:
            id = GATT_AICS_CLIENT_SET_GAIN_SETTING_CFM;
        break;

        case aics_client_write_unmute_pending:
            id = GATT_AICS_CLIENT_UNMUTE_CFM;
        break;

        case aics_client_write_mute_pending:
            id = GATT_AICS_CLIENT_MUTE_CFM;
        break;

        case aics_client_write_set_manual_gain_mode_pending:
            id = GATT_AICS_CLIENT_SET_MANUAL_GAIN_MODE_CFM;
        break;

        case aics_client_write_set_automatic_gain_mode_pending:
            id = GATT_AICS_CLIENT_SET_AUTOMATIC_GAIN_MODE_CFM;
        break;

        default:
        break;
    }

    return id;
}

/***************************************************************************/
static void aicsClientHandleWriteValueRespCfm(
                                     GAICS *const aics_client,
                                     const CsrBtGattWriteCfm *const write_cfm)
{
    if (aics_client != NULL)
    {
        if (write_cfm->handle == aics_client->handles.inputStateCccHandle)
        {
            aicsClientSendAicsClientWriteCfm(aics_client,
                                             write_cfm->resultCode,
                                             GATT_AICS_CLIENT_INPUT_STATE_SET_NTF_CFM);
        }
        else if (write_cfm->handle == aics_client->handles.inputStatusCccHandle)
        {
            aicsClientSendAicsClientWriteCfm(aics_client,
                                             write_cfm->resultCode,
                                             GATT_AICS_CLIENT_INPUT_STATUS_SET_NTF_CFM);
        }
        else if (write_cfm->handle == aics_client->handles.audioInputDescriptionCccHandle)
        {
            aicsClientSendAicsClientWriteCfm(aics_client,
                                             write_cfm->resultCode,
                                             GATT_AICS_CLIENT_AUDIO_INPUT_DESC_SET_NTF_CFM);
        }
        else if (write_cfm->handle == aics_client->handles.audioInputControlPointHandle)
        {
            GattAicsClientMessageId id = aicsClientSetMessageIdWriteCfm(aics_client->pending_cmd);


            if (id < GATT_AICS_CLIENT_MESSAGE_TOP)
            {
                aicsClientSendAicsClientWriteCfm(aics_client,
                                                 write_cfm->resultCode,
                                                 id);
            }
        }
        else if (write_cfm->handle == aics_client->handles.audioInputDescriptionHandle)
        {
            aicsClientSendAicsClientWriteCfm(aics_client,
                                             write_cfm->resultCode,
                                             GATT_AICS_CLIENT_SET_AUDIO_INPUT_DESC_CFM);
        }

        aics_client->pending_cmd = aics_client_pending_none;
    }
    else
    {
        GATT_AICS_CLIENT_PANIC("Null instance\n");
    }
}

/****************************************************************************/
static void aicsClientHandleReadValueResp(GAICS *aics_client,
                                         const CsrBtGattReadCfm *read_cfm)
{
    switch (aics_client->pending_cmd)
    {
        case aics_client_read_pending:
        {
            if (read_cfm->handle == aics_client->handles.inputStateHandle)
            {
                /* Send read GATT_AICS_CLIENT_READ_INPUT_STATE_CFM message to application */
                aicsSendReadInputStateCfm(aics_client,
                                          read_cfm->resultCode,
                                          read_cfm->value);
            }
            else if (read_cfm->handle == aics_client->handles.gainSettingPropertiesHandle)
            {
                /* Send read GATT_AICS_CLIENT_READ_GAIN_SET_PROPERTIES_CFM message to application */
                aicsSendReadGainSetPropertiesCfm(aics_client,
                                                 read_cfm->resultCode,
                                                 read_cfm->value);
            }
            else if (read_cfm->handle == aics_client->handles.inputTypeHandle)
            {
                /* Send read GATT_AICS_CLIENT_READ_INPUT_TYPE_CFM message to application */
                aicsSendReadInputTypeCfm(aics_client,
                                         read_cfm->resultCode,
                                         read_cfm->value);
            }
            else if (read_cfm->handle == aics_client->handles.inputStatusHandle)
            {
                /* Send read GATT_AICS_CLIENT_READ_INPUT_STATUS_CFM message to application */
                aicsSendReadInputStatusCfm(aics_client,
                                           read_cfm->resultCode,
                                           read_cfm->value);
            }
            else if (read_cfm->handle == aics_client->handles.audioInputDescriptionHandle)
            {
                /* Send read GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CFM message to application */
                aicsSendReadAudioInputDescCfm(aics_client,
                                              read_cfm->resultCode,
                                              read_cfm->valueLength,
                                              read_cfm->value);
            }
        }
        break;

        case aics_client_read_pending_ccc:
        {
            if (read_cfm->handle == aics_client->handles.inputStateCccHandle)
            {
                /* Send read GATT_AICS_CLIENT_READ_INPUT_STATE_CCC_CFM message to application */
                aicsSendReadCccCfm(aics_client,
                                   read_cfm->resultCode,
                                   read_cfm->valueLength,
                                   read_cfm->value,
                                   GATT_AICS_CLIENT_READ_INPUT_STATE_CCC_CFM);
            }
            else if (read_cfm->handle == aics_client->handles.inputStatusCccHandle)
            {
                /* Send read GATT_AICS_CLIENT_READ_INPUT_STATUS_CCC_CFM message to application */
                aicsSendReadCccCfm(aics_client,
                                   read_cfm->resultCode,
                                   read_cfm->valueLength,
                                   read_cfm->value,
                                   GATT_AICS_CLIENT_READ_INPUT_STATUS_CCC_CFM);
            }
            else if (read_cfm->handle == aics_client->handles.audioInputDescriptionCccHandle)
            {
                /* Send read GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CCC_CFM message to application */
                aicsSendReadCccCfm(aics_client,
                                   read_cfm->resultCode,
                                   read_cfm->valueLength,
                                   read_cfm->value,
                                   GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CCC_CFM);
            }
        }
        break;

        default:
        {
            /* No other pending read values expected */
            GATT_AICS_CLIENT_ERROR("Read value response not expected [0x%x]\n",
                                    aics_client->pending_cmd);
        }
        break;
    }

    aics_client->pending_cmd = aics_client_pending_none;
}

/****************************************************************************/
static void aicsClientHandleClientNotification(GAICS *aics_client,
                                               const CsrBtGattClientNotificationInd *ind)
{
    if (ind->valueHandle == aics_client->handles.inputStateHandle)
    {
        MAKE_AICS_CLIENT_MESSAGE(GattAicsClientInputStateInd)

        message->srvcHndl = aics_client->srvcElem->service_handle;
        message->gainSetting = (int8) ind->value[0];
        message->mute = (GattAicsClientMute) ind->value[1];
        message->gainMode = (GattAicsClientGainMode) ind->value[2];
        message->changeCounter = ind->value[3];

        /* Send the confirmation message to app task  */
        AicsMessageSend(aics_client->app_task, GATT_AICS_CLIENT_INPUT_STATE_IND, message);
    }
    else if (ind->valueHandle == aics_client->handles.inputStatusHandle)
    {
        MAKE_AICS_CLIENT_MESSAGE(GattAicsClientInputStatusInd)

        message->srvcHndl = aics_client->srvcElem->service_handle;
        message->inputStatus = ind->value[0];
        
        AicsMessageSend(aics_client->app_task, GATT_AICS_CLIENT_INPUT_STATUS_IND, message);
    }
    else if (ind->valueHandle == aics_client->handles.audioInputDescriptionHandle)
    {
        MAKE_AICS_CLIENT_MESSAGE_WITH_LEN(GattAicsClientAudioInputDescInd,
                                          ind->valueLength);
        message->audioInputDesc = (uint8 *) CsrPmemZalloc(ind->valueLength);

        message->srvcHndl = aics_client->srvcElem->service_handle;
        message->sizeValue = ind->valueLength;

        memcpy(message->audioInputDesc, ind->value, ind->valueLength);

        AicsMessageSend(aics_client->app_task, GATT_AICS_CLIENT_AUDIO_INPUT_DESC_IND, message);
    }
}

/****************************************************************************/
static void aicsClientHandleGattMsg(void *task, MsgId id, Msg msg)
{
    GAICS *gatt_client = (GAICS *)task;
    
    switch (id)
    {
        case CSR_BT_GATT_DISCOVER_CHARAC_CFM:
        {
            aicsClientHandleDiscoverAllAicsCharacteristicsResp(gatt_client,
                                              (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *)msg);
        }
        break;

        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM:
        {
            aicsClientHandleDiscoverAllCharacteristicDescriptorsResp(gatt_client,
                                  (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *)msg);
        }
        break;
        
        case CSR_BT_GATT_WRITE_CFM:
        {
            /* Write/Notification Confirmation */
           aicsClientHandleWriteValueRespCfm(gatt_client,
                                             (const CsrBtGattWriteCfm *)msg);
        }
        break;

        case CSR_BT_GATT_READ_CFM:
        {
            aicsClientHandleReadValueResp(gatt_client,
                                         (const CsrBtGattReadCfm *)msg);
        }
        break;

        case CSR_BT_GATT_CLIENT_NOTIFICATION_IND:
        {
            aicsClientHandleClientNotification(gatt_client,
                                          (const CsrBtGattClientNotificationInd *)msg);
        }
        break;

        default:
        {
            /* Unrecognised GATT Manager message */
            GATT_AICS_CLIENT_WARNING("AICS Client GattMgr Msg not handled [0x%x]\n", id);
        }
        break;
    }
}

/****************************************************************************/
static void aicsClientSetFlagWritePending(GAICS * aics_client, aics_client_control_point_opcodes_t opcode)
{
    switch(opcode)
    {
        case aics_client_set_gain_setting_op:
            aics_client->pending_cmd = aics_client_write_set_gain_setting_pending;
        break;

        case aics_client_unmute_op:
            aics_client->pending_cmd = aics_client_write_unmute_pending;
        break;

        case aics_client_mute_op:
            aics_client->pending_cmd = aics_client_write_mute_pending;
        break;

        case aics_client_set_manual_gain_mode_op:
             aics_client->pending_cmd = aics_client_write_set_manual_gain_mode_pending;
        break;

        case aics_client_set_automatic_gain_mode_op:
            aics_client->pending_cmd = aics_client_write_set_automatic_gain_mode_pending;
        break;

        default:
        break;
    }
}

/***************************************************************************/
static void  aicsClientHandleInternalMessage(void *task, MsgId id, Msg msg)
{
    GAICS * aics_client = (GAICS *)task;

    GATT_AICS_CLIENT_INFO("Message id (%d)\n",id);

    if (aics_client)
    {
        switch(id)
        {
            case AICS_CLIENT_INTERNAL_MSG_READ_CCC:
            {
                uint16 handle = ((const AICS_CLIENT_INTERNAL_MSG_READ_CCC_T*)msg)->handle;

                aicsClientHandleInternalRead(aics_client, handle);
                aics_client->pending_cmd = aics_client_read_pending_ccc;
            }
            break;

            case AICS_CLIENT_INTERNAL_MSG_READ:
            {
                uint16 handle = ((const AICS_CLIENT_INTERNAL_MSG_READ_T*)msg)->handle;

                aicsClientHandleInternalRead(aics_client, handle);
                aics_client->pending_cmd = aics_client_read_pending;
            }
            break;

            case AICS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ:
            {
                AICS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ_T *message = (AICS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ_T *)msg;

                aicsClientHandleInternalRegisterForNotification(aics_client,
                                                                message->enable,
                                                                message->handle);

                aics_client->pending_cmd = aics_client_write_notification_pending;
            }
            break;

            case AICS_CLIENT_INTERNAL_MSG_WRITE:
            {
                AICS_CLIENT_INTERNAL_MSG_WRITE_T *message = (AICS_CLIENT_INTERNAL_MSG_WRITE_T *)msg;

                if (message->handle == aics_client->handles.audioInputDescriptionHandle)
                {
                    aicsClientHandleInternalWriteWithoutResponse(aics_client,
                                                                 message->handle,
                                                                 message->size_value,
                                                                 message->value);
                }
                else if (message->handle == aics_client->handles.audioInputControlPointHandle)
                {
                    aicsClientHandleInternalWrite(aics_client,
                                                  message->handle,
                                                  message->size_value,
                                                  message->value);

                    aicsClientSetFlagWritePending(aics_client,
                                                  (aics_client_control_point_opcodes_t) message->value[0]);
                }

            }
            break;

            default:
            {
                /* Internal unrecognised messages */
                GATT_AICS_CLIENT_WARNING("Unknown Message received from Internal To lib \n");
            }
            break;
        }
    }
}

/****************************************************************************/
void gattAicsClientMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *message = NULL;
    gatt_aics_client *inst = *((gatt_aics_client **)gash);

    if (CsrSchedMessageGet(&eventClass, &message))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                CsrBtGattPrim *id = message;
                GAICS *aics_client = (GAICS *) GetServiceClientByGattMsg(&inst->service_handle_list, message);
                void *msg = GetGattManagerMsgFromGattMsg(message, id);

                if(aics_client)
                    aicsClientHandleGattMsg(aics_client, *id, msg);

                if(msg!=message)
                {
                    CsrPmemFree(msg);
                }

                CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, message);
            }
            break;
            case AICS_CLIENT_PRIM:
            {
                aics_client_internal_msg_t *id = (aics_client_internal_msg_t *)message;
                GAICS *aics_client = (GAICS *) GetServiceClientByServiceHandle(message);

                aicsClientHandleInternalMessage(aics_client, *id, message);
            }
            break;

            default:
                GATT_AICS_CLIENT_WARNING("GAICS: Client Msg not handled [0x%x]\n", eventClass);
            break;
        }

        SynergyMessageFree(eventClass, message);
    }
}


/* Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_vocs_client_private.h"
#include "gatt_vocs_client_debug.h"
#include "gatt_vocs_client.h"
#include "gatt_vocs_client_discovery.h"
#include "gatt_vocs_client_common.h"
#include "gatt_vocs_client_read.h"
#include "gatt_vocs_client_write.h"
#include "gatt_vocs_client_notification.h"
#include "gatt_vocs_client_init.h"
#include "csr_bt_gatt_client_util_lib.h"

#include "csr_bt_gatt_lib.h"

#define VOCS_CLIENT_AUDIO_LOC_SIZE   4


/***************************************************************************/
static void vocsClientHandleWriteValueRespCfm(
                                     GVOCS *const vocs_client,
                                     const CsrBtGattWriteCfm *write_cfm)
{
    if (vocs_client != NULL)
    {
        if (write_cfm->handle == vocs_client->handles.offsetStateCccHandle)
        {
            vocsClientSendVocsClientWriteCfm(vocs_client,
                                             write_cfm->resultCode,
                                             GATT_VOCS_CLIENT_OFFSET_STATE_SET_NTF_CFM);
        }
        else if (write_cfm->handle == vocs_client->handles.audioLocationCccHandle)
        {
            vocsClientSendVocsClientWriteCfm(vocs_client,
                                             write_cfm->resultCode,
                                             GATT_VOCS_CLIENT_AUDIO_LOCATION_SET_NTF_CFM);
        }
        else if (write_cfm->handle == vocs_client->handles.audioOutputDescriptionCccHandle)
        {
            vocsClientSendVocsClientWriteCfm(vocs_client,
                                             write_cfm->resultCode,
                                             GATT_VOCS_CLIENT_AUDIO_OUTPUT_DESC_SET_NTF_CFM);
        }
        else if (write_cfm->handle == vocs_client->handles.volumeOffsetControlPointHandle)
        {
            vocsClientSendVocsClientWriteCfm(vocs_client,
                                             write_cfm->resultCode,
                                             GATT_VOCS_CLIENT_SET_VOLUME_OFFSET_CFM);
        }
        else if (write_cfm->handle == vocs_client->handles.audioLocationHandle)
        {
            vocsClientSendVocsClientWriteCfm(vocs_client,
                                             write_cfm->resultCode,
                                             GATT_VOCS_CLIENT_SET_AUDIO_LOC_CFM);
        }
        else if (write_cfm->handle == vocs_client->handles.audioOutputDescriptionHandle)
        {
            vocsClientSendVocsClientWriteCfm(vocs_client,
                                             write_cfm->resultCode,
                                             GATT_VOCS_CLIENT_SET_AUDIO_OUTPUT_DESC_CFM);
        }

        vocs_client->pending_cmd = vocs_client_pending_none;
    }
    else
    {
        GATT_VOCS_CLIENT_PANIC("Null instance\n");
    }
}

/****************************************************************************/
static void vocsClietHandleReadValueResp(GVOCS *vocs_client,
                                         const CsrBtGattReadCfm *read_cfm)
{
    switch (vocs_client->pending_cmd)
    {
        case vocs_client_read_pending:
        {
            if (read_cfm->handle == vocs_client->handles.offsetStateHandle)
            {
                /* Send read GATT_VOCS_CLIENT_READ_OFFSET_STATE_CFM message to application */
                vocsSendReadOffsetStateCfm(vocs_client,
                                           read_cfm->resultCode,
                                           read_cfm->value);
            }
            else if (read_cfm->handle == vocs_client->handles.audioLocationHandle)
            {
                /* Send read GATT_VOCS_CLIENT_READ_AUDIO_LOCATION_CFM message to application */
                vocsSendReadAudioLocationCfm(vocs_client,
                                             read_cfm->resultCode,
                                             read_cfm->value);
            }
            else if (read_cfm->handle == vocs_client->handles.audioOutputDescriptionHandle)
            {
                /* Send read GATT_VOCS_CLIENT_READ_AUDIO_OUTPUT_DESC_CFM message to application */
                vocsSendReadAudioOutputDescCfm(vocs_client,
                                               read_cfm->resultCode,
                                               read_cfm->valueLength,
                                               read_cfm->value);
            }
        }
        break;

        case vocs_client_read_pending_ccc:
        {
            if (read_cfm->handle == vocs_client->handles.offsetStateCccHandle)
            {
                /* Send read GATT_VOCS_CLIENT_READ_OFFSET_STATE_CCC_CFM message to application */
                vocsSendReadCccCfm(vocs_client,
                                   read_cfm->resultCode,
                                   read_cfm->valueLength,
                                   read_cfm->value,
                                   GATT_VOCS_CLIENT_READ_OFFSET_STATE_CCC_CFM);
            }
            else if (read_cfm->handle == vocs_client->handles.audioLocationCccHandle)
            {
                /* Send read GATT_VOCS_CLIENT_READ_AUDIO_LOCATION_CCC_CFM message to application */
                vocsSendReadCccCfm(vocs_client,
                                   read_cfm->resultCode,
                                   read_cfm->valueLength,
                                   read_cfm->value,
                                   GATT_VOCS_CLIENT_READ_AUDIO_LOCATION_CCC_CFM);
            }
            else if (read_cfm->handle == vocs_client->handles.audioOutputDescriptionCccHandle)
            {
                /* Send read GATT_VOCS_CLIENT_READ_AUDIO_OUTPUT_DESC_CCC_CFM message to application */
                vocsSendReadCccCfm(vocs_client,
                                   read_cfm->resultCode,
                                   read_cfm->valueLength,
                                   read_cfm->value,
                                   GATT_VOCS_CLIENT_READ_AUDIO_OUTPUT_DESC_CCC_CFM);
            }

        }
        break;

        default:
        {
            /* No other pending read values expected */
            GATT_VOCS_CLIENT_ERROR("Read value response not expected [0x%x]\n",
                                    vocs_client->pending_cmd);
        }
        break;
    }

    vocs_client->pending_cmd = vocs_client_pending_none;
}

/****************************************************************************/
static void vocsClientHandleClientNotification(GVOCS *vocs_client,
                                               const CsrBtGattClientNotificationInd *ind)
{
    if (ind->valueHandle == vocs_client->handles.offsetStateHandle)
    {
        if(ind->valueLength == VOCS_CLIENT_OFFSET_STATE_VALUE_SIZE)
        {
            MAKE_VOCS_CLIENT_MESSAGE(GattVocsClientOffsetStateInd)

            message->srvcHdnl = vocs_client->srvcElem->service_handle;
            message->volumeOffset = ((uint16)(ind->value[0])) | (((uint16) ind->value[1]) << 8);
            message->changeCounter = ind->value[2];

            /* Send the confirmation message to app task  */
            VocsMessageSend(vocs_client->app_task, GATT_VOCS_CLIENT_OFFSET_STATE_IND, message);
        }
    }
    else if (ind->valueHandle == vocs_client->handles.audioLocationHandle)
    {
        if(ind->valueLength == VOCS_CLIENT_AUDIO_LOCATION_VALUE_SIZE)
        {
            MAKE_VOCS_CLIENT_MESSAGE(GattVocsClientAudioLocationInd)

            message->srvcHdnl = vocs_client->srvcElem->service_handle;
            message->audioLocation = (GattVocsClientAudioLoc)(((uint32) ind->value[0])       |
                                                              (((uint32) ind->value[1]) << 8) |
                                                              (((uint32) ind->value[2]) << 16)|
                                                              (((uint32) ind->value[3]) << 24));

            VocsMessageSend(vocs_client->app_task, GATT_VOCS_CLIENT_AUDIO_LOCATION_IND, message);
        }
    }
    else if (ind->valueHandle == vocs_client->handles.audioOutputDescriptionHandle)
    {
        MAKE_VOCS_CLIENT_MESSAGE_WITH_LEN(GattVocsClientAudioOutputDescInd,
                                          ind->valueLength);

        message->audioOutputDesc = (uint8 *) CsrPmemZalloc(ind->valueLength);

        message->srvcHdnl = vocs_client->srvcElem->service_handle;
        message->sizeValue = ind->valueLength;

        memcpy(message->audioOutputDesc, ind->value, ind->valueLength);

        VocsMessageSend(vocs_client->app_task, GATT_VOCS_CLIENT_AUDIO_OUTPUT_DESC_IND, message);
    }
}

/****************************************************************************/
static void vocsClientHandleGattMsg(void *task, MsgId id, Msg msg)
{
    GVOCS *gatt_client = (GVOCS *)task;
    
    switch (id)
    {
        case CSR_BT_GATT_DISCOVER_CHARAC_CFM:
        {
            vocsClientHandleDiscoverAllVocsCharacteristicsResp(gatt_client,
                                                     (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *)msg);
        }
        break;

        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM:
        {
            vocsClientHandleDiscoverAllVocsCharacteristicDescriptorsResp(gatt_client,
                                                               (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *)msg);
        }
        break;

        case CSR_BT_GATT_WRITE_CFM:
        {
            /* Write/Notification Confirmation */
            vocsClientHandleWriteValueRespCfm(gatt_client,
                                             (const CsrBtGattWriteCfm*) msg);
        }
        break;

        case CSR_BT_GATT_READ_CFM:
        {
            vocsClietHandleReadValueResp(gatt_client,
                                (const CsrBtGattReadCfm *) msg);
        }
        break;

        case CSR_BT_GATT_CLIENT_NOTIFICATION_IND:
        {
            vocsClientHandleClientNotification(gatt_client,
                                        (const CsrBtGattClientNotificationInd *) msg);
        }
        break;

        default:
        {
            /* Unrecognised GATT Manager message */
            GATT_VOCS_CLIENT_WARNING("GVOCS: VOCS Client GattMgr Msg not handled [0x%x]\n", id);
        }
        break;
    }
}

/***************************************************************************/
static void  vocsClientHandleInternalMessage(void *task, MsgId id, Msg msg)
{
    GVOCS * vocs_client = (GVOCS *)task;

    GATT_VOCS_CLIENT_INFO("Message id (%d)\n",id);

    if (vocs_client)
    {
        switch(id)
        {
            case VOCS_CLIENT_INTERNAL_MSG_READ_CCC:
            {
                uint16 handle = ((const VOCS_CLIENT_INTERNAL_MSG_READ_CCC_T*)msg)->handle;

                vocsClientHandleInternalRead(vocs_client,
                                             handle);

                vocs_client->pending_cmd = vocs_client_read_pending_ccc;
            }
            break;

            case VOCS_CLIENT_INTERNAL_MSG_READ:
            {
                uint16 handle = ((const VOCS_CLIENT_INTERNAL_MSG_READ_T*)msg)->handle;

                vocsClientHandleInternalRead(vocs_client,
                                             handle);

                vocs_client->pending_cmd = vocs_client_read_pending;
            }
            break;

            case VOCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ:
            {
                VOCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ_T *message = (VOCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ_T *)msg;

                vocsClientHandleInternalRegisterForNotification(vocs_client,
                                                                message->enable,
                                                                message->handle);

                vocs_client->pending_cmd = vocs_client_write_notification_pending;
            }
            break;

            case VOCS_CLIENT_INTERNAL_MSG_WRITE:
            {
                VOCS_CLIENT_INTERNAL_MSG_WRITE_T *message = (VOCS_CLIENT_INTERNAL_MSG_WRITE_T *)msg;

                if (message->handle == vocs_client->handles.audioOutputDescriptionHandle)
                {
                    vocsClientHandleInternalWriteWithoutResponse(vocs_client,
                                                                 message->handle,
                                                                 message->size_value,
                                                                 message->value);

                    vocs_client->pending_cmd = vocs_client_set_audio_desc_pending;
                }
                else if (message->handle == vocs_client->handles.volumeOffsetControlPointHandle)
                {
                    vocsClientHandleInternalWrite(vocs_client,
                                                  message->handle,
                                                  message->size_value,
                                                  message->value);

                    vocs_client->pending_cmd = vocs_client_write_set_vol_offset_pending;
                }
            }
            break;

            case VOCS_CLIENT_INTERNAL_MSG_AUDIO_LOC_WRITE:
            {
                VOCS_CLIENT_INTERNAL_MSG_AUDIO_LOC_WRITE_T *message = (VOCS_CLIENT_INTERNAL_MSG_AUDIO_LOC_WRITE_T *)msg;

                uint16 value_size = VOCS_CLIENT_AUDIO_LOC_SIZE;
                uint8 value[VOCS_CLIENT_AUDIO_LOC_SIZE];

                value[0] = (uint8) message->value;
                value[1] = (uint8) (message->value >> 8);
                value[2] = (uint8) (message->value >> 16);
                value[3] = (uint8) (message->value >> 24);

                vocsClientHandleInternalWriteWithoutResponse(vocs_client,
                                                             message->handle,
                                                             value_size,
                                                             value);

                vocs_client->pending_cmd = vocs_client_set_audio_loc;
            }
            break;

            default:
            {
                /* Internal unrecognised messages */
                GATT_VOCS_CLIENT_WARNING("Unknown Message received from Internal To lib \n");
            }
            break;
        }
    }
}

/****************************************************************************/
void gattVocsClientMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *message = NULL;
    gatt_vocs_client *inst = *((gatt_vocs_client **)gash);

    if (CsrSchedMessageGet(&eventClass, &message))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                CsrBtGattPrim *id = message;
                GVOCS *vocs_client = (GVOCS *) GetServiceClientByGattMsg(&inst->service_handle_list, message);
                void *msg = GetGattManagerMsgFromGattMsg(message, id);

                if(vocs_client)
                {
                    vocsClientHandleGattMsg(vocs_client, *id, msg);
                }

                if(msg!=message)
                {
                    CsrPmemFree(msg);
                }

                CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, message);
            }
            break;

            case VOCS_CLIENT_PRIM:
            {
                vocs_client_internal_msg_t *id = (vocs_client_internal_msg_t *)message;
                GVOCS *vocs_client = (GVOCS *) GetServiceClientByServiceHandle(message);

                vocsClientHandleInternalMessage(vocs_client, *id, message);
            }
            break;

            default:
                GATT_VOCS_CLIENT_WARNING("GVOCS: Client Msg not handled [0x%x]\n", eventClass);
            break;
        }

        SynergyMessageFree(eventClass, message);
    }
}


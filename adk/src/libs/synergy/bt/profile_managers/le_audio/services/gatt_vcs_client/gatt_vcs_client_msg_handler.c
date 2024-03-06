/* Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_vcs_client_private.h"
#include "gatt_vcs_client_debug.h"
#include "gatt_vcs_client.h"
#include "gatt_vcs_client_discovery.h"
#include "gatt_vcs_client_common.h"
#include "gatt_vcs_client_read.h"
#include "gatt_vcs_client_write.h"
#include "gatt_vcs_client_notification.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "gatt_vcs_client_init.h"
#include "gatt_vcs_client_common_util.h"

#include "csr_bt_gatt_lib.h"

/***************************************************************************/
static GattVcsClientMessageId setMessageIdWriteCfm(uint16 command)
{
    GattVcsClientMessageId id = GATT_VCS_CLIENT_MESSAGE_TOP;

    switch(command)
    {
        case vcs_client_write_rel_vol_down_pending:
            id = GATT_VCS_CLIENT_REL_VOL_DOWN_CFM;
        break;

        case vcs_client_write_rel_vol_up_pending:
            id = GATT_VCS_CLIENT_REL_VOL_UP_CFM;
        break;

        case vcs_client_write_unmute_rel_vol_down_pending:
            id = GATT_VCS_CLIENT_UNMUTE_REL_VOL_DOWN_CFM;
        break;

        case vcs_client_write_unmute_rel_vol_up_pending:
            id = GATT_VCS_CLIENT_UNMUTE_REL_VOL_UP_CFM;
        break;

        case vcs_client_write_absolute_vol_pending:
            id = GATT_VCS_CLIENT_ABS_VOL_CFM;
        break;

        case vcs_client_write_unmute_pending:
            id = GATT_VCS_CLIENT_UNMUTE_CFM;
        break;

        case vcs_client_write_mute_pending:
            id = GATT_VCS_CLIENT_MUTE_CFM;
        break;

        default:
        break;
    }

    return id;
}

/***************************************************************************/
static void vcsClientHandleWriteValueRespCfm(GVCSC *const vcs_client, const CsrBtGattWriteCfm *const write_cfm)
{
    if (vcs_client != NULL)
    {
        if (write_cfm->handle == vcs_client->handles.volumeStateCccHandle)
        {
            vcsClientSendVcsClientWriteCfm(vcs_client,
                                           write_cfm->resultCode,
                                           GATT_VCS_CLIENT_VOLUME_STATE_SET_NTF_CFM);
        }
        else if (write_cfm->handle == vcs_client->handles.volumeFlagsCccHandle)
        {
            vcsClientSendVcsClientWriteCfm(vcs_client,
                                           write_cfm->resultCode,
                                           GATT_VCS_CLIENT_VOLUME_FLAG_SET_NTF_CFM);
        }
        else if (write_cfm->handle == vcs_client->handles.volumeControlPointHandle)
        {
            GattVcsClientMessageId id = setMessageIdWriteCfm(vcs_client->pending_cmd);


            if (id < GATT_VCS_CLIENT_MESSAGE_TOP)
            {
                vcsClientSendVcsClientWriteCfm(vcs_client,
                                               write_cfm->resultCode,
                                               id);
            }
        }

        vcs_client->pending_cmd = vcs_client_pending_none;
    }
    else
    {
        GATT_VCS_CLIENT_PANIC("Null instance\n");
    }
}

/****************************************************************************/
static void handleVcsClientNotification(GVCSC *vcs_client, const CsrBtGattClientNotificationInd *ind)
{
    if (ind->valueHandle == vcs_client->handles.volumeStateHandle)
    {
        if(ind->valueLength == VCS_CLIENT_VOLUME_STATE_VALUE_SIZE)
        {
            MAKE_VCS_CLIENT_MESSAGE(GattVcsClientVolumeStateInd)

            message->srvcHndl = vcs_client->srvcElem->service_handle;
            message->volumeState = ind->value[0];
            message->mute = ind->value[1];
            message->changeCounter = ind->value[2];

            /* Send the confirmation message to app task  */
            VcsMessageSend(vcs_client->app_task, GATT_VCS_CLIENT_VOLUME_STATE_IND, message);
        }
    }
    else if (ind->valueHandle == vcs_client->handles.volumeFlagsHandle)
    {
        if(ind->valueLength == VCS_CLIENT_VOLUME_FLAG_VALUE_SIZE)
        {
            MAKE_VCS_CLIENT_MESSAGE(GattVcsClientVolumeFlagInd)

            message->srvcHndl = vcs_client->srvcElem->service_handle;
            message->volumeFlag = ind->value[0];

            VcsMessageSend(vcs_client->app_task, GATT_VCS_CLIENT_VOLUME_FLAG_IND, message);
        }
    }

}

/****************************************************************************/
static void handleReadValueResp(GVCSC *vcs_client, const CsrBtGattReadCfm *read_cfm)
{
    switch (vcs_client->pending_cmd)
    {
        case vcs_client_read_pending:
        {
            if (read_cfm->handle == vcs_client->handles.volumeStateHandle)
            {
                /* Send read GATT_VCS_CLIENT_READ_VOLUME_STATE_CFM message to application */
                vcsSendReadVolumeStateCfm(vcs_client,
                                          read_cfm->resultCode,
                                          read_cfm->value);
            }
            else if (read_cfm->handle == vcs_client->handles.volumeFlagsHandle)
            {
                /* Send read GATT_VCS_CLIENT_READ_VOLUME_FLAG_CFM message to application */
                vcsSendReadVolumeFlagCfm(vcs_client,
                                         read_cfm->resultCode,
                                         read_cfm->valueLength,
                                         read_cfm->value);
            }
        }
        break;

        case vcs_client_read_pending_ccc:
        {
            if (read_cfm->handle == vcs_client->handles.volumeStateCccHandle)
            {
                /* Send read GATT_VCS_CLIENT_READ_VOLUME_STATE_CCC_CFM message to application */
                vcsSendReadCccCfm(vcs_client,
                                  read_cfm->resultCode,
                                  read_cfm->valueLength,
                                  read_cfm->value,
                                  GATT_VCS_CLIENT_READ_VOLUME_STATE_CCC_CFM);
            }
            else if (read_cfm->handle == vcs_client->handles.volumeFlagsCccHandle)
            {
                /* Send read GATT_VCS_CLIENT_READ_VOLUME_FLAG_CCC_CFM message to application */
                vcsSendReadCccCfm(vcs_client,
                                  read_cfm->resultCode,
                                  read_cfm->valueLength,
                                  read_cfm->value,
                                  GATT_VCS_CLIENT_READ_VOLUME_FLAG_CCC_CFM);
            }

        }
        break;

        default:
        {
            /* No other pending read values expected */
            GATT_VCS_CLIENT_ERROR("GVCSC: Read value response not expected [0x%x]\n",
                                    vcs_client->pending_cmd);
        }
        break;
    }

    vcs_client->pending_cmd = vcs_client_pending_none;
}

/****************************************************************************/
static void handleGattMsg(void *task, MsgId id, Msg msg)
{
    GVCSC *gatt_client = (GVCSC *)task;
    
    switch (id)
    {
        case CSR_BT_GATT_DISCOVER_CHARAC_CFM:
        {
            handleDiscoverAllVcsCharacteristicsResp(gatt_client,
                                                     (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *)msg);
        }
        break;

        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM:
        {
            handleDiscoverAllVcsCharacteristicDescriptorsResp(gatt_client,
                                                               (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *)msg);
        }
        break;

        case CSR_BT_GATT_WRITE_CFM:
        {
            /* Write/Notification Confirmation */
            vcsClientHandleWriteValueRespCfm(gatt_client,
                                             (const CsrBtGattWriteCfm*) msg);
        }
        break;

        case CSR_BT_GATT_READ_CFM:
        {
            handleReadValueResp(gatt_client,
                                (const CsrBtGattReadCfm *) msg);
        }
        break;

        case CSR_BT_GATT_CLIENT_NOTIFICATION_IND:
        {
            handleVcsClientNotification(gatt_client,
                                        (const CsrBtGattClientNotificationInd *) msg);
        }
        break;

        default:
        {
            /* Unrecognised GATT Manager message */
            GATT_VCS_CLIENT_WARNING("GVCSC: VCS Client GattMgr Msg not handled [0x%x]\n", id);
        }
        break;
    }
}

/****************************************************************************/
static void setFlagWritePending(GVCSC * vcs_client, vcs_client_control_point_opcodes_t opcode)
{
    switch(opcode)
    {
        case vcs_client_relative_volume_down_op:
            vcs_client->pending_cmd = vcs_client_write_rel_vol_down_pending;
        break;

        case vcs_client_relative_volume_up_op:
            vcs_client->pending_cmd = vcs_client_write_rel_vol_up_pending;
        break;

        case vcs_client_unmute_relative_volume_down_op:
            vcs_client->pending_cmd = vcs_client_write_unmute_rel_vol_down_pending;
        break;

        case vcs_client_unmute_relative_volume_up_op:
            vcs_client->pending_cmd = vcs_client_write_unmute_rel_vol_up_pending;
        break;

        case vcs_client_set_absolute_volume_op:
            vcs_client->pending_cmd = vcs_client_write_absolute_vol_pending;
        break;

        case vcs_client_unmute_op:
            vcs_client->pending_cmd = vcs_client_write_unmute_pending;
        break;

        case vcs_client_mute_op:
            vcs_client->pending_cmd = vcs_client_write_mute_pending;
        break;

        default:
        break;
    }
}

/***************************************************************************/
static void  handleVcsInternalMessage(void *task, MsgId id, Msg msg)
{
    GVCSC * vcs_client = (GVCSC *)task;

    GATT_VCS_CLIENT_INFO("Message id (%d)\n",id);

    if (vcs_client)
    {
        switch(id)
        {
            case VCS_CLIENT_INTERNAL_MSG_READ_CCC:
            {
                uint16 handle = ((const VCS_CLIENT_INTERNAL_MSG_READ_CCC_T*)msg)->handle;

                vcsClientHandleInternalRead(vcs_client,
                                            handle);

                vcs_client->pending_cmd = vcs_client_read_pending_ccc;
            }
            break;

            case VCS_CLIENT_INTERNAL_MSG_READ:
            {
                uint16 handle = ((const VCS_CLIENT_INTERNAL_MSG_READ_T*)msg)->handle;

                vcsClientHandleInternalRead(vcs_client,
                                            handle);

                vcs_client->pending_cmd = vcs_client_read_pending;
            }
            break;

            case VCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ:
            {
                VCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ_T* message = (VCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ_T*) msg;

                vcsClientHandleInternalRegisterForNotification(vcs_client,
                                                               message->enable,
                                                               message->handle);

                vcs_client->pending_cmd = vcs_client_write_notification_pending;
            }
            break;

            case VCS_CLIENT_INTERNAL_MSG_WRITE:
            {
                VCS_CLIENT_INTERNAL_MSG_WRITE_T* message = (VCS_CLIENT_INTERNAL_MSG_WRITE_T*) msg;

                vcsClientHandleInternalWrite(vcs_client,
                                             message->handle,
                                             message->size_value,
                                             message->value);

                setFlagWritePending(vcs_client, (vcs_client_control_point_opcodes_t) message->value[0]);

            }
            break;

            default:
            {
                /* Internal unrecognised messages */
                GATT_VCS_CLIENT_WARNING("Unknown Message received from Internal To lib \n");
            }
            break;
        }
    }
}

/****************************************************************************/
void gattVcsClientMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *message = NULL;
    gatt_vcs_client *inst = *((gatt_vcs_client **)gash);

    if (CsrSchedMessageGet(&eventClass, &message))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                CsrBtGattPrim *id = message;
                GVCSC *vcs_client = (GVCSC *) GetServiceClientByGattMsg(&inst->service_handle_list, message);
                void *msg = GetGattManagerMsgFromGattMsg(message, id);

                if(vcs_client)
                {
                    handleGattMsg(vcs_client, *id, msg);
                }

                if(msg!=message)
                {
                    CsrPmemFree(msg);
                }

                CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, message);
            }
            break;

            case VCS_CLIENT_PRIM:
            {
                vcs_client_internal_msg_t *id = (vcs_client_internal_msg_t *)message;
                GVCSC *vcs_client = (GVCSC *) GetServiceClientByServiceHandle(message);

                handleVcsInternalMessage(vcs_client, *id, message);
            }
            break;

            default:
                GATT_VCS_CLIENT_WARNING("GVCSC: Client Msg not handled [0x%x]\n", eventClass);
            break;
        }

        SynergyMessageFree(eventClass, message);
    }
}


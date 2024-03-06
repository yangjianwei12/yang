/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#include "gatt_mics_client_private.h"
#include "gatt_mics_client_debug.h"
#include "gatt_mics_client.h"
#include "gatt_mics_client_discovery.h"
#include "gatt_mics_client_common.h"
#include "gatt_mics_client_read.h"
#include "gatt_mics_client_write.h"
#include "gatt_mics_client_notification.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "gatt_mics_client_init.h"
#include "gatt_mics_client_common_util.h"

#include "csr_bt_gatt_lib.h"

/***************************************************************************/
static void micsClientHandleWriteValueRespCfm(GMICSC *const mics_client, const CsrBtGattWriteCfm *const write_cfm)
{
    if (mics_client != NULL)
    {
        if (write_cfm->handle == mics_client->handles.muteCccHandle)
        {
            micsClientSendMicsClientWriteCfm(mics_client,
                                           write_cfm->resultCode,
                                           GATT_MICS_CLIENT_NTF_CFM);
        }
        else if (write_cfm->handle == mics_client->handles.muteHandle)
        {
            micsClientSendMicsClientWriteCfm(mics_client,
                                           write_cfm->resultCode,
                                           GATT_MICS_CLIENT_SET_MUTE_VALUE_CFM);
        }

        mics_client->pending_cmd = mics_client_pending_none;
    }
    else
    {
        GATT_MICS_CLIENT_PANIC("Null instance\n");
    }
}

/****************************************************************************/
static void handleMicsClientNotification(GMICSC *mics_client, const CsrBtGattClientNotificationInd *ind)
{
    if (ind->valueHandle == mics_client->handles.muteHandle)
    {
        if(ind->valueLength == MICS_CLIENT_MUTE_VALUE_SIZE)
        {
            MAKE_MICS_CLIENT_MESSAGE(GattMicsClientMuteValueInd)

            message->srvcHndl = mics_client->srvcElem->service_handle;
            message->muteValue = ind->value[0];

            /* Send the confirmation message to app task  */
            MicsMessageSend(mics_client->app_task, GATT_MICS_CLIENT_MUTE_VALUE_IND, message);
        }
    }
}

/****************************************************************************/
static void handleReadValueResp(GMICSC *mics_client, const CsrBtGattReadCfm *read_cfm)
{
    switch (mics_client->pending_cmd)
    {
        case mics_client_read_pending:
        {
            if (read_cfm->handle == mics_client->handles.muteHandle)
            {
                /* Send read GATT_MICS_CLIENT_READ_MUTE_VALUE_CFM message to application */
                micsSendReadMuteValueCfm(mics_client,
                                          read_cfm->resultCode,
                                          read_cfm->value);
            }
        }
        break;

        case mics_client_read_pending_ccc:
        {
            if (read_cfm->handle == mics_client->handles.muteCccHandle)
            {
                /* Send read GATT_MICS_CLIENT_READ_MUTE_VALUE_CCC_CFM message to application */
                micsSendReadCccCfm(mics_client,
                                  read_cfm->resultCode,
                                  read_cfm->valueLength,
                                  read_cfm->value,
                                  GATT_MICS_CLIENT_READ_MUTE_VALUE_CCC_CFM);
            }
        }
        break;

        default:
        {
            /* No other pending read values expected */
            GATT_MICS_CLIENT_ERROR("GMICSC: Read value response not expected [0x%x]\n",
                                    mics_client->pending_cmd);
        }
        break;
    }

    mics_client->pending_cmd = mics_client_pending_none;
}

/****************************************************************************/
static void handleGattMsg(void *task, MsgId id, Msg msg)
{
    GMICSC *gatt_client = (GMICSC *)task;
    
    switch (id)
    {
        case CSR_BT_GATT_DISCOVER_CHARAC_CFM:
        {
            handleDiscoverAllMicsCharacteristicsResp(gatt_client,
                                                     (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *)msg);
        }
        break;

        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM:
        {
            handleDiscoverAllMicsCharacteristicDescriptorsResp(gatt_client,
                                                               (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *)msg);
        }
        break;

        case CSR_BT_GATT_WRITE_CFM:
        {
            /* Write/Notification Confirmation */
            micsClientHandleWriteValueRespCfm(gatt_client,
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
            handleMicsClientNotification(gatt_client,
                                        (const CsrBtGattClientNotificationInd *) msg);
        }
        break;

        default:
        {
            /* Unrecognised GATT Manager message */
            GATT_MICS_CLIENT_WARNING("GMICSC: MICS Client GattMgr Msg not handled [0x%x]\n", id);
        }
        break;
    }
}

/****************************************************************************/
static void setFlagWritePending(GMICSC * mics_client, mics_client_control_point_opcodes_t opcode)
{
    switch(opcode)
    {
        case mics_client_set_mute_value_op:
            mics_client->pending_cmd = mics_client_set_mute_value_pending;
        break;

        default:
        break;
    }
}

/***************************************************************************/
static void  handleMicsInternalMessage(void *task, MsgId id, Msg msg)
{
    GMICSC * mics_client = (GMICSC *)task;

    GATT_MICS_CLIENT_INFO("Message id (%d)\n",id);

    if (mics_client)
    {
        switch(id)
        {
            case MICS_CLIENT_INTERNAL_MSG_READ_CCC:
            {
                uint16 handle = ((const MICS_CLIENT_INTERNAL_MSG_READ_CCC_T*)msg)->handle;

                micsClientHandleInternalRead(mics_client,
                                            handle);

                mics_client->pending_cmd = mics_client_read_pending_ccc;
            }
            break;

            case MICS_CLIENT_INTERNAL_MSG_READ:
            {
                uint16 handle = ((const MICS_CLIENT_INTERNAL_MSG_READ_T*)msg)->handle;

                micsClientHandleInternalRead(mics_client,
                                            handle);

                mics_client->pending_cmd = mics_client_read_pending;
            }
            break;

            case MICS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ:
            {
                MICS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ_T* message = (MICS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ_T*) msg;

                micsClientHandleInternalRegisterForNotification(mics_client,
                                                               message->enable,
                                                               message->handle);

                mics_client->pending_cmd = mics_client_write_notification_pending;
            }
            break;

            case MICS_CLIENT_INTERNAL_MSG_WRITE:
            {
                MICS_CLIENT_INTERNAL_MSG_WRITE_T* message = (MICS_CLIENT_INTERNAL_MSG_WRITE_T*) msg;

                micsClientHandleInternalWrite(mics_client,
                                             message->handle,
                                             message->size_value,
                                             message->value);

                setFlagWritePending(mics_client, (mics_client_control_point_opcodes_t) message->value[0]);

            }
            break;

            default:
            {
                /* Internal unrecognised messages */
                GATT_MICS_CLIENT_WARNING("Unknown Message received from Internal To lib \n");
            }
            break;
        }
    }
}

/****************************************************************************/
void gattMicsClientMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *message = NULL;
    gatt_mics_client *inst = *((gatt_mics_client **)gash);

    if (CsrSchedMessageGet(&eventClass, &message))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                CsrBtGattPrim *id = message;
                GMICSC *mics_client = (GMICSC *) GetServiceClientByGattMsg(&inst->service_handle_list, message);
                void *msg = GetGattManagerMsgFromGattMsg(message, id);

                if(mics_client)
                {
                    handleGattMsg(mics_client, *id, msg);
                }

                if(msg!=message)
                {
                    CsrPmemFree(msg);
                }

                CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, message);
            }
            break;

            case MICS_CLIENT_PRIM:
            {
                mics_client_internal_msg_t *id = (mics_client_internal_msg_t *)message;
                GMICSC *mics_client = (GMICSC *) GetServiceClientByServiceHandle(message);

                handleMicsInternalMessage(mics_client, *id, message);
            }
            break;

            default:
                GATT_MICS_CLIENT_WARNING("GMICSC: Client Msg not handled [0x%x]\n", eventClass);
            break;
        }

        SynergyMessageFree(eventClass, message);
    }
}


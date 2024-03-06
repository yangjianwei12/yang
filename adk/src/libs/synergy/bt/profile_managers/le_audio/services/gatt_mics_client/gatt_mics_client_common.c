/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#include "gatt_mics_client_private.h"
#include "gatt_mics_client_common.h"

#include "csr_bt_gatt_lib.h"

#define GATT_MICS_CLIENT_OPCODE_NOT_SUPPORTED_ERR    (0x0081)

/*******************************************************************************/
void micsClientSendMicsClientWriteCfm(GMICSC *const mics_client,
                                    const status_t status,
                                    GattMicsClientMessageId id)
{
    if ((id == GATT_MICS_CLIENT_NTF_CFM) || 
        (id == GATT_MICS_CLIENT_SET_MUTE_VALUE_CFM))
    {
        MAKE_MICS_CLIENT_MESSAGE(GattMicsClientNtfCfm);

        /* Fill in client reference */
        message->srvcHndl = mics_client->srvcElem->service_handle;

        /* Fill in the status */
        message->status = status;

        /* Send the confirmation message to app task  */
        MicsMessageSend(mics_client->app_task, id, message);
    }
}

/*******************************************************************************/
void micsSendReadCccCfm(GMICSC *mics_client,
                       status_t status,
                       uint16 size_value,
                       const uint8 *value,
                       GattMicsClientMessageId id)
{
    MAKE_MICS_CLIENT_MESSAGE_WITH_LEN(GattMicsClientReadMuteValueCccCfm, size_value);

    message->srvcHndl = mics_client->srvcElem->service_handle;
    message->status = status;
    message->sizeValue = size_value;

    CsrMemMove(message->value, value, size_value);

    MicsMessageSend(mics_client->app_task, id, message);
}

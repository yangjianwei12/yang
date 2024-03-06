/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_aics_client.h"
#include "gatt_aics_client_private.h"
#include "gatt_aics_client_common.h"

#include <gatt_manager.h>
#include <gatt.h>

/*******************************************************************************/
void aicsClientSendAicsClientWriteCfm(GAICS *const aics_client,
                                      const gatt_status_t status,
                                      GattAicsClientMessageId id)
{
    /* We will use GATT_AICS_CLIENT_INPUT_STATE_SET_NTF_CFM to create the message
     * because the structure of all the write confirmations is the same,
     * but we will send the right message using the id parameter */
    MAKE_AICS_CLIENT_MESSAGE(GattAicsClientInputStateSetNtfCfm);

    /* Fill in client reference */
    message->srvcHndl = aics_client->srvc_hndl;

    /* Fill in the status */
    message->status = status;

    /* Send the confirmation message to app task  */
    MessageSend(aics_client->app_task, id, message);
}

/*******************************************************************************/
void aicsSendReadCccCfm(GAICS *vcs_client,
                        gatt_status_t status,
                        uint16 size_value,
                        const uint8 *value,
                        GattAicsClientMessageId id)
{
    /* We will use GATT_AICS_CLIENT_READ_INPUT_STATE_CCC_CFM to create the message
     * because the structure of all the CCC read confirmations is the same,
     * but we will send the right message using the id parameter */
    MAKE_AICS_CLIENT_MESSAGE_WITH_LEN(GattAicsClientReadInputStateCccCfm,
                                      size_value);

    memset(message, 0, sizeof(GattAicsClientReadInputStateCccCfm) + ((size_value - 1) * sizeof(uint8)));

    message->srvcHndl = vcs_client->srvc_hndl;
    message->status = status;
    message->sizeValue = size_value;

    memmove(message->value, value, size_value);

    MessageSend(vcs_client->app_task, id, message);
}


/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */


#include "gatt_vcs_client_private.h"
#include "gatt_vcs_client_common.h"

#include <gatt_manager.h>
#include <gatt.h>

#define GATT_VCS_CLIENT_OPCODE_NOT_SUPPORTED_ERR    (0x0081)

/*******************************************************************************/
void vcsClientSendVcsClientWriteCfm(GVCSC *const vcs_client,
                                    const gatt_status_t status,
                                    GattVcsClientMessageId id)
{
    if (id != GATT_VCS_CLIENT_MESSAGE_TOP)
    {
        /* We will use GATT_VCS_CLIENT_VOLUME_STATE_SET_NTF_CFM to create the message
         * because the structute of all the write confermations is the same,
         * but we will send the right message using the id parameter */
        MAKE_VCS_CLIENT_MESSAGE(GattVcsClientVolumeStateSetNtfCfm);

        /* Fill in client reference */
        message->srvcHndl = vcs_client->srvc_hndl;

        /* Fill in the status */
        message->status = status;

        /* Send the confirmation message to app task  */
        MessageSend(vcs_client->app_task, id, message);
    }
}

/*******************************************************************************/
void vcsSendReadCccCfm(GVCSC *vcs_client,
                       gatt_status_t status,
                       uint16 size_value,
                       const uint8 *value,
                       GattVcsClientMessageId id)
{
    /* We will use GATT_VCS_CLIENT_READ_VOLUME_STATE_CCC_CFM to create the message
     * because the structure of all the CCC read confirmations is the same,
     * but we will send the right message using the id parameter */
    MAKE_VCS_CLIENT_MESSAGE_WITH_LEN(GattVcsClientReadVolumeStateCccCfm, size_value);

    message->srvcHndl = vcs_client->srvc_hndl;
    message->status = status;
    message->sizeValue = size_value;

    memmove(message->value, value, size_value);

    MessageSend(vcs_client->app_task, id, message);
}

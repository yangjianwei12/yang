/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt.h>
#include <gatt_manager.h>

#include "gatt_vcs_client.h"
#include "gatt_vcs_client_debug.h"
#include "gatt_vcs_client_private.h"
#include "gatt_vcs_client_common.h"
#include "gatt_vcs_client_read.h"

/***************************************************************************/
void vcsClientHandleInternalRead(const GVCSC * vcs_client, uint16 handle)
{
    GattManagerReadCharacteristicValue((Task)&vcs_client->lib_task, handle);
}

/***************************************************************************/
static void vcsClientHandleReadRequest(const GVCSC *gatt_vcs_client,
                                       uint16 handle,
                                       bool isReadCcc)
{
    /* Check parameters */
    if (gatt_vcs_client == NULL)
    {
        GATT_VCS_CLIENT_PANIC(("GVCSC: Invalid parameters\n"));
    }
    else
    {
        if (isReadCcc)
        {
            MAKE_VCS_CLIENT_INTERNAL_MESSAGE(VCS_CLIENT_INTERNAL_MSG_READ_CCC);

            message->handle = handle;

            MessageSendConditionally((Task)&gatt_vcs_client->lib_task,
                                     VCS_CLIENT_INTERNAL_MSG_READ_CCC,
                                     message,
                                     &gatt_vcs_client->pending_cmd);
        }
        else
        {
            MAKE_VCS_CLIENT_INTERNAL_MESSAGE(VCS_CLIENT_INTERNAL_MSG_READ);

            message->handle = handle;

            MessageSendConditionally((Task)&gatt_vcs_client->lib_task,
                                     VCS_CLIENT_INTERNAL_MSG_READ,
                                     message,
                                     &gatt_vcs_client->pending_cmd);
        }
    }
}

/****************************************************************************/
void GattVcsClientReadVolumeStateCccRequest(ServiceHandle clntHndl)
{
    GVCSC *gatt_vcs_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_vcs_client)
    {
        vcsClientHandleReadRequest(gatt_vcs_client,
                                   gatt_vcs_client->handles.volumeStateCccHandle,
                                   TRUE);
    }
    else
    {
        GATT_VCS_CLIENT_DEBUG_PANIC(("Invalid VCS Client instance!\n"));
    }
}

/****************************************************************************/
void GattVcsClientReadVolumeFlagCCCRequest(ServiceHandle clntHndl)
{
    GVCSC *gatt_vcs_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_vcs_client && gatt_vcs_client->handles.volumeFlagsCccHandle)
    {
        vcsClientHandleReadRequest(gatt_vcs_client,
                                   gatt_vcs_client->handles.volumeFlagsCccHandle,
                                   TRUE);
    }
    else
    {
        if (!gatt_vcs_client)
        {
            GATT_VCS_CLIENT_DEBUG_PANIC(("Invalid VCS Client instance!\n"));
        }
        else
        {
            GATT_VCS_CLIENT_DEBUG_INFO(("Notify property for Volume Flag not supported by the remote device\n"));
        }
    }

}

/****************************************************************************/
void GattVcsClientReadVolumeStateRequest(ServiceHandle clntHndl)
{
    GVCSC *gatt_vcs_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_vcs_client)
    {
        vcsClientHandleReadRequest(gatt_vcs_client,
                                   gatt_vcs_client->handles.volumeStateHandle,
                                   FALSE);
    }
    else
    {
        GATT_VCS_CLIENT_DEBUG_PANIC(("Invalid VCS Client instance!\n"));
    }
}

/*******************************************************************************/
void GattVcsClientReadVolumeFlagRequest(ServiceHandle clntHndl)
{
    GVCSC *gatt_vcs_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_vcs_client)
    {
        vcsClientHandleReadRequest(gatt_vcs_client,
                                   gatt_vcs_client->handles.volumeFlagsHandle,
                                   FALSE);
    }
    else
    {
        GATT_VCS_CLIENT_DEBUG_PANIC(("Invalid VCS Client instance!\n"));
    }
}

/*******************************************************************************/
void vcsSendReadVolumeStateCfm(GVCSC *vcs_client,
                               gatt_status_t status,
                               const uint8 *value)
{
    MAKE_VCS_CLIENT_MESSAGE(GattVcsClientReadVolumeStateCfm);

    message->srvcHndl = vcs_client->srvc_hndl;
    message->status = status;

    if (value)
    {
        message->volumeSetting = value[0];
        message->mute = value[1];
        message->changeCounter = value[2];

    }
    else
    {
        message->volumeSetting = 0;
        message->mute = 0;
        message->changeCounter = 0;

    }

    MessageSend(vcs_client->app_task, GATT_VCS_CLIENT_READ_VOLUME_STATE_CFM, message);
}

/*******************************************************************************/
void vcsSendReadVolumeFlagCfm(GVCSC *vcs_client,
                              gatt_status_t status,
                              uint16 size_value,
                              const uint8 *value)
{
    MAKE_VCS_CLIENT_MESSAGE(GattVcsClientReadVolumeFlagCfm);

    message->srvcHndl = vcs_client->srvc_hndl;

    if (size_value != VCS_CLIENT_VOLUME_FLAG_VALUE_SIZE)
    {
        message->status = GATT_VCS_CLIENT_STATUS_FAILED;
        message->volumeFlag = 0;
    }
    else
    {
        message->status = status;
        message->volumeFlag = value[0];
    }

    MessageSend(vcs_client->app_task, GATT_VCS_CLIENT_READ_VOLUME_FLAG_CFM, message);
}

/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_vcs_client.h"
#include "gatt_vcs_client_debug.h"
#include "gatt_vcs_client_private.h"
#include "gatt_vcs_client_common.h"
#include "gatt_vcs_client_read.h"


/***************************************************************************/
void vcsClientHandleInternalRead(const GVCSC * vcs_client, uint16 handle)
{
    CsrBtGattReadReqSend(vcs_client->srvcElem->gattId,
                         vcs_client->srvcElem->cid,
                         handle,
                         0);
}

/***************************************************************************/
static void vcsClientHandleReadRequest(const GVCSC *client,
                                       uint16 handle,
                                       bool isReadCcc)
{
    /* Check parameters */
    if (client == NULL)
    {
#ifdef adk_port
        GATT_VCS_CLIENT_PANIC(("GVCSC: Invalid parameters\n"));
#endif
    }
    else
    {
        if (isReadCcc)
        {
            MAKE_VCS_CLIENT_INTERNAL_MESSAGE(VCS_CLIENT_INTERNAL_MSG_READ_CCC);

            message->srvc_hndl = client->srvcElem->service_handle;
            message->handle = handle;

            VcsMessageSendConditionally(client->lib_task,
                                        VCS_CLIENT_INTERNAL_MSG_READ_CCC,
                                        message,
                                        &client->pending_cmd);
        }
        else
        {
            MAKE_VCS_CLIENT_INTERNAL_MESSAGE(VCS_CLIENT_INTERNAL_MSG_READ);

            message->srvc_hndl = client->srvcElem->service_handle;
            message->handle = handle;

            VcsMessageSendConditionally(client->lib_task,
                                        VCS_CLIENT_INTERNAL_MSG_READ,
                                        message,
                                        &client->pending_cmd);
        }
    }
}

/****************************************************************************/
void GattVcsClientReadVolumeStateCccRequest(ServiceHandle clntHndl)
{
    GVCSC *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        vcsClientHandleReadRequest(client,
                                   client->handles.volumeStateCccHandle,
                                   TRUE);
    }
    else
    {
        GATT_VCS_CLIENT_PANIC("Invalid VCS Client instance!\n");
    }
}

/****************************************************************************/
void GattVcsClientReadVolumeFlagCCCRequest(ServiceHandle clntHndl)
{
    GVCSC *client = ServiceHandleGetInstanceData(clntHndl);

    if (client && client->handles.volumeFlagsCccHandle)
    {
        vcsClientHandleReadRequest(client,
                                   client->handles.volumeFlagsCccHandle,
                                   TRUE);
    }
    else
    {
        if (!client)
        {
            GATT_VCS_CLIENT_PANIC("Invalid VCS Client instance!\n");
        }
        else
        {
            GATT_VCS_CLIENT_ERROR("Notify property for Volume Flag not supported by the remote device\n");
        }
    }

}

/****************************************************************************/
void GattVcsClientReadVolumeStateRequest(ServiceHandle clntHndl)
{
    GVCSC *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        vcsClientHandleReadRequest(client,
                                   client->handles.volumeStateHandle,
                                   FALSE);
    }
    else
    {
        GATT_VCS_CLIENT_PANIC("Invalid VCS Client instance!\n");
    }
}

/*******************************************************************************/
void GattVcsClientReadVolumeFlagRequest(ServiceHandle clntHndl)
{
    GVCSC *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        vcsClientHandleReadRequest(client,
                                   client->handles.volumeFlagsHandle,
                                   FALSE);
    }
    else
    {
        GATT_VCS_CLIENT_PANIC("Invalid VCS Client instance!\n");
    }
}

/*******************************************************************************/
void vcsSendReadVolumeStateCfm(GVCSC *vcs_client,
                               status_t status,
                               const uint8 *value)
{
    MAKE_VCS_CLIENT_MESSAGE(GattVcsClientReadVolumeStateCfm);

    message->svcHndl = vcs_client->srvcElem->service_handle;
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

    VcsMessageSend(vcs_client->app_task, GATT_VCS_CLIENT_READ_VOLUME_STATE_CFM, message);
}

/*******************************************************************************/
void vcsSendReadVolumeFlagCfm(GVCSC *vcs_client,
                              status_t status,
                              uint16 size_value,
                              const uint8 *value)
{
    MAKE_VCS_CLIENT_MESSAGE(GattVcsClientReadVolumeFlagCfm);

    message->srvcHndl = vcs_client->srvcElem->service_handle;

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

    VcsMessageSend(vcs_client->app_task, GATT_VCS_CLIENT_READ_VOLUME_FLAG_CFM, message);
}

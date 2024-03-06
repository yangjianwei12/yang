/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt_vcs_client.h>
#include <gatt_vocs_client.h>
#include <gatt_aics_client.h>

#include "vcp.h"
#include "vcp_debug.h"
#include "vcp_init.h"
#include "vcp_common.h"
#include "vcp_msg_handler.h"

/******************************************************************************/
void vcpSendInitCfm(VCP * vcp_inst, VcpStatus status)
{
    MAKE_VCP_MESSAGE(VcpInitCfm);

    message->status = status;

    if (!vcp_inst || status != VCP_STATUS_SUCCESS)
    {
        message->prflHndl = 0;
        message->vocsNum = 0;
        message->aicsNum = 0;
    }
    else
    {
        message->prflHndl = vcp_inst->vcp_srvc_hdl;
        message->vocsNum = vcp_inst->vocs_num;
        message->aicsNum = vcp_inst->aics_num;
    }

    MessageSend(vcp_inst->app_task, VCP_INIT_CFM, message);
}

/****************************************************************************/
static void vcpVcsInitReq(VCP *vcp_inst,
                          VcpInitData *client_init_params,
                          VcpHandles *device_data)
{
    GattVcsClientInitData vcs_init_data;
    GattVcsClientDeviceData vcs_device_data;

    vcs_init_data.cid = client_init_params->cid;
    vcs_init_data.startHandle = device_data->vcsHandle.startHandle;
    vcs_init_data.endHandle = device_data->vcsHandle.endHandle;

    vcs_device_data.volumeStateHandle = device_data->vcsHandle.volumeStateHandle;
    vcs_device_data.volumeStateCccHandle = device_data->vcsHandle.volumeStateCccHandle;
    vcs_device_data.volumeFlagsHandle = device_data->vcsHandle.volumeFlagsHandle;
    vcs_device_data.volumeFlagsCccHandle = device_data->vcsHandle.volumeFlagsCccHandle;
    vcs_device_data.volumeControlPointHandle = device_data->vcsHandle.volumeControlPointHandle;

    GattVcsClientInitReq(&vcp_inst->lib_task, &vcs_init_data, &vcs_device_data);
}

/***************************************************************************/
void VcpInitReq(Task appTask,
                VcpInitData *clientInitParams,
                VcpHandles *deviceData,
                bool includedServices)
{
    VCP *vcp_inst = NULL;
    VcpProfileHandle profile_hndl = 0;
    uint8 i;

    if (appTask == NULL)
    {
        VCP_PANIC(("Application Task NULL\n"));
    }

    profile_hndl = (VcpProfileHandle) ServiceHandleNewInstance((void **) &vcp_inst, sizeof(VCP));

    if (profile_hndl)
    {
        /* Reset all the service library memory */
        memset(vcp_inst, 0, sizeof(VCP));

        /* Set up library handler for external messages */
        vcp_inst->lib_task.handler = vcpMsgHandler;

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        vcp_inst->app_task = appTask;

        vcp_inst->cid = clientInitParams->cid;

        vcp_inst->vcp_srvc_hdl = profile_hndl;

        vcp_inst->secondary_service_req = includedServices;

        vcpSendInitCfm(vcp_inst, VCP_STATUS_IN_PROGRESS);

        if(deviceData)
        {
            /* It's a peer device: we already know handles, no need to do discovery */
            vcp_inst->is_peer_device = TRUE;

            vcp_inst->start_handle = deviceData->vcsHandle.startHandle;
            vcp_inst->end_handle = deviceData->vcsHandle.endHandle;

            vcp_inst->vocs_num = deviceData->vocsHandleLength;
            vcp_inst->aics_num = deviceData->aicsHandleLength;

            /* Check if the application requested the VCP Secondary service,
             * if so after the initialisation of the VCS Client
             * the  VOCS and/or AICS Clients must also be initialised.
             */
            if (deviceData->vocsHandle)
            {
                /* There are VOCS Client to initialise: we can't initialise them now, but we have to wait
                 * the confirmation of the VCS Client initialisation. We cache the data provided by the app
                 * in a dynamic list. Wre use a dynamic list instead of an array, even if we know the number of
                 * VOCS instances we have to inialise, because in the case in which the remote divice is not
                 * a peer device we have to discover its secondary services, we don't know how many of them we
                 * will find until we finish the discovery. In that case we need a dynamic list to save the
                 * discovery data. In order to not have too many data structure (array and list) with the same
                 * meaning, we chose to use always the list.
                 */
                vcp_inst->vocs_counter = deviceData->vocsHandleLength;

                for (i=0; i<deviceData->vocsHandleLength; i++)
                {
                    vcpAddVocsInst(vcp_inst,
                                   &(deviceData->vocsHandle[i].handles),
                                   (deviceData->vocsHandle[i].startHandle),
                                   (deviceData->vocsHandle[i].endHandle));
                }
            }

            if (deviceData->aicsHandle)
            {
                /* Same thing for the AICS instances*/
                vcp_inst->aics_counter = deviceData->aicsHandleLength;

                for (i=0; i<deviceData->aicsHandleLength; i++)
                {
                    vcpAddAicsInst(vcp_inst,
                                   &(deviceData->aicsHandle[i].handles),
                                   (deviceData->aicsHandle[i].startHandle),
                                   (deviceData->aicsHandle[i].endHandle));
                }
            }

            /* We can start now the initialisation of all the necessary client:
             * we start with the VCS Client.
             */
            vcpVcsInitReq(vcp_inst, clientInitParams, deviceData);
        }
        else
        {
            gatt_uuid_t uuid = VCP_VCS_UUID;

            /* First time we connect to this device: we need to discovery the characteristic handles */
            GattDiscoverPrimaryServiceRequest(&vcp_inst->lib_task,
                                              clientInitParams->cid,
                                              gatt_uuid16,
                                              &uuid);
        }
    }
    else
    {
        VCP_DEBUG_PANIC(("Memory alllocation of VCP Profile instance failed!\n"));
        vcpSendInitCfm(vcp_inst, VCP_STATUS_FAILED);
    }
}

/****************************************************************************/
void vcpHandleAicsClientInitResp(VCP *vcp_inst,
                                 const GattAicsClientInitCfm * message)
{
    if(message->status == GATT_AICS_CLIENT_STATUS_SUCCESS)
    {
        /* The initialisation of a AICS client instance is done */
        vcp_inst->aics_counter -= 1;

        /* We have to save the service handle of the client instance in a list, because when
         * we finish all the VCP clients initialisation we have to send this list to the application.
         * The application need to know there are more instance in order to handle them in all the
         * VCP procedure it wants to perform.
         */
        vcpAddElementAicsSrvcHndlList(vcp_inst, message->srvcHndl);

        if (!vcp_inst->aics_counter)
        {
            /* There are no more other AICS instance to initialise:
             * - destroy the instance lists
             * - send VCP_PROFILE_INT_CFM
             * - destroy the service handles lists
             */
            VcpDestroyReqAllInstList(vcp_inst);

            vcpSendInitCfm(vcp_inst, VCP_STATUS_SUCCESS);
        }
    }
    else
    {
        /* One of the AICS Client Initialisation failed. */
        /* We have to destroy all the list we have. */
        VcpDestroyReqAllInstList(vcp_inst);
        vcpDestroyReqAllSrvcHndlList(vcp_inst);

        vcpSendInitCfm(vcp_inst, VCP_STATUS_FAILED);

        if(!ServiceHandleFreeInstanceData(vcp_inst->vcp_srvc_hdl))
        {
            VCP_PANIC(("Freeing of memory instance failed\n"));
        }
    }
}

/****************************************************************************/
void vcpHandleVocsClientInitResp(VCP *vcp_inst,
                                 const GattVocsClientInitCfm * message)
{
    if(message->status == GATT_VOCS_CLIENT_STATUS_SUCCESS)
    {
        /* The initialisation of a VOCS client instance is done */
        vcp_inst->vocs_counter -= 1;

        /* We have to save the service handle of the client instance in a list, because when
         * we finish all the VCP client initialisations, we have to send this list to the application.
         * The application need to know there are more instance in order to handle them in all the
         * VCP procedure it wants to perform.
         */
        vcpAddElementVocsSrvcHndlList(vcp_inst, message->srvcHdnl);

        if (!vcp_inst->vocs_counter)
        {
            /* There are no more other VOCS instance to initialise.
             * We can proceed to initialise the AICS instances.
             */
            if (vcp_inst->aics_counter)
            {
                vcp_aics_inst_t *ptr = vcp_inst->first_aics_inst;

                while(ptr)
                {
                    GattAicsClientInitData init_data;

                    init_data.cid = ptr->cid;
                    init_data.startHandle = ptr->start_handle;
                    init_data.endHandle = ptr->end_handle;

                    if(vcp_inst->is_peer_device)
                    {
                        /* We have a peer device: we already know its VCP handles*/
                        GattAicsClientInitReq(&vcp_inst->lib_task,
                                              &init_data,
                                              &(ptr->device_data));
                    }
                    else
                    {
                        /* We haven't a peer device: we don't know its VCP handles*/
                        GattAicsClientInitReq(&vcp_inst->lib_task,
                                              &init_data,
                                              NULL);
                    }

                    ptr = ptr->next;
                }
            }
            else
            {
                /* There sre no AICS instances to initialise.
                 * We can send the Initialisation Profile confirmation to the application.
                 */
                vcpSendInitCfm(vcp_inst, VCP_STATUS_SUCCESS);
            }
        }
    }
    else
    {
        /* One of the VOCS Client Initialisation failed */
        /* We have to destroy all the list we have */
        VcpDestroyReqAllInstList(vcp_inst);

        if (!vcp_inst->first_vocs_srvc_hndl && !vcp_inst->last_vocs_srvc_hndl)
        {
            /* There is a list of VOCS service handles: we need to destroy it */
            VcpDestroyReqVocsSrvcHndlList(vcp_inst);
        }

        vcpSendInitCfm(vcp_inst, VCP_STATUS_FAILED);

        if(!ServiceHandleFreeInstanceData(vcp_inst->vcp_srvc_hdl))
        {
            VCP_PANIC(("Freeing of memory instance failed\n"));
        }
    }
}

/****************************************************************************/
void vcpHandleVcsClientInitResp(VCP *vcp_inst,
                                const GattVcsClientInitCfm * message)
{
    if(message->status == GATT_VCS_CLIENT_STATUS_SUCCESS)
    {
        vcp_inst->vcs_srvc_hdl = message->srvcHndl;

        if (vcp_inst->secondary_service_req && !vcp_inst->is_peer_device)
        {
            GattFindIncludedServicesRequest(&vcp_inst->lib_task,
                                            vcp_inst->cid,
                                            vcp_inst->start_handle,
                                            vcp_inst->end_handle);
        }
        else if (!vcp_inst->secondary_service_req)
        {
            /* The applicantion didn't request the discovery of the secondary service:
             * we can send the confirmation of the initialisation
             */
            vcpSendInitCfm(vcp_inst, VCP_STATUS_SUCCESS);
        }
        else if (vcp_inst->is_peer_device)
        {
            /* It's a pair device and the application should have already provided
             * the info to initialise all the VOCS and/or AICS Clients. We can start
             * the initialisations.
             */
             vcpStartScndrSrvcInit(vcp_inst);
        }
    }
    else
    {
        /* The initialisation of VCS Client failed:
         * we need to destroy all the existed instance lists.*/
        VcpDestroyReqAllInstList(vcp_inst);

        vcpSendInitCfm(vcp_inst, VCP_STATUS_FAILED);

        if(!ServiceHandleFreeInstanceData(vcp_inst->vcp_srvc_hdl))
        {
            VCP_PANIC(("Freeing of memory instance failed\n"));
        }
    }
}

/***************************************************************************/
void vcpStartScndrSrvcInit(VCP *vcp_inst)
{
    if (vcp_inst->vocs_counter)
    {
        vcp_vocs_inst_t *ptr = vcp_inst->first_vocs_inst;

        while(ptr)
        {
           GattVocsClientInitData init_data;

           init_data.cid = ptr->cid;
           init_data.startHandle = ptr->start_handle;
           init_data.endHandle = ptr->end_handle;

           if (vcp_inst->is_peer_device)
           {
               GattVocsClientInitReq(&vcp_inst->lib_task,
                                     &init_data,
                                     &(ptr->device_data));
           }
           else
           {
               GattVocsClientInitReq(&vcp_inst->lib_task,
                                     &init_data,
                                     NULL);
           }

           ptr = ptr->next;
        }
    }
    else if(vcp_inst->aics_counter)
    {
        vcp_aics_inst_t *ptr = vcp_inst->first_aics_inst;

        while(ptr)
        {
            GattAicsClientInitData init_data;

            init_data.cid = ptr->cid;
            init_data.startHandle = ptr->start_handle;
            init_data.endHandle = ptr->end_handle;

            if (vcp_inst->is_peer_device)
            {
                GattAicsClientInitReq(&vcp_inst->lib_task,
                                      &init_data,
                                      &(ptr->device_data));
            }
            else
            {
                GattAicsClientInitReq(&vcp_inst->lib_task,
                                      &init_data,
                                      NULL);
            }

            ptr = ptr->next;
        }
    }
    else
    {
        /* No secondary services in the remote device */
        vcpSendInitCfm(vcp_inst, VCP_STATUS_SUCCESS);
    }
}

/***************************************************************************/
void VcpSetInitialVolReq(VcpProfileHandle profileHandle, uint8 initialVol)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        MAKE_VCP_INTERNAL_MESSAGE(VCP_INTERNAL_SET_INITIAL_VOL_OP);

        message->prfl_hndl = profileHandle;
        message->initial_vol = initialVol;

        MessageSendConditionally(&vcp_inst->lib_task,
                                 VCP_INTERNAL_SET_INITIAL_VOL_OP,
                                 message,
                                 &vcp_inst->pending_op);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid VCP profile instance\n"));
    }
}

/***************************************************************************/
void vcpSendSetInitialVolOpCfm(VCP *vcp_inst,
                               VcpStatus status)
{
    MAKE_VCP_MESSAGE(VcpSetInitialVolCfm);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->status = status;

    MessageSend(vcp_inst->app_task, VCP_SET_INITIAL_VOL_CFM, message);
}

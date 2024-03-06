/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <panic.h>

#include <gatt_vcs_client.h>

#include "vcp.h"
#include "vcp_private.h"
#include "vcp_destroy.h"
#include "vcp_common.h"
#include "vcp_debug.h"

void vcpSendDestroyCfm(VCP * vcp_inst, VcpStatus status)
{
    MAKE_VCP_MESSAGE(VcpDestroyCfm);

    message->status = status;
    message->prflHndl = vcp_inst->vcp_srvc_hdl;

    MessageSend(vcp_inst->app_task, VCP_DESTROY_CFM, message);
}

/******************************************************************************/
void VcpDestroyReq(VcpProfileHandle profileHandle)
{
    VCP * vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        /* Send confirmation message with status in progress */
        vcpSendDestroyCfm(vcp_inst, VCP_STATUS_IN_PROGRESS);

        /* Terminate VCS Client */
        GattVcsClientTerminateReq(vcp_inst->vcs_srvc_hdl);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile handle\n"));
    }
}

/******************************************************************************/
void vcpSendVcsTerminateCfm(VCP *vcp_inst,
                            VcpStatus status,
                            GattVcsClientDeviceData handles)
{
    MAKE_VCP_MESSAGE(VcpVcsTerminateCfm);

    message->status = status;
    message->prflHndl = vcp_inst->vcp_srvc_hdl;

    memcpy(&(message->vcsHandle), &handles, sizeof(GattVcsClientDeviceData));

    MessageSend(vcp_inst->app_task, VCP_VCS_TERMINATE_CFM, message);
}

/****************************************************************************/
void vcpHandleAicsClientTerminateResp(VCP *vcp_inst,
                                      const GattAicsClientTerminateCfm * message)
{
    if (message->status == GATT_AICS_CLIENT_STATUS_SUCCESS)
    {
        /* A AICS Client instance has been terminated successfully. */
        vcp_inst->aics_counter -= 1;

        /* Save the aics characteristic handles to send to the application */
        vcpAddAicsInst(vcp_inst,
                       &message->deviceData,
                       message->startHandle,
                       message->endHandle);

        if (!vcp_inst->aics_counter)
        {
            /* We can destroy all the list we have in the profile context */
            vcpDestroyProfileInst(vcp_inst);
        }
    }
    else
    {
        vcpSendDestroyCfm(vcp_inst, VCP_STATUS_FAILED);
    }
}

/****************************************************************************/
void vcpHandleVocsClientTerminateResp(VCP *vcp_inst,
                                      const GattVocsClientTerminateCfm * message)
{
    if (message->status == GATT_VOCS_CLIENT_STATUS_SUCCESS)
    {
        /* A VOCS Client instance has been terminated successfully. */
        vcp_inst->vocs_counter -= 1;

        /* Save the vocs characteristic handles to send to the application */
        vcpAddVocsInst(vcp_inst,
                       &(message->deviceData),
                       vcp_inst->start_handle,
                       vcp_inst->end_handle);

        if (!vcp_inst->vocs_counter)
        {
            /* All the VOCS Client instances are terminated */
            if (vcp_inst->aics_num)
            {
                /* There are AICS instances to terminate. */
                vcp_aics_srvc_hndl_t *ptr = vcp_inst->first_aics_srvc_hndl;
                vcp_inst->aics_counter = vcp_inst->aics_num;

                while(ptr)
                {
                    GattAicsClientTerminateReq(ptr->srvc_hdnl);
                    ptr = ptr->next;
                }
            }
            else
            {
                /* There are no instances of AICS to terminate */
                vcpDestroyProfileInst(vcp_inst);
            }
        }
    }
    else
    {
        vcpSendDestroyCfm(vcp_inst, VCP_STATUS_FAILED);
    }
}

/****************************************************************************/
void vcpHandleVcsClientTerminateResp(VCP *vcp_inst,
                                     const GattVcsClientTerminateCfm * message)
{
    if (message->status == GATT_VCS_CLIENT_STATUS_SUCCESS)
    {
        /* VCS Client has been terminated successfully. */

        /* Send the vcs characteristic handles to the application */
        vcpSendVcsTerminateCfm(vcp_inst,
                               VCP_STATUS_SUCCESS,
                               message->deviceData);

        if (vcp_inst->vocs_num)
        {
            /* If there are VOCS Client instances initialised, we have to terminate them */
            vcp_vocs_srvc_hndl_t *ptr = vcp_inst->first_vocs_srvc_hndl;
            vcp_inst->vocs_counter = vcp_inst->vocs_num;

            while(ptr)
            {
                GattVocsClientTerminateReq(ptr->srvc_hdnl);
                ptr = ptr->next;
            }
        }
        else if (vcp_inst->aics_num)
        {
            /* If there are no instances of VOCS, but there are AICS instances, we need to
             * terminate them too.
            */
            vcp_aics_srvc_hndl_t *ptr = vcp_inst->first_aics_srvc_hndl;
            vcp_inst->aics_counter = vcp_inst->aics_num;

            while(ptr)
            {
                GattAicsClientTerminateReq(ptr->srvc_hdnl);
                ptr = ptr->next;
            }
        }
        else
        {
            /* There are no instances of VOCS or AICS */
            vcpDestroyProfileInst(vcp_inst);
        }
    }
    else
    {
        vcpSendDestroyCfm(vcp_inst, VCP_STATUS_FAILED);
    }
}

/****************************************************************************/
void vcpSendIncludedServicesTerminateCfm(VCP *vcp_inst, VcpStatus status)
{
    vcp_vocs_inst_t *vocs_ptr = vcp_inst->first_vocs_inst;
    vcp_aics_inst_t *aics_ptr = vcp_inst->first_aics_inst;

    if (vcp_inst->vocs_num)
    {
        while(vocs_ptr)
        {
            MAKE_VCP_MESSAGE(VcpVocsTerminateCfm);

            message->status = status;
            message->prflHndl = vcp_inst->vcp_srvc_hdl;
            message->vocsSizeValue = vcp_inst->vocs_num;
            message->startHandle = vocs_ptr->start_handle;
            message->endHandle = vocs_ptr->end_handle;

            memcpy(&message->vocsValue, &vocs_ptr->device_data, sizeof(GattVocsClientDeviceData));

            vocs_ptr = vocs_ptr->next;

            if(vocs_ptr)
            {
                message->moreToCome = TRUE;
            }
            else
            {
                message->moreToCome = FALSE;
            }

            MessageSend(vcp_inst->app_task, VCP_VOCS_TERMINATE_CFM, message);
        }
    }
    else
    {
        MAKE_VCP_MESSAGE(VcpVocsTerminateCfm);

        message->status = status;
        message->prflHndl = vcp_inst->vcp_srvc_hdl;
        message->vocsSizeValue = 0;
        message->startHandle = 0;
        message->endHandle = 0;

        memset(&message->vocsValue, 0, sizeof(GattVocsClientDeviceData));

        message->moreToCome = FALSE;

        MessageSend(vcp_inst->app_task, VCP_VOCS_TERMINATE_CFM, message);
    }

    if (vcp_inst->aics_num)
    {
        while(aics_ptr)
        {
            MAKE_VCP_MESSAGE(VcpAicsTerminateCfm);

            message->status = status;
            message->prflHndl = vcp_inst->vcp_srvc_hdl;
            message->aicsSizeValue = vcp_inst->aics_num;
            message->startHandle = aics_ptr->start_handle;
            message->endHandle = aics_ptr->end_handle;

            memcpy(&message->aicsValue, &aics_ptr->device_data, sizeof(GattAicsClientDeviceData));

            aics_ptr = aics_ptr->next;

            if(aics_ptr)
            {
                message->moreToCome = TRUE;
            }
            else
            {
                message->moreToCome =FALSE;
            }

            MessageSend(vcp_inst->app_task, VCP_AICS_TERMINATE_CFM, message);
        }
    }
    else
    {
        MAKE_VCP_MESSAGE(VcpAicsTerminateCfm);

        message->status = status;
        message->prflHndl = vcp_inst->vcp_srvc_hdl;
        message->aicsSizeValue = 0;
        message->startHandle = 0;
        message->endHandle = 0;

        memset(&message->aicsValue, 0, sizeof(GattAicsClientDeviceData));

        message->moreToCome = FALSE;

        MessageSend(vcp_inst->app_task, VCP_AICS_TERMINATE_CFM, message);
    }
}

/****************************************************************************/
void vcpDestroyProfileInst(VCP *vcp_inst)
{
    bool res = FALSE;
    Task app_task = vcp_inst->app_task;

    /* Send the VOCS and AICS characteristic handles to the application */
    vcpSendIncludedServicesTerminateCfm(vcp_inst, VCP_STATUS_SUCCESS);

    /* We can destroy all the list we have in the profile context */
    vcpDestroyReqAllSrvcHndlList(vcp_inst);
    VcpDestroyReqAllInstList(vcp_inst);

    /* Clear pending messages */
    MessageFlushTask((Task)&vcp_inst->lib_task);

    /* Send the confirmation message */
    MAKE_VCP_MESSAGE(VcpDestroyCfm);
    message->prflHndl = vcp_inst->vcp_srvc_hdl;

    /* Free the profile instance memory */
    res = ServiceHandleFreeInstanceData((ServiceHandle) (vcp_inst->vcp_srvc_hdl));

    if (res)
    {
        message->status = VCP_STATUS_SUCCESS;
    }
    else
    {
        message->status = VCP_STATUS_FAILED;
    }

    MessageSend(app_task, VCP_DESTROY_CFM, message);
}

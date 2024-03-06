/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <stdlib.h>
#include <panic.h>

#include <gatt_vcs_client.h>
#include <gatt_aics_client.h>

#include "vcp.h"
#include "vcp_debug.h"
#include "vcp_private.h"
#include "vcp_notification.h"
#include "vcp_common.h"

/***************************************************************************/
static void vcpSendVcsSetNtfCfm(VCP *vcp_inst,
                                gatt_status_t status,
                                VcpMessageId id)
{
    /* We will use VCP_VOLUME_STATE_SET_NTF_CFM to create the message
     * because the structute of all the set notification confermations is the same,
     * but we will send the right message using the id parameter.
     */
    MAKE_VCP_MESSAGE(VcpVolumeStateSetNtfCfm);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->status = status;

    MessageSend(vcp_inst->app_task, id, message);
}

/***************************************************************************/
static void vcpSendInclSrvcSetNtfCfm(VCP *vcp_inst,
                                     ServiceHandle srvc_hdnl,
                                     gatt_status_t status,
                                     VcpMessageId id)
{
    /* We will use VcpOffsetStateSetNtfCfm to create the message
     * because the structute of all the set notification confermations is the same,
     * but we will send the right message using the id parameter.
     */
    MAKE_VCP_MESSAGE(VcpOffsetStateSetNtfCfm);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->status = status;
    message->srvcHndl = srvc_hdnl;

    MessageSend(vcp_inst->app_task, id, message);
}

/***************************************************************************/
void vcpHandleVcsVolumeStateSetNtfCfm(VCP *vcp_inst,
                                      const GattVcsClientVolumeStateSetNtfCfm *msg)
{
    vcpSendVcsSetNtfCfm(vcp_inst,
                        msg->status,
                        VCP_VOLUME_STATE_SET_NTF_CFM);
}

/***************************************************************************/
void vcpHandleVcsVolumeFlagSetNtfCfm(VCP *vcp_inst,
                                     const GattVcsClientVolumeFlagSetNtfCfm *msg)
{
    vcpSendVcsSetNtfCfm(vcp_inst, msg->status, VCP_VOLUME_FLAG_SET_NTF_CFM);
}

/***************************************************************************/
void vcpHandleVocsOffsetStateSetNtfCfm(VCP *vcp_inst,
                                       const GattVocsClientOffsetStateSetNtfCfm *msg)
{
    vcpSendInclSrvcSetNtfCfm(vcp_inst,
                             msg->srvcHndl,
                             msg->status,
                             VCP_OFFSET_STATE_SET_NTF_CFM);
}

/***************************************************************************/
void vcpHandleVocsAudioLocationSetNtfCfm(VCP *vcp_inst,
                                         const GattVocsClientAudioLocationSetNtfCfm *msg)
{
    vcpSendInclSrvcSetNtfCfm(vcp_inst,
                             msg->srvcHndl,
                             msg->status,
                             VCP_AUDIO_LOCATION_SET_NTF_CFM);
}

/***************************************************************************/
void vcpHandleVocsAudioOutputDescSetNtfCfm(VCP *vcp_inst,
                                           const GattVocsClientAudioOutputDescSetNtfCfm *msg)
{
    vcpSendInclSrvcSetNtfCfm(vcp_inst,
                             msg->srvcHndl,
                             msg->status,
                             VCP_AUDIO_OUTPUT_DESC_SET_NTF_CFM);
}

/****************************************************************************/
void VcpVolumeStateRegisterForNotificationReq(VcpProfileHandle profileHandle,
                                              bool notificationsEnable)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        GattVcsClientVolStateRegisterForNotificationReq(vcp_inst->vcs_srvc_hdl,
                                                        notificationsEnable);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/****************************************************************************/
void VcpVolumeFlagRegisterForNotificationReq(VcpProfileHandle profileHandle,
                                             bool notificationsEnable)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        GattVcsClientVolFlagRegisterForNotificationReq(vcp_inst->vcs_srvc_hdl,
                                                       notificationsEnable);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/****************************************************************************/
static void vcpVocsRegisterForNtf(VCP *vcp_inst,
                                  ServiceHandle vocs_clnt_hndl,
                                  bool notifications_enable,
                                  VcpMessageId id,
                                  void (* vocs_client_func)(ServiceHandle clnt_hndl, bool notifications_enable))
{
    if (vocs_clnt_hndl)
    {
        /* We have to apply the operation to a specific VOCS instance */
        /* Check if the client handle is a valid one */
        if (vcpIsValidVocsInst(vcp_inst, vocs_clnt_hndl))
        {
            (*vocs_client_func)(vocs_clnt_hndl, notifications_enable);
        }
        else
        {
            vcpSendInclSrvcSetNtfCfm(vcp_inst,
                                     vocs_clnt_hndl,
                                     gatt_status_failure,
                                     id);
        }
    }
    else
    {
        /* We have to apply the operation to all the VOCS instance */
        vcp_vocs_srvc_hndl_t *ptr = vcp_inst->first_vocs_srvc_hndl;

        while(ptr)
        {
            (*vocs_client_func)(ptr->srvc_hdnl, notifications_enable);
            ptr = ptr->next;
        }
    }
}

/****************************************************************************/
void VcpOffsetStateRegisterForNotificationReq(VcpProfileHandle profileHandle,
                                           ServiceHandle vocsClntHndl,
                                           bool notificationsEnable)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpVocsRegisterForNtf(vcp_inst,
                              vocsClntHndl,
                              notificationsEnable,
                              VCP_OFFSET_STATE_SET_NTF_CFM,
                              GattVocsClientOffsetStateRegisterForNotificationReq);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/****************************************************************************/
void VcpAudioLocationRegisterForNotificationReq(VcpProfileHandle profileHandle,
                                                ServiceHandle vocsClntHndl,
                                                bool notificationsEnable)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpVocsRegisterForNtf(vcp_inst,
                              vocsClntHndl,
                              notificationsEnable,
                              VCP_AUDIO_LOCATION_SET_NTF_CFM,
                              GattVocsClientAudioLocationRegisterForNotificationReq);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/****************************************************************************/
void VcpAudioOutputDescRegisterForNotificationReq(VcpProfileHandle profileHandle,
                                                  ServiceHandle vocsClntHndl,
                                                  bool notificationsEnable)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpVocsRegisterForNtf(vcp_inst,
                              vocsClntHndl,
                              notificationsEnable,
                              VCP_AUDIO_OUTPUT_DESC_SET_NTF_CFM,
                              GattVocsClientAudioOutputDescRegisterForNotificationReq);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/***************************************************************************/
void vcpHandleAicsInputStateSetNtfCfm(VCP *vcp_inst,
                                      const GattAicsClientInputStateSetNtfCfm *msg)
{
    vcpSendInclSrvcSetNtfCfm(vcp_inst,
                             msg->srvcHndl,
                             msg->status,
                             VCP_INPUT_STATE_SET_NTF_CFM);
}

/***************************************************************************/
static void vcpAicsRegisterForNtf(VCP *vcp_inst,
                                  ServiceHandle aics_clnt_hndl,
                                  bool notifications_enable,
                                  VcpMessageId id,
                                  void (* aics_client_func)(ServiceHandle clnt_hndl, bool notifications_enable))
{
    if (aics_clnt_hndl)
    {
        /* Check if the client handle is a valid one */
        if (vcpIsValidAicsInst(vcp_inst, aics_clnt_hndl))
        {
            (*aics_client_func)(aics_clnt_hndl,
                                notifications_enable);
        }
        else
        {
            vcpSendInclSrvcSetNtfCfm(vcp_inst,
                                     aics_clnt_hndl,
                                     gatt_status_failure,
                                     id);
        }
    }
    else
    {
        vcp_aics_srvc_hndl_t *ptr = vcp_inst->first_aics_srvc_hndl;

        while(ptr)
        {
            (*aics_client_func)(ptr->srvc_hdnl,
                                notifications_enable);
            ptr = ptr->next;
        }
    }
}

/***************************************************************************/
void VcpInputStateRegisterForNotificationReq(VcpProfileHandle profileHandle,
                                             ServiceHandle aicsClntHndl,
                                             bool notificationsEnable)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpAicsRegisterForNtf(vcp_inst,
                              aicsClntHndl,
                              notificationsEnable,
                              VCP_INPUT_STATE_SET_NTF_CFM,
                              GattAicsClientInputStateRegisterForNotificationReq);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/***************************************************************************/
void vcpHandleAicsInputStatusSetNtfCfm(VCP *vcp_inst,
                                       const GattAicsClientInputStatusSetNtfCfm *msg)
{
    vcpSendInclSrvcSetNtfCfm(vcp_inst,
                             msg->srvcHndl,
                             msg->status,
                             VCP_INPUT_STATUS_SET_NTF_CFM);
}

/***************************************************************************/
void VcpInputStatusRegisterForNotificationReq(VcpProfileHandle profileHandle,
                                              ServiceHandle aicsClntHndl,
                                              bool notificationsEnable)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpAicsRegisterForNtf(vcp_inst,
                              aicsClntHndl,
                              notificationsEnable,
                              VCP_INPUT_STATUS_SET_NTF_CFM,
                              GattAicsClientInputStatusRegisterForNotificationReq);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/***************************************************************************/
void vcpHandleAicsAudioInputDescSetNtfCfm(VCP *vcp_inst,
                                          const GattAicsClientAudioInputDescSetNtfCfm *msg)
{
    vcpSendInclSrvcSetNtfCfm(vcp_inst,
                             msg->srvcHndl,
                             msg->status,
                             VCP_AUDIO_INPUT_DESC_SET_NTF_CFM);
}

/***************************************************************************/
void VcpAudioInputDescRegisterForNotificationReq(VcpProfileHandle profileHandle,
                                                 ServiceHandle aicsClntHndl,
                                                 bool notificationsEnable)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpAicsRegisterForNtf(vcp_inst,
                              aicsClntHndl,
                              notificationsEnable,
                              VCP_AUDIO_INPUT_DESC_SET_NTF_CFM,
                              GattAicsClientAudioInputDescRegisterForNotificationReq);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

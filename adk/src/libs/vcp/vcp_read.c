/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt_vcs_client.h>
#include <gatt_vocs_client.h>
#include <gatt_aics_client.h>

#include "vcp.h"
#include "vcp_debug.h"
#include "vcp_private.h"
#include "vcp_common.h"
#include "vcp_read.h"
#include "vcp_write.h"
#include "vcp_init.h"

/***************************************************************************/
static void vcpVcsSendReadCccCfm(VCP *vcp_inst,
                                 gatt_status_t status,
                                 uint16 size_value,
                                 const uint8 *value,
                                 GattVcsClientMessageId id)
{
    /* We will use VCP_READ_VOLUME_STATE_CCC_CFM to create the message
     * because the structure of all the CCC read confirmations is the same,
     * but we will send the right message using the id parameter */
    MAKE_VCP_MESSAGE_WITH_LEN(VcpReadVolumeStateCccCfm, size_value);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->status = status;
    message->sizeValue = size_value;

    memmove(message->value, value, size_value);

    MessageSend(vcp_inst->app_task, id, message);
}

/***************************************************************************/
static void vcpIncldSrvcSendReadCccCfm(VCP *vcp_inst,
                                       ServiceHandle srvc_hndl,
                                       gatt_status_t status,
                                       uint16 size_value,
                                       const uint8 *value,
                                       GattVcsClientMessageId id)
{
    /* We will use VCP_READ_OFFSET_STATE_CCC_CFM to create the message
     * because the structure of all the CCC read confirmations is the same,
     * but we will send the right message using the id parameter */
    uint16 sizeValue = size_value ? size_value : 1;

    MAKE_VCP_MESSAGE_WITH_LEN(VcpReadOffsetStateCccCfm, sizeValue);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = srvc_hndl;
    message->status = status;
    message->sizeValue = size_value;

    if (message->sizeValue)
    {
        memmove(message->value, value, size_value);
    }
    else
    {
        message->value[0] = 0;
    }

    MessageSend(vcp_inst->app_task, id, message);
}

/***************************************************************************/
static void vcpSendReadVolumeStateCfm(VCP *vcp_inst,
                                      gatt_status_t status,
                                      uint8 volume_setting,
                                      uint8 mute,
                                      uint8 change_counter)
{
    MAKE_VCP_MESSAGE(VcpReadVolumeStateCfm);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->status = status;

    if (message->status == gatt_status_success)
    {
        message->volumeSetting = volume_setting;
        message->mute = mute;
        message->changeCounter = change_counter;
    }
    else
    {
        message->volumeSetting = 0;
        message->mute = 0;
        message->changeCounter = 0;
    }

    MessageSend(vcp_inst->app_task, VCP_READ_VOLUME_STATE_CFM, message);
}

/***************************************************************************/
static void vcpSendReadVolumeFlagCfm(VCP *vcp_inst,
                                     gatt_status_t status,
                                     const uint8 *value)
{
    MAKE_VCP_MESSAGE(VcpReadVolumeFlagCfm);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->status = status;

    if (value)
    {
        message->volumeFlag = value[0];
    }
    else
    {
        message->volumeFlag = 0;
    }

    MessageSend(vcp_inst->app_task, VCP_READ_VOLUME_FLAG_CFM, message);
}

/***************************************************************************/
void VcpReadVolumeFlagCccRequest(VcpProfileHandle profileHandle)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        GattVcsClientReadVolumeFlagCCCRequest(vcp_inst->vcs_srvc_hdl);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profileHandle\n"));
    }
}

/****************************************************************************/
void VcpReadVolumeStateCccRequest(VcpProfileHandle profileHandle)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        GattVcsClientReadVolumeStateCccRequest(vcp_inst->vcs_srvc_hdl);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/****************************************************************************/
void vcpHandleVcsReadVolumeStateCccCfm(VCP *vcp_inst,
                                       const GattVcsClientReadVolumeStateCccCfm *msg)
{
    vcpVcsSendReadCccCfm(vcp_inst,
                         msg->status,
                         msg->sizeValue,
                         msg->value,
                         VCP_READ_VOLUME_STATE_CCC_CFM);
}

/****************************************************************************/
void vcpHandleVcsReadVolumeFlagCccCfm(VCP *vcp_inst,
                                      const GattVcsClientReadVolumeFlagCccCfm *msg)
{
    vcpVcsSendReadCccCfm(vcp_inst,
                         msg->status,
                         msg->sizeValue,
                         msg->value,
                         VCP_READ_VOLUME_FLAG_CCC_CFM);
}

/****************************************************************************/
static vcp_vcs_control_point_opcodes_t vcpGetVcsOpcodeFromPendingCmd(uint16 pending_cmd)
{
    switch(pending_cmd)
    {
        case vcp_pending_relative_volume_down_op:
            return vcp_relative_volume_down_op;

        case vcp_pending_relative_volume_up_op:
            return vcp_relative_volume_up_op;

        case vcp_pending_unmute_relative_volume_down_op:
            return vcp_unmute_relative_volume_down_op;

        case vcp_pending_unmute_relative_volume_up_op:
            return vcp_unmute_relative_volume_up_op;

        case vcp_pending_set_absolute_volume_op:
            return vcp_set_absolute_volume_op;

        case vcp_pending_unmute_op:
            return vcp_unmute_op;

        case vcp_pending_mute_op:
            return vcp_mute_op;

        case vcp_pending_set_initial_vol_op:
            return vcp_set_absolute_volume_op;

        default:
            return vcp_vcs_last_op;
    }
}

/****************************************************************************/
void vcpHandleVcsReadVolumeStateCfm(VCP *vcp_inst,
                                    const GattVcsClientReadVolumeStateCfm *msg)
{
    if (msg->status == gatt_status_success)
    {
        /* Update the change counter */
        vcp_inst->vcs_change_counter = msg->changeCounter;
    }

    if (vcp_inst->pending_op == vcp_pending_op_none ||
        (vcp_inst->pending_op == vcp_pending_set_initial_vol_op &&
        !vcp_inst->volume_setting_pending))
    {
        /* No pending vcs control point operation */
        vcpSendReadVolumeStateCfm(vcp_inst,
                                  msg->status,
                                  msg->volumeSetting,
                                  msg->mute,
                                  msg->changeCounter);

        if (vcp_inst->pending_op == vcp_pending_set_initial_vol_op)
        {
            vcpSendSetInitialVolOpCfm(vcp_inst,VCP_STATUS_VOL_PERSISTED);
            vcp_inst->pending_op = vcp_pending_op_none;
        }
    }
    else if (vcp_inst->pending_op <= vcp_pending_mute_op ||
             vcp_inst->pending_op == vcp_pending_set_initial_vol_op)
    {
        /* If the reading has been performed after a vcs control point operation failed with
         * an invalid change counter error, we need to try again to execute that operation.
        */
        vcpVcsControlPointOp(vcp_inst->vcp_srvc_hdl,
                             vcpGetVcsOpcodeFromPendingCmd(vcp_inst->pending_op),
                             vcp_inst->volume_setting_pending);
    }
}

/****************************************************************************/
void vcpHandleVcsReadVolumeFlagCfm(VCP *vcp_inst,
                                   const GattVcsClientReadVolumeFlagCfm *msg)
{
    if (vcp_inst->pending_op == vcp_pending_set_initial_vol_op)
    {
        if (msg->status == gatt_status_success)
        {
            if (!msg->volumeFlag)
            {
                GattVcsClientAbsoluteVolRequest(vcp_inst->vcs_srvc_hdl,
                                              vcp_inst->vcs_change_counter,
                                              vcp_inst->volume_setting_pending);
            }
            else
            {
                vcp_inst->volume_setting_pending = 0;
                GattVcsClientReadVolumeStateRequest(vcp_inst->vcs_srvc_hdl);
            }
        }
        else
        {
            vcpSendSetInitialVolOpCfm(vcp_inst, VCP_STATUS_FAILED);
        }
    }
    else
    {
        vcpSendReadVolumeFlagCfm(vcp_inst,
                                 msg->status,
                                 &(msg->volumeFlag));
    }
}

/*******************************************************************************/
void VcpVolumeStateRequest(VcpProfileHandle profileHandle)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        GattVcsClientReadVolumeStateRequest(vcp_inst->vcs_srvc_hdl);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/*******************************************************************************/
void VcpReadVolumeFlagRequest(VcpProfileHandle profileHandle)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        GattVcsClientReadVolumeFlagRequest(vcp_inst->vcs_srvc_hdl);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/*******************************************************************************/
void vcpHandleVocsReadOffsetStateCccCfm(VCP *vcp_inst,
                                        const GattVocsClientReadOffsetStateCccCfm *msg)
{
    vcpIncldSrvcSendReadCccCfm(vcp_inst,
                               msg->srvcHdnl,
                               msg->status,
                               msg->sizeValue,
                               msg->value,
                               VCP_READ_OFFSET_STATE_CCC_CFM);
}

/*******************************************************************************/
void vcpHandleVocsReadAudioLocationCccCfm(VCP *vcp_inst,
                                          const GattVocsClientReadAudioLocationCccCfm *msg)
{
    vcpIncldSrvcSendReadCccCfm(vcp_inst,
                               msg->srvcHdnl,
                               msg->status,
                               msg->sizeValue,
                               msg->value,
                               VCP_READ_AUDIO_LOCATION_CCC_CFM);
}

/*******************************************************************************/
void vcpHandleVocsReadAudioOutputDescCccCfm(VCP *vcp_inst,
                                            const GattVocsClientReadAudioOutputDescCccCfm *msg)
{
    vcpIncldSrvcSendReadCccCfm(vcp_inst,
                               msg->srvcHdnl,
                               msg->status,
                               msg->sizeValue,
                               msg->value,
                               VCP_READ_AUDIO_OUTPUT_DESC_CCC_CFM);
}

/*******************************************************************************/
static void vcpVocsReadCCCReq(VCP *vcp_inst,
                              ServiceHandle vocs_clnt_hndl,
                              void (*vocs_client_func)(ServiceHandle clnt_hndl),
                              VcpMessageId id)
{
    if (vocs_clnt_hndl)
    {
        /* Check if the client handle is a valid one */
        if (vcpIsValidVocsInst(vcp_inst, vocs_clnt_hndl))
        {
            (*vocs_client_func)(vocs_clnt_hndl);
        }
        else
        {
            vcpIncldSrvcSendReadCccCfm(vcp_inst,
                                       vocs_clnt_hndl,
                                       gatt_status_failure,
                                       0,
                                       NULL,
                                       id);
        }
    }
    else
    {
        vcp_vocs_srvc_hndl_t *ptr = vcp_inst->first_vocs_srvc_hndl;

        while(ptr)
        {
            (*vocs_client_func)(ptr->srvc_hdnl);
            ptr = ptr->next;
        }
    }
}

/*******************************************************************************/
void VcpReadOffsetStateCccRequest(VcpProfileHandle profileHandle,
                                  ServiceHandle vocsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpVocsReadCCCReq(vcp_inst,
                          vocsClntHndl,
                          GattVocsClientReadOffsetStateCccRequest,
                          VCP_READ_OFFSET_STATE_CCC_CFM);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/*******************************************************************************/
void VcpReadAudioLocationCccRequest(VcpProfileHandle profileHandle,
                                    ServiceHandle vocsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpVocsReadCCCReq(vcp_inst,
                          vocsClntHndl,
                          GattVocsClientReadAudioLocationCccRequest,
                          VCP_READ_AUDIO_LOCATION_CCC_CFM);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/*******************************************************************************/
void VcpReadAudioOutputDescCccRequest(VcpProfileHandle profileHandle,
                                      ServiceHandle vocsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpVocsReadCCCReq(vcp_inst,
                          vocsClntHndl,
                          GattVocsClientReadAudiOutputDescCccRequest,
                          VCP_READ_AUDIO_OUTPUT_DESC_CCC_CFM);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/***************************************************************************/
static void vcpProfileSendReadOffsetStateCfm(VCP *vcp_inst,
                                             ServiceHandle srvc_hndl,
                                             gatt_status_t status,
                                             int16 volume_offset,
                                             uint8 change_counter)
{
    MAKE_VCP_MESSAGE(VcpReadOffsetStateCfm);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = srvc_hndl;
    message->status = status;
    message->volumeOffset = volume_offset;
    message->changeCounter = change_counter;

    MessageSend(vcp_inst->app_task, VCP_READ_OFFSET_STATE_CFM, message);
}

/*******************************************************************************/
void vcpHandleVocsReadOffsetStateCfm(VCP *vcp_inst,
                                     const GattVocsClientReadOffsetStateCfm *msg)
{
    if (msg->status == gatt_status_success)
    {
        /* Update the change counter */
        vcp_inst->vocs_change_counter = msg->changeCounter;
    }

    if (vcp_inst->pending_op == vcp_pending_op_none)
    {
        /* No pending vocs control point operation */
        vcpProfileSendReadOffsetStateCfm(vcp_inst,
                                         msg->srvcHdnl,
                                         msg->status,
                                         msg->volumeOffset,
                                         msg->changeCounter);
    }
    else if (vcp_inst->pending_op == vcp_pending_set_volume_offset_op)
    {
        /* If the reading has been performed after a vocs control point operation failed with
         * an invalid change counter error, we need to try again to execute that operation.
        */
        vcpVocsControlPointOp(vcp_inst->vcp_srvc_hdl,
                              msg->srvcHdnl,
                              vcp_vocs_set_volume_offset_op,
                              vcp_inst->volume_offset_pending);
    }
}

/***************************************************************************/
static void vcpSendReadAudioLocationCfm(VCP *vcp_inst,
                                        ServiceHandle srvc_hndl,
                                        gatt_status_t status,
                                        GattVocsClientAudioLoc value)
{
    MAKE_VCP_MESSAGE(VcpReadAudioLocationCfm);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = srvc_hndl;
    message->status = status;
    message->audioLocation = value;

    MessageSend(vcp_inst->app_task, VCP_READ_AUDIO_LOCATION_CFM, message);
}

/***************************************************************************/
static void vcpSendReadAudioOutputDescCfm(VCP *vcp_inst,
                                          ServiceHandle srvc_hndl,
                                          gatt_status_t status,
                                          uint16 size_value,
                                          uint8 *value)
{
    uint8 nullValue = 0;
    uint8 *checkedValue = value ? value : (&nullValue);
    uint16 sizeValue = size_value ? size_value : 1;

    MAKE_VCP_MESSAGE_WITH_VALUE(VcpReadAudioOutputDescCfm,
                                sizeValue,
                                checkedValue);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = srvc_hndl;
    message->status = status;

    MessageSend(vcp_inst->app_task, VCP_READ_AUDIO_OUTPUT_DESC_CFM, message);
}

/*******************************************************************************/
void vcpHandleVocsReadAudioLocationeCfm(VCP *vcp_inst,
                                        const GattVocsClientReadAudioLocationCfm *msg)
{
    vcpSendReadAudioLocationCfm(vcp_inst,
                                msg->srvcHdnl,
                                msg->status,
                                msg->audioLocation);
}

/*******************************************************************************/
void vcpHandleVocsReadAudioOutputDescCfm(VCP *vcp_inst,
                                         const GattVocsClientReadAudioOutputDescCfm *msg)
{
    vcpSendReadAudioOutputDescCfm(vcp_inst,
                                  msg->srvcHdnl,
                                  msg->status,
                                  msg->sizeValue,
                                  msg->audioOuputDesc);
}

/*******************************************************************************/
void VcpReadOffsetStateRequest(VcpProfileHandle profileHandle,
                               ServiceHandle vocsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        if (vocsClntHndl)
        {
            /* Check if the client handle is a valid one */
            if (vcpIsValidVocsInst(vcp_inst, vocsClntHndl))
            {
                GattVocsClientReadOffsetStateRequest(vocsClntHndl);
            }
            else
            {
                vcpProfileSendReadOffsetStateCfm(vcp_inst, vocsClntHndl, gatt_status_failure, 0, 0);
            }
        }
        else
        {
            vcp_vocs_srvc_hndl_t *ptr = vcp_inst->first_vocs_srvc_hndl;

            while(ptr)
            {
                GattVocsClientReadOffsetStateRequest(ptr->srvc_hdnl);
                ptr = ptr->next;
            }
        }
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/*******************************************************************************/
void VcpReadAudioLocationRequest(VcpProfileHandle profileHandle,
                                 ServiceHandle vocsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        if (vocsClntHndl)
        {
            /* Check if the client handle is a valid one */
            if (vcpIsValidVocsInst(vcp_inst, vocsClntHndl))
            {
                GattVocsClientReadAudioLocationRequest(vocsClntHndl);
            }
            else
            {
                vcpSendReadAudioLocationCfm(vcp_inst,
                                            vocsClntHndl,
                                            gatt_status_failure,
                                            GATT_VOCS_CLIENT_AUDIO_LOC_INVALID);
            }
        }
        else
        {
            vcp_vocs_srvc_hndl_t *ptr = vcp_inst->first_vocs_srvc_hndl;

            while(ptr)
            {
                GattVocsClientReadAudioLocationRequest(ptr->srvc_hdnl);
                ptr = ptr->next;
            }
        }
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/*******************************************************************************/
void VcpReadAudioOutputDescRequest(VcpProfileHandle profileHandle,
                                   ServiceHandle vocsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        if (vocsClntHndl)
        {
            /* Check if the client handle is a valid one */
            if (vcpIsValidVocsInst(vcp_inst, vocsClntHndl))
            {
                GattVocsClientReadAudioOutputDescRequest(vocsClntHndl);
            }
            else
            {
                vcpSendReadAudioOutputDescCfm(vcp_inst,
                                              vocsClntHndl,
                                              gatt_status_failure,
                                              0,
                                              NULL);
            }
        }
        else
        {
            vcp_vocs_srvc_hndl_t *ptr = vcp_inst->first_vocs_srvc_hndl;

            while(ptr)
            {
                GattVocsClientReadAudioOutputDescRequest(ptr->srvc_hdnl);
                ptr = ptr->next;
            }
        }
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/*******************************************************************************/
void vcpHandleAicsReadInputStateCccCfm(VCP *vcp_inst,
                                       const GattAicsClientReadInputStateCccCfm *msg)
{
    vcpIncldSrvcSendReadCccCfm(vcp_inst,
                               msg->srvcHndl,
                               msg->status,
                               msg->sizeValue,
                               msg->value,
                               VCP_READ_INPUT_STATE_CCC_CFM);
}

/*******************************************************************************/
static void vcpAicsReadCCCReq(VCP *vcp_inst,
                              ServiceHandle aics_clnt_hndl,
                              void (*aics_client_func)(ServiceHandle clnt_hndl),
                              VcpMessageId id)
{
    if (aics_clnt_hndl)
    {
        /* Check if the client handle is a valid one */
        if (vcpIsValidAicsInst(vcp_inst, aics_clnt_hndl))
        {
            (*aics_client_func)(aics_clnt_hndl);
        }
        else
        {
            vcpIncldSrvcSendReadCccCfm(vcp_inst,
                                       aics_clnt_hndl,
                                       gatt_status_failure,
                                       0,
                                       NULL,
                                       id);
        }
    }
    else
    {
        vcp_aics_srvc_hndl_t *ptr = vcp_inst->first_aics_srvc_hndl;

        while(ptr)
        {
            (*aics_client_func)(ptr->srvc_hdnl);
            ptr = ptr->next;
        }
    }
}

/*******************************************************************************/
void VcpReadInputStateCccRequest(VcpProfileHandle profileHhandle,
                                 ServiceHandle aicsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHhandle);

    if (vcp_inst)
    {
        vcpAicsReadCCCReq(vcp_inst,
                          aicsClntHndl,
                          GattAicsClientReadInputStateCccRequest,
                          VCP_READ_INPUT_STATE_CCC_CFM);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/*******************************************************************************/
void vcpHandleAicsReadInputStatusCccCfm(VCP *vcp_inst,
                                        const GattAicsClientReadInputStatusCccCfm *msg)
{
    vcpIncldSrvcSendReadCccCfm(vcp_inst,
                               msg->srvcHndl,
                               msg->status,
                               msg->sizeValue,
                               msg->value,
                               VCP_READ_INPUT_STATUS_CCC_CFM);
}

/*******************************************************************************/
void VcpReadInputStatusCccRequest(VcpProfileHandle profileHandle,
                                  ServiceHandle aicsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpAicsReadCCCReq(vcp_inst,
                          aicsClntHndl,
                          GattAicsClientReadInputStatusCccRequest,
                          VCP_READ_INPUT_STATUS_CCC_CFM);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/*******************************************************************************/
void vcpHandleAicsReadAudioInputDescCccCfm(VCP *vcp_inst,
                                           const GattAicsClientReadAudioInputDescCccCfm *msg)
{
    vcpIncldSrvcSendReadCccCfm(vcp_inst,
                               msg->srvcHndl,
                               msg->status,
                               msg->sizeValue,
                               msg->value,
                               VCP_READ_AUDIO_INPUT_DESC_CCC_CFM);
}

/*******************************************************************************/
void VcpReadAudioInputDescCccRequest(VcpProfileHandle profileHandle,
                                     ServiceHandle aicsClnHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpAicsReadCCCReq(vcp_inst,
                          aicsClnHndl,
                          GattAicsClientReadAudiInputDescCccRequest,
                          VCP_READ_AUDIO_INPUT_DESC_CCC_CFM);
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/***************************************************************************/
static void vcpSendReadInputStateCfm(VCP *vcp_inst,
                                     ServiceHandle srvc_hndl,
                                     gatt_status_t status,
                                     int8 gain_setting,
                                     GattAicsClientMute mute,
                                     GattAicsClientGainMode gain_mode,
                                     uint8 change_counter)
{
    MAKE_VCP_MESSAGE(VcpReadInputStateCfm);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = srvc_hndl;
    message->status = status;
    message->gainSetting = gain_setting;
    message->mute = mute;
    message->gainMode = gain_mode;
    message->changeCounter = change_counter;

    MessageSend(vcp_inst->app_task, VCP_READ_INPUT_STATE_CFM, message);
}

/***************************************************************************/
static vcp_aics_control_point_opcodes_t vcpGetAicsOpcodeFromPendingCmd(uint16 pending_cmd)
{
    switch(pending_cmd)
    {
        case vcp_pending_set_gain_setting_op:
            return vcp_aics_set_gain_setting_op;

        case vcp_pending_aics_unmute_op:
            return vcp_aics_unmute_op;

        case vcp_pending_aics_mute_op:
            return vcp_aics_mute_op;

        case vcp_pending_set_mnl_gain_mode_op:
            return vcp_aics_set_mnl_gain_mode_op;

        case vcp_pending_set_atmtc_gain_mode_op:
            return vcp_aics_set_atmtc_gain_mode_op;

        default:
            return vcp_aics_last_op;
    }
}

/*******************************************************************************/
void vcpHandleAicsReadInputStateCfm(VCP *vcp_inst,
                                    const GattAicsClientReadInputStateCfm *msg)
{
    if (msg->status == gatt_status_success)
    {
        /* Update the change counter */
        vcp_inst->aics_change_counter = msg->changeCounter;
    }

    if (vcp_inst->pending_op == vcp_pending_op_none)
    {
        /* No pending aics control point operation */
        vcpSendReadInputStateCfm(vcp_inst,
                                 msg->srvcHndl,
                                 msg->status,
                                 msg->gainSetting,
                                 msg->mute,
                                 msg->gainMode,
                                 msg->changeCounter);
    }
    else if (vcp_inst->pending_op >= vcp_pending_set_gain_setting_op &&
             vcp_inst->pending_op <= vcp_pending_set_atmtc_gain_mode_op)
    {
        /* If the reading has been performed after a aics control point operation failed with
         * an invalid change counter error, we need to try again to execute that operation.
        */
        vcpAicsControlPointOp(vcp_inst->vcp_srvc_hdl,
                              msg->srvcHndl,
                              vcpGetAicsOpcodeFromPendingCmd(vcp_inst->pending_op),
                              vcp_inst->gain_setting_pending);
    }
}

/*******************************************************************************/
void VcpReadInputStateRequest(VcpProfileHandle profileHandle,
                              ServiceHandle aicsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        if (aicsClntHndl)
        {
            /* Check if the client handle is a valid one */
            if (vcpIsValidAicsInst(vcp_inst, aicsClntHndl))
            {
                GattAicsClientReadInputStateRequest(aicsClntHndl);
            }
            else
            {
                vcpSendReadInputStateCfm(vcp_inst, aicsClntHndl, gatt_status_failure, 0, 0, 0, 0);
            }
        }
        else
        {
            vcp_aics_srvc_hndl_t *ptr = vcp_inst->first_aics_srvc_hndl;

            while(ptr)
            {
                GattAicsClientReadInputStateRequest(ptr->srvc_hdnl);
                ptr = ptr->next;
            }
        }
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/***************************************************************************/
static void vcpSendReadGainSettingPropertiesCfm(VCP *vcp_inst,
                                                ServiceHandle srvc_hndl,
                                                gatt_status_t status,
                                                uint8 gain_set_units,
                                                int8 gain_set_min,
                                                int8 gain_set_max)
{
    MAKE_VCP_MESSAGE(VcpReadGainSetPropertiesCfm);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = srvc_hndl;
    message->status = status;
    message->gainSettingMax = gain_set_max;
    message->gainSettingMin = gain_set_min;
    message->gainSettingUnits = gain_set_units;

    MessageSend(vcp_inst->app_task, VCP_READ_GAIN_SET_PROPERTIES_CFM, message);
}

/*******************************************************************************/
void vcpHandleAicsReadGainSetPropertiesCfm(VCP *vcp_inst,
                                           const GattAicsClientReadGainSetPropertiesCfm *msg)
{
    vcpSendReadGainSettingPropertiesCfm(vcp_inst,
                                        msg->srvcHndl,
                                        msg->status,
                                        msg->gainSettingUnits,
                                        msg->gainSettingMin,
                                        msg->gainSettingMax);
}

/*******************************************************************************/
void VcpReadGainSetProperRequest(VcpProfileHandle profileHandle,
                                 ServiceHandle aicsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        if (aicsClntHndl)
        {
            /* Check if the client handle is a valid one */
            if (vcpIsValidAicsInst(vcp_inst, aicsClntHndl))
            {
                GattAicsClientReadGainSetPropertiesReq(aicsClntHndl);
            }
            else
            {
                vcpSendReadGainSettingPropertiesCfm(vcp_inst,
                                                    aicsClntHndl,
                                                    gatt_status_failure,
                                                    0, 0, 0);
            }
        }
        else
        {
            vcp_aics_srvc_hndl_t *ptr = vcp_inst->first_aics_srvc_hndl;

            while(ptr)
            {
                GattAicsClientReadGainSetPropertiesReq(ptr->srvc_hdnl);
                ptr = ptr->next;
            }
        }
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/***************************************************************************/
static void vcpSendReadInputTypeCfm(VCP *vcp_inst,
                                    ServiceHandle srvc_hndl,
                                    gatt_status_t status,
                                    GattAicsClientInputType input_type)
{
    MAKE_VCP_MESSAGE(VcpReadInputTypeCfm);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = srvc_hndl;
    message->status = status;
    message->inputType = input_type;

    MessageSend(vcp_inst->app_task, VCP_READ_INPUT_TYPE_CFM, message);
}

/*******************************************************************************/
void vcpHandleAicsReadInputTypeCfm(VCP *vcp_inst,
                                   const GattAicsClientReadInputTypeCfm *msg)
{
    vcpSendReadInputTypeCfm(vcp_inst,
                            msg->srvcHndl,
                            msg->status,
                            msg->inputType);
}

/*******************************************************************************/
void VcpReadInputTypeRequest(VcpProfileHandle profileHandle,
                             ServiceHandle aicsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        if (aicsClntHndl)
        {
            /* Check if the client handle is a valid one */
            if (vcpIsValidAicsInst(vcp_inst, aicsClntHndl))
            {
                GattAicsClientReadInputTypeReq(aicsClntHndl);
            }
            else
            {
                vcpSendReadInputTypeCfm(vcp_inst,
                                        aicsClntHndl,
                                        gatt_status_failure,
                                        GATT_AICS_CLIENT_INP_TPY_INVALID);
            }
        }
        else
        {
            vcp_aics_srvc_hndl_t *ptr = vcp_inst->first_aics_srvc_hndl;

            while(ptr)
            {
                GattAicsClientReadInputTypeReq(ptr->srvc_hdnl);
                ptr = ptr->next;
            }
        }
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/***************************************************************************/
static void vcpSendReadInputStatuCfm(VCP *vcp_inst,
                                     ServiceHandle srvc_hndl,
                                     gatt_status_t status,
                                     uint8 input_status)
{
    MAKE_VCP_MESSAGE(VcpReadInputStatusCfm);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = srvc_hndl;
    message->status = status;
    message->inputStatus = input_status;

    MessageSend(vcp_inst->app_task, VCP_READ_INPUT_STATUS_CFM, message);
}

/*******************************************************************************/
void vcpHandleAicsReadInputStatusCfm(VCP *vcp_inst,
                                     const GattAicsClientReadInputStatusCfm *msg)
{
    vcpSendReadInputStatuCfm(vcp_inst,
                             msg->srvcHndl,
                             msg->status,
                             msg->inputStatus);
}

/*******************************************************************************/
void VcpReadInputStatusRequest(VcpProfileHandle profileHandle,
                               ServiceHandle aicsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        if (aicsClntHndl)
        {
            /* Check if the client handle is a valid one */
            if (vcpIsValidAicsInst(vcp_inst, aicsClntHndl))
            {
                GattAicsClientReadInputStatusReq(aicsClntHndl);
            }
            else
            {
                vcpSendReadInputStatuCfm(vcp_inst,
                                         aicsClntHndl,
                                         gatt_status_failure,
                                         GATT_AICS_CLIENT_INPUT_STATUS_INVALID);
            }
        }
        else
        {
            vcp_aics_srvc_hndl_t *ptr = vcp_inst->first_aics_srvc_hndl;

            while(ptr)
            {
                GattAicsClientReadInputStatusReq(ptr->srvc_hdnl);
                ptr = ptr->next;
            }
        }
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

/***************************************************************************/
static void vcpSendReadAudioInputDescCfm(VCP *vcp_inst,
                                         ServiceHandle srvc_hndl,
                                         gatt_status_t status,
                                         uint16 size_value,
                                         uint8 *value)
{    
    uint8 nullValue = 0;
    uint8 *checkedValue = value ? value : (&nullValue);
    uint16 sizeValue = size_value ? size_value : 1;

    MAKE_VCP_MESSAGE_WITH_VALUE(VcpReadAudioInputDescCfm,
                                sizeValue,
                                checkedValue);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = srvc_hndl;
    message->status = status;

    MessageSend(vcp_inst->app_task, VCP_READ_AUDIO_INPUT_DESC_CFM, message);
}

/*******************************************************************************/
void vcpHandleAicsReadAudioInputDescCfm(VCP *vcp_inst,
                                        const GattAicsClientReadAudioInputDescCfm *msg)
{
    vcpSendReadAudioInputDescCfm(vcp_inst,
                                 msg->srvcHndl,
                                 msg->status,
                                 msg->sizeValue,
                                 msg->audioInputDesc);
}

/*******************************************************************************/
void VcpReadAudioInputDescRequest(VcpProfileHandle profileHandle,
                                  ServiceHandle aicsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        if (aicsClntHndl)
        {
            /* Check if the client handle is a valid one */
            if (vcpIsValidAicsInst(vcp_inst, aicsClntHndl))
            {
                GattAicsClientReadAudioInputDescRequest(aicsClntHndl);
            }
            else
            {
                vcpSendReadAudioInputDescCfm(vcp_inst,
                                             aicsClntHndl,
                                             gatt_status_failure,
                                             0,
                                             NULL);
            }
        }
        else
        {
            vcp_aics_srvc_hndl_t *ptr = vcp_inst->first_aics_srvc_hndl;

            while(ptr)
            {
                GattAicsClientReadAudioInputDescRequest(ptr->srvc_hdnl);
                ptr = ptr->next;
            }
        }
    }
    else
    {
        VCP_DEBUG_PANIC(("Invalid profile_handle\n"));
    }
}

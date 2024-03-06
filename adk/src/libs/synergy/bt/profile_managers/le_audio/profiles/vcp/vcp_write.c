/* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_vocs_client.h"
#include "gatt_vcs_client.h"

#include "vcp.h"
#include "vcp_write.h"
#include "vcp_debug.h"
#include "vcp_private.h"
#include "vcp_common.h"
#include "vcp_init.h"

typedef enum __vcp_vocs_set_param
{
    vcp_vocs_set_param_volume_offset,
    vcp_vocs_set_param_audio_location,
    vcp_vocs_set_param_audio_output_desc,
    vcp_vocs_set_param_invalid
} vcp_vocs_set_param_t;

/***************************************************************************/
static void vcpSendVolumeControlPointOpCfm(VCP *vcp_inst,
                                           status_t status,
                                           VcpMessageId id)
{
    if (id != VCP_MESSAGE_TOP)
    {
        /* We will use VCP_REL_VOL_DOWN_CFM to create the message
         * because the structure of all the others write confirmations is the same.
         * We will send the right message using the id parameter */
        MAKE_VCP_MESSAGE(VcpRelVolDownCfm);

        message->id = id;
        message->prflHndl = vcp_inst->vcp_srvc_hdl;
        message->status = status;

        VcpMessageSend(vcp_inst->app_task, message);
    }
}

/***************************************************************************/
static VcpMessageId vcpGetVcsMessageIdFromOpcode(vcp_vcs_control_point_opcodes_t opcode)
{
    VcpMessageId id = VCP_MESSAGE_TOP;

    switch(opcode)
    {
        case vcp_relative_volume_down_op:
        {
            id = VCP_REL_VOL_DOWN_CFM;
        }
        break;

        case vcp_relative_volume_up_op:
        {
            id = VCP_REL_VOL_UP_CFM;
        }
        break;

        case vcp_unmute_relative_volume_down_op:
        {
            id = VCP_UNMUTE_REL_VOL_DOWN_CFM;
        }
        break;

        case vcp_unmute_relative_volume_up_op:
        {
            id = VCP_UNMUTE_REL_VOL_UP_CFM;
        }
        break;

        case vcp_set_absolute_volume_op:
        {
            id = VCP_ABS_VOL_CFM;
        }
        break;

        case vcp_unmute_op:
        {
            id = VCP_UNMUTE_CFM;
        }
        break;

        case vcp_mute_op:
        {
            id = VCP_MUTE_CFM;
        }
        break;

        default:
        break;
    }

    return id;
}

/****************************************************************************/
void vcpVcsControlPointOp(ServiceHandle profile_handle,
                          vcp_vcs_control_point_opcodes_t opcode,
                          uint8 volume_settting_operand)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profile_handle);

    if (vcp_inst)
    {
        switch(opcode)
        {
            case vcp_relative_volume_down_op:
            {
                GattVcsClientRelativeVolDownRequest(vcp_inst->vcs_srvc_hdl,
                                                  vcp_inst->vcs_change_counter);
            }
            break;

            case vcp_relative_volume_up_op:
            {
                GattVcsClientRelativeVolUpRequest(vcp_inst->vcs_srvc_hdl,
                                                vcp_inst->vcs_change_counter);
            }
            break;

            case vcp_unmute_relative_volume_down_op:
            {
                GattVcsClientUnmuteRelativeVolDownRequest(vcp_inst->vcs_srvc_hdl,
                                                        vcp_inst->vcs_change_counter);
            }
            break;

            case vcp_unmute_relative_volume_up_op:
            {
                GattVcsClientUnmuteRelativeVolUpRequest(vcp_inst->vcs_srvc_hdl,
                                                      vcp_inst->vcs_change_counter);
            }
            break;

            case vcp_set_absolute_volume_op:
            {
                vcp_inst->volume_setting_pending = volume_settting_operand;
                GattVcsClientAbsoluteVolRequest(vcp_inst->vcs_srvc_hdl,
                                              vcp_inst->vcs_change_counter,
                                              volume_settting_operand);
            }
            break;

            case vcp_unmute_op:
            {
                GattVcsClientUnmuteRequest(vcp_inst->vcs_srvc_hdl,
                                      vcp_inst->vcs_change_counter);
            }
            break;

            case vcp_mute_op:
            {
                GattVcsClientMuteRequest(vcp_inst->vcs_srvc_hdl,
                                    vcp_inst->vcs_change_counter);
            }
            break;

            default:
            {
                VCP_ERROR("Invalid VCS control point opcode\n");
                vcpSendVolumeControlPointOpCfm(vcp_inst,
                                               CSR_BT_GATT_RESULT_INTERNAL_ERROR,
                                               vcpGetVcsMessageIdFromOpcode(opcode));
        }
        }
    }
    else
    {
        VCP_DEBUG("Invalid VCP Profile instance\n");
    }
}

/****************************************************************************/
static vcp_internal_msg_t vcpGetInternalMessageIdFromOpcode(vcp_vcs_control_point_opcodes_t opcode)
{
    vcp_internal_msg_t id = VCP_INTERNAL_MSG_BASE;

    switch(opcode)
    {
        case vcp_relative_volume_down_op:
        {
            id = VCP_INTERNAL_REL_VOL_DOWN;
        }
        break;

        case vcp_relative_volume_up_op:
        {
            id = VCP_INTERNAL_REL_VOL_UP;
        }
        break;

        case vcp_unmute_relative_volume_down_op:
        {
            id = VCP_INTERNAL_UNMUTE_REL_VOL_DOWN;
        }
        break;

        case vcp_unmute_relative_volume_up_op:
        {
            id = VCP_INTERNAL_UNMUTE_REL_VOL_UP;
        }
        break;

        case vcp_set_absolute_volume_op:
        {
            id = VCP_INTERNAL_ABS_VOL;
        }
        break;

        case vcp_unmute_op:
        {
            id = VCP_INTERNAL_UNMUTE;
        }
        break;

        case vcp_mute_op:
        {
            id = VCP_INTERNAL_MUTE;
        }
        break;

        default:
        break;
    }

    return id;
}

/****************************************************************************/
static void vcpVcsSendInternalMsg(VCP *vcp_inst,
                                  vcp_vcs_control_point_opcodes_t opcode,
                                  uint8 volume_setting)
{
    MAKE_VCP_INTERNAL_MESSAGE(VCP_INTERNAL_REL_VOL_DOWN);

    message->id = vcpGetInternalMessageIdFromOpcode(opcode);
    message->prfl_hndl = vcp_inst->vcp_srvc_hdl;
    message->volume_setting = volume_setting;

    VcpMessageSendConditionally(vcp_inst->lib_task,
                                vcpGetInternalMessageIdFromOpcode(opcode),
                                message,
                                &vcp_inst->pending_op);
}

/****************************************************************************/
void VcpRelativeVolumeDownRequest(VcpProfileHandle profileHandle)

{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpVcsSendInternalMsg(vcp_inst, vcp_relative_volume_down_op, 0);
    }
    else
    {
       VCP_DEBUG("Invalid profile_handle\n");
    }
}

/****************************************************************************/
void VcpRelativeVolumeUpRequest(VcpProfileHandle profileHandle)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpVcsSendInternalMsg(vcp_inst, vcp_relative_volume_up_op, 0);
    }
    else
    {
       VCP_DEBUG("Invalid profile_handle\n");
    }
}

/****************************************************************************/
void VcpUnmuteRelativeVolumeDownRequest(VcpProfileHandle profileHandle)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpVcsSendInternalMsg(vcp_inst, vcp_unmute_relative_volume_down_op, 0);
    }
    else
    {
        VCP_DEBUG("Invalid profile_handle\n");
    }
}

/****************************************************************************/
void VcpUnmuteRelativeVolumeUpRequest(VcpProfileHandle profileHandle)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpVcsSendInternalMsg(vcp_inst, vcp_unmute_relative_volume_up_op, 0);
    }
    else
    {
        VCP_DEBUG("Invalid profile_handle\n");
    }
}

/****************************************************************************/
void VcpAbsoluteVolumeRequest(VcpProfileHandle profileHandle, uint8 volumeSetting)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpVcsSendInternalMsg(vcp_inst, vcp_set_absolute_volume_op, volumeSetting);
    }
    else
    {
        VCP_DEBUG("Invalid profile_handle\n");
    }
}

/****************************************************************************/
void VcpUnmuteRequest(VcpProfileHandle profileHandle)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpVcsSendInternalMsg(vcp_inst, vcp_unmute_op, 0);
    }
    else
    {
        VCP_DEBUG("Invalid profile_handle\n");
    }
}

/****************************************************************************/
void VcpMuteRequest(VcpProfileHandle profileHandle)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpVcsSendInternalMsg(vcp_inst, vcp_mute_op, 0);
    }
    else
    {
        VCP_DEBUG("Invalid profile_handle\n");
    }
}

/****************************************************************************/
static void vcpHandleVcsControlPointOpCfm(VCP *vcp_inst,
                                                 status_t status,
                                                 vcp_vcs_control_point_opcodes_t opcode)
{
    if (status == VCP_VCS_INVALID_CHANGE_COUNTER_ERR)
    {
        /* We need to read the Volume State characteristic to retrieve the right value
         * of change counter.
        */
        if (vcp_inst->retryCounter <= VCP_VCS_SET_ABS_VOL_MAX_RETRY_NUM)
        {
            vcp_inst->retryCounter++;
            GattVcsClientReadVolumeStateRequest(vcp_inst->vcs_srvc_hdl);
        }
        else
        {
            vcp_inst->volume_setting_pending = 0;
            vcp_inst->pending_op = vcp_pending_op_none; 
            vcp_inst->retryCounter = 0;
            vcpSendVolumeControlPointOpCfm(vcp_inst,
                                           status,
                                           vcpGetVcsMessageIdFromOpcode(opcode));
        }
    }
    else
    {
        if (vcp_inst->pending_op == vcp_pending_set_initial_vol_op)
        {
            if (status == CSR_BT_GATT_RESULT_SUCCESS)
            {
                vcpSendSetInitialVolOpCfm(vcp_inst, VCP_STATUS_SUCCESS);
            }
            else
            {
                vcpSendSetInitialVolOpCfm(vcp_inst, VCP_STATUS_FAILED);
            }
        }
        else
        {
            vcpSendVolumeControlPointOpCfm(vcp_inst,
                                           status,
                                           vcpGetVcsMessageIdFromOpcode(opcode));
        }
        vcp_inst->retryCounter = 0;
        vcp_inst->volume_setting_pending = 0;
        vcp_inst->pending_op = vcp_pending_op_none;
    }
}

/****************************************************************************/
void vcpHandleVcsRelativeVolumeDownOp(VCP *vcp_inst,
                                      const GattVcsClientRelVolDownCfm *msg)
{
    vcpHandleVcsControlPointOpCfm(vcp_inst,
                                  msg->status,
                                  vcp_relative_volume_down_op);
}

/****************************************************************************/
void vcpHandleVcsRelativeVolumeUpOp(VCP *vcp_inst,
                                    const GattVcsClientRelVolUpCfm *msg)
{
    vcpHandleVcsControlPointOpCfm(vcp_inst,
                                  msg->status,
                                  vcp_relative_volume_up_op);
}

/****************************************************************************/
void vcpHandleVcsUnmuteRelativeVolumeDownOp(VCP *vcp_inst,
                                            const GattVcsClientUnmuteRelVolDownCfm *msg)
{
    vcpHandleVcsControlPointOpCfm(vcp_inst,
                                  msg->status,
                                  vcp_unmute_relative_volume_down_op);
}

/****************************************************************************/
void vcpHandleVcsUnmuteRelativeVolumeUpOp(VCP *vcp_inst,
                                          const GattVcsClientUnmuteRelVolUpCfm *msg)
{
    vcpHandleVcsControlPointOpCfm(vcp_inst,
                                  msg->status,
                                  vcp_unmute_relative_volume_up_op);
}

/****************************************************************************/
void vcpHandleVcsSetAbsoluteVolumeOp(VCP *vcp_inst,
                                     const GattVcsClientAbsVolCfm *msg)
{
    vcpHandleVcsControlPointOpCfm(vcp_inst,
                                  msg->status,
                                  vcp_set_absolute_volume_op);
}

/****************************************************************************/
void vcpHandleVcsUnmuteOp(VCP *vcp_inst,
                          const GattVcsClientUnmuteCfm *msg)
{
    vcpHandleVcsControlPointOpCfm(vcp_inst,
                                  msg->status,
                                  vcp_unmute_op);
}

/****************************************************************************/
void vcpHandleVcsMuteOp(VCP *vcp_inst,
                        const GattVcsClientMuteCfm *msg)
{
    vcpHandleVcsControlPointOpCfm(vcp_inst,
                                  msg->status,
                                  vcp_mute_op);
}

/***************************************************************************/
static void vcpSendVocsSetCfm(VCP *vcp_inst,
                              ServiceHandle srvc_hndl,
                              status_t status,
                              GattVocsClientMessageId id)
{
    MAKE_VCP_MESSAGE(VcpSetVolumeOffsetCfm);

    message->id = id;
    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = srvc_hndl;
    message->status = status;

    VcpMessageSend(vcp_inst->app_task, message);
}

/****************************************************************************/
void vcpVocsControlPointOp(VcpProfileHandle profile_handle,
                           ServiceHandle vocs_srvc_hndl,
                           vcp_vocs_control_point_opcodes_t opcode,
                           int16 volume_offset_operand)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profile_handle);

    if (vcp_inst)
    {
        switch(opcode)
        {
            case vcp_vocs_set_volume_offset_op:
            {
                GattVocsClientSetVolumeOffsetReq(vocs_srvc_hndl,
                                                vcp_inst->vocs_change_counter,
                                                volume_offset_operand);
            }
            break;

            default:
            {
                VCP_ERROR("Invalid VOCS control point opcode\n");
                vcpSendVocsSetCfm(vcp_inst,
                                         vocs_srvc_hndl,
                                         CSR_BT_GATT_RESULT_INTERNAL_ERROR,
                                         VCP_SET_VOLUME_OFFSET_CFM);
            }
        }
    }
    else
    {
        VCP_DEBUG("Invalid VCP Profile instance\n");
    }
}

#ifndef EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
/****************************************************************************/
void vcpHandleVocsSetVolOffsetOp(VCP *vcp_inst,
                                 const GattVocsClientSetVolumeOffsetCfm *msg)
{
    if (msg->status == (status_t) VCP_VOCS_INVALID_CHANGE_COUNTER_ERR)
    {
        /* We need to read the Offset State characteristic to retrieve the right value
         * of change counter.
         */
        GattVocsClientReadOffsetStateRequest(msg->srvcHndl);
    }
    else
    {
        vcpSendVocsSetCfm(vcp_inst,
                          msg->srvcHndl,
                          msg->status,
                          VCP_SET_VOLUME_OFFSET_CFM);

        vcp_inst->volume_offset_pending = 0;
        vcp_inst->pending_op = vcp_pending_op_none;
    }
}

/****************************************************************************/
void vcpHandleVocsSetAudioLocCfm(VCP *vcp_inst,
                                 const GattVocsClientSetAudioLocCfm *msg)
{
    vcpSendVocsSetCfm(vcp_inst,
                      msg->srvcHndl,
                      msg->status,
                      VCP_SET_AUDIO_LOCATION_CFM);
}

/****************************************************************************/
void vcpHandleVocsSetAudioOutputDescCfm(VCP *vcp_inst,
                                        const GattVocsClientSetAudioOutputDescCfm *msg)
{
    vcpSendVocsSetCfm(vcp_inst,
                      msg->srvcHndl,
                      msg->status,
                      VCP_SET_AUDIO_OUTPUT_DESC_CFM);
}

/****************************************************************************/
static void vcpVocsSendInternalMsg(VCP *vcp_inst,
                                   ServiceHandle vocs_srvc_hndl,
                                   vcp_vocs_control_point_opcodes_t opcode,
                                   int16 volume_offset)
{
    MAKE_VCP_INTERNAL_MESSAGE(VCP_INTERNAL_SET_VOL_OFFSET);

    message->id = VCP_INTERNAL_SET_VOL_OFFSET;
    message->prfl_hndl = vcp_inst->vcp_srvc_hdl;
    message->vocs_srvc_hndl = vocs_srvc_hndl;
    message->opcode = opcode;
    message->volume_offset = volume_offset;

    VcpMessageSendConditionally(vcp_inst->lib_task,
                             VCP_INTERNAL_SET_VOL_OFFSET,
                             message,
                             &vcp_inst->pending_op);
}

/****************************************************************************/
static void vcpVocsExecuteFuncFromSetParam(VCP *vcp_inst,
                                           ServiceHandle vocs_clnt_hndl,
                                           void *param_1,
                                           void *param_2,
                                           vcp_vocs_set_param_t set_param)
{
    switch (set_param)
    {
        case vcp_vocs_set_param_volume_offset:
        {
            vcpVocsSendInternalMsg(vcp_inst,
                                   vocs_clnt_hndl,
                                   (*((vcp_vocs_control_point_opcodes_t *) param_1)),
                                   (*((int16 *) param_2)));
    }
        break;

        case vcp_vocs_set_param_audio_location:
        {
            GattVocsClientSetAudioLocReq(vocs_clnt_hndl,
                                         (*((GattVocsClientAudioLoc *) param_1)));
        }
            break;

        case vcp_vocs_set_param_audio_output_desc:
        {
            GattVocsClientSetAudioOutputDescReq(vocs_clnt_hndl,
                                                (*((uint16 *) param_1)),
                                                (uint8 *) param_2);
        }
            break;

        default:
        break;
    }
}

/****************************************************************************/
static void vcpVocsSetReq(VCP *vcp_inst,
                          ServiceHandle vocs_clnt_hndl,
                          vcp_vocs_set_param_t set_param,
                          void *param_1,
                          void *param_2,
                          VcpMessageId id)
{
    if (vocs_clnt_hndl)
    {
        /* Check if the client handle is a valid one */
        if (vcpIsValidVocsInst(vcp_inst, vocs_clnt_hndl))
        {
            vcpVocsExecuteFuncFromSetParam(vcp_inst,
                                           vocs_clnt_hndl,
                                           param_1,
                                           param_2,
                                           set_param);
        }
        else
        {
            vcpSendVocsSetCfm(vcp_inst,
                              vocs_clnt_hndl,
                              CSR_BT_GATT_RESULT_INTERNAL_ERROR,
                              id);
        }
    }
    else
    {
        vcp_vocs_srvc_hndl_t *ptr = vcp_inst->first_vocs_srvc_hndl;

        while(ptr)
        {
            vcpVocsExecuteFuncFromSetParam(vcp_inst,
                                           ptr->srvc_hdnl,
                                           param_1,
                                           param_2,
                                           set_param);
            ptr = ptr->next;
        }
    }
}

/****************************************************************************/
void VcpSetVolumeOffsetRequest(VcpProfileHandle profileHandle,
                               int16 volumeOffset,
                               ServiceHandle vocsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcp_vocs_control_point_opcodes_t opcode = vcp_vocs_set_volume_offset_op;
        int16 vol_offset = volumeOffset;

        vcpVocsSetReq(vcp_inst,
                      vocsClntHndl,
                      vcp_vocs_set_param_volume_offset,
                      (void *) (&opcode),
                      (void *) (&vol_offset),
                      VCP_SET_VOLUME_OFFSET_CFM);
    }
    else
    {
        VCP_DEBUG("Invalid profile_handle\n");
    }
}

/****************************************************************************/
void VcpSetAudioLocationRequest(VcpProfileHandle profileHandle,
                                GattVocsClientAudioLoc audioLocVal,
                                ServiceHandle vocsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        GattVocsClientAudioLoc audio_location = audioLocVal;

        vcpVocsSetReq(vcp_inst,
                      vocsClntHndl,
                      vcp_vocs_set_param_audio_location,
                      (void *) (&audio_location),
                      NULL,
                      VCP_SET_AUDIO_LOCATION_CFM);
    }
    else
    {
        VCP_DEBUG("Invalid profile_handle\n");
    }
}

/****************************************************************************/
void VcpSetAudioOutputDescRequest(VcpProfileHandle profileHandle,
                              uint16               valueSize,
                              uint8               *audioOutputDescVal,
                              ServiceHandle     vocsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        uint16 size = valueSize;

        vcpVocsSetReq(vcp_inst,
                      vocsClntHndl,
                      vcp_vocs_set_param_audio_output_desc,
                      (void *) (&size),
                      (void *) audioOutputDescVal,
                      VCP_SET_AUDIO_OUTPUT_DESC_CFM);
    }
    else
    {
        VCP_DEBUG("Invalid profile_handle\n");
    }
}
#endif

#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
/***************************************************************************/
static void vcpSendAudioInputControlPointOpCfm(VCP *vcp_inst,
                                               ServiceHandle srvc_hndl,
                                               status_t status,
                                               VcpMessageId id)
{
    if (id != VCP_MESSAGE_TOP)
    {
        /* We will use VCP_SET_GAIN_SETTING_CFM to create the message
         * because the structure of all the others write confirmations is the same.
         * We will send the right message using the id parameter */
        MAKE_VCP_MESSAGE(VcpSetGainSettingCfm);

        message->id = id;
        message->prflHndl = vcp_inst->vcp_srvc_hdl;
        message->srvcHndl = srvc_hndl;
        message->status = status;

        VcpMessageSend(vcp_inst->app_task, message);
    }
}

/***************************************************************************/
static VcpMessageId vcpGetAicsMessageIdFromOpcode(vcp_aics_control_point_opcodes_t opcode)
{
    VcpMessageId id = VCP_MESSAGE_TOP;

    switch(opcode)
    {
        case vcp_aics_set_gain_setting_op:
        {
            id = VCP_SET_GAIN_SETTING_CFM;
        }
        break;

        case vcp_aics_unmute_op:
        {
            id = VCP_AICS_UNMUTE_CFM;
        }
        break;

        case vcp_aics_mute_op:
        {
            id = VCP_AICS_MUTE_CFM;
        }
        break;

        case vcp_aics_set_mnl_gain_mode_op:
        {
            id = VCP_AICS_SET_MANUAL_GAIN_MODE_CFM;
        }
        break;

        case vcp_aics_set_atmtc_gain_mode_op:
        {
            id = VCP_AICS_SET_AUTOMATIC_GAIN_MODE_CFM;
        }
        break;

        default:
        break;
    }

    return id;
}

/****************************************************************************/
static void vcpHandleAicsControlPointOpCfm(VCP *vcp_inst,
                                           ServiceHandle srvc_hndl,
                                           status_t status,
                                           vcp_aics_control_point_opcodes_t opcode)
{
    if (status == VCP_AICS_INVALID_CHANGE_COUNTER_ERR)
    {
        /* We need to read the Input State characteristic to retrieve the right value
         * of change counter.
        */
        GattAicsClientReadInputStateRequest(srvc_hndl);
    }
    else
    {
        vcpSendAudioInputControlPointOpCfm(vcp_inst,
                                           srvc_hndl,
                                           status,
                                           vcpGetAicsMessageIdFromOpcode(opcode));

        vcp_inst->gain_setting_pending = 0;
        vcp_inst->pending_op = vcp_pending_op_none;
    }
}

/****************************************************************************/
void vcpHandleAicsSetGainSettingOp(VCP *vcp_inst,
                                   const GattAicsClientSetGainSettingCfm *msg)
{
    vcpHandleAicsControlPointOpCfm(vcp_inst,
                                   msg->srvcHndl,
                                   msg->status,
                                   vcp_aics_set_gain_setting_op);
}
#endif

/****************************************************************************/
void vcpAicsControlPointOp(VcpProfileHandle profile_handle,
                           ServiceHandle srvc_hndl,
                           vcp_aics_control_point_opcodes_t opcode,
                           int8 gain_settting_operand)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profile_handle);

    if (vcp_inst)
    {
        switch(opcode)
        {
            case vcp_aics_set_gain_setting_op:
            {
                vcp_inst->pending_op = vcp_pending_set_gain_setting_op;
                vcp_inst->gain_setting_pending = gain_settting_operand;
                GattAicsClientSetGainSettingReq(srvc_hndl,
                                                vcp_inst->aics_change_counter,
                                                gain_settting_operand);
            }
            break;

            case vcp_aics_unmute_op:
            {
                vcp_inst->pending_op = vcp_pending_aics_unmute_op;
                GattAicsClientUnmuteReq(srvc_hndl,
                                        vcp_inst->aics_change_counter);
            }
            break;

            case vcp_aics_mute_op:
            {
                vcp_inst->pending_op = vcp_pending_aics_mute_op;
                GattAicsClientMuteReq(srvc_hndl,
                                      vcp_inst->aics_change_counter);
            }
            break;

            case vcp_aics_set_mnl_gain_mode_op:
            {
                vcp_inst->pending_op = vcp_pending_set_mnl_gain_mode_op;
                GattAicsClientSetManualGainModeReq(srvc_hndl,
                                                   vcp_inst->aics_change_counter);
            }
            break;

            case vcp_aics_set_atmtc_gain_mode_op:
            {
                vcp_inst->pending_op = vcp_pending_set_atmtc_gain_mode_op;
                GattAicsClientSetAutomaticGainModeReq(srvc_hndl,
                                                      vcp_inst->aics_change_counter);
            }
            break;

            default:
            {
                VCP_ERROR("Invalid AICS control point opcode\n");
#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
                vcpSendAudioInputControlPointOpCfm(vcp_inst,
                                                   srvc_hndl,
                                                   CSR_BT_GATT_RESULT_INTERNAL_ERROR,
                                                   vcpGetAicsMessageIdFromOpcode(opcode));
#endif
            }
        }
    }
    else
    {
        VCP_PANIC("Invalid VCP Profile instance\n");
    }
}

#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
/****************************************************************************/
static vcp_internal_msg_t vcpAicsGetInternalMessageIdFromOpcode(vcp_aics_control_point_opcodes_t opcode)
{
    vcp_internal_msg_t id = VCP_INTERNAL_MSG_BASE;

    switch(opcode)
    {
        case vcp_aics_set_gain_setting_op:
        {
            id = VCP_INTERNAL_SET_GAIN_SETTING;
        }
        break;

        case vcp_aics_unmute_op:
        {
            id = VCP_INTERNAL_AICS_UNMUTE;
        }
        break;

        case vcp_aics_mute_op:
        {
            id = VCP_INTERNAL_AICS_MUTE;
        }
        break;

        case vcp_aics_set_mnl_gain_mode_op:
        {
            id = VCP_INTERNAL_AICS_SET_MANUAL_GAIN_MODE;
        }
        break;

        case vcp_aics_set_atmtc_gain_mode_op:
        {
            id = VCP_INTERNAL_AICS_SET_AUTOMATIC_GAIN_MODE;
    }
    break;

        default:
        break;
    }

    return id;
}

/****************************************************************************/
static void vcpProfileAicsSendInternalMsg(VCP *vcp_inst,
                                          ServiceHandle srvc_hndl,
                                          vcp_aics_control_point_opcodes_t opcode,
                                          int8 gain_setting)
{
    MAKE_VCP_INTERNAL_MESSAGE(VCP_INTERNAL_SET_GAIN_SETTING);

    message->id = vcpAicsGetInternalMessageIdFromOpcode(opcode);
    message->prfl_hndl = vcp_inst->vcp_srvc_hdl;
    message->srvc_hndl = srvc_hndl;
    message->gain_setting = gain_setting;

    VcpMessageSendConditionally(vcp_inst->lib_task,
                                vcpAicsGetInternalMessageIdFromOpcode(opcode),
                                message,
                                &vcp_inst->pending_op);
}

/****************************************************************************/
static void vcpAicsAudioInputControlPointOp(VCP * vcp_inst,
                                            int16 gain_setting,
                                            ServiceHandle aics_clnt_hndl,
                                            vcp_aics_control_point_opcodes_t opcode)
{
    if (aics_clnt_hndl)
    {
        /* Check if the client handle is a valid one */
        if (vcpIsValidAicsInst(vcp_inst, aics_clnt_hndl))
        {
            vcpProfileAicsSendInternalMsg(vcp_inst,
                                          aics_clnt_hndl,
                                          opcode,
                                          (int8)gain_setting);
        }
        else
        {
            vcpSendAudioInputControlPointOpCfm(vcp_inst,
                                               aics_clnt_hndl,
                                               CSR_BT_GATT_RESULT_INTERNAL_ERROR,
                                               vcpGetAicsMessageIdFromOpcode(opcode));
        }
    }
    else
    {
        vcp_aics_srvc_hndl_t *ptr = vcp_inst->first_aics_srvc_hndl;

        while(ptr)
        {
            vcpProfileAicsSendInternalMsg(vcp_inst,
                                          ptr->srvc_hdnl,
                                          opcode,
                                          (int8)gain_setting);
            ptr = ptr->next;
        }
    }
}

/****************************************************************************/
void VcpSetGainSettingRequest(VcpProfileHandle profileHandle,
                              int16 gainSetting,
                              ServiceHandle aicsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpAicsAudioInputControlPointOp(vcp_inst,
                                        gainSetting,
                                        aicsClntHndl,
                                        vcp_aics_set_gain_setting_op);
    }
    else
    {
        VCP_DEBUG("Invalid profile_handle\n");
    }
}

/****************************************************************************/
void vcpHandleAicsUnmuteOp(VCP *vcp_inst,
                           const GattAicsClientUnmuteCfm *msg)
{
    vcpHandleAicsControlPointOpCfm(vcp_inst,
                                   msg->srvcHndl,
                                   msg->status,
                                   vcp_aics_unmute_op);
}

/****************************************************************************/
void VcpAicsSetUnmuteRequest(VcpProfileHandle profileHandle,
                             ServiceHandle aicsClntHndl)

{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpAicsAudioInputControlPointOp(vcp_inst,
                                        0,
                                        aicsClntHndl,
                                        vcp_aics_unmute_op);
    }
    else
    {
        VCP_DEBUG("Invalid profile_handle\n");
    }
}

/****************************************************************************/
void vcpHandleAicsMuteOp(VCP *vcp_inst,
                         const GattAicsClientMuteCfm *msg)
{
    vcpHandleAicsControlPointOpCfm(vcp_inst,
                                   msg->srvcHndl,
                                   msg->status,
                                   vcp_aics_mute_op);
}

/****************************************************************************/
void VcpAicsSetMuteRequest(VcpProfileHandle profileHandle,
                           ServiceHandle aicsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpAicsAudioInputControlPointOp(vcp_inst,
                                        0,
                                        aicsClntHndl,
                                        vcp_aics_mute_op);
    }
    else
    {
        VCP_DEBUG("Invalid profile_handle\n");
    }
}

/****************************************************************************/
void vcpHandleAicsSetManualGainModeOp(VCP *vcp_inst,
                                      const GattAicsClientSetManualGainModeCfm *msg)
{
    vcpHandleAicsControlPointOpCfm(vcp_inst,
                                   msg->srvcHndl,
                                   msg->status,
                                   vcp_aics_set_mnl_gain_mode_op);
}

/****************************************************************************/
void VcpSetManualGainModeRequest(VcpProfileHandle profileHandle,
                                 ServiceHandle aicsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpAicsAudioInputControlPointOp(vcp_inst,
                                        0,
                                        aicsClntHndl,
                                        vcp_aics_set_mnl_gain_mode_op);
    }
    else
    {
        VCP_DEBUG("Invalid profile_handle\n");
    }
}

/****************************************************************************/
void vcpHandleAicsSetAutomaticGainModeOp(VCP *vcp_inst,
                                         const GattAicsClientSetAutomaticGainModeCfm *msg)
{
    vcpHandleAicsControlPointOpCfm(vcp_inst,
                                   msg->srvcHndl,
                                   msg->status,
                                   vcp_aics_set_atmtc_gain_mode_op);
}

/****************************************************************************/
void VcpSetAutomaticGainModeRequest(VcpProfileHandle profileHandle,
                                    ServiceHandle aicsClntHndl)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst)
    {
        vcpAicsAudioInputControlPointOp(vcp_inst,
                                        0,
                                        aicsClntHndl,
                                        vcp_aics_set_atmtc_gain_mode_op);
    }
    else
    {
        VCP_DEBUG("Invalid profile_handle\n");
    }
}

/****************************************************************************/
static void vcpSendSetAudioInputDescCfm(VCP *vcp_inst,
                                        ServiceHandle srvc_hndl,
                                        status_t status)
{
    MAKE_VCP_MESSAGE(VcpSetAudioInputDescCfm);

    message->id = VCP_SET_AUDIO_INPUT_DESC_CFM;
    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = srvc_hndl;
    message->status = status;

    VcpMessageSend(vcp_inst->app_task, message);
}

/****************************************************************************/
void vcpHandleAicsSetAudioInputDescCfm(VCP *vcp_inst,
                                       const GattAicsClientSetAudioInputDescCfm *msg)
{
    vcpSendSetAudioInputDescCfm(vcp_inst,
                                msg->srvcHndl,
                                msg->status);
}

/****************************************************************************/
void VcpSetAudioInputDescRequest(VcpProfileHandle profileHandle,
                                 uint16           valueSize,
                                 uint8           *audioInputDescVal,
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
                GattAicsClientSetAudioInputDescReq(aicsClntHndl,
                                                   valueSize,
                                                   audioInputDescVal);
            }
            else
            {
                vcpSendSetAudioInputDescCfm(vcp_inst,
                                            aicsClntHndl,
                                            CSR_BT_GATT_RESULT_INTERNAL_ERROR);
            }
        }
        else
        {
            vcp_aics_srvc_hndl_t *ptr = vcp_inst->first_aics_srvc_hndl;

            while(ptr)
            {
                GattAicsClientSetAudioInputDescReq(ptr->srvc_hdnl,
                                                   valueSize,
                                                   audioInputDescVal);
                ptr = ptr->next;
            }
        }
    }
    else
    {
        VCP_DEBUG("Invalid profileHandle\n");
    }
}
#endif
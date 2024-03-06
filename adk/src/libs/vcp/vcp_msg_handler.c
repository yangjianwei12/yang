/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt.h>
#include <gatt_vcs_client.h>
#include <gatt_vocs_client.h>
#include <gatt_aics_client.h>

#include "vcp_private.h"
#include "vcp_msg_handler.h"
#include "vcp_debug.h"
#include "vcp_init.h"
#include "vcp_destroy.h"
#include "vcp_indication.h"
#include "vcp_read.h"
#include "vcp_write.h"
#include "vcp_notification.h"
#include "vcp_common.h"

/****************************************************************************/
static void vcpHandleDiscoverPrimaryServiceResp(VCP *vcp_inst,
                                                const GATT_DISCOVER_PRIMARY_SERVICE_CFM_T * message)
{
    if (message->status == gatt_status_success)
    {
        if(message->uuid[0] == VCP_VCS_UUID)
        {
            GattVcsClientInitData init_data;

            init_data.cid = message->cid;
            init_data.startHandle = message->handle;
            init_data.endHandle = message->end;

            vcp_inst->start_handle = message->handle;
            vcp_inst->end_handle = message->end;

            GattVcsClientInitReq(&vcp_inst->lib_task, &init_data, NULL);
        }
        else
        {
            vcpSendInitCfm(vcp_inst, VCP_STATUS_DISCOVERY_ERR);

            if(!ServiceHandleFreeInstanceData(vcp_inst->vcp_srvc_hdl))
            {
                VCP_PANIC(("Freeing of memory instance failed\n"));
            }
        }
    }
    else
    {
        vcpSendInitCfm(vcp_inst, VCP_STATUS_DISCOVERY_ERR);
        PanicFalse(ServiceHandleFreeInstanceData((ServiceHandle) vcp_inst->vcp_srvc_hdl));
    }
}

/****************************************************************************/
static void vcpHandleGattFindIncludedServicesResp(VCP *vcp_inst,
                                                  const GATT_FIND_INCLUDED_SERVICES_CFM_T * message)
{
    if (message->status == gatt_status_success)
    {
        if(message->uuid[0] == VCP_VOCS_UUID)
        {
            /* A new VOCS instance has been discovered: save it in the list
             * of the VOCS instances to initise.
             */
            GattVocsClientDeviceData inst_data;

            vcp_inst->vocs_counter += 1;

            inst_data.audioLocationCccHandle = 0;
            inst_data.audioLocationHandle = 0;
            inst_data.audioOutputDescriptionCccHandle = 0;
            inst_data.audioOutputDescriptionHandle = 0;
            inst_data.offsetStateCccHandle = 0;
            inst_data.offsetStateHandle = 0;
            inst_data.volumeOffsetControlPointHandle = 0;

            vcpAddVocsInst(vcp_inst,
                           &inst_data,
                           message->handle,
                           message->end);
        }
        else if (message->uuid[0] == VCP_AICS_UUID)
        {
            /* A new AICS instance has been discovered: save it in the list
             * of the AICS instances to initise.
             */
            GattAicsClientDeviceData inst_data;

            vcp_inst->aics_counter += 1;

            inst_data.audioInputControlPointHandle = 0;
            inst_data.audioInputDescriptionCccHandle = 0;
            inst_data.audioInputDescriptionHandle = 0;
            inst_data.gainSettingPropertiesHandle = 0;
            inst_data.inputStateCccHandle = 0;
            inst_data.inputStatusCccHandle = 0;
            inst_data.inputStatusHandle = 0;

            vcpAddAicsInst(vcp_inst,
                                  &inst_data,
                                  message->handle,
                                  message->end);
        }
        else
        {
            VCP_DEBUG_INFO(("Invalid UUID of an included service\n"));
        }
    }
    else
    {
        VcpDestroyReqAllInstList(vcp_inst);

        vcpSendInitCfm(vcp_inst, VCP_STATUS_DISCOVERY_ERR);

        if(!ServiceHandleFreeInstanceData(vcp_inst->vcp_srvc_hdl))
        {
            VCP_PANIC(("Freeing of memory instance failed\n"));
        }
        return;
    }

    if(!message->more_to_come)
    {
        vcp_inst->vocs_num = vcp_inst->vocs_counter;
        vcp_inst->aics_num = vcp_inst->aics_counter;
        vcpStartScndrSrvcInit(vcp_inst);
    }
}

/****************************************************************************/
static void vcpHandleGattMsg(Task task, MessageId id, Message msg)
{
    VCP *vcp_inst = (VCP *)task;

    switch (id)
    {
        case GATT_DISCOVER_PRIMARY_SERVICE_CFM:
        {
            vcpHandleDiscoverPrimaryServiceResp(vcp_inst,
                                                (const GATT_DISCOVER_PRIMARY_SERVICE_CFM_T *)msg);
        }
        break;

        case GATT_FIND_INCLUDED_SERVICES_CFM:
        {
            vcpHandleGattFindIncludedServicesResp(vcp_inst,
                                                  (const GATT_FIND_INCLUDED_SERVICES_CFM_T *)msg);
        }
        break;

        default:
        {
            /* Unrecognised GATT Manager message */
            VCP_DEBUG_PANIC(("Gatt Msg not handled [0x%x]\n", id));
        }
        break;
    }
}

/*************************************************************/
static void vcpHandleGattVcsClientMsg(Task task, MessageId id, Message msg)
{
    VCP *vcp_inst = (VCP *)task;

    switch (id)
    {
        case GATT_VCS_CLIENT_INIT_CFM:
        {
            vcpHandleVcsClientInitResp(vcp_inst,
                                       (const GattVcsClientInitCfm *)msg);
        }
        break;

        case GATT_VCS_CLIENT_TERMINATE_CFM:
        {
            vcpHandleVcsClientTerminateResp(vcp_inst,
                                            (const GattVcsClientTerminateCfm *)msg);
        }
        break;

        case GATT_VCS_CLIENT_VOLUME_STATE_SET_NTF_CFM:
        {
            vcpHandleVcsVolumeStateSetNtfCfm(vcp_inst,
                                             (const GattVcsClientVolumeStateSetNtfCfm *)msg);
        }
        break;

        case GATT_VCS_CLIENT_VOLUME_FLAG_SET_NTF_CFM:
        {
            vcpHandleVcsVolumeFlagSetNtfCfm(vcp_inst,
                                            (const GattVcsClientVolumeFlagSetNtfCfm *)msg);
        }
        break;

        case GATT_VCS_CLIENT_READ_VOLUME_STATE_CCC_CFM:
        {
            vcpHandleVcsReadVolumeStateCccCfm(vcp_inst,
                                              (const GattVcsClientReadVolumeStateCccCfm *)msg);
        }
        break;

        case GATT_VCS_CLIENT_READ_VOLUME_FLAG_CCC_CFM:
        {
            vcpHandleVcsReadVolumeFlagCccCfm(vcp_inst,
                                             (const GattVcsClientReadVolumeFlagCccCfm *)msg);
        }
        break;

        case GATT_VCS_CLIENT_READ_VOLUME_STATE_CFM:
        {
            vcpHandleVcsReadVolumeStateCfm(vcp_inst,
                                           (const GattVcsClientReadVolumeStateCfm*)msg);
        }
        break;

        case GATT_VCS_CLIENT_READ_VOLUME_FLAG_CFM:
        {
            vcpHandleVcsReadVolumeFlagCfm(vcp_inst,
                                          (const GattVcsClientReadVolumeFlagCfm *)msg);
        }
        break;

        case GATT_VCS_CLIENT_REL_VOL_DOWN_CFM:
        {
            vcpHandleVcsRelativeVolumeDownOp(vcp_inst,
                                             (const GattVcsClientRelVolDownCfm *)msg);
        }
        break;

        case GATT_VCS_CLIENT_REL_VOL_UP_CFM:
        {
            vcpHandleVcsRelativeVolumeUpOp(vcp_inst,
                                           (const GattVcsClientRelVolUpCfm *)msg);
        }
        break;

        case GATT_VCS_CLIENT_UNMUTE_REL_VOL_DOWN_CFM:
        {
            vcpHandleVcsUnmuteRelativeVolumeDownOp(vcp_inst,
                                                   (const GattVcsClientUnmuteRelVolDownCfm *)msg);
        }
        break;

        case GATT_VCS_CLIENT_UNMUTE_REL_VOL_UP_CFM:
        {
            vcpHandleVcsUnmuteRelativeVolumeUpOp(vcp_inst,
                                                 (const GattVcsClientUnmuteRelVolUpCfm *)msg);
        }
        break;

        case GATT_VCS_CLIENT_ABS_VOL_CFM:
        {
            vcpHandleVcsSetAbsoluteVolumeOp(vcp_inst,
                                            (const GattVcsClientAbsVolCfm *)msg);
        }
        break;

        case GATT_VCS_CLIENT_UNMUTE_CFM:
        {
            vcpHandleVcsUnmuteOp(vcp_inst,
                                 (const GattVcsClientUnmuteCfm *)msg);
        }
        break;

        case GATT_VCS_CLIENT_MUTE_CFM:
        {
            vcpHandleVcsMuteOp(vcp_inst,
                               (const GattVcsClientMuteCfm *)msg);
        }
        break;

        case GATT_VCS_CLIENT_VOLUME_STATE_IND:
        {
             vcpHandleVcsVolumeStateInd(vcp_inst,
                                        (const GattVcsClientVolumeStateInd*)msg);
        }
        break;

        case GATT_VCS_CLIENT_VOLUME_FLAG_IND:
        {
            vcpHandleVcsVolumeFlagInd(vcp_inst,
                                      (const GattVcsClientVolumeFlagInd *)msg);
        }

        default:
        {
            /* Unrecognised GATT VCS Client message */
            VCP_DEBUG_PANIC(("Gatt VCS Client Msg not handled [0x%x]\n", id));
        }
        break;
    }
}

/****************************************************************************/
static void vcpHandleGattVocsClientMsg(Task task, MessageId id, Message msg)
{
    VCP *vcp_inst = (VCP *)task;

    switch (id)
    {
        case GATT_VOCS_CLIENT_INIT_CFM:
        {
            vcpHandleVocsClientInitResp(vcp_inst,
                                        (const GattVocsClientInitCfm *)msg);
        }
        break;

        case GATT_VOCS_CLIENT_TERMINATE_CFM:
        {
            vcpHandleVocsClientTerminateResp(vcp_inst,
                                             (const GattVocsClientTerminateCfm *)msg);
        }
        break;

        case GATT_VOCS_CLIENT_OFFSET_STATE_SET_NTF_CFM:
        {
            vcpHandleVocsOffsetStateSetNtfCfm(vcp_inst,
                                              (const GattVocsClientOffsetStateSetNtfCfm *)msg);
        }

        break;

        case GATT_VOCS_CLIENT_AUDIO_LOCATION_SET_NTF_CFM:
        {
           vcpHandleVocsAudioLocationSetNtfCfm(vcp_inst,
                                               (const GattVocsClientAudioLocationSetNtfCfm *)msg);
        }
        break;

        case GATT_VOCS_CLIENT_AUDIO_OUTPUT_DESC_SET_NTF_CFM:
        {
            vcpHandleVocsAudioOutputDescSetNtfCfm(vcp_inst,
                                                  (const GattVocsClientAudioOutputDescSetNtfCfm *)msg);
        }
        break;

        case GATT_VOCS_CLIENT_READ_OFFSET_STATE_CCC_CFM:
        {
            vcpHandleVocsReadOffsetStateCccCfm(vcp_inst,
                                               (const GattVocsClientReadOffsetStateCccCfm *)msg);
        }
        break;

        case GATT_VOCS_CLIENT_READ_AUDIO_LOCATION_CCC_CFM:
        {
            vcpHandleVocsReadAudioLocationCccCfm(vcp_inst,
                                                 (const GattVocsClientReadAudioLocationCccCfm *)msg);
        }
        break;

        case GATT_VOCS_CLIENT_READ_AUDIO_OUTPUT_DESC_CCC_CFM:
        {
            vcpHandleVocsReadAudioOutputDescCccCfm(vcp_inst,
                                                   (const GattVocsClientReadOffsetStateCccCfm *)msg);
        }
        break;

        case GATT_VOCS_CLIENT_READ_OFFSET_STATE_CFM:
        {
           vcpHandleVocsReadOffsetStateCfm(vcp_inst,
                                           (const GattVocsClientReadOffsetStateCfm *)msg);
        }
        break;

        case GATT_VOCS_CLIENT_READ_AUDIO_LOCATION_CFM:
        {
            vcpHandleVocsReadAudioLocationeCfm(vcp_inst,
                                                      (const GattVocsClientReadAudioLocationCfm *)msg);
        }
        break;

        case GATT_VOCS_CLIENT_READ_AUDIO_OUTPUT_DESC_CFM:
        {
            vcpHandleVocsReadAudioOutputDescCfm(vcp_inst,
                                                (const GattVocsClientReadAudioOutputDescCfm *)msg);
        }
        break;

        case GATT_VOCS_CLIENT_SET_VOLUME_OFFSET_CFM:
        {
            vcpHandleVocsSetVolOffsetOp(vcp_inst,
                                        (const GattVocsClientSetVolumeOffsetCfm *)msg);
        }
        break;

        case GATT_VOCS_CLIENT_SET_AUDIO_LOC_CFM:
        {
            vcpHandleVocsSetAudioLocCfm(vcp_inst,
                                        (const GattVocsClientSetAudioLocCfm *)msg);
        }
        break;

        case GATT_VOCS_CLIENT_SET_AUDIO_OUTPUT_DESC_CFM:
        {
            vcpHandleVocsSetAudioOutputDescCfm(vcp_inst,
                                               (const GattVocsClientSetAudioOutputDescCfm *) msg);
        }
        break;

        case GATT_VOCS_CLIENT_OFFSET_STATE_IND:
        {
            vcpHandleVocsOffsetStateInd(vcp_inst,
                                        (const GattVocsClientOffsetStateInd *)msg);
        }
        break;

        case GATT_VOCS_CLIENT_AUDIO_LOCATION_IND:
        {
            vcpHandleVocsAudioLocationInd(vcp_inst,
                                          (const GattVocsClientAudioLocationInd *)msg);
        }
        break;

        case GATT_VOCS_CLIENT_AUDIO_OUTPUT_DESC_IND:
        {
            vcpHandleVocsAudioOutputDescInd(vcp_inst,
                                            (const GattVocsClientAudioOutputDescInd *)msg);
        }
        break;

        default:
        {
            /* Unrecognised GATT VOCS Client message */
            VCP_DEBUG_PANIC(("Gatt VOCS Client Msg not handled [0x%x]\n", id));
        }
        break;
    }
}

/****************************************************************************/
static void vcpHandleGattAicsClientMsg(Task task, MessageId id, Message msg)
{
    VCP *vcp_inst = (VCP *)task;

    switch (id)
    {
        case GATT_AICS_CLIENT_INIT_CFM:
        {
            vcpHandleAicsClientInitResp(vcp_inst,
                                        (const GattAicsClientInitCfm *) msg);
        }
        break;

        case GATT_AICS_CLIENT_TERMINATE_CFM:
        {
            vcpHandleAicsClientTerminateResp(vcp_inst,
                                             (const GattAicsClientTerminateCfm *)msg);
        }
        break;

        case GATT_AICS_CLIENT_INPUT_STATE_SET_NTF_CFM:
        {
            vcpHandleAicsInputStateSetNtfCfm(vcp_inst,
                                             (const GattAicsClientInputStateSetNtfCfm *)msg);
        }
        break;

        case GATT_AICS_CLIENT_INPUT_STATUS_SET_NTF_CFM:
        {
            vcpHandleAicsInputStatusSetNtfCfm(vcp_inst,
                                              (const GattAicsClientInputStatusSetNtfCfm *)msg);
        }
        break;

        case GATT_AICS_CLIENT_AUDIO_INPUT_DESC_SET_NTF_CFM:
        {
            vcpHandleAicsAudioInputDescSetNtfCfm(vcp_inst,
                                                 (const GattAicsClientAudioInputDescSetNtfCfm *)msg);
        }
        break;

        case GATT_AICS_CLIENT_READ_INPUT_STATE_CCC_CFM:
        {
            vcpHandleAicsReadInputStateCccCfm(vcp_inst,
                                              (const GattAicsClientReadInputStateCccCfm *)msg);
        }
        break;

        case GATT_AICS_CLIENT_READ_INPUT_STATUS_CCC_CFM:
        {
            vcpHandleAicsReadInputStatusCccCfm(vcp_inst,
                                               (const GattAicsClientReadInputStatusCccCfm *)msg);
        }
        break;

        case GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CCC_CFM:
        {
            vcpHandleAicsReadAudioInputDescCccCfm(vcp_inst,
                                                  (const GattAicsClientReadAudioInputDescCccCfm *)msg);
        }
        break;

        case GATT_AICS_CLIENT_READ_INPUT_STATE_CFM:
        {
            vcpHandleAicsReadInputStateCfm(vcp_inst,
                                           (const GattAicsClientReadInputStateCfm *)msg);
        }
        break;

        case GATT_AICS_CLIENT_READ_GAIN_SET_PROPERTIES_CFM:
        {
            vcpHandleAicsReadGainSetPropertiesCfm(vcp_inst,
                                                  (const GattAicsClientReadGainSetPropertiesCfm *)msg);
        }
        break;

        case GATT_AICS_CLIENT_READ_INPUT_TYPE_CFM:
        {
            vcpHandleAicsReadInputTypeCfm(vcp_inst,
                                          (const GattAicsClientReadInputTypeCfm *)msg);
        }
        break;

        case GATT_AICS_CLIENT_READ_INPUT_STATUS_CFM:
        {
            vcpHandleAicsReadInputStatusCfm(vcp_inst,
                                            (const GattAicsClientReadInputStatusCfm *)msg);
        }
        break;

        case GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CFM:
        {
            vcpHandleAicsReadAudioInputDescCfm(vcp_inst,
                                               (const GattAicsClientReadAudioInputDescCfm *)msg);
        }
        break;

        case GATT_AICS_CLIENT_SET_GAIN_SETTING_CFM:
        {
            vcpHandleAicsSetGainSettingOp(vcp_inst,
                                          (const GattAicsClientSetGainSettingCfm *) msg);
        }
        break;

        case GATT_AICS_CLIENT_UNMUTE_CFM:
        {
            vcpHandleAicsUnmuteOp(vcp_inst,
                                  (const GattAicsClientUnmuteCfm *) msg);
        }
        break;

        case GATT_AICS_CLIENT_MUTE_CFM:
        {
            vcpHandleAicsMuteOp(vcp_inst,
                                (const GattAicsClientMuteCfm *)msg);
        }
        break;

        case GATT_AICS_CLIENT_SET_MANUAL_GAIN_MODE_CFM:
        {
            vcpHandleAicsSetManualGainModeOp(vcp_inst,
                                             (const GattAicsClientSetManualGainModeCfm *)msg);
        }
        break;

        case GATT_AICS_CLIENT_SET_AUTOMATIC_GAIN_MODE_CFM:
        {
            vcpHandleAicsSetAutomaticGainModeOp(vcp_inst,
                                                (const GattAicsClientSetAutomaticGainModeCfm *)msg);
        }
        break;

        case GATT_AICS_CLIENT_SET_AUDIO_INPUT_DESC_CFM:
        {
            vcpHandleAicsSetAudioInputDescCfm(vcp_inst,
                                              (const GattAicsClientSetAudioInputDescCfm *) msg);
        }
        break;

        case GATT_AICS_CLIENT_INPUT_STATE_IND:
        {
            vcpHandleAicsInputStateInd(vcp_inst,
                                       (const GattAicsClientInputStateInd *)msg);
        }
        break;

        case GATT_AICS_CLIENT_INPUT_STATUS_IND:
        {
            vcpHandleAicsInputStatusInd(vcp_inst,
                                        (const GattAicsClientInputStatusInd *)msg);
        }
        break;

        case GATT_AICS_CLIENT_AUDIO_INPUT_DESC_IND:
        {
            vcpHandleAicsAudioInputDescInd(vcp_inst,
                                           (const GattAicsClientAudioInputDescInd *)msg);
        }
        break;

        default:
        {
            /* Unrecognised GATT AICS Client message */
            VCP_DEBUG_PANIC(("Gatt AICS Client Msg not handled [0x%x]\n", id));
        }
        break;
    }
}

/***************************************************************************/
static void  vcpHandleInternalMessage(Task task, MessageId id, Message msg)
{
   VCP * vcp_inst = (VCP *)task;

    VCP_DEBUG_INFO(("Message id (%d)\n",id));

    if (vcp_inst)
    {
        switch(id)
        {
            case VCP_INTERNAL_REL_VOL_DOWN:
            {
                VCP_INTERNAL_REL_VOL_DOWN_T *message = (VCP_INTERNAL_REL_VOL_DOWN_T *) msg;

                vcp_inst->pending_op = vcp_pending_relative_volume_down_op;

                vcpVcsControlPointOp(message->prfl_hndl,
                                     vcp_relative_volume_down_op,
                                     0);
            }
            break;

            case VCP_INTERNAL_REL_VOL_UP:
            {
                VCP_INTERNAL_REL_VOL_UP_T *message = (VCP_INTERNAL_REL_VOL_UP_T *) msg;

                vcp_inst->pending_op = vcp_pending_relative_volume_up_op;

                vcpVcsControlPointOp(message->prfl_hndl,
                                     vcp_relative_volume_up_op,
                                     message->volume_setting);
            }
            break;

            case VCP_INTERNAL_UNMUTE_REL_VOL_DOWN:
            {
                VCP_INTERNAL_UNMUTE_REL_VOL_DOWN_T *message = (VCP_INTERNAL_UNMUTE_REL_VOL_DOWN_T *) msg;

                vcp_inst->pending_op = vcp_pending_unmute_relative_volume_down_op;

                vcpVcsControlPointOp(message->prfl_hndl,
                                     vcp_unmute_relative_volume_down_op,
                                     message->volume_setting);
            }
            break;

            case VCP_INTERNAL_UNMUTE_REL_VOL_UP:
            {
                VCP_INTERNAL_UNMUTE_REL_VOL_UP_T *message = (VCP_INTERNAL_UNMUTE_REL_VOL_UP_T *) msg;

                vcp_inst->pending_op = vcp_pending_unmute_relative_volume_up_op;

                vcpVcsControlPointOp(message->prfl_hndl,
                                     vcp_unmute_relative_volume_up_op,
                                     message->volume_setting);
            }
            break;

            case VCP_INTERNAL_ABS_VOL:
            {
                 VCP_INTERNAL_ABS_VOL_T *message = (VCP_INTERNAL_ABS_VOL_T *) msg;

                 vcp_inst->pending_op = vcp_pending_set_absolute_volume_op;

                 vcpVcsControlPointOp(message->prfl_hndl,
                                      vcp_set_absolute_volume_op,
                                      message->volume_setting);
            }
            break;

            case VCP_INTERNAL_UNMUTE:
            {
                VCP_INTERNAL_UNMUTE_T *message = (VCP_INTERNAL_UNMUTE_T *) msg;

                vcp_inst->pending_op = vcp_pending_unmute_op;

                vcpVcsControlPointOp(message->prfl_hndl,
                                     vcp_unmute_op,
                                     message->volume_setting);
            }
            break;

            case VCP_INTERNAL_MUTE:
            {
                VCP_INTERNAL_MUTE_T *message = (VCP_INTERNAL_MUTE_T *) msg;

                vcp_inst->pending_op = vcp_pending_mute_op;

                vcpVcsControlPointOp(message->prfl_hndl,
                                     vcp_mute_op,
                                     message->volume_setting);
            }
            break;

            case VCP_INTERNAL_SET_VOL_OFFSET:
            {
                VCP_INTERNAL_SET_VOL_OFFSET_T *message = (VCP_INTERNAL_SET_VOL_OFFSET_T *) msg;

                vcp_inst->pending_op = vcp_pending_set_volume_offset_op;
                vcp_inst->volume_offset_pending = message->volume_offset;

                vcpVocsControlPointOp(message->prfl_hndl,
                                      message->vocs_srvc_hndl,
                                      vcp_vocs_set_volume_offset_op,
                                      message->volume_offset);
            }
            break;

            case VCP_INTERNAL_SET_GAIN_SETTING:
            {
                VCP_INTERNAL_SET_GAIN_SETTING_T *message = (VCP_INTERNAL_SET_GAIN_SETTING_T *) msg;

                vcpAicsControlPointOp(message->prfl_hndl,
                                      message->srvc_hndl,
                                      vcp_aics_set_gain_setting_op,
                                      message->gain_setting);
            }
            break;

            case VCP_INTERNAL_AICS_UNMUTE:
            {
                VCP_INTERNAL_AICS_UNMUTE_T *message = (VCP_INTERNAL_AICS_UNMUTE_T *) msg;

                vcpAicsControlPointOp(message->prfl_hndl,
                                      message->srvc_hndl,
                                      vcp_aics_unmute_op,
                                      message->gain_setting);
            }
            break;

            case VCP_INTERNAL_AICS_MUTE:
            {
                VCP_INTERNAL_AICS_MUTE_T *message = (VCP_INTERNAL_AICS_MUTE_T *) msg;

                vcpAicsControlPointOp(message->prfl_hndl,
                                      message->srvc_hndl,
                                      vcp_aics_mute_op,
                                      message->gain_setting);
            }
            break;

            case VCP_INTERNAL_AICS_SET_MANUAL_GAIN_MODE:
            {
                VCP_INTERNAL_AICS_MUTE_T *message = (VCP_INTERNAL_AICS_MUTE_T *) msg;

                vcpAicsControlPointOp(message->prfl_hndl,
                                      message->srvc_hndl,
                                      vcp_aics_set_mnl_gain_mode_op,
                                      message->gain_setting);
            }
            break;

            case VCP_INTERNAL_AICS_SET_AUTOMATIC_GAIN_MODE:
            {
                VCP_INTERNAL_AICS_SET_AUTOMATIC_GAIN_MODE_T *message = (VCP_INTERNAL_AICS_SET_AUTOMATIC_GAIN_MODE_T *) msg;

                vcpAicsControlPointOp(message->prfl_hndl,
                                      message->srvc_hndl,
                                      vcp_aics_set_atmtc_gain_mode_op,
                                      message->gain_setting);
            }
            break;

            case VCP_INTERNAL_SET_INITIAL_VOL_OP:
            {
                VCP_INTERNAL_SET_INITIAL_VOL_OP_T *message = (VCP_INTERNAL_SET_INITIAL_VOL_OP_T *) msg;

                vcp_inst->pending_op = vcp_pending_set_initial_vol_op;
                vcp_inst->volume_setting_pending = message->initial_vol;

                VcpReadVolumeFlagRequest(message->prfl_hndl);
            }
            break;

            default:
            {
                /* Internal unrecognised messages */
                VCP_DEBUG_PANIC(("Unknown Message received from Internal To lib \n"));
            }
            break;
        }
    }
}

/****************************************************************************/
void vcpMsgHandler(Task task, MessageId id, Message msg)
{
    if ((id >= GATT_MESSAGE_BASE) && (id < GATT_MESSAGE_TOP))
    {
        vcpHandleGattMsg(task, id, msg);
    }
    /* Check message is internal Message */
    else if((id > VCP_INTERNAL_MSG_BASE) && (id < VCP_INTERNAL_MSG_TOP))
    {
        vcpHandleInternalMessage(task,id,msg);
    }
    else if ((id >= GATT_VCS_CLIENT_MESSAGE_BASE) && (id < GATT_VCS_CLIENT_MESSAGE_TOP))
    {
        vcpHandleGattVcsClientMsg(task, id, msg);
    }
    else if ((id >= GATT_VOCS_CLIENT_MESSAGE_BASE) && (id < GATT_VOCS_CLIENT_MESSAGE_TOP))
    {
        vcpHandleGattVocsClientMsg(task, id, msg);
    }
    else if ((id >= GATT_AICS_CLIENT_MESSAGE_BASE) && (id < GATT_AICS_CLIENT_MESSAGE_TOP))
    {
        vcpHandleGattAicsClientMsg(task, id, msg);
    }
    else
    {
        VCP_DEBUG_PANIC(("Profile Msg not handled [0x%x]\n", id));
    }
}


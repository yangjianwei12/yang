/* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "gatt_vcs_client.h"
#include "gatt_vocs_client.h"
#include "gatt_aics_client.h"

#include "vcp_private.h"
#include "vcp_debug.h"
#include "vcp_init.h"
#include "vcp_destroy.h"
#include "vcp_indication.h"
#include "vcp_read.h"
#include "vcp_write.h"
#include "vcp_notification.h"
#include "vcp_common.h"
#include "gatt_service_discovery_lib.h"

CsrBool vcpInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ProfileHandleListElm_t *profile_hndl_elm = (ProfileHandleListElm_t *)elem;
    ServiceHandle profile_handle = *(ServiceHandle *)data;

    return (profile_hndl_elm->profile_handle == profile_handle);
}

CsrBool vcpProfileHndlFindByBtConnId(CsrCmnListElm_t *elem, void *data)
{
    ProfileHandleListElm_t *profile_hndl_elm = (ProfileHandleListElm_t *)elem;
    CsrBtConnId     btConnId   = *(CsrBtConnId *) data;
    VCP *vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(profile_hndl_elm->profile_handle);

    if(vcp_inst)
    {
        return (vcp_inst->cid == btConnId);
    }

    return FALSE;
}

CsrBool vcpProfileHndlFindByVcsSrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ProfileHandleListElm_t *profile_hndl_elm = (ProfileHandleListElm_t *)elem;
    ServiceHandle vcs_srvc_hndl = *(ServiceHandle *)data;
    VCP *vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(profile_hndl_elm->profile_handle);

    if(vcp_inst)
        return (vcp_inst->vcs_srvc_hdl == vcs_srvc_hndl);

    return FALSE;
}


/****************************************************************************/
#if 0
static void vcpHandleGattFindIncludedServicesResp(VCP *vcp_inst,
                                                  const GATT_FIND_INCLUDED_SERVICES_CFM_T * message)
{
    if (message->status == gatt_status_success)
    {
        if(message->uuid[0] == VCP_VOCS_UUID)
        {
            /* A new VOCS instance has been discovered: save it in the list
             * of the VOCS instances to initise. */

            gatt_vocs_client_device_data_t inst_data;

            vcp_inst->vocs_counter += 1;

            inst_data.audio_location_ccc_handle = 0;
            inst_data.audio_location_handle = 0;
            inst_data.audio_output_description_ccc_handle = 0;
            inst_data.audio_output_description_handle = 0;
            inst_data.offset_state_ccc_handle = 0;
            inst_data.offset_state_handle = 0;
            inst_data.volume_offset_control_point_handle = 0;

            vcpAddVocsInst(vcp_inst,
                           &inst_data,
                           message->handle,
                           message->end);
        }
        else if (message->uuid[0] == VCP_AICS_UUID)
        {
            /* A new AICS instance has been discovered: save it in the list
             * of the AICS instances to initise. */

            gatt_aics_client_device_data_t inst_data;

            vcp_inst->aics_counter += 1;

            inst_data.audio_input_control_point_handle = 0;
            inst_data.audio_input_description_ccc_handle = 0;
            inst_data.audio_input_description_handle = 0;
            inst_data.gain_setting_properties_handle = 0;
            inst_data.input_state_ccc_handle = 0;
            inst_data.input_status_ccc_handle = 0;
            inst_data.input_status_handle = 0;

            vcpAddAicsInst(vcp_inst,
                                  &inst_data,
                                  message->handle,
                                  message->end);
        }
        else
        {
            VCP_ERROR("Invalid UUID of an included service\n");
        }
    }
    else
    {
        VcpDestroyReqAllInstList(vcp_inst);

        vcpSendInitCfm(vcp_inst, VCP_STATUS_DISCOVERY_ERR);
        return;
    }

    if(!message->more_to_come)
    {
        vcp_inst->vocs_num = vcp_inst->vocs_counter;
        vcp_inst->aics_num = vcp_inst->aics_counter;
        vcpStartScndrSrvcInit(vcp_inst);
    }
}
#endif

/****************************************************************************/
static void handleGattSrvcDiscMsg(vcp_main_inst *inst, void *msg)
{
    VCP *vcp_inst = NULL;
    ProfileHandleListElm_t* elem = NULL;
    GattSdPrim* prim = (GattSdPrim*)msg;

    switch (*prim)
    {
        case GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM:
        {
            GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *cfm =
                (GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *) msg;

            elem = VCP_FIND_PROFILE_HANDLE_BY_BTCONNID(inst->profile_handle_list, cfm->cid);
            if (elem)
                vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (vcp_inst == NULL)
                return;

            if ((cfm->result == GATT_SD_RESULT_SUCCESS) && cfm->srvcInfoCount)
            {
                GattVcsClientInitData init_data;
                VCP_DEBUG("(VCP) : Start Hndl = 0x%x, End Hndl = 0x%x, Id = 0x%x\n",
                    cfm->srvcInfo[0].startHandle, cfm->srvcInfo[0].endHandle, cfm->srvcInfo[0].srvcId);

                init_data.cid = cfm->cid;
                init_data.startHandle = cfm->srvcInfo[0].startHandle;
                init_data.endHandle = cfm->srvcInfo[0].endHandle;

                vcp_inst->start_handle = cfm->srvcInfo[0].startHandle;
                vcp_inst->end_handle = cfm->srvcInfo[0].endHandle;

                GattVcsClientInitReq(vcp_inst->lib_task, &init_data, NULL);
            }
            else
            {
                vcpSendInitCfm(vcp_inst, VCP_STATUS_DISCOVERY_ERR);
                VCP_REMOVE_SERVICE_HANDLE(inst->profile_handle_list, vcp_inst->vcp_srvc_hdl);
                FREE_VCP_CLIENT_INST(vcp_inst->vcp_srvc_hdl);
            }

            if (cfm->srvcInfoCount && cfm->srvcInfo)
                CsrPmemFree(cfm->srvcInfo);

            break;
        }

        default:
        {
            /* Unrecognised GATT Manager message */
            VCP_WARNING("Gatt SD Msg not handled \n");
        }
        break;
    }
}

/*************************************************************/
static void vcpHandleGattVcsClientMsg(vcp_main_inst *inst, void *msg)
{
    VCP * vcp_inst = NULL;
    ProfileHandleListElm_t* elem = NULL;
    GattVcsClientMessageId *prim = (GattVcsClientMessageId *)msg;

    VCP_INFO("vcpHandleGattVcsClientMsg: MESSAGE:GattVcsClientMessageId [0x%x]", *prim);

    switch (*prim)
    {
        case GATT_VCS_CLIENT_INIT_CFM:
        {
            const GattVcsClientInitCfm* message;
            message = (GattVcsClientInitCfm*) msg;
            /* Find vcp instance using connection_id_t */
            elem = VCP_FIND_PROFILE_HANDLE_BY_BTCONNID(inst->profile_handle_list,
                                                       message->cid);
            if (elem)
                vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (vcp_inst)
                vcpHandleVcsClientInitResp(vcp_inst,
                                       (const GattVcsClientInitCfm *)msg);
        }
        break;

        case GATT_VCS_CLIENT_TERMINATE_CFM:
        {
            const GattVcsClientTerminateCfm* message;
            message = (GattVcsClientTerminateCfm*) msg;
            /* Find vcp instance using connection_id_t */
            elem = VCP_FIND_PROFILE_HANDLE_BY_VCS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->srvcHndl);
            if (elem)
                vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(vcp_inst)
            {
                vcpHandleVcsClientTerminateResp(vcp_inst,
                                                (const GattVcsClientTerminateCfm *)msg);
            }
            else
            {
                VCP_PANIC("Invalid VCP Profile instance\n");
            }

            VCP_REMOVE_SERVICE_HANDLE(inst->profile_handle_list, elem->profile_handle);
        }
        break;

        case GATT_VCS_CLIENT_VOLUME_STATE_SET_NTF_CFM:
        {
            const GattVcsClientVolumeStateSetNtfCfm* message;
            message = (GattVcsClientVolumeStateSetNtfCfm*) msg;

            /* Find vcp instance using vcs service handle */
            elem = VCP_FIND_PROFILE_HANDLE_BY_VCS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->srvcHndl);
            if (elem)
                vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(vcp_inst)
            {
                vcpHandleVcsVolumeStateSetNtfCfm(vcp_inst,
                                                 (const GattVcsClientVolumeStateSetNtfCfm *)msg);
            }
            else
            {
                VCP_PANIC("Invalid VCP Profile instance\n");
            }
        }
        break;

        case GATT_VCS_CLIENT_VOLUME_FLAG_SET_NTF_CFM:
        {
            const GattVcsClientVolumeFlagSetNtfCfm* message;
            message = (GattVcsClientVolumeFlagSetNtfCfm*) msg;

            /* Find vcp instance using vcs service handle */
            elem = VCP_FIND_PROFILE_HANDLE_BY_VCS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->srvcHndl);
            if (elem)
                vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(vcp_inst)
            {
                vcpHandleVcsVolumeFlagSetNtfCfm(vcp_inst,
                                                (const GattVcsClientVolumeFlagSetNtfCfm *)msg);
            }
            else
            {
                VCP_PANIC("Invalid VCP Profile instance\n");
            }
        }
        break;

        case GATT_VCS_CLIENT_READ_VOLUME_STATE_CCC_CFM:
        {
            const GattVcsClientReadVolumeStateCccCfm* message;
            message = (GattVcsClientReadVolumeStateCccCfm*) msg;

            /* Find vcp instance using vcs service handle */
            elem = VCP_FIND_PROFILE_HANDLE_BY_VCS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->srvcHndl);
            if (elem)
                vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(vcp_inst)
            {
                vcpHandleVcsReadVolumeStateCccCfm(vcp_inst,
                                                  (const GattVcsClientReadVolumeStateCccCfm *)msg);
            }
            else
            {
                VCP_PANIC("Invalid VCP Profile instance\n");
            }
        }
        break;

        case GATT_VCS_CLIENT_READ_VOLUME_FLAG_CCC_CFM:
        {
            const GattVcsClientReadVolumeFlagCccCfm* message;
            message = (GattVcsClientReadVolumeFlagCccCfm*) msg;

            /* Find vcp instance using vcs service handle */
            elem = VCP_FIND_PROFILE_HANDLE_BY_VCS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->srvcHndl);
            if (elem)
                vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(vcp_inst)
            {
                vcpHandleVcsReadVolumeFlagCccCfm(vcp_inst,
                                                 (const GattVcsClientReadVolumeFlagCccCfm *)msg);
            }
            else
            {
                VCP_PANIC("Invalid VCP Profile instance\n");
            }
        }
        break;

        case GATT_VCS_CLIENT_READ_VOLUME_STATE_CFM:
        {
            const GattVcsClientReadVolumeStateCfm* message;
            message = (GattVcsClientReadVolumeStateCfm*) msg;

            /* Find vcp instance using vcs service handle */
            elem = VCP_FIND_PROFILE_HANDLE_BY_VCS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->svcHndl);
            if (elem)
                vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(vcp_inst)
            {
                vcpHandleVcsReadVolumeStateCfm(vcp_inst,
                                               (const GattVcsClientReadVolumeStateCfm*)msg);
            }
            else
            {
                VCP_PANIC("Invalid VCP Profile instance\n");
            }
        }
        break;

        case GATT_VCS_CLIENT_READ_VOLUME_FLAG_CFM:
        {
            const GattVcsClientReadVolumeFlagCfm* message;
            message = (GattVcsClientReadVolumeFlagCfm*) msg;

            /* Find vcp instance using vcs service handle */
            elem = VCP_FIND_PROFILE_HANDLE_BY_VCS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->srvcHndl);
            if (elem)
                vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(vcp_inst)
            {
                vcpHandleVcsReadVolumeFlagCfm(vcp_inst,
                                              (const GattVcsClientReadVolumeFlagCfm *)msg);
            }
            else
            {
                VCP_PANIC("Invalid VCP Profile instance\n");
            }
        }
        break;

        case GATT_VCS_CLIENT_REL_VOL_DOWN_CFM:
        {
            const GattVcsClientRelVolDownCfm* message;
            message = (GattVcsClientRelVolDownCfm*) msg;

            /* Find vcp instance using vcs service handle */
            elem = VCP_FIND_PROFILE_HANDLE_BY_VCS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->srvcHndl);
            if (elem)
                vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(vcp_inst)
            {
                vcpHandleVcsRelativeVolumeDownOp(vcp_inst,
                                                 (const GattVcsClientRelVolDownCfm *)msg);
            }
            else
            {
                VCP_PANIC("Invalid VCP Profile instance\n");
            }
        }
        break;

        case GATT_VCS_CLIENT_REL_VOL_UP_CFM:
        {
            const GattVcsClientRelVolUpCfm* message;
            message = (GattVcsClientRelVolUpCfm*) msg;

            /* Find vcp instance using vcs service handle */
            elem = VCP_FIND_PROFILE_HANDLE_BY_VCS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->srvcHndl);
            if (elem)
                vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(vcp_inst)
            {
                vcpHandleVcsRelativeVolumeUpOp(vcp_inst,
                                               (const GattVcsClientRelVolUpCfm *)msg);
            }
            else
            {
                VCP_PANIC("Invalid VCP Profile instance\n");
            }
        }
        break;

        case GATT_VCS_CLIENT_UNMUTE_REL_VOL_DOWN_CFM:
        {
            const GattVcsClientUnmuteRelVolDownCfm* message;
            message = (GattVcsClientUnmuteRelVolDownCfm*) msg;

            /* Find vcp instance using vcs service handle */
            elem = VCP_FIND_PROFILE_HANDLE_BY_VCS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->srvcHndl);
            if (elem)
                vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(vcp_inst)
            {
                vcpHandleVcsUnmuteRelativeVolumeDownOp(vcp_inst,
                                                       (const GattVcsClientUnmuteRelVolDownCfm *)msg);
            }
            else
            {
                VCP_PANIC("Invalid VCP Profile instance\n");
            }
        }
        break;

        case GATT_VCS_CLIENT_UNMUTE_REL_VOL_UP_CFM:
        {
            const GattVcsClientUnmuteRelVolUpCfm* message;
            message = (GattVcsClientUnmuteRelVolUpCfm*) msg;

            /* Find vcp instance using vcs service handle */
            elem = VCP_FIND_PROFILE_HANDLE_BY_VCS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->srvcHndl);
            if (elem)
                vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(vcp_inst)
            {
                vcpHandleVcsUnmuteRelativeVolumeUpOp(vcp_inst,
                                                     (const GattVcsClientUnmuteRelVolUpCfm *)msg);
            }
            else
            {
                VCP_PANIC("Invalid VCP Profile instance\n");
            }
        }
        break;

        case GATT_VCS_CLIENT_ABS_VOL_CFM:
        {
            const GattVcsClientAbsVolCfm* message;
            message = (GattVcsClientAbsVolCfm*) msg;

            /* Find vcp instance using vcs service handle */
            elem = VCP_FIND_PROFILE_HANDLE_BY_VCS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->srvcHndl);
            if (elem)
                vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(vcp_inst)
            {
                vcpHandleVcsSetAbsoluteVolumeOp(vcp_inst,
                                                (const GattVcsClientAbsVolCfm *)msg);
            }
            else
            {
                VCP_PANIC("Invalid VCP Profile instance\n");
            }
        }
        break;

        case GATT_VCS_CLIENT_UNMUTE_CFM:
        {
            const GattVcsClientUnmuteCfm* message;
            message = (GattVcsClientUnmuteCfm*) msg;

            /* Find vcp instance using vcs service handle */
            elem = VCP_FIND_PROFILE_HANDLE_BY_VCS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->srvcHndl);
            if (elem)
                vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(vcp_inst)
            {
                vcpHandleVcsUnmuteOp(vcp_inst,
                                     (const GattVcsClientUnmuteCfm *)msg);
            }
            else
            {
                VCP_PANIC("Invalid VCP Profile instance\n");
            }
        }
        break;

        case GATT_VCS_CLIENT_MUTE_CFM:
        {
            const GattVcsClientMuteCfm* message;
            message = (GattVcsClientMuteCfm*) msg;

            /* Find vcp instance using vcs service handle */
            elem = VCP_FIND_PROFILE_HANDLE_BY_VCS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->srvcHndl);
            if (elem)
                vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(vcp_inst)
            {
                vcpHandleVcsMuteOp(vcp_inst,
                                   (const GattVcsClientMuteCfm *)msg);
            }
            else
            {
                VCP_PANIC("Invalid VCP Profile instance\n");
            }
        }
        break;

        case GATT_VCS_CLIENT_VOLUME_STATE_IND:
        {
            const GattVcsClientVolumeStateInd* message;
            message = (GattVcsClientVolumeStateInd*) msg;

            /* Find vcp instance using vcs service handle */
            elem = VCP_FIND_PROFILE_HANDLE_BY_VCS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->srvcHndl);
            if (elem)
                vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(vcp_inst)
            {
                vcpHandleVcsVolumeStateInd(vcp_inst,
                                           (const GattVcsClientVolumeStateInd*)msg);
            }
            else
            {
                VCP_PANIC("Invalid VCP Profile instance\n");
            }
        }
        break;

        case GATT_VCS_CLIENT_VOLUME_FLAG_IND:
        {
            const GattVcsClientVolumeFlagInd* message;
            message = (GattVcsClientVolumeFlagInd*) msg;

            /* Find vcp instance using vcs service handle */
            elem = VCP_FIND_PROFILE_HANDLE_BY_VCS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->srvcHndl);
            if (elem)
                vcp_inst = FIND_VCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(vcp_inst)
            {
                vcpHandleVcsVolumeFlagInd(vcp_inst,
                                          (const GattVcsClientVolumeFlagInd *)msg);
            }
            else
            {
                VCP_PANIC("Invalid VCP Profile instance\n");
            }
        }

        default:
        {
            /* Unrecognised GATT VCS Client message */
            VCP_WARNING("Gatt VCS Client Msg not handled [0x%x]\n", *prim);
        }
        break;
    }
}

#ifndef EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
/****************************************************************************/
static void vcpHandleGattVocsClientMsg(vcp_main_inst *inst, void *msg)
{
    VCP * vcp_inst = NULL;
    GattVocsClientMessageId *prim = (GattVocsClientMessageId *)msg;

    if(vcp_inst)
    {
        switch (*prim)
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
                                                    (GattVocsClientReadAudioOutputDescCfm *)msg);
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
                VCP_WARNING("Gatt VOCS Client Msg not handled [0x%x]\n", *prim);
            }
            break;
        }
    }
    else
    {
        VCP_DEBUG("Invalid VCP Profile instance\n");
    }

    CSR_UNUSED(inst);
}
#endif

#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
/****************************************************************************/
static void vcpHandleGattAicsClientMsg(vcp_main_inst *inst, void *msg)
{
    VCP * vcp_inst = NULL;
    GattAicsClientMessageId *prim = (GattAicsClientMessageId *)msg;

    if(vcp_inst)
    {
        switch (*prim)
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
                                                   (GattAicsClientReadAudioInputDescCfm *)msg);
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
                VCP_WARNING("Gatt AICS Client Msg not handled [0x%x]\n", *prim);
            }
            break;
        }
    }
    else
    {
        VCP_PANIC("Invalid VCP Profile instance\n");
    }

    CSR_UNUSED(inst);
}
#endif

/***************************************************************************/
static void  vcpHandleInternalMessage(vcp_main_inst *inst, void *msg)
{
    VCP * vcp_inst = NULL;
    vcp_internal_msg_t *prim = (vcp_internal_msg_t *)msg;

    VCP_INFO("vcpHandleInternalMessage:Message id:vcp_internal_msg_t (%d)\n", *prim);

    if (inst)
    {
        switch(*prim)
        {
            case VCP_INTERNAL_REL_VOL_DOWN:
            {
                VCP_INTERNAL_REL_VOL_DOWN_T *message = (VCP_INTERNAL_REL_VOL_DOWN_T *)msg;

                vcp_inst = FIND_VCP_INST_BY_SERVICE_HANDLE(message->prfl_hndl);

                if(vcp_inst)
                {
                    vcp_inst->pending_op = vcp_pending_relative_volume_down_op;

                    vcpVcsControlPointOp(message->prfl_hndl,
                                         vcp_relative_volume_down_op,
                                         0);
                }
            }
            break;

            case VCP_INTERNAL_REL_VOL_UP:
            {
                VCP_INTERNAL_REL_VOL_UP_T *message = (VCP_INTERNAL_REL_VOL_UP_T *) msg;

                vcp_inst = FIND_VCP_INST_BY_SERVICE_HANDLE(message->prfl_hndl);

                if(vcp_inst)
                {
                    vcp_inst->pending_op = vcp_pending_relative_volume_up_op;

                    vcpVcsControlPointOp(message->prfl_hndl,
                                         vcp_relative_volume_up_op,
                                         message->volume_setting);
                }
            }
            break;

            case VCP_INTERNAL_UNMUTE_REL_VOL_DOWN:
            {
                VCP_INTERNAL_UNMUTE_REL_VOL_DOWN_T *message = (VCP_INTERNAL_UNMUTE_REL_VOL_DOWN_T *) msg;

                vcp_inst = FIND_VCP_INST_BY_SERVICE_HANDLE(message->prfl_hndl);

                if(vcp_inst)
                {
                    vcp_inst->pending_op = vcp_pending_unmute_relative_volume_down_op;

                    vcpVcsControlPointOp(message->prfl_hndl,
                                         vcp_unmute_relative_volume_down_op,
                                         message->volume_setting);
                }
            }
            break;

            case VCP_INTERNAL_UNMUTE_REL_VOL_UP:
            {
                VCP_INTERNAL_UNMUTE_REL_VOL_UP_T *message = (VCP_INTERNAL_UNMUTE_REL_VOL_UP_T *) msg;

                vcp_inst = FIND_VCP_INST_BY_SERVICE_HANDLE(message->prfl_hndl);


                if(vcp_inst)
                {
                    vcp_inst->pending_op = vcp_pending_unmute_relative_volume_up_op;

                    vcpVcsControlPointOp(message->prfl_hndl,
                                         vcp_unmute_relative_volume_up_op,
                                         message->volume_setting);
                }
            }
            break;

            case VCP_INTERNAL_ABS_VOL:
            {
                 VCP_INTERNAL_ABS_VOL_T *message = (VCP_INTERNAL_ABS_VOL_T *) msg;

                 vcp_inst = FIND_VCP_INST_BY_SERVICE_HANDLE(message->prfl_hndl);

                 if(vcp_inst)
                 {
                     vcp_inst->pending_op = vcp_pending_set_absolute_volume_op;

                     vcpVcsControlPointOp(message->prfl_hndl,
                                          vcp_set_absolute_volume_op,
                                          message->volume_setting);
                 }
            }
            break;

            case VCP_INTERNAL_UNMUTE:
            {
                VCP_INTERNAL_UNMUTE_T *message = (VCP_INTERNAL_UNMUTE_T *) msg;

                vcp_inst = FIND_VCP_INST_BY_SERVICE_HANDLE(message->prfl_hndl);

                if(vcp_inst)
                {
                    vcp_inst->pending_op = vcp_pending_unmute_op;

                    vcpVcsControlPointOp(message->prfl_hndl,
                                         vcp_unmute_op,
                                         message->volume_setting);
                }
            }
            break;

            case VCP_INTERNAL_MUTE:
            {
                VCP_INTERNAL_MUTE_T *message = (VCP_INTERNAL_MUTE_T *) msg;

                vcp_inst = FIND_VCP_INST_BY_SERVICE_HANDLE(message->prfl_hndl);

                if(vcp_inst)
                {
                    vcp_inst->pending_op = vcp_pending_mute_op;

                    vcpVcsControlPointOp(message->prfl_hndl,
                                         vcp_mute_op,
                                         message->volume_setting);
                }
            }
            break;

            case VCP_INTERNAL_SET_VOL_OFFSET:
            {
                VCP_INTERNAL_SET_VOL_OFFSET_T *message = (VCP_INTERNAL_SET_VOL_OFFSET_T *) msg;

                vcp_inst = FIND_VCP_INST_BY_SERVICE_HANDLE(message->prfl_hndl);

                if(vcp_inst)
                {
                    vcp_inst->pending_op = vcp_pending_set_volume_offset_op;
                    vcp_inst->volume_offset_pending = message->volume_offset;

                    vcpVocsControlPointOp(message->prfl_hndl,
                                          message->vocs_srvc_hndl,
                                          vcp_vocs_set_volume_offset_op,
                                          message->volume_offset);
                }
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

                vcp_inst = FIND_VCP_INST_BY_SERVICE_HANDLE(message->prfl_hndl);

                if(vcp_inst)
                {
                    vcp_inst->pending_op = vcp_pending_set_initial_vol_op;
                    vcp_inst->volume_setting_pending = message->initial_vol;

                    VcpReadVolumeFlagRequest(message->prfl_hndl);
                }
            }
            break;

            default:
            {
                /* Internal unrecognised messages */
                VCP_WARNING("Unknown Message received from Internal To lib \n");
            }
            break;
        }
    }
}

/****************************************************************************/
void vcpMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *msg = NULL;
    vcp_main_inst *inst = (vcp_main_inst * )*gash;

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case GATT_SRVC_DISC_PRIM:
                handleGattSrvcDiscMsg(inst, msg);
                break;
            case VCP_PRIM:
                vcpHandleInternalMessage(inst, msg);
                break;
            case VCS_CLIENT_PRIM:
                vcpHandleGattVcsClientMsg(inst, msg);
                break;
#ifndef EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
            case VOCS_CLIENT_PRIM:
                vcpHandleGattVocsClientMsg(inst, msg);
                break;
#endif				
#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
            case AICS_CLIENT_PRIM:
                vcpHandleGattAicsClientMsg(inst, msg);
                break;
#endif
            default:
                VCP_WARNING("Profile Msg not handled \n");
        }

        SynergyMessageFree(eventClass, msg);
    }
}


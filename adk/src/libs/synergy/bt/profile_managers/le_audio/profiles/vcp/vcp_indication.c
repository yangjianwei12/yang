/* Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "vcp.h"
#include "vcp_indication.h"

/****************************************************************************/
void vcpHandleVcsVolumeStateInd(VCP *vcp_inst,
                                const GattVcsClientVolumeStateInd *ind)
{
    MAKE_VCP_MESSAGE(VcpVolumeStateInd);

    message->id = VCP_VOLUME_STATE_IND;
    /* Update change counter value */
    vcp_inst->vcs_change_counter = ind->changeCounter;

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->volumeState = ind->volumeState;
    message->mute = ind->mute;
    message->changeCounter = ind->changeCounter;

    VcpMessageSend(vcp_inst->app_task, message);
}

/****************************************************************************/
void vcpHandleVcsVolumeFlagInd(VCP *vcp_inst,
                               const GattVcsClientVolumeFlagInd *ind)
{
    MAKE_VCP_MESSAGE(VcpVolumeFlagInd);

    message->id = VCP_VOLUME_FLAG_IND;
    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->volumeFlag = ind->volumeFlag;

    VcpMessageSend(vcp_inst->app_task, message);
}

#ifndef EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
/****************************************************************************/
void vcpHandleVocsOffsetStateInd(VCP *vcp_inst,
                                 const GattVocsClientOffsetStateInd *ind)
{
    MAKE_VCP_MESSAGE(VcpOffsetStateInd);

    /* Update change counter value */
    vcp_inst->vocs_change_counter = ind->changeCounter;

    message->id = VCP_OFFSET_STATE_IND;
    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = ind->srvcHdnl;
    message->volumeOffset = ind->volumeOffset;
    message->changeCounter = ind->changeCounter;

    VcpMessageSend(vcp_inst->app_task, message);
}

/****************************************************************************/
void vcpHandleVocsAudioLocationInd(VCP *vcp_inst,
                                   const GattVocsClientAudioLocationInd *ind)
{
    MAKE_VCP_MESSAGE(VcpAudioLocationInd);

    message->id = VCP_AUDIO_LOCATION_IND;
    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = ind->srvcHdnl;
    message->audioLocation = ind->audioLocation;

    VcpMessageSend(vcp_inst->app_task, message);
}

/****************************************************************************/
void vcpHandleVocsAudioOutputDescInd(VCP *vcp_inst,
                                     const GattVocsClientAudioOutputDescInd *ind)
{
    MAKE_VCP_MESSAGE_WITH_LEN(VcpAudioOutputDescInd,
                              ind->sizeValue);

    message->id = VCP_AUDIO_OUTPUT_DESC_IND;
    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = ind->srvcHdnl;
    message->sizeValue = ind->sizeValue;

    memcpy(message->audioOutputDesc, ind->audioOutputDesc, ind->sizeValue);

    VcpMessageSend(vcp_inst->app_task, message);
}
#endif

#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
/****************************************************************************/
void vcpHandleAicsInputStateInd(VCP *vcp_inst,
                                const GattAicsClientInputStateInd *ind)
{
    MAKE_VCP_MESSAGE(VcpInputStateInd);

    /* Update change counter value */
    vcp_inst->aics_change_counter = ind->changeCounter;

    message->id = VCP_INPUT_STATE_IND;
    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = ind->srvcHndl;
    message->gainSetting = ind->gainSetting;
    message->mute = ind->mute;
    message->gainMode = ind->gainMode;
    message->changeCounter = ind->changeCounter;

    VcpMessageSend(vcp_inst->app_task, message);
}

/****************************************************************************/
void vcpHandleAicsInputStatusInd(VCP *vcp_inst,
                                 const GattAicsClientInputStatusInd *ind)
{
    MAKE_VCP_MESSAGE(VcpInputStatusInd);

    message->id = VCP_INPUT_STATUS_IND;
    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = ind->srvcHndl;
    message->inputStatus = ind->inputStatus;

    VcpMessageSend(vcp_inst->app_task, message);
}

/****************************************************************************/
void vcpHandleAicsAudioInputDescInd(VCP *vcp_inst,
                                    const GattAicsClientAudioInputDescInd *ind)
{
    MAKE_VCP_MESSAGE_WITH_LEN(VcpAudioInputDescInd,
                              ind->sizeValue);

    message->id = VCP_AUDIO_INPUT_DESC_IND;
    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = ind->srvcHndl;
    message->sizeValue = ind->sizeValue;

    memcpy(message->audioInputDesc, ind->audioInputDesc, ind->sizeValue);

    VcpMessageSend(vcp_inst->app_task, message);
}
#endif

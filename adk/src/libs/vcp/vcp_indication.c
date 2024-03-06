/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <panic.h>

#include "vcp.h"
#include "vcp_indication.h"

/****************************************************************************/
void vcpHandleVcsVolumeStateInd(VCP *vcp_inst,
                                const GattVcsClientVolumeStateInd *ind)
{
    MAKE_VCP_MESSAGE(VcpVolumeStateInd);

    /* Update change counter value */
    vcp_inst->vcs_change_counter = ind->changeCounter;

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->volumeState = ind->volumeState;
    message->mute = ind->mute;
    message->changeCounter = ind->changeCounter;

    MessageSend(vcp_inst->app_task, VCP_VOLUME_STATE_IND, message);
}

/****************************************************************************/
void vcpHandleVcsVolumeFlagInd(VCP *vcp_inst,
                               const GattVcsClientVolumeFlagInd *ind)
{
    MAKE_VCP_MESSAGE(VcpVolumeFlagInd);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->volumeFlag = ind->volumeFlag;

    MessageSend(vcp_inst->app_task, VCP_VOLUME_FLAG_IND, message);
}

/****************************************************************************/
void vcpHandleVocsOffsetStateInd(VCP *vcp_inst,
                                 const GattVocsClientOffsetStateInd *ind)
{
    /* Update change counter value */
    vcp_inst->vocs_change_counter = ind->changeCounter;

    MAKE_VCP_MESSAGE(VcpOffsetStateInd);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = ind->srvcHdnl;
    message->volumeOffset = ind->volumeOffset;
    message->changeCounter = ind->changeCounter;

    MessageSend(vcp_inst->app_task, VCP_OFFSET_STATE_IND, message);
}

/****************************************************************************/
void vcpHandleVocsAudioLocationInd(VCP *vcp_inst,
                                   const GattVocsClientAudioLocationInd *ind)
{
    MAKE_VCP_MESSAGE(VcpAudioLocationInd);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = ind->srvcHdnl;
    message->audioLocation = ind->audioLocation;

    MessageSend(vcp_inst->app_task, VCP_AUDIO_LOCATION_IND, message);
}

/****************************************************************************/
void vcpHandleVocsAudioOutputDescInd(VCP *vcp_inst,
                                     const GattVocsClientAudioOutputDescInd *ind)
{
    MAKE_VCP_MESSAGE_WITH_LEN(VcpAudioOutputDescInd,
                              ind->sizeValue);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = ind->srvcHdnl;
    message->sizeValue = ind->sizeValue;

    memcpy(message->audioOutputDesc, ind->audioOutputDesc, ind->sizeValue);

    MessageSend(vcp_inst->app_task, VCP_AUDIO_OUTPUT_DESC_IND, message);
}

/****************************************************************************/
void vcpHandleAicsInputStateInd(VCP *vcp_inst,
                                const GattAicsClientInputStateInd *ind)
{
    /* Update change counter value */
    vcp_inst->aics_change_counter = ind->changeCounter;

    MAKE_VCP_MESSAGE(VcpInputStateInd);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = ind->srvcHndl;
    message->gainSetting = ind->gainSetting;
    message->mute = ind->mute;
    message->gainMode = ind->gainMode;
    message->changeCounter = ind->changeCounter;

    MessageSend(vcp_inst->app_task, VCP_INPUT_STATE_IND, message);
}

/****************************************************************************/
void vcpHandleAicsInputStatusInd(VCP *vcp_inst,
                                 const GattAicsClientInputStatusInd *ind)
{
    MAKE_VCP_MESSAGE(VcpInputStatusInd);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = ind->srvcHndl;
    message->inputStatus = ind->inputStatus;

    MessageSend(vcp_inst->app_task, VCP_INPUT_STATUS_IND, message);
}

/****************************************************************************/
void vcpHandleAicsAudioInputDescInd(VCP *vcp_inst,
                                    const GattAicsClientAudioInputDescInd *ind)
{
    MAKE_VCP_MESSAGE_WITH_LEN(VcpAudioInputDescInd,
                              ind->sizeValue);

    message->prflHndl = vcp_inst->vcp_srvc_hdl;
    message->srvcHndl = ind->srvcHndl;
    message->sizeValue = ind->sizeValue;

    memcpy(message->audioInputDesc, ind->audioInputDesc, ind->sizeValue);

    MessageSend(vcp_inst->app_task, VCP_AUDIO_INPUT_DESC_IND, message);
}

/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef VCP_NOTIFICATION_H_
#define VCP_NOTIFICATION_H_

#include <gatt_vcs_client.h>
#include <gatt_vocs_client.h>

#include "vcp_private.h"

/***************************************************************************
NAME
    vcpHandleVcsVolumeStateSetNtfCfm

DESCRIPTION
    Handle a GATT_VCS_CLIENT_VOLUME_STATE_SET_NTF_CFM message.
*/
void vcpHandleVcsVolumeStateSetNtfCfm(VCP *vcp_inst,
                                      const GattVcsClientVolumeStateSetNtfCfm *msg);
/***************************************************************************
NAME
    vcpHandleVcsVolumeFlagSetNtfCfm

DESCRIPTION
    Handle a GATT_VCS_CLIENT_VOLUME_FLAG_SET_NTF_CFM message.
*/
void vcpHandleVcsVolumeFlagSetNtfCfm(VCP *vcp_inst,
                                     const GattVcsClientVolumeFlagSetNtfCfm *msg);

/***************************************************************************
NAME
    vcpHandleVocsOffsetStateSetNtfCfm

DESCRIPTION
    Handle a GATT_VOCS_CLIENT_OFFSET_STATE_SET_NTF_CFM message.
*/
void vcpHandleVocsOffsetStateSetNtfCfm(VCP *vcp_inst,
                                       const GattVocsClientOffsetStateSetNtfCfm *msg);
/***************************************************************************
NAME
    vcpHandleVocsAudioLocationSetNtfCfm

DESCRIPTION
    Handle a GATT_VOCS_CLIENT_AUDIO_LOCATION_SET_NTF_CFM message.
*/
void vcpHandleVocsAudioLocationSetNtfCfm(VCP *vcp_inst,
                                         const GattVocsClientAudioLocationSetNtfCfm *msg);

/***************************************************************************
NAME
    vcpHandleVocsAudioOutputDescSetNtfCfm

DESCRIPTION
    Handle a GATT_VOCS_CLIENT_AUDIO_OUTPUT_DESC_SET_NTF_CFM message.
*/
void vcpHandleVocsAudioOutputDescSetNtfCfm(VCP *vcp_inst,
                                           const GattVocsClientAudioOutputDescSetNtfCfm *msg);

/***************************************************************************
NAME
    vcpHandleAicsInputStateSetNtfCfm

DESCRIPTION
    Handle a GATT_AICS_CLIENT_INPUT_STATE_SET_NTF_CFM message.
*/
void vcpHandleAicsInputStateSetNtfCfm(VCP *vcp_inst,
                                      const GattAicsClientInputStateSetNtfCfm *msg);

/***************************************************************************
NAME
    vcpHandleAicsInputStatusSetNtfCfm

DESCRIPTION
    Handle a GATT_AICS_CLIENT_INPUT_STATUS_SET_NTF_CFM message.
*/
void vcpHandleAicsInputStatusSetNtfCfm(VCP *vcp_inst,
                                       const GattAicsClientInputStatusSetNtfCfm *msg);

/***************************************************************************
NAME
    vcpHandleAicsAudioInputDescSetNtfCfm

DESCRIPTION
    Handle a GATT_AICS_CLIENT_AUDIO_INPUT_DESC_SET_NTF_CFM message.
*/
void vcpHandleAicsAudioInputDescSetNtfCfm(VCP *vcp_inst,
                                          const GattAicsClientAudioInputDescSetNtfCfm *msg);

#endif

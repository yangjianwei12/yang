/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef VCP_READ_H_
#define VCP_READ_H_

#include <gatt_vcs_client.h>

#include "vcp_private.h"

/***************************************************************************
NAME
    vcpHandleVcsReadVolumeStateCccCfm

DESCRIPTION
    Handles the GATT_VCS_CLIENT_READ_VOLUME_STATE_CCC_CFM message.
*/
void vcpHandleVcsReadVolumeStateCccCfm(VCP *vcp_inst,
                                       const GattVcsClientReadVolumeStateCccCfm *msg);

/***************************************************************************
NAME
    vcpHandleVcsReadVolumeFlagCccCfm

DESCRIPTION
    Handles the GATT_VCS_CLIENT_READ_VOLUME_FLAG_CCC_CFM message.
*/
void vcpHandleVcsReadVolumeFlagCccCfm(VCP *vcp_inst,
                                      const GattVcsClientReadVolumeFlagCccCfm *msg);

/***************************************************************************
NAME
    vcpHandleVcsReadVolumeStateCfm

DESCRIPTION
    Handles the GATT_VCS_CLIENT_READ_VOLUME_STATE_CFM messsage.
*/
void vcpHandleVcsReadVolumeStateCfm(VCP *vcp_inst,
                                    const GattVcsClientReadVolumeStateCfm *msg);

/***************************************************************************
NAME
    vcpHandleVcsReadVolumeStateCfm

DESCRIPTION
    Handles the GATT_VCS_CLIENT_READ_VOLUME_FLAG_CFM messsage.
*/
void vcpHandleVcsReadVolumeFlagCfm(VCP *vcp_inst,
                                   const GattVcsClientReadVolumeFlagCfm *msg);

/***************************************************************************
NAME
    vcpHandleVocsReadOffsetStateCccCfm

DESCRIPTION
    Handles the GATT_VOCS_CLIENT_READ_OFFSET_STATE_CCC_CFM message.
*/
void vcpHandleVocsReadOffsetStateCccCfm(VCP *vcp_inst,
                                        const GattVocsClientReadOffsetStateCccCfm *msg);

/***************************************************************************
NAME
    vcpHandleVocsReadAudioLocationCccCfm

DESCRIPTION
    Handles the GATT_VOCS_CLIENT_READ_AUDIO_LOCATION_CCC_CFM message.
*/
void vcpHandleVocsReadAudioLocationCccCfm(VCP *vcp_inst,
                                          const GattVocsClientReadAudioLocationCccCfm *msg);

/***************************************************************************
NAME
    vcpHandleVocsReadAudioOutputDescCccCfm

DESCRIPTION
    Handles the GATT_VOCS_CLIENT_READ_AUDIO_OUTPUT_DESC_CCC_CFM message.
*/
void vcpHandleVocsReadAudioOutputDescCccCfm(VCP *vcp_inst,
                                            const GattVocsClientReadAudioOutputDescCccCfm *msg);

/***************************************************************************
NAME    vcpHandleVocsReadOffsetStateCfm

DESCRIPTION
    Handles the GATT_VOCS_CLIENT_READ_OFFSET_STATE_CFM message.
*/
void vcpHandleVocsReadOffsetStateCfm(VCP *vcp_inst,
                                     const GattVocsClientReadOffsetStateCfm *msg);

/***************************************************************************
NAME    vcpHandleVocsReadAudioLocationeCfm

DESCRIPTION
    Handles the GATT_VOCS_CLIENT_READ_AUDIO_LOCATION_CFM message.
*/
void vcpHandleVocsReadAudioLocationeCfm(VCP *vcp_inst,
                                        const GattVocsClientReadAudioLocationCfm *msg);

/***************************************************************************
NAME    vcpHandleVocsReadAudioOutputDescCfm

DESCRIPTION
    Handles the GATT_VOCS_CLIENT_READ_AUDIO_OUTPUT_DESC_CFM message.
*/
void vcpHandleVocsReadAudioOutputDescCfm(VCP *vcp_inst,
                                         const GattVocsClientReadAudioOutputDescCfm *msg);

/***************************************************************************
NAME    vcpHandleAicsReadInputStateCccCfm

DESCRIPTION
    Handles the GATT_AICS_CLIENT_READ_INPUT_STATE_CCC_CFM message.
*/
void vcpHandleAicsReadInputStateCccCfm(VCP *vcp_inst,
                                       const GattAicsClientReadInputStateCccCfm *msg);

/***************************************************************************
NAME    vcpHandleAicsReadInputStatusCccCfm

DESCRIPTION
    Handles the GATT_AICS_CLIENT_READ_INPUT_STATUS_CCC_CFM  message.
*/
void vcpHandleAicsReadInputStatusCccCfm(VCP *vcp_inst,
                                        const GattAicsClientReadInputStatusCccCfm *msg);

/***************************************************************************
NAME    vcpHandleAicsReadAudioInputDescCccCfm

DESCRIPTION
    Handles the GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CCC_CFM  message.
*/
void vcpHandleAicsReadAudioInputDescCccCfm(VCP *vcp_inst,
                                           const GattAicsClientReadAudioInputDescCccCfm *msg);

/***************************************************************************
NAME    vcpHandleAicsReadInputStateCfm

DESCRIPTION
    Handles the GATT_AICS_CLIENT_READ_INPUT_STATE_CFM  message.
*/
void vcpHandleAicsReadInputStateCfm(VCP *vcp_inst,
                                    const GattAicsClientReadInputStateCfm *msg);

/***************************************************************************
NAME    vcpHandleAicsReadGainSetPropertiesCfm

DESCRIPTION
    Handles the GATT_AICS_CLIENT_READ_GAIN_SET_PROPERTIES_CFM  message.
*/
void vcpHandleAicsReadGainSetPropertiesCfm(VCP *vcp_inst,
                                           const GattAicsClientReadGainSetPropertiesCfm *msg);

/***************************************************************************
NAME    vcpHandleAicsReadInputTypeCfm

DESCRIPTION
    Handles the GATT_AICS_CLIENT_READ_INPUT_TYPE_CFM  message.
*/
void vcpHandleAicsReadInputTypeCfm(VCP *vcp_inst,
                                   const GattAicsClientReadInputTypeCfm *msg);

/***************************************************************************
NAME    vcpHandleAicsReadInputStatusCfm

DESCRIPTION
    Handles the GATT_AICS_CLIENT_READ_INPUT_STATUS_CFM message.
*/
void vcpHandleAicsReadInputStatusCfm(VCP *vcp_inst,
                                     const GattAicsClientReadInputStatusCfm *msg);

/***************************************************************************
NAME    vcpHandleAicsReadAudioInputDescCfm

DESCRIPTION
    Handles the GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CFM  message.
*/
void vcpHandleAicsReadAudioInputDescCfm(VCP *vcp_inst,
                                        const GattAicsClientReadAudioInputDescCfm *msg);

#endif

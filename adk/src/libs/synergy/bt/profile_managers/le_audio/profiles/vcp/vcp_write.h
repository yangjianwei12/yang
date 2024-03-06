/* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef VCP_WRITE_H_
#define VCP_WRITE_H_

#include "vcp_private.h"

/***************************************************************************
NAME
    vcpHandleVcsRelativeVolumeDownOp

DESCRIPTION
    Handle the GATT_VCS_CLIENT_REL_VOL_DOWN_CFM message.
*/
void vcpHandleVcsRelativeVolumeDownOp(VCP *vcp_inst,
                                      const GattVcsClientRelVolDownCfm *msg);

/***************************************************************************
NAME
    vcpHandleVcsRelativeVolumeUpOp

DESCRIPTION
    Handle the GATT_VCS_CLIENT_REL_VOL_UP_CFM message.
*/
void vcpHandleVcsRelativeVolumeUpOp(VCP *vcp_inst,
                                    const GattVcsClientRelVolUpCfm *msg);

/***************************************************************************
NAME
    vcpHandleVcsUnmuteRelativeVolumeDownOp

DESCRIPTION
    Handle the GATT_VCS_CLIENT_UNMUTE_REL_VOL_DOWN_CFM message.
*/
void vcpHandleVcsUnmuteRelativeVolumeDownOp(VCP *vcp_inst,
                                            const GattVcsClientUnmuteRelVolDownCfm *msg);

/***************************************************************************
NAME
    vcpHandleVcsUnmuteRelativeVolumeUpOp

DESCRIPTION
    Handle the GATT_VCS_CLIENT_UNMUTE_REL_VOL_UP_CFM message.
*/
void vcpHandleVcsUnmuteRelativeVolumeUpOp(VCP *vcp_inst,
                                          const GattVcsClientUnmuteRelVolUpCfm *msg);

/***************************************************************************
NAME
    vcpHandleVcsSetAbsoluteVolumeOp

DESCRIPTION
    Handle the GATT_VCS_CLIENT_ABS_VOL_CFM message.
*/
void vcpHandleVcsSetAbsoluteVolumeOp(VCP *vcp_inst,
                                     const GattVcsClientAbsVolCfm *msg);

/***************************************************************************
NAME
    vcpHandleVcsUnmuteOp

DESCRIPTION
    Handle the GATT_VCS_CLIENT_UNMUTE_CFM message.
*/
void vcpHandleVcsUnmuteOp(VCP *vcp_inst,
                          const GattVcsClientUnmuteCfm *msg);

/***************************************************************************
NAME
    vcpHandleVcsMuteOp

DESCRIPTION
    Handle the GATT_VCS_CLIENT_MUTE_CFM message.
*/
void vcpHandleVcsMuteOp(VCP *vcp_inst,
                        const GattVcsClientMuteCfm *msg);

/***************************************************************************
NAME
    vcpVcsControlPointOp

DESCRIPTION
    Perform the specified vcs control point opration.
*/
void vcpVcsControlPointOp(ServiceHandle profile_handle,
                          vcp_vcs_control_point_opcodes_t opcode,
                          uint8 volume_settting_operand);

/***************************************************************************
NAME
    vcpVocsControlPointOp

DESCRIPTION
    Perform the specified vocs control point opration.
*/
void vcpVocsControlPointOp(VcpProfileHandle profile_handle,
                           ServiceHandle vocs_srvc_hndl,
                           vcp_vocs_control_point_opcodes_t opcode,
                           int16 volume_offset_operand);

#ifndef EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
/***************************************************************************
NAME
    vcpHandleVocsSetVolOffsetOp

DESCRIPTION
    Handle the GATT_VOCS_CLIENT_SET_VOLUME_OFFSET_CFM message.
*/
void vcpHandleVocsSetVolOffsetOp(VCP *vcp_inst,
                                 const GattVocsClientSetVolumeOffsetCfm *msg);

/***************************************************************************
NAME
    vcpHandleVocsSetAudioLocCfm

DESCRIPTION
    Handle the GATT_VOCS_CLIENT_SET_AUDIO_LOC_CFM message.
*/
void vcpHandleVocsSetAudioLocCfm(VCP *vcp_inst,
                                 const GattVocsClientSetAudioLocCfm *msg);

/***************************************************************************
NAME
    vcpHandleVocsSetAudioOutputDescCfm

DESCRIPTION
    Handle the GATT_VOCS_CLIENT_SET_AUDIO_OUTPUT_DESC_CFM message.
*/
void vcpHandleVocsSetAudioOutputDescCfm(VCP *vcp_inst,
                                        const GattVocsClientSetAudioOutputDescCfm *msg);
#endif

#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
/***************************************************************************
NAME
    vcpHandleAicsSetGainSettingOp

DESCRIPTION
    Handle the GATT_AICS_CLIENT_SET_GAIN_SETTING_CFM message.
*/
void vcpHandleAicsSetGainSettingOp(VCP *vcp_inst,
                                   const GattAicsClientSetGainSettingCfm *msg);
#endif
/***************************************************************************
NAME
    vcpAicsControlPointOp

DESCRIPTION
    Perform the specified aics control point opration.
*/
void vcpAicsControlPointOp(VcpProfileHandle profile_handle,
                           ServiceHandle srvc_hndl,
                           vcp_aics_control_point_opcodes_t opcode,
                           int8 gain_settting_operand);

#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
/***************************************************************************
NAME
    vcpHandleAicsUnmuteOp

DESCRIPTION
    Handle the GATT_AICS_CLIENT_UNMUTE_CFM message.
*/
void vcpHandleAicsUnmuteOp(VCP *vcp_inst,
                           const GattAicsClientUnmuteCfm *msg);

/***************************************************************************
NAME
    vcpHandleAicsMuteOp

DESCRIPTION
    Handle the GATT_AICS_CLIENT_MUTE_CFM message.
*/
void vcpHandleAicsMuteOp(VCP *vcp_inst,
                         const GattAicsClientMuteCfm *msg);

/***************************************************************************
NAME
    vcpHandleAicsSetManualGainModeOp

DESCRIPTION
    Handle the GATT_AICS_CLIENT_SET_MANUAL_GAIN_MODE_CFM message.
*/
void vcpHandleAicsSetManualGainModeOp(VCP *vcp_inst,
                                      const GattAicsClientSetManualGainModeCfm *msg);

/***************************************************************************
NAME
    vcpHandleAicsSetAutomaticGainModeOp

DESCRIPTION
    Handle the GATT_AICS_CLIENT_SET_AUTOMATIC_GAIN_MODE_CFM message.
*/
void vcpHandleAicsSetAutomaticGainModeOp(VCP *vcp_inst,
                                         const GattAicsClientSetAutomaticGainModeCfm *msg);

/***************************************************************************
NAME
    vcpHandleAicsSetAudioInputDescCfm

DESCRIPTION
    Handle the GATT_AICS_CLIENT_SET_AUDIO_INPUT_DESC_CFM message.
*/
void vcpHandleAicsSetAudioInputDescCfm(VCP *vcp_inst,
                                       const GattAicsClientSetAudioInputDescCfm *msg);

#endif /* #ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE */
#endif /* #ifndef VCP_WRITE_H_ */

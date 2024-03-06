/* Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef VCP_INDICATION_H_
#define VCP_INDICATION_H_

#include "gatt_vcs_client.h"

#include "vcp_private.h"

/***************************************************************************
NAME
    vcpHandleVcsVolumeStateInd

DESCRIPTION
    Handle a GATT_VCS_CLIENT_VOLUME_STATE_IND message.
*/
void vcpHandleVcsVolumeStateInd(VCP *vcp_inst,
                                const GattVcsClientVolumeStateInd *ind);

/***************************************************************************
NAME
    vcpHandleVcsVolumeFlagInd

DESCRIPTION
    Handle a GATT_VCS_CLIENT_VOLUME_FLAG_IND message.
*/
void vcpHandleVcsVolumeFlagInd(VCP *vcp_inst,
                               const GattVcsClientVolumeFlagInd *ind);

#ifndef EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
/***************************************************************************
NAME
    vcpHandleVocsOffsetStateInd

DESCRIPTION
    Handle a GATT_VOCS_CLIENT_OFFSET_STATE_IND message.
*/
void vcpHandleVocsOffsetStateInd(VCP *vcp_inst,
                                 const GattVocsClientOffsetStateInd *ind);

/***************************************************************************
NAME
    vcpHandleVocsAudioLocationInd

DESCRIPTION
    Handle a GATT_VOCS_CLIENT_AUDIO_LOCATION_IND message.
*/
void vcpHandleVocsAudioLocationInd(VCP *vcp_inst,
                                   const GattVocsClientAudioLocationInd *ind);

/***************************************************************************
NAME
    vcpHandleVocsAudioOutputDescInd

DESCRIPTION
    Handle a GATT_VOCS_CLIENT_AUDIO_OUTPUT_DESC_IND message.
*/
void vcpHandleVocsAudioOutputDescInd(VCP *vcp_inst,
                                     const GattVocsClientAudioOutputDescInd *ind);
#endif

#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
/***************************************************************************
NAME
    vcpHandleAicsInputStateInd

DESCRIPTION
    Handle a GATT_AICS_CLIENT_INPUT_STATE_IND message.
*/
void vcpHandleAicsInputStateInd(VCP *vcp_inst,
                                const GattAicsClientInputStateInd *ind);

/***************************************************************************
NAME
    vcpHandleAicsInputStatusInd

DESCRIPTION
    Handle a GATT_AICS_CLIENT_INPUT_STATUS_IND message.
*/
void vcpHandleAicsInputStatusInd(VCP *vcp_inst,
                                 const GattAicsClientInputStatusInd *ind);

/***************************************************************************
NAME
    vcpHandleAicsAudioInputDescInd

DESCRIPTION
    Handle a VCP_AUDIO_INPUT_DESC_IND message.
*/
void vcpHandleAicsAudioInputDescInd(VCP *vcp_inst,
                                    const GattAicsClientAudioInputDescInd *ind);

#endif /* #ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE */
#endif /* #ifndef VCP_INDICATION_H_ */
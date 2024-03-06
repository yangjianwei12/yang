/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_VCS_CLIENT_READ_H_
#define GATT_VCS_CLIENT_READ_H_

#include "gatt_vcs_client_private.h"

/***************************************************************************
NAME
    vcsClientHandleInternalRead

DESCRIPTION
    Handles the internal VCS_CLIENT_INTERNAL_MSG_READ and VCS_CLIENT_INTERNAL_MSG_READ_CCC
    message.
*/
void vcsClientHandleInternalRead(const GVCSC * vcs_client,
                                               uint16 handle);

/***************************************************************************
NAME
    vcsSendReadVolumeStateCfm

DESCRIPTION
    Send GATT_VCS_CLIENT_READ_VOLUME_STATE_CFM message as a result of a
    reading of Volume State on the remote device.
*/
void vcsSendReadVolumeStateCfm(GVCSC *vcs_client,
                               gatt_status_t status,
                               const uint8 *value);

/***************************************************************************
NAME
    vcsSendReadVolumeFlagCfm

DESCRIPTION
    Sends a GATT_VCS_CLIENT_READ_VOLUME_FLAG_CFM message to the application task
    as a result of a reading of Volume Flag on the remote device.
*/
void vcsSendReadVolumeFlagCfm(GVCSC *vcs_client,
                              gatt_status_t status,
                              uint16 size_value,
                              const uint8 *value);
#endif

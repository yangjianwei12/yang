/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_VOCS_CLIENT_READ_H_
#define GATT_VOCS_CLIENT_READ_H_

#include "gatt_vocs_client_private.h"

/***************************************************************************
NAME
    vocsClientHandleInternalRead

DESCRIPTION
    Handles the internal VOCS_CLIENT_INTERNAL_MSG_READ and
    VOCS_CLIENT_INTERNAL_MSG_READ_CCC message.
*/
void vocsClientHandleInternalRead(const GVOCS * vocs_client, uint16 handle);

/***************************************************************************
NAME
    vocsSendReadOffsetStateCfm

DESCRIPTION
    Send GATT_VOCS_CLIENT_READ_OFFSET_STATE_CFM message as a result of a
    reading of Offset State on the remote device.
*/
void vocsSendReadOffsetStateCfm(GVOCS *vocs_client,
                                status_t status,
                                const uint8 *value);

/***************************************************************************
NAME
    vocsSendReadAudioLocationCfm

DESCRIPTION
    Sends a GATT_VOCS_CLIENT_READ_AUDIO_LOCATION_CFM message to the application task
    as a result of a reading of Audio Location on the remote device.
*/
void vocsSendReadAudioLocationCfm(GVOCS *vocs_client,
                                  status_t status,
                                  const uint8 *value);

/***************************************************************************
NAME
    vocsSendReadAudioOutputDescCfm

DESCRIPTION
    Sends a GATT_VOCS_CLIENT_READ_AUDIO_OUTPUT_DESC_CFM message to the application task
    as a result of a reading of Audio Outout Description on the remote device.
*/
void vocsSendReadAudioOutputDescCfm(GVOCS *vocs_client,
                                    status_t status,
                                    uint16 size_value,
                                    const uint8 *value);
#endif

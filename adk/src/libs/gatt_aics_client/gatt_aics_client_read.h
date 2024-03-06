/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_AICS_CLIENT_READ_H_
#define GATT_AICS_CLIENT_READ_H_

#include "gatt_aics_client_private.h"

/***************************************************************************
NAME
    aicsSendReadInputStateCfm

DESCRIPTION
    Send GATT_AICS_CLIENT_READ_INPUT_STATE_CFM message as a result of a
    reading of Input State on the remote device.
*/
void aicsSendReadInputStateCfm(GAICS *aics_client,
                               gatt_status_t status,
                               const uint8 *value);

/***************************************************************************
NAME
    aicsSendReadGainSetPropertiesCfm

DESCRIPTION
    Send GATT_AICS_CLIENT_READ_GAIN_SET_PROPERTIES_CFM message as a
    result of a reading of Gain SettingProperties on the remote device.
*/
void aicsSendReadGainSetPropertiesCfm(GAICS *aics_client,
                                      gatt_status_t status,
                                      const uint8 *value);
/***************************************************************************
NAME
    aicsSendReadInputTypeCfm

DESCRIPTION
    Send GATT_AICS_CLIENT_READ_INPUT_TYPE_CFM message as a
    result of a reading of Input Type on the remote device.
*/
void aicsSendReadInputTypeCfm(GAICS *aics_client,
                              gatt_status_t status,
                              const uint8 *value);

/***************************************************************************
NAME
    aicsSendReadInputStatusCfm

DESCRIPTION
    Send GATT_AICS_CLIENT_READ_INPUT_STATUS_CFM message as a
    result of a reading of Input Status on the remote device.
*/
void aicsSendReadInputStatusCfm(GAICS *aics_client,
                                gatt_status_t status,
                                const uint8 *value);

/***************************************************************************
NAME
    aicsSendReadAudioInputDescCfm

DESCRIPTION
    Send GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CFM message as a
    result of a reading of Input Status on the remote device.
*/
void aicsSendReadAudioInputDescCfm(GAICS *aics_client,
                                   gatt_status_t status,
                                   uint16 size_value,
                                   const uint8 *value);

#endif

/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*/

#ifndef MICP_READ_H_
#define MICP_READ_H_

#include "gatt_mics_client.h"

#include "micp_private.h"

/***************************************************************************
NAME
    micpHandleMicsReadMuteValueCccCfm

DESCRIPTION
    Handles the GATT_MICS_CLIENT_READ_MUTE_VALUE_CCC_CFM message.
*/
void micpHandleMicsReadMuteValueCccCfm(MICP *micp_inst,
                                       const GattMicsClientReadMuteValueCccCfm *msg);

/***************************************************************************
NAME
    micpHandleMicsReadMuteValueCfm

DESCRIPTION
    Handles the GATT_MICS_CLIENT_READ_MUTE_VALUE_CFM messsage.
*/
void micpHandleMicsReadMuteValueCfm(MICP *micp_inst,
                                    const GattMicsClientReadMuteValueCfm *msg);

/***************************************************************************
NAME    micpHandleAicsReadInputStateCccCfm

DESCRIPTION
    Handles the GATT_AICS_CLIENT_READ_INPUT_STATE_CCC_CFM message.
*/
void micpHandleAicsReadInputStateCccCfm(MICP *micp_inst,
                                       const GattAicsClientReadInputStateCccCfm *msg);

/***************************************************************************
NAME    micpHandleAicsReadInputStatusCccCfm

DESCRIPTION
    Handles the GATT_AICS_CLIENT_READ_INPUT_STATUS_CCC_CFM  message.
*/
void micpHandleAicsReadInputStatusCccCfm(MICP *micp_inst,
                                        const GattAicsClientReadInputStatusCccCfm *msg);

/***************************************************************************
NAME    micpHandleAicsReadAudioInputDescCccCfm

DESCRIPTION
    Handles the GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CCC_CFM  message.
*/
void micpHandleAicsReadAudioInputDescCccCfm(MICP *micp_inst,
                                           const GattAicsClientReadAudioInputDescCccCfm *msg);

/***************************************************************************
NAME    micpHandleAicsReadInputStateCfm

DESCRIPTION
    Handles the GATT_AICS_CLIENT_READ_INPUT_STATE_CFM  message.
*/
void micpHandleAicsReadInputStateCfm(MICP *micp_inst,
                                    const GattAicsClientReadInputStateCfm *msg);

/***************************************************************************
NAME    micpHandleAicsReadGainSetPropertiesCfm

DESCRIPTION
    Handles the GATT_AICS_CLIENT_READ_GAIN_SET_PROPERTIES_CFM  message.
*/
void micpHandleAicsReadGainSetPropertiesCfm(MICP *micp_inst,
                                           const GattAicsClientReadGainSetPropertiesCfm *msg);

/***************************************************************************
NAME    micpHandleAicsReadInputTypeCfm

DESCRIPTION
    Handles the GATT_AICS_CLIENT_READ_INPUT_TYPE_CFM  message.
*/
void micpHandleAicsReadInputTypeCfm(MICP *micp_inst,
                                   const GattAicsClientReadInputTypeCfm *msg);

/***************************************************************************
NAME    micpHandleAicsReadInputStatusCfm

DESCRIPTION
    Handles the GATT_AICS_CLIENT_READ_INPUT_STATUS_CFM  message.
*/
void micpHandleAicsReadInputStatusCfm(MICP *micp_inst,
                                     const GattAicsClientReadInputStatusCfm *msg);

/***************************************************************************
NAME    micpHandleAicsReadAudioInputDescCfm

DESCRIPTION
    Handles the GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CFM  message.
*/
void micpHandleAicsReadAudioInputDescCfm(MICP *micp_inst,
                                        GattAicsClientReadAudioInputDescCfm *msg);

#endif

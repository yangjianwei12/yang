/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*/

#ifndef MICP_INDICATION_H_
#define MICP_INDICATION_H_

#include "gatt_mics_client.h"

#include "micp_private.h"

/***************************************************************************
NAME
    micpHandleMicsMuteValueInd

DESCRIPTION
    Handle a GATT_MICS_CLIENT_MUTE_VALUE_IND message.
*/
void micpHandleMicsMuteValueInd(MICP *micp_inst,
                                const GattMicsClientMuteValueInd *ind);

/***************************************************************************
NAME
    micpHandleAicsInputStateInd

DESCRIPTION
    Handle a GATT_AICS_CLIENT_INPUT_STATE_IND message.
*/
void micpHandleAicsInputStateInd(MICP *micp_inst,
                                const GattAicsClientInputStateInd *ind);

/***************************************************************************
NAME
    micpHandleAicsInputStatusInd

DESCRIPTION
    Handle a GATT_AICS_CLIENT_INPUT_STATUS_IND message.
*/
void micpHandleAicsInputStatusInd(MICP *micp_inst,
                                 const GattAicsClientInputStatusInd *ind);

/***************************************************************************
NAME
    micpHandleAicsAudioInputDescInd

DESCRIPTION
    Handle a MICP_AUDIO_INPUT_DESC_IND message.
*/
void micpHandleAicsAudioInputDescInd(MICP *micp_inst,
                                    const GattAicsClientAudioInputDescInd *ind);

#endif

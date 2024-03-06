/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#ifndef GATT_MICS_CLIENT_READ_H_
#define GATT_MICS_CLIENT_READ_H_

#include "gatt_mics_client_private.h"

/***************************************************************************
NAME
    micsClientHandleInternalRead

DESCRIPTION
    Handles the internal MICS_CLIENT_INTERNAL_MSG_READ and MICS_CLIENT_INTERNAL_MSG_READ_CCC
    message.
*/
void micsClientHandleInternalRead(const GMICSC * mics_client,
                                               uint16 handle);

/***************************************************************************
NAME
    micsSendReadMuteValueCfm

DESCRIPTION
    Send GATT_MICS_CLIENT_READ_MUTE_VALUE_CFM message as a result of a
    reading of Mute characteristic on the remote device.
*/
void micsSendReadMuteValueCfm(GMICSC *mics_client,
                               status_t status,
                               const uint8 *value);

#endif

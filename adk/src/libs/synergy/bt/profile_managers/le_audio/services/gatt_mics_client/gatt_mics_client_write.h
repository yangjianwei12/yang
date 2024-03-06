/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#ifndef GATT_MICS_CLIENT_WRITE_H_
#define GATT_MICS_CLIENT_WRITE_H_


#include "gatt_mics_client.h"

/* Size of the Mute Characteristic in number of octets */
#define MICS_CLIENT_MUTE_CHARACTERISTIC_SIZE (1)

/***************************************************************************
NAME
    micsClientHandleInternalWrite

DESCRIPTION
    Handle the MICS_CLIENT_INTERNAL_MSG_WRITE message.
*/
void micsClientHandleInternalWrite(GMICSC *const mics_client,
                                  uint16 handle,
                                  uint16 size_value,
                                  uint8 * value);

#endif

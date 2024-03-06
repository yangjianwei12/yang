/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_AICS_CLIENT_WRITE_H_
#define GATT_AICS_CLIENT_WRITE_H_


#include "gatt_aics_client_private.h"

/***************************************************************************
NAME
    aicsClientHandleInternalWrite

DESCRIPTION
    Handle the AICS_CLIENT_INTERNAL_MSG_WRITE message.
*/
void aicsClientHandleInternalWrite(GAICS *const aics_client,
                                   uint16 handle,
                                   uint16 size_value,
                                   uint8 * value);

/***************************************************************************
NAME
    vocsClientHandleInternalWriteWithoutResponse

DESCRIPTION
    Perform a GATT Write without response.
*/
void aicsClientHandleInternalWriteWithoutResponse(GAICS *const aics_client,
                                                  uint16 handle,
                                                  uint16 size_value,
                                                  uint8 *value);

#endif

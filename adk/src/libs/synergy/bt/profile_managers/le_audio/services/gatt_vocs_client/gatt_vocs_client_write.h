/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_VOCS_CLIENT_WRITE_H_
#define GATT_VOCS_CLIENT_WRITE_H_

#include "gatt_vocs_client_private.h"

/***************************************************************************
NAME
    vocsClientHandleInternalWrite

DESCRIPTION
    Handle the VOCS_CLIENT_INTERNAL_MSG_WRITE message.
*/
void vocsClientHandleInternalWrite(GVOCS *const vocs_client,
                                   uint16 handle,
                                   uint16 size_value,
                                   uint8 * value);

/***************************************************************************
NAME
    vocsClientHandleInternalWriteWithoutResponse

DESCRIPTION
    Handle the VOCS_CLIENT_INTERNAL_MSG_AUDIO_LOC_WRITE message.
*/
void vocsClientHandleInternalWriteWithoutResponse(GVOCS *const vocs_client,
                                                  uint16 handle,
                                                  uint16 size_value,
                                                  uint8 *value);

#endif

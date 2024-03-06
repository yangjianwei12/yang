/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_MCS_CLIENT_WRITE_H_
#define GATT_MCS_CLIENT_WRITE_H_


#include "gatt_mcs_client.h"

/* Size of the Media Control Point Characteristic in number of octets
   for the opcodes without any parameters */
#define MCS_CLIENT_OPCODE_SIZE (1)

/* Size of the Media Control Point Characteristic in number of octects
   for the opcodes with values */
#define MCS_CLIENT_OPCODE_SIZE_PARAM (MCS_CLIENT_OPCODE_SIZE + sizeof(int32))

/***************************************************************************
NAME
    mcsClientHandleInternalWrite

DESCRIPTION
    Handle the MCS_CLIENT_INTERNAL_MSG_WRITE_REQ message.
*/
void mcsClientHandleInternalWrite(GMCSC *const mcsClient,
                                  MediaPlayerAttributeMask charac,
                                  uint16 sizeValue,
                                  uint8 * value);

void handleMcsWriteValueResp(GMCSC *mcsClient, uint16 handle, status_t resultCode);

void mcsClientHandleInternalSetMediaControlPoint(GMCSC *const mcsClient,
                                                 GattMcsOpcode op,
                                                 int32 val);

#endif

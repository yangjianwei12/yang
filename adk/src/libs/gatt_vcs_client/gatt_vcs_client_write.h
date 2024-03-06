/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_VCS_CLIENT_WRITE_H_
#define GATT_VCS_CLIENT_WRITE_H_


#include "gatt_vcs_client.h"

/* Size of the Volume Control Point Characteristic in number of octets
   for the opcodes 0x00-0x03 and 0x05-0x06 */
#define VCS_CLIENT_VOL_CTRL_POINT_CHARACTERISTIC_SIZE_1 (2)

/* Size of the Volume Control Point Characteristic in number of octects
   for the opcode 0x04 */
#define VCS_CLIENT_VOL_CTRL_POINT_CHARACTERISTIC_SIZE_2 (3)

/***************************************************************************
NAME
    vcsClientHandleInternalWrite

DESCRIPTION
    Handle the VCS_CLIENT_INTERNAL_MSG_WRITE message.
*/
void vcsClientHandleInternalWrite(GVCSC *const vcs_client,
                                  uint16 handle,
                                  uint16 size_value,
                                  uint8 * value);

#endif

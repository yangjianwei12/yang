/*******************************************************************************

Copyright (C) 2007 - 2016 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

REVISION:          $Revision: #1 $

*******************************************************************************/

#include "sdplib_private.h"

#ifdef INSTALL_PAN_MODULE

/******************************************************************************

    function sdp_get_uuid  --  get a UUID

******************************************************************************/

uint16_t sdp_get_uuid (const uint8_t *uuid, uint16_t uuidlen)
{
    switch (uuidlen)
    {
    case 2:
        return (uuid[0] << 8) | uuid[1];
    case 4:
        if ((uuid[0] | uuid[1]) == 0)
        {
            return (uuid[2] << 8) | uuid[3];
        }
        break;
    case 16:
        /*
         * The base UUID -- 00000000-0000-1000-8000-00805F9B34FB -- is given
         * in http://www.bluetooth.org/assigned-numbers/sdp.htm
         */
        if ((uuid[0] | uuid[1] |
             uuid[4] | uuid[5] |
             (uuid[6] ^ 0x10) | uuid[7] |
         (uuid[8] ^ 0x80) | uuid[9] |
             uuid[10] | (uuid[11] ^ 0x80) |
             (uuid[12] ^ 0x5f) | (uuid[13] ^ 0x9b) |
         (uuid[14] ^ 0x34) | (uuid[15] ^ 0xfb)) == 0)
        {
            return (uuid[2] << 8) | uuid[3];
        }
        break;
    }
    return NO_UUID;
}

#endif


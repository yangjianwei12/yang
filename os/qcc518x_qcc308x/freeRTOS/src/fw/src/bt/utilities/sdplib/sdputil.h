/*******************************************************************************

Copyright (C) 2007 - 2016 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

REVISION:          $Revision: #1 $

*******************************************************************************/
#ifndef _SDP_UTIL_H_
#define _SDP_UTIL_H_

#ifndef BUILD_FOR_HOST
#include "bt.h"
#else
#include "qbl_adapter_types.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef INSTALL_PAN_MODULE
/******************************************************************************

    function sdp_get_uuid  --  get a UUID

The UUID must have a length of 2, 4 or 16 octets and
be stored MSB first (i.e. big-endianly).
If the UUID can't be expressed as a 16-bit UUID, NO_UUID is returned
(note this value has been chosen to be unlikely to be a valid UUID).

Currently only used within bnep and thus guarded by INSTALL_PAN_MODULE

******************************************************************************/

extern uint16_t sdp_get_uuid (const uint8_t *uuid, uint16_t uuidlen);

#define NO_UUID 0

#endif

#ifdef __cplusplus
}
#endif 

#endif

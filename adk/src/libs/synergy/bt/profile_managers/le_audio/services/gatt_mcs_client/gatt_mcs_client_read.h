/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_MCS_CLIENT_READ_H_
#define GATT_MCS_CLIENT_READ_H_

#include "gatt_mcs_client_private.h"

/***************************************************************************
NAME
    mcsClientHandleInternalRead

DESCRIPTION
    Handles the internal MCS_CLIENT_INTERNAL_MSG_READ_REQ message.
*/
void mcsClientHandleInternalRead(const GMCSC * mcsClient,
                                 MediaPlayerAttribute charac);

void handleMcsReadValueResp(GMCSC *mcsClient, uint16 handle, status_t resultCode, uint16 valueLength, uint8 *value);
#endif

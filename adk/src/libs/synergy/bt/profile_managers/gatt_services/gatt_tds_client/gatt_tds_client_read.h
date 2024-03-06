/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_TDS_CLIENT_READ_H_
#define GATT_TDS_CLIENT_READ_H_

#include "gatt_tds_client_private.h"

/***************************************************************************
NAME
    tdsClientHandleInternalRead

DESCRIPTION
    Handles the internal TDS_CLIENT_INTERNAL_MSG_READ_REQ message.
*/
void tdsClientHandleInternalRead(const GTDSC * tdsClient,
                                 TdsCharAttribute charac);

void handleTdsReadValueResp(GTDSC *tdsClient, uint16 handle, status_t resultCode, uint16 valueLength, uint8 *value);
#endif

/* Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef CSIP_NOTIFICATION_H_
#define CSIP_NOTIFICATION_H_

#include "gatt_csis_client.h"

#include "csip_private.h"

/***************************************************************************
NAME
    csipSendCsisSetNtfCfm

DESCRIPTION
    Handle a GATT_CSIS_CLIENT_NOTIFICATION_CFM message.
*/

void csipSendCsisSetNtfCfm(Csip *csipInst, CsrBtResultCode status);

#endif

/* Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef CSIP_WRITE_H_
#define CSIP_WRITE_H_

#include "csip_private.h"

/***************************************************************************
NAME
    csipSendSetLockCfm

DESCRIPTION
    Handle the GATT_CSIS_CLIENT_WRITE_LOCK_CFM message.
*/
void csipSendSetLockCfm(Csip *csipInst,
                                CsrBtResultCode status);

#endif

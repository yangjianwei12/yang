/* Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef CSIP_READ_H_
#define CSIP_READ_H_

#include "gatt_csis_client.h"

#include "csip_private.h"

/***************************************************************************
NAME
    csipSendReadCSInfoCfm

DESCRIPTION
    Handles the GATT_CSIS_CLIENT_READ_CS_INFO_CFM message.
*/
void csipSendReadCSInfoCfm(Csip *csipInst,
						   CsrBtResultCode status,
						   CsipCsInfo csInfoType,
						   uint16 sizeValue,
						   const uint8 *value);

#endif

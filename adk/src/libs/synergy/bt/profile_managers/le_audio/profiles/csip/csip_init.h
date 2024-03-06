/* Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef CSIP_INIT_H_
#define CSIP_INIT_H_

#include "gatt_csis_client.h"

#include "csip_private.h"

/***************************************************************************
NAME
    csipSendInitCfm
    
DESCRIPTION
    Send a CSIP_INIT_CFM message to the application.
*/
void csipSendInitCfm(Csip * csipInst, CsipStatus status);

/***************************************************************************
NAME
    csipHandleCsisClientInitResp

DESCRIPTION
    Handle the GATT_CSIS_CLIENT_INIT_CFM message.
*/
void csipHandleCsisClientInitResp(Csip *csipInst,
                                const GattCsisClientInitCfm * message);

/***************************************************************************
NAME
    csipSendSetInitialVolOpCfm

DESCRIPTION
    Send the CSIP_SET_INITIAL_VOL_CFM message.
*/
void csipSendSetInitialVolOpCfm(Csip *csipInst,
                               CsipStatus status);

#endif

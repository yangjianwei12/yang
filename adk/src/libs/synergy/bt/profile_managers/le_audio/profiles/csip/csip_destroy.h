/* Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef CSIP_DESTROY_H_
#define CSIP_DESTROY_H_

#include "csip_private.h"

/***************************************************************************
NAME
    csipSendDestroyCfm
    
DESCRIPTION
    Send the CSIP_DESTROY_CFM message.
*/
void csipSendDestroyCfm(Csip* csipInst, CsipStatus status, CsipHandles* handles);


/***************************************************************************
NAME
    csipHandleCsisClientTerminateResp

DESCRIPTION
    Handle the GATT_CSIS_CLIENT_TERMINATE_CFM message.
*/
void csipHandleCsisClientTerminateResp(Csip *csipInst,
                                     const GattCsisClientTerminateCfm * message);

/***************************************************************************
NAME
    csipDestroyProfileInst

DESCRIPTION
    Destroy the profile memory instance.
*/
void csipDestroyProfileInst(Csip *csipInst, CsipHandles* handles);

#endif

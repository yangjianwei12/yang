/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_MCS_CLIENT_INIT_H_
#define GATT_MCS_CLIENT_INIT_H_

#include "gatt_mcs_client_private.h"

/***************************************************************************
NAME
    gattMcsClientSendInitCfm
    
DESCRIPTION
    Send a GATT_MCS_CLIENT_INIT_CFM message to the registered client task with
    the supplied status code.
*/
void gattMcsClientSendInitCfm(GMCSC *gattMcsClient,
                              GattMcsClientStatus status);

#endif

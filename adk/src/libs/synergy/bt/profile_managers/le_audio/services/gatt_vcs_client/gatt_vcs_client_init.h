/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_VCS_CLIENT_INIT_H_
#define GATT_VCS_CLIENT_INIT_H_

#include "gatt_vcs_client_private.h"

/***************************************************************************
NAME
    gattVcsClientSendInitCfm
    
DESCRIPTION
    Send a GATT_VCS_CLIENT_INIT_CFM message to the registered client task with
    the supplied status code.
*/
void gattVcsClientSendInitCfm(GVCSC *gatt_vcs_client,
                              GattVcsClientStatus status);

#endif

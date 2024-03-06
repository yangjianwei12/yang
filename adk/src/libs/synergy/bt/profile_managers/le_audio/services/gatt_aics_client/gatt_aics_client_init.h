/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_AICS_CLIENT_INIT_H_
#define GATT_AICS_CLIENT_INIT_H_

#include "gatt_aics_client_private.h"

/***************************************************************************
NAME
    gattAicsClientSendInitCfm
    
DESCRIPTION
    Send a GATT_AICS_CLIENT_INIT_CFM message to the registered client task with
    the supplied status code.
*/
void gattAicsClientSendInitCfm(GAICS *gatt_aics_client,
                               GattAicsClientStatus status);

#endif

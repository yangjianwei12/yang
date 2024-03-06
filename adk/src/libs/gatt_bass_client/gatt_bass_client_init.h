/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_BASS_CLIENT_INIT_H_
#define GATT_BASS_CLIENT_INIT_H_

#include "gatt_bass_client_private.h"
#include "gatt_bass_client.h"

/***************************************************************************
NAME
    gattBassClientSendInitCfm
    
DESCRIPTION
    Send a GATT_BASS_CLIENT_INIT_CFM message to the registered client task with
    the supplied status code.
*/
void gattBassClientSendInitCfm(GBASSC *gatt_bass_client,
                               GattBassClientStatus status);

#endif

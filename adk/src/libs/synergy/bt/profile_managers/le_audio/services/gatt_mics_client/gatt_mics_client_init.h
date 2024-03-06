/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#ifndef GATT_MICS_CLIENT_INIT_H_
#define GATT_MICS_CLIENT_INIT_H_

#include "gatt_mics_client_private.h"

/***************************************************************************
NAME
    gattMicsClientSendInitCfm
    
DESCRIPTION
    Send a GATT_MICS_CLIENT_INIT_CFM message to the registered client task with
    the supplied status code.
*/
void gattMicsClientSendInitCfm(GMICSC *gatt_mics_client,
                              GattMicsClientStatus status);

#endif

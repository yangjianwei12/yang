/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_VOCS_CLIENT_INIT_H_
#define GATT_VOCS_CLIENT_INIT_H_

#include "gatt_vocs_client.h"
#include "gatt_vocs_client_private.h"

/***************************************************************************
NAME
    vocsClientSendInitCfm
    
DESCRIPTION
    Send a GATT_VOCS_CLIENT_INIT_CFM message to the registered client task with
    the supplied status code.
*/
void vocsClientSendInitCfm(GVOCS *gatt_vocs_client,
                           GattVocsClientStatus status);

#endif

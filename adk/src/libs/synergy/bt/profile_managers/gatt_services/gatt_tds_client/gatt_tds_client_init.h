/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_TDS_CLIENT_INIT_H_
#define GATT_TDS_CLIENT_INIT_H_

#include "gatt_tds_client_private.h"

/***************************************************************************
NAME
    gattTdsClientSendInitCfm
    
DESCRIPTION
    Send a GATT_TDS_CLIENT_INIT_CFM message to the registered client task with
    the supplied status code.
*/
void gattTdsClientSendInitCfm(GTDSC *gattTdsClient,
                              GattTdsClientStatus status);

#endif

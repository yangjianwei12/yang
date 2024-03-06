/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_TBS_CLIENT_INIT_H_
#define GATT_TBS_CLIENT_INIT_H_


#include "gatt_telephone_bearer_client_private.h"


/***************************************************************************
NAME
    gattTbsClientSendInitComplete
    
PARAMETERS
    tbs_client The TBS client task.
    status The status code to add to the message.

DESCRIPTION
    Send a GATT_TBS_CLIENT_INIT_CFM message to the registered client task.
    
RETURN
    void
*/
void gattTbsClientSendInitComplete(GTBSC *tbs_client, GattTelephoneBearerClientStatus status);


#endif

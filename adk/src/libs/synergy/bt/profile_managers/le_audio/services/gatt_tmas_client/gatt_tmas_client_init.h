/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_TMAS_CLIENT_INIT_H_
#define GATT_TMAS_CLIENT_INIT_H_

#include "gatt_tmas_client_private.h"

/***************************************************************************
NAME
    gattTmasClientSendInitCfm
    
DESCRIPTION
    Send a GATT_TMAS_CLIENT_INIT_CFM message to the registered client task with
    the supplied status code.
*/
void gattTmasClientSendInitCfm(GTMASC *gattTmasClient,
                               GattTmasClientStatus status);

#endif

/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_GMAS_CLIENT_INIT_H_
#define GATT_GMAS_CLIENT_INIT_H_

#include "gatt_gmas_client_private.h"

/***************************************************************************
NAME
    gattGmasClientSendInitCfm
    
DESCRIPTION
    Send a GATT_GMAS_CLIENT_INIT_CFM message to the registered client task with
    the supplied status code.
*/
void gattGmasClientSendInitCfm(GGMASC *gattGmasClient,
                               GattGmasClientStatus status);

#endif

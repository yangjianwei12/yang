/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_GMAS_CLIENT_DISCOVERY_H_
#define GATT_GMAS_CLIENT_DISCOVERY_H_

#include "gatt_gmas_client_private.h"
#include "csr_bt_gatt_client_util_lib.h"

/***************************************************************************
NAME
    gattGmasClientHandleDiscoverAllGmasCharacteristicsResp

DESCRIPTION
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM message for 'Discover All Characteristics' response.
*/
void gattGmasClientHandleDiscoverAllGmasCharacteristicsResp(GGMASC *gattGmasClient,
                                                            const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm);

#endif

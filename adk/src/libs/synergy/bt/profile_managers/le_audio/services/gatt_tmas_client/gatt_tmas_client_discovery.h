/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef GATT_TMAS_CLIENT_DISCOVERY_H_
#define GATT_TMAS_CLIENT_DISCOVERY_H_

#include "gatt_tmas_client_private.h"
#include "csr_bt_gatt_client_util_lib.h"

/***************************************************************************
NAME
    gattTmasClientHandleDiscoverAllTmasCharacteristicsResp

DESCRIPTION
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM message for 'Discover All Characteristics' response.
*/
void gattTmasClientHandleDiscoverAllTmasCharacteristicsResp(GTMASC *gattTmasClient,
                                                            const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm);

#endif

/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_TDS_CLIENT_DISCOVERY_H_
#define GATT_TDS_CLIENT_DISCOVERY_H_

#include "gatt_tds_client_private.h"
#include "csr_bt_gatt_client_util_lib.h"

/***************************************************************************
NAME
    handleDiscoverAllTdsCharacteristicsResp

DESCRIPTION
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM message for 'Discover All Characteristics' response.
*/
void handleDiscoverAllTdsCharacteristicsResp(GTDSC *gattTdsClient, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm);


/***************************************************************************
NAME
    discoverAllTdsCharacteristicDescriptors

DESCRIPTION
    Discover all characteristics descriptors for the characteristic with range of start_handle to end_handle.
*/
void discoverAllTdsCharacteristicDescriptors(GTDSC *gattTdsClient);


/***************************************************************************
NAME
    handleDiscoverAllTdsCharacteristicDescriptorsResp

DESCRIPTION
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM message  for 'Discover All Characteristic Descriptors' response.
*/
void handleDiscoverAllTdsCharacteristicDescriptorsResp(GTDSC *gattTdsClient,
                                                       const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm);

#endif

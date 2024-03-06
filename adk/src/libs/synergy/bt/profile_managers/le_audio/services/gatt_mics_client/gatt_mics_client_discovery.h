/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#ifndef GATT_MICS_CLIENT_DISCOVERY_H_
#define GATT_MICS_CLIENT_DISCOVERY_H_

#include "gatt_mics_client_private.h"
#include "csr_bt_gatt_client_util_lib.h"

/***************************************************************************
NAME
    handleDiscoverAllMicsCharacteristicsResp

DESCRIPTION
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM message for 'Discover All Characteristics' response.
*/
void handleDiscoverAllMicsCharacteristicsResp(GMICSC *gatt_mics_client, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm);


/***************************************************************************
NAME
    discoverAllMicsCharacteristicDescriptors

DESCRIPTION
    Discover all characteristics descriptors for the characteristic with range of start_handle to end_handle.
*/
void discoverAllMicsCharacteristicDescriptors(GMICSC *gatt_mics_client);


/***************************************************************************
NAME
    handleDiscoverAllMicsCharacteristicDescriptorsResp

DESCRIPTION
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM message  for 'Discover All Characteristic Descriptors' response.
*/
void handleDiscoverAllMicsCharacteristicDescriptorsResp(GMICSC *gatt_mics_client,
                                                       const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm);

#endif

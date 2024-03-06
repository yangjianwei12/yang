/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_VCS_CLIENT_DISCOVERY_H_
#define GATT_VCS_CLIENT_DISCOVERY_H_

#include <gatt_manager.h>

#include "gatt_vcs_client_private.h"

/***************************************************************************
NAME
    handleDiscoverAllVcsCharacteristicsResp

DESCRIPTION
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM message for 'Discover All Characteristics' response.
*/
void handleDiscoverAllVcsCharacteristicsResp(GVCSC *gatt_vcs_client, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm);


/***************************************************************************
NAME
    discoverAllVcsCharacteristicDescriptors

DESCRIPTION
    Discover all characteristics descriptors for the characteristic with range of start_handle to end_handle.
*/
void discoverAllVcsCharacteristicDescriptors(GVCSC *gatt_vcs_client);


/***************************************************************************
NAME
    handleDiscoverAllVcsCharacteristicDescriptorsResp

DESCRIPTION
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM message  for 'Discover All Characteristic Descriptors' response.
*/
void handleDiscoverAllVcsCharacteristicDescriptorsResp(GVCSC *gatt_vcs_client,
                                                       const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm);

#endif

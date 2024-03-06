/* Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_AICS_CLIENT_DISCOVERY_H_
#define GATT_AICS_CLIENT_DISCOVERY_H_

#include <gatt_manager.h>

#include "gatt_aics_client_private.h"

/***************************************************************************
NAME
    aicsClientHandleDiscoverAllVcsCharacteristicsResp

DESCRIPTION
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM message for 'Discover All Characteristics' response.
*/
void aicsClientHandleDiscoverAllAicsCharacteristicsResp(GAICS *gatt_aics_client, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm);


/***************************************************************************
NAME
    aicsClientDiscoverAllCharacteristicDescriptors

DESCRIPTION
    Discover all characteristics descriptors for the characteristic with range of start_handle to end_handle.
*/
void aicsClientDiscoverAllCharacteristicDescriptors(GAICS *gatt_aics_client);


/***************************************************************************
NAME
    aicsClientHandleDiscoverAllCharacteristicDescriptorsResp

DESCRIPTION
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM message  for 'Discover All Characteristic Descriptors' response.
*/
void aicsClientHandleDiscoverAllCharacteristicDescriptorsResp(GAICS *gatt_aics_client,
                                       const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm);

#endif

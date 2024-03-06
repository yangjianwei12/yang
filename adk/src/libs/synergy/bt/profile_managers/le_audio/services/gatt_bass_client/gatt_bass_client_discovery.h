/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_BASS_CLIENT_DISCOVERY_H_
#define GATT_BASS_CLIENT_DISCOVERY_H_

#include "gatt_bass_client_private.h"
#include "csr_bt_gatt_client_util_lib.h"

/***************************************************************************
NAME
    bassClientHandleDiscoverAllCharacteristicsResp

DESCRIPTION
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM message for
    'Discover All Characteristics' response.
*/
void bassClientHandleDiscoverAllCharacteristicsResp(GBASSC *gatt_bass_client,
                                                    const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm);


/***************************************************************************
NAME
    bassClientdiscoverAllBassCharacteristicDescriptors

DESCRIPTION
    Discover all characteristics descriptors for the characteristic with range of start_handle to end_handle.
*/
void bassClientdiscoverAllBassCharacteristicDescriptors(GBASSC *gatt_bass_client);


/***************************************************************************
NAME
    bassClientHandleDiscoverAllCharacteristicDescriptorsResp

DESCRIPTION
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM message
    for 'Discover All Characteristic Descriptors' response.
*/
void bassClientHandleDiscoverAllCharacteristicDescriptorsResp(GBASSC *gatt_bass_client,
                                                              const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm);

#endif

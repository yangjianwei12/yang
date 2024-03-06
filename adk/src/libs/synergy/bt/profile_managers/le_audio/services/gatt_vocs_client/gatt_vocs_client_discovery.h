/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_VOCS_CLIENT_DISCOVERY_H_
#define GATT_VOCS_CLIENT_DISCOVERY_H_

#include "csr_bt_gatt_prim.h"

#include "gatt_vocs_client_private.h"

/***************************************************************************
NAME
    vocsClientHandleDiscoverAllVocsCharacteristicsResp

DESCRIPTION
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM message for 'Discover All Characteristics' response.
*/
void vocsClientHandleDiscoverAllVocsCharacteristicsResp(GVOCS *gatt_vocs_client,
                                             const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm);

/***************************************************************************
NAME
    vocsClientdiscoverAllVocsCharacteristicDescriptors

DESCRIPTION
    Discover all characteristics descriptors for the characteristic with range of start_handle to end_handle.
*/
void vocsClientdiscoverAllVocsCharacteristicDescriptors(GVOCS *gatt_vocs_client);


/***************************************************************************
NAME
    vocsClientHandleDiscoverAllVocsCharacteristicDescriptorsResp

DESCRIPTION
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM message  for 'Discover All Characteristic Descriptors' response.
*/
void vocsClientHandleDiscoverAllVocsCharacteristicDescriptorsResp(GVOCS *gatt_vocs_client,
                                       const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm);

#endif

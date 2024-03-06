/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_TBS_CLIENT_DISCOVER_H_
#define GATT_TBS_CLIENT_DISCOVER_H_

#include "gatt_telephone_bearer_client_private.h"

/***************************************************************************
NAME
    discoverAllTbsCharacteristics

DESCRIPTION
    Discover all characteristics for the service.
*/
void discoverAllTbsCharacteristics(GTBSC *tbs_client);


/***************************************************************************
NAME
    handleDiscoverAllTbsCharacteristicsResp

DESCRIPTION
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM message for 'Discover All Characteristics' response.
*/
void handleDiscoverAllTbsCharacteristicsResp(GTBSC *tbs_client, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *msg);


/***************************************************************************
NAME
    discoverAllTbsCharacteristics

DESCRIPTION
    Discover all TBS characteristics descriptors
*/
void discoverAllTbsCharacteristicDescriptors(GTBSC *tbs_client);


/***************************************************************************
NAME
    handleDiscoverAllTbsCharacteristicDescriptorsResp

DESCRIPTION
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM message  for 'Discover All Characteristic Descriptors' response..
*/
void handleDiscoverAllTbsCharacteristicDescriptorsResp(GTBSC *tbs_client, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm);


#endif

/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#ifndef GATT_AMS_CLIENT_DISCOVER_H_
#define GATT_AMS_CLIENT_DISCOVER_H_

#include "gatt_ams_client.h"
#include <gatt_lib.h>


/*
    Handles CSR_BT_GATT_REGISTER_CFM message.
*/
void handleAmsClientRegisterCfm(GAMS *ams, const CsrBtGattRegisterCfm *cfm);

/*
    Handles CSR_BT_GATT_UNREGISTER_CFM message.
*/
void handleAmsClientUnRegisterCfm(GAMS *ams, const CsrBtGattUnregisterCfm *cfm);

/*
    Handles CSR_BT_GATT_CLIENT_REGISTER_SERVICE_CFM message.
*/
void handleAmsClientRegisterServiceCfm(GAMS *ams, const CsrBtGattClientRegisterServiceCfm *cfm);

/*
    Handles CSR_BT_GATT_DISCOVER_CHARAC_IND message.
*/
void handleAmsDiscoverAllCharacteristicsInd(GAMS *ams, const CsrBtGattDiscoverCharacInd *ind);

/*
   Handles CSR_BT_GATT_DISCOVER_CHARAC_CFM message.
*/
void handleAmsDiscoverAllCharacteristicsCfm(GAMS *ams, const CsrBtGattDiscoverCharacCfm *cfm);

/*
    Discover all characteristics descriptors for the characteristic with range of start_handle to end_handle.
*/
bool gattAmsDiscoverAllCharacteristicDescriptors(GAMS *ams, uint16 start_handle, uint16 end_handle);

/*
   Handles CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_IND message.
*/
void handleAmsDiscoverAllCharacteristicDescriptorsInd(GAMS *ams, const CsrBtGattDiscoverCharacDescriptorsInd *ind);

/*
   Handles CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM message.
*/
void handleAmsDiscoverAllCharacteristicDescriptorsCfm(GAMS *ams, const CsrBtGattDiscoverCharacDescriptorsCfm *cfm);


/*
   Helper function to get the endhandle for discovering characteristic descriptor of NS
*/
uint16 gattAmsfindEndHandleForCharDesc(GAMS *ams, uint16 startHandle, uint16 endHandle, uint8 characteristic);

#endif /* GATT_AMS_CLIENT_DISCOVER_H_ */

/* Copyright (c) 2014 - 2019 Qualcomm Technologies International, Ltd. */

#ifndef GATT_APPLE_NOTIFICATION_CLIENT_DISCOVER_H_
#define GATT_APPLE_NOTIFICATION_CLIENT_DISCOVER_H_

#include "gatt_apple_notification_client.h"
#include <gatt_lib.h>

/***************************************************************************
NAME
    handleAncsClientRegisterCfm

DESCRIPTION
    Handles CSR_BT_GATT_REGISTER_CFM message.
*/
void handleAncsClientRegisterCfm(GANCS *ancs,const CsrBtGattRegisterCfm *cfm);


/***************************************************************************
NAME
    handleAncsClientUnRegisterCfm

DESCRIPTION
    Handles CSR_BT_GATT_UNREGISTER_CFM message.
*/
void handleAncsClientUnRegisterCfm(GANCS *ancs, const CsrBtGattUnregisterCfm *cfm);

/***************************************************************************
NAME
    handleAncsClientRegisterServiceCfm

DESCRIPTION
    Handles CSR_BT_GATT_CLIENT_REGISTER_SERVICE_CFM message.
*/

void handleAncsClientRegisterServiceCfm(GANCS *ancs, const CsrBtGattClientRegisterServiceCfm *cfm);

/***************************************************************************
NAME
    handleAncsDiscoverAllCharacteristicsInd

DESCRIPTION
    Handles CSR_BT_GATT_DISCOVER_CHARAC_IND message for 'Discover All Characteristics' response.
*/
void handleAncsDiscoverAllCharacteristicsInd(GANCS *ancs, const CsrBtGattDiscoverCharacInd *ind);

/***************************************************************************
NAME
    handleAncsDiscoverAllCharacteristicsResp

DESCRIPTION
    Handles CSR_BT_GATT_DISCOVER_CHARAC_CFM message for 'Discover All Characteristics' response.
*/
void handleAncsDiscoverAllCharacteristicsCfm(GANCS *ancs, const CsrBtGattDiscoverCharacCfm *cfm);

/***************************************************************************
NAME
    handleAncsDiscoverAllCharacteristicDescriptorsInd

DESCRIPTION
    Discover all characteristics descriptors for the characteristic with range of start_handle to end_handle.
*/
void handleAncsDiscoverAllCharacteristicDescriptorsInd(GANCS *ancs, const CsrBtGattDiscoverCharacDescriptorsInd *ind);

/***************************************************************************
NAME
    handleAncsDiscoverAllCharacteristicDescriptorsCfm

DESCRIPTION
    Discover all characteristics descriptors for the characteristic with range of start_handle to end_handle.
*/
void handleAncsDiscoverAllCharacteristicDescriptorsCfm(GANCS *ancs, const CsrBtGattDiscoverCharacDescriptorsCfm *cfm);

/***************************************************************************
NAME
    ancsDiscoverAllCharacteristicDescriptors

DESCRIPTION
    Discover all characteristics descriptors for the characteristic with range of start_handle to end_handle.
*/
bool ancsDiscoverAllCharacteristicDescriptors(GANCS *ancs, uint16 start_handle, uint16 end_handle);

uint16 findEndHandleForCharDesc(GANCS *ancs, uint16 startHandle, uint16 endHandle, uint8 charesteristic);

#endif /* GATT_APPLE_NOTIFICATION_CLIENT_DISCOVER_H_ */

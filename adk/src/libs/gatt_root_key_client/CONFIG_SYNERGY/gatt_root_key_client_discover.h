/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROOT_KEY_CLIENT_DISCOVER_H_
#define GATT_ROOT_KEY_CLIENT_DISCOVER_H_

#include "gatt_root_key_client_private.h"

/*!
    Discover primary service.
*/
void rootKeyDiscoverPrimaryService(GATT_ROOT_KEY_CLIENT *instance);

/*!
    Handles CSR_BT_GATT_DISCOVER_SERVICES_IND message for 'Discover Primary Service' response.
*/
void rootKeyHandleDiscoverPrimaryServiceInd(GATT_ROOT_KEY_CLIENT *instance, 
                                            const CsrBtGattDiscoverServicesInd *ind);

/*!
    Handles CSR_BT_GATT_DISCOVER_SERVICES_CFM message for 'Discover Primary Service' response.
*/
void rootKeyHandleDiscoverPrimaryServiceCfm(GATT_ROOT_KEY_CLIENT *instance, 
                                            const CsrBtGattDiscoverServicesCfm *cfm);

/*!
    Handles CSR_BT_GATT_CLIENT_REGISTER_SERVICE_CFM  message for 'Register Service Handles' response.
*/
void rootKeyHandleRegisterServiceCfm(GATT_ROOT_KEY_CLIENT *instance, 
                                     const CsrBtGattClientRegisterServiceCfm *cfm);

/*!
    Discover all characteristics for the service.
*/
void rootKeyDiscoverAllCharacteristics(GATT_ROOT_KEY_CLIENT *instance);

/*!
    Handles CSR_BT_GATT_DISCOVER_CHARAC_IND message for 'Discover All Characteristics' response.
*/
void rootKeyHandleDiscoverAllCharacteristicsInd(GATT_ROOT_KEY_CLIENT *instance, 
                                                 const CsrBtGattDiscoverCharacInd *ind);


/*!
    Handles CSR_BT_GATT_DISCOVER_CHARAC_CFM message for 'Discover All Characteristics' response.
*/
void rootKeyHandleDiscoverAllCharacteristicsCfm(GATT_ROOT_KEY_CLIENT *instance, 
                                                const CsrBtGattDiscoverCharacCfm *cfm);

/*!
    Discover all characteristics descriptors for the characteristic with range of start_handle to end_handle.
*/
void rootKeyDiscoverAllCharacteristicDescriptors(GATT_ROOT_KEY_CLIENT *instance, uint16 start_handle, uint16 end_handle);


/*!
    Handles CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_IND message  for 'Discover All Characteristic Descriptors' response..
*/
void rootKeyHandleDiscoverAllCharacteristicDescriptorsInd(GATT_ROOT_KEY_CLIENT *instance, const CsrBtGattDiscoverCharacDescriptorsInd *ind);

/*!
    Handles CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM message  for 'Discover All Characteristic Descriptors' response..
*/
void rootKeyHandleDiscoverAllCharacteristicDescriptorsCfm(GATT_ROOT_KEY_CLIENT *instance, const CsrBtGattDiscoverCharacDescriptorsCfm *cfm);

#endif /* GATT_ROOT_KEY_CLIENT_DISCOVER_H_ */

/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROLE_SELECTION_CLIENT_DISCOVER_H_
#define GATT_ROLE_SELECTION_CLIENT_DISCOVER_H_

#include "gatt_role_selection_client_private.h"


/*!
    Discovers Primary Service.
*/
void roleSelectionDiscoverPrimaryService(GATT_ROLE_SELECTION_CLIENT *instance);


/*!
    Handles CSR_BT_GATT_DISCOVER_SERVICES_IND message  for 'Discover Primary Service' response.
*/
void roleSelectionHandleDiscoverPrimaryServiceInd(GATT_ROLE_SELECTION_CLIENT *instance, 
                                                  const CsrBtGattDiscoverServicesInd *ind);


/*!
    Handles CSR_BT_GATT_DISCOVER_SERVICES_CFM message  for 'Discover Primary Service' response.
*/
void roleSelectionHandleDiscoverPrimaryServiceCfm(GATT_ROLE_SELECTION_CLIENT *instance, 
                                                  const CsrBtGattDiscoverServicesCfm *cfm);

/*!
    Handles CSR_BT_GATT_CLIENT_REGISTER_SERVICE_CFM  message for 'Register Service Handles' response.
*/
void roleSelectionHandleRegisterServiceCfm(GATT_ROLE_SELECTION_CLIENT *instance, 
                                           const CsrBtGattClientRegisterServiceCfm *cfm);

/*!
    Discover all characteristics for the service.
*/
void roleSelectionDiscoverAllCharacteristics(GATT_ROLE_SELECTION_CLIENT *instance);


/*!
    Handles CSR_BT_GATT_DISCOVER_CHARAC_IND message for 'Discover All Characteristics' response.
*/
void roleSelectionHandleDiscoverAllCharacteristicsInd(GATT_ROLE_SELECTION_CLIENT *instance, 
                                                      const CsrBtGattDiscoverCharacInd *ind);


/*!
    Handles CSR_BT_GATT_DISCOVER_CHARAC_CFM message for 'Discover All Characteristics' response.
*/
void roleSelectionHandleDiscoverAllCharacteristicsCfm(GATT_ROLE_SELECTION_CLIENT *instance, 
                                                      const CsrBtGattDiscoverCharacCfm *cfm);

/*!
    Discover all characteristics descriptors for the characteristic with range of start_handle to end_handle.
*/
void roleSelectionDiscoverAllCharacteristicDescriptors(GATT_ROLE_SELECTION_CLIENT *instance, uint16 start_handle, uint16 end_handle);

/*!
    Handles CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_IND message  for 'Discover All Characteristic Descriptors' response.
*/
void roleSelectionHandleDiscoverAllCharacteristicDescriptorsInd(GATT_ROLE_SELECTION_CLIENT *instance, const CsrBtGattDiscoverCharacDescriptorsInd *ind);


/*!
    Handles CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM message  for 'Discover All Characteristic Descriptors' response.
*/
void roleSelectionHandleDiscoverAllCharacteristicDescriptorsCfm(GATT_ROLE_SELECTION_CLIENT *instance, const CsrBtGattDiscoverCharacDescriptorsCfm *cfm);

#endif /* GATT_ROLE_SELECTION_CLIENT_DISCOVER_H_ */

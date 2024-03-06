/*!
   \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.\n
               All Rights Reserved.\n
               Qualcomm Technologies International, Ltd. Confidential and Proprietary.
   \file
   \addtogroup le_advertising_manager_legacy
   \brief      Manage execution of callbacks to construct adverts and scan response
   @{
*/

#ifndef LE_ADVERTSING_MANAGER_DATA_EXTENDED_H_
#define LE_ADVERTSING_MANAGER_DATA_EXTENDED_H_

#include "le_advertising_manager.h"
#include "le_advertising_manager_legacy_private.h"


#if defined(INCLUDE_ADVERTISING_EXTENSIONS)

/*! Used to register and use the first application advertising set */
#define ADV_HANDLE_APP_SET_1            1


void leAdvertisingManager_RegisterExtendedDataIf(le_adv_data_set_t set);

#else

#define leAdvertisingManager_RegisterExtendedDataIf(set) (UNUSED(set))
    
#endif /* INCLUDE_ADVERTISING_EXTENSIONS*/


#endif /* LE_ADVERTSING_MANAGER_DATA_EXTENDED_H_ */
/*! @} */
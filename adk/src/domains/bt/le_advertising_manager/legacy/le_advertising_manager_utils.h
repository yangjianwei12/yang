/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_advertising_manager_legacy
    \brief      Definitons of internal LE advertising manager common utilities.
    @{
*/

#ifndef LE_ADVERTSING_MANAGER_UTILS_H_
#define LE_ADVERTSING_MANAGER_UTILS_H_

#include "le_advertising_manager_legacy.h"

/*! \brief Checks whether two sets of input parameters are matched
    \param params1 first set of parameters
    \param params2 second set of parameters
    
    \return TRUE, if parameters matched, FALSE if not matched.    
 */
bool LeAdvertisingManager_ParametersMatch(const le_adv_data_params_t * params1, const le_adv_data_params_t * params2);

le_adv_data_set_handle * leAdvertisingManager_GetReferenceToHandleForDataSet(le_adv_data_set_t set);

void leAdvertisingManager_FreeHandleForDataSet(le_adv_data_set_t set);

Task leAdvertisingManager_GetTaskForDataSet(le_adv_data_set_t set);

bool leAdvertisingManager_CheckIfHandleExists(le_adv_data_set_t set);

le_adv_data_set_handle leAdvertisingManager_CreateNewDataSetHandle(le_adv_data_set_t set);

#endif /* LE_ADVERTSING_MANAGER_UTILS_H_ */
/*! @} */
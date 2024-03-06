/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup    le_advertising_manager_multi_set
    \brief  Functions to set and retrieve the default advertising parameters to use.

        Unless an individual advertising item overrides the advertising parameters
        with its own GetItemParameters implementation it will use the default
        parameters.

        The application can set its own config for some of the parameters by setting a
        config table at initialisation time and then selecting which row of the table
        to use at runtime. For example it could use faster default parameters when
        in pairing mode but otherwise use slower parameters to save power.

    @{
*/

#ifndef LE_ADVERTISING_MANAGER_DEFAULT_PARAMETERS_H
#define LE_ADVERTISING_MANAGER_DEFAULT_PARAMETERS_H


/*! \brief Initalise the default advertising parameters. */
void LeAdvertisingManager_DefaultParametersInit(void);

/*! \brief Handle a LE_ADVERTISING_MANAGER_INTERNAL_DEFAULT_PARAMETERS_FALLBACK_TIMEOUT */
void LeAdvertisingManager_HandleInternalDefaultParametersFallbackTimeout(
        const LE_ADVERTISING_MANAGER_INTERNAL_DEFAULT_PARAMETERS_FALLBACK_TIMEOUT_T *msg);

#endif // LE_ADVERTISING_MANAGER_DEFAULT_PARAMETERS_H

/*! @} */
/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_bap
    \brief
    @{
*/

#ifndef UNICAST_SERVER_ROLE_ADVERTISING_H_
#define UNICAST_SERVER_ROLE_ADVERTISING_H_

/*! \brief Setup the LE advertising data for the unicast server role

    \returns TRUE if the advertising data was setup successfully, else FALSE
 */
bool LeBapUnicastServer_SetupLeAdvertisingData(void);

/*! \brief Trigger an update the LE advertising data for the unicast server role

    \returns TRUE if the update was successful, else FALSE
 */
bool LeBapUnicastServer_UpdateLeAdvertisingData(void);

#endif /* UNICAST_SERVER_ROLE_ADVERTISING_H_ */
/*! @} */
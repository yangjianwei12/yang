/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup    csip
    \brief      Coordinated Set Le Advertising functionality
    @{
*/

#ifndef CSIP_SET_MEMBER_ADVERTISING_H_
#define CSIP_SET_MEMBER_ADVERTISING_H_

/*! \brief Setup the coordinated set LE advertising data

    \param psri The (6 octet) PSRI to advertise
    \returns TRUE if the advertising data was setup successfully, else FALSE
 */
bool CsipSetMember_SetupLeAdvertisingData(uint8 * psri);

/*! \brief Update the coordinated set LE advertising data with new psri

    \param psri The new PSRI to advertise
    \returns TRUE if the update was successful, else FALSE
 */
bool CsipSetMember_UpdatePsri(uint8 * psri);

#endif /* CSIP_SET_MEMBER_ADVERTISING_H_ */
/*! @} */
/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \brief
    \addtogroup le_peer_pairing_service
    @{
*/

#ifndef PEER_PAIR_LE_ADVERTISING_H_
#define PEER_PAIR_LE_ADVERTISING_H_

/*! \brief Setup the LE advertising data for peer pairing

    \returns TRUE if the advertising data was setup successfully, else FALSE
 */
bool PeerPairLe_SetupLeAdvertisingData(void);

/*! \brief Trigger an update the LE advertising data for peer pairing

    \returns TRUE if the update was successful, else FALSE
 */
bool PeerPairLe_UpdateLeAdvertisingData(void);

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

/*! \brief Unregister LE advertising item for peer pairing */

void PeerPairLe_UnregisterAdvertisingItem(void);

#else

#define PeerPairLe_UnregisterAdvertisingItem() ((void)0)

#endif /* INCLUDE_LEGACY_LE_ADVERTISING_MANAGER */
#endif /* PEER_PAIR_LE_ADVERTISING_H_ */

/*! @} */
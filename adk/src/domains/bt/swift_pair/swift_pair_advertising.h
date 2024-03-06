/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\addtogroup swift_pair
\brief

@{
*/

#ifndef SWIFT_PAIR_ADVERTISING_H_
#define SWIFT_PAIR_ADVERTISING_H_

/*! \brief Setup the LE advertising data for Swift Pair

    \returns TRUE if the advertising data was setup successfully, else FALSE
 */
bool SwiftPair_SetupLeAdvertisingData(void);

/*! \brief Trigger an update the LE advertising data for Swift Pair

    \param discoverable enables/disables discoverable advertising data
    \returns TRUE if the update was, else FALSE
 */
bool SwiftPair_UpdateLeAdvertisingData(bool discoverable);

#ifdef SWIFT_PAIR_AFTER_PFR
void SwiftPair_PfrPrimary(bool isPrimary);
#endif

#endif /* SWIFT_PAIR_ADVERTISING_H_ */
/*! @} */
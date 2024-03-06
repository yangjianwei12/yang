/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       headset_sirk_config.h
\brief      headset sirk generation, storage and retrieval
*/

#ifndef HEADSET_SIRK_CONFIG_H_
#define HEADSET_SIRK_CONFIG_H_

#ifdef ENABLE_LE_AUDIO_CSIP

/* Sirk key size */
#define SIZE_SIRK_KEY             (16*sizeof(uint8))

/*! \brief Sets the stored SIRK key in the PS in Le advertising data of csip.

    \return TRUE if successful
*/
bool HeadsetSirk_RetrieveAndUpdateSirkToCsip(void);

/*! \brief Generate the sirk key by using key_a and key_b and stores in the PS

    \param sirk - Sirk key in which the generated key will be added
    \param key_a - Randomly generated key a
    \param key_b - Randomly generated key b
*/
void HeadsetSirk_GenerateAndStoreSirkKey(uint8 *sirk, uint8 *key_a, uint8 *key_b);

/*! \brief Stores the sirk key in the PS.

    The configuration table can be passed directly to the ui component in
    domains.

    \param sirk - The sirk key which needs to be stored.
*/
void HeadsetSirk_StoreSirkKey(uint8 *sirk);

#endif /* ENABLE_LE_AUDIO_CSIP */

#endif /* HEADSET_SIRK_CONFIG_H_ */


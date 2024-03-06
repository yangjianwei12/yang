/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\version    %%version
\file       headset_sm.c
\brief      headset application sirk generator, store and retrieve.

*/

#ifdef ENABLE_LE_AUDIO_CSIP

/* local includes */
#include "headset_sirk_config.h"

/* framework includes */
#include <ps_key_map.h>
#include <csip_set_member.h>
#include "logging.h"

#define SIRK_PSKEY_STORAGE_LENGTH       SIZE_SIRK_KEY

static void headsetSirk_generate(uint8 *sirk, uint8 *key_a, uint8 *key_b)
{
    for (uint8 i = 0; i < SIRK_PSKEY_STORAGE_LENGTH; i++)
    {
        sirk[i] = key_a[i] ^ key_b[i];

        /* Fill in with constant if the resultant is 0 */
        if (sirk[i] == 0)
        {
            sirk[i] = 0x81;
        }

        DEBUG_LOG("sirk : 0x%02x, key_a : 0x%02x, key_b : 0x%02x ", sirk[i], key_a[i], key_b[i]);
    }
}

static bool headsetSirk_RetrieveSirkKey(uint8 *sirk_key)
{
    uint16 num_of_words = PsRetrieve(PS_KEY_CSIP_SIRK_KEY, NULL, 0);
    uint16 read_words;
    bool is_success = FALSE;

    if (num_of_words > 0)
    {
        read_words = PsRetrieve(PS_KEY_CSIP_SIRK_KEY, sirk_key, SIRK_PSKEY_STORAGE_LENGTH);

        if (read_words > 0)
        {
            is_success = TRUE;
        }

        DEBUG_LOG("headsetSirk_RetrieveSirkKey read_words : %d, num_of_words : %d ", read_words, num_of_words);
    }

    return is_success;
}

void HeadsetSirk_StoreSirkKey(uint8 *sirk)
{
    PsStore(PS_KEY_CSIP_SIRK_KEY, sirk, SIRK_PSKEY_STORAGE_LENGTH);
}

void HeadsetSirk_GenerateAndStoreSirkKey(uint8 *sirk, uint8 *key_a, uint8 *key_b)
{
    DEBUG_LOG("HeadsetSirk_GenerateAndStoreSirkKey");
    headsetSirk_generate(sirk, key_a, key_b);
    HeadsetSirk_StoreSirkKey(sirk);
}

bool HeadsetSirk_RetrieveAndUpdateSirkToCsip(void)
{
    uint8 sirk[SIRK_PSKEY_STORAGE_LENGTH] = {0};

    DEBUG_LOG("HeadsetSirk_RetrieveAndUpdateSirkToCsip");
    if (headsetSirk_RetrieveSirkKey(sirk))
    {
        CsipSetMember_SetSirkKey(sirk);
        return TRUE;
    }

    return FALSE;
}

#endif /* ENABLE_LE_AUDIO_CSIP */

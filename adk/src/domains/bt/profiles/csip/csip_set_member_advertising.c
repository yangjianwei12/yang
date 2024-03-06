/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    csip
    \brief
*/

#include "csip_set_member_advertising.h"

#include "lea_advertising_policy.h"

#include <logging.h>
#include <panic.h>
#include <stdlib.h>

#define CSIP_SET_MEMBER_ADVERT_DATA_ITEMS       1
#define CSIP_SET_MEMBER_ADVERT_SIZE             8
#define CSIP_SET_MEMBER_PSRI_START_OCTECT       2
#define PSRI_SIZE                               6

static uint8 * current_psri = NULL;

static const uint8 csip_set_member_advert_data[CSIP_SET_MEMBER_ADVERT_SIZE] = {
    CSIP_SET_MEMBER_ADVERT_SIZE - 1,
    ble_ad_type_rsi_data,
    0,0,0,0,0,0,
};

static uint8 csipSetMember_GetAdvertisingDataSize(void)
{
    return CSIP_SET_MEMBER_ADVERT_SIZE;
}

static uint8 csipSetMember_GetAdvertisingData(const lea_adv_policy_adv_param_t *params,
                                              uint8 *advert_buffer,
                                              uint8 advert_buffer_length)
{
    UNUSED(params);

    DEBUG_LOG("csipSetMember_GetAdvertisingData");
    PanicFalse(advert_buffer_length >= CSIP_SET_MEMBER_ADVERT_SIZE);

    memcpy(advert_buffer, csip_set_member_advert_data, CSIP_SET_MEMBER_ADVERT_SIZE);
    memcpy(&advert_buffer[CSIP_SET_MEMBER_PSRI_START_OCTECT], current_psri, PSRI_SIZE);
    return CSIP_SET_MEMBER_ADVERT_SIZE;
}

static const lea_adv_policy_clients_callback_t csip_set_member_callback =
{
    .GetAdvertisingDataSize = csipSetMember_GetAdvertisingDataSize,
    .GetAdvertisingData = csipSetMember_GetAdvertisingData
};

bool CsipSetMember_SetupLeAdvertisingData(uint8 * psri)
{
    current_psri = psri;
    LeaAdvertisingPolicy_RegisterClient(&csip_set_member_callback);
    return TRUE;
}

bool CsipSetMember_UpdatePsri(uint8 * psri)
{
    free(current_psri);
    current_psri = psri;
    LeaAdvertisingPolicy_UpdateAdvertisingItems();
    return TRUE;
}


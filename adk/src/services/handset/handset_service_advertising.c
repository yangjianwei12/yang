/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    handset_service
    \brief
*/

#include "handset_service_advertising.h"

#include "handset_service_protected.h"
#include "handset_service_config.h"
#include "handset_service_sm.h"

static inline bool handsetService_IsConnectableAdvertisingAllowed(void)
{
    return (HandsetService_IsBleConnectable()
            && (HandsetServiceSm_GetLeAclConnectionCount() < handsetService_LeAclMaxConnections()));
}

bool HandsetService_UpdateAdvertising(void)
{
     bool advert_allowed = handsetService_IsConnectableAdvertisingAllowed();

     if (advert_allowed)
     {
#ifdef ENABLE_LEA_TARGETED_ANNOUNCEMENT
         LeaAdvertisingPolicy_SetAdvertisingMode(lea_adv_policy_mode_undirected,
                                                 lea_adv_policy_announcement_type_none,
                                                 lea_adv_policy_announcement_type_general,
                                                 NULL);
#endif
     }

     return LeAdvertisingManager_EnableConnectableAdvertising(HandsetService_GetTask(), advert_allowed);
}

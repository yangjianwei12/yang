/*******************************************************************************

Copyright (C) 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"
#include <string.h>

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_ext_adv_set_params_v2_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_EXT_ADV_SET_PARAMS_V2_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_ext_adv_set_params_v2_req(
    uint8_t  adv_handle,
    uint16_t adv_event_properties,
    uint32_t primary_adv_interval_min,
    uint32_t primary_adv_interval_max,
    uint8_t  primary_adv_channel_map,
    uint8_t  own_addr_type,
    TYPED_BD_ADDR_T *peer_addr,
    uint8_t  adv_filter_policy,
    uint16_t primary_adv_phy,
    uint8_t  secondary_adv_max_skip,
    uint16_t secondary_adv_phy,
    uint16_t adv_sid,
    int8_t   adv_tx_pwr,
    uint8_t  scan_req_notify_enable,
    uint8_t  primary_adv_phy_options,
    uint8_t  secondary_adv_phy_options,
    DM_UPRIM_T **pp_prim
    )
{
    DM_ULP_EXT_ADV_SET_PARAMS_V2_REQ_T *prim =
            zpnew(DM_ULP_EXT_ADV_SET_PARAMS_V2_REQ_T);

    prim->type = DM_ULP_EXT_ADV_SET_PARAMS_V2_REQ;
    prim->adv_handle = adv_handle;
    prim->adv_event_properties = adv_event_properties;
    prim->primary_adv_interval_min = primary_adv_interval_min;
    prim->primary_adv_interval_max = primary_adv_interval_max;
    prim->primary_adv_channel_map = primary_adv_channel_map;
    prim->own_addr_type = own_addr_type;

    if (peer_addr != NULL)
    {
        prim->peer_addr = *peer_addr;
    }
    else
    {
        memset(&prim->peer_addr, 0, sizeof(TYPED_BD_ADDR_T));
    }

    prim->adv_filter_policy = adv_filter_policy;
    prim->primary_adv_phy = primary_adv_phy;
    prim->secondary_adv_max_skip = secondary_adv_max_skip;
    prim->secondary_adv_phy = secondary_adv_phy;
    prim->adv_sid = adv_sid;
    prim->adv_tx_pwr = adv_tx_pwr;
    prim->scan_req_notify_enable = scan_req_notify_enable;
    prim->primary_adv_phy_options = primary_adv_phy_options;
    prim->secondary_adv_phy_options = secondary_adv_phy_options;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}

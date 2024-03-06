/*!
\copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      stereo_topology control of advertising parameters.
*/

#include "stereo_topology_private.h"
#include "stereo_topology_config.h"
#include "stereo_topology_advertising.h"

#include <bt_device.h>
#include <pairing.h>
#include <le_advertising_manager.h>
#include <logging.h>

void stereoTopology_UpdateAdvertisingParams(void)
{
    stereo_topology_le_adv_params_set_type_t next = stereo_topology_le_adv_params_set_type_unset;
    stereo_topology_le_adv_params_set_type_t current = StereoTopologyGetTaskData()->active_interval;

    /* if we have a peer speaker */
    if(BtDevice_IsPairedWithPeer())
    {
        if(!PairingIsIdle() || !appDeviceIsPeerConnected())
        {
            next = stereo_topology_le_adv_params_set_type_fast;
        }
        else if(appDeviceIsBredrHandsetConnected())
        {
            next = stereo_topology_le_adv_params_set_type_slow;
        }
        else
        {
            next = stereo_topology_le_adv_params_set_type_fast_fallback;
        }
    }
    else
    {
        /* if we are in pairing, then we need to be in fast adv */
        if(!PairingIsIdle())
        {
            next = stereo_topology_le_adv_params_set_type_fast;
        }
        /* if either phone got connected, or we timed-out pairing without success
           then we need to be in slow adv */
        else if (appDeviceIsBredrHandsetConnected() || PairingIsIdle())
        {
            next = stereo_topology_le_adv_params_set_type_slow;
        }
        /* any other case, go to fast adv (most unlikely to be in this case) */
        else
        {
            next = stereo_topology_le_adv_params_set_type_fast;
        }
    }

    if (next != current)
    {
        DEBUG_LOG_INFO("stereoTopology_UpdateAdvertisingParams enum:stereo_topology_le_adv_params_set_type_t:%d ->enum:stereo_topology_le_adv_params_set_type_t:%d", current, next);

        LeAdvertisingManager_ParametersSelect(next);

        StereoTopologyGetTaskData()->active_interval = next;
    }
}


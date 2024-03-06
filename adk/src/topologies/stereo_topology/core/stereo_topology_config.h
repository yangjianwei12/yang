/*!
\copyright  Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Configuration / parameters used by the topology.
*/

#ifndef BREDR_SCAN_MANAGER_H
#define BREDR_SCAN_MANAGER_H

#include "bredr_scan_manager.h"
#include "le_advertising_manager.h"

/*! Inquiry scan parameter set */
extern const bredr_scan_manager_parameters_t stereo_inquiry_scan_params;

/*! Page scan parameter set */
extern const bredr_scan_manager_parameters_t stereo_page_scan_params;

/*! LE Advertising parameter set */
extern const le_adv_parameters_t stereo_le_adv_params;

/*! LE Advertisement parameter set type */
typedef enum
{
    stereo_topology_le_adv_params_set_type_fast = le_adv_advertising_config_set_1,
    stereo_topology_le_adv_params_set_type_slow = le_adv_advertising_config_set_2,
    stereo_topology_le_adv_params_set_type_fast_fallback = le_adv_advertising_config_set_3,
    stereo_topology_le_adv_params_set_type_unset = le_adv_advertising_config_set_total,
}stereo_topology_le_adv_params_set_type_t;


/*! Timeout for a Stereo Topology Stop command to complete (in seconds).

    \note This should be set such that in a normal case all activities will
        have completed.
 */
#define StereoTopologyConfig_StereoTopologyStopTimeoutS()         (10)

/*! Time for Secondary to wait for BR/EDR ACL connection to Primary following
    role selection, before falling back to retry role selection and potentially
    becoming an acting primary. */
#define StereoTopologyConfig_SecondaryPeerConnectTimeoutMs()   (12000)


/*! Time for Primary to wait for BR/EDR ACL connection to be made by the Secondary
 following role selection, before falling back to retry role selection. */
#define StereoTopologyConfig_PrimaryPeerConnectTimeoutMs()     (10240)


#define StereoTopologyConfig_PeerProfiles() (DEVICE_PROFILE_PEERSIG            | \
                                             DEVICE_PROFILE_MIRROR)\



/*! \brief Disconnect reason code to be sent to the remote side when powering off.

           Note: For BREDR the disconnect reason code HCI_ERROR_OETC_USER is always sent.
           For BLE, when USE_OETC_POWERING_OFF_REASON_CODE_FOR_BLE flag is enabled,the HCI
           diconnect reason code of HCI_ERROR_OETC_POWERING_OFF will be sent, else 
           HCI_ERROR_OETC_USER will be sent 
*/
#ifdef USE_OETC_POWERING_OFF_REASON_CODE_FOR_BLE
#define HCI_BLE_DISCONNECT_REASON_CODE_FOR_POWEROFF      (HCI_ERROR_OETC_POWERING_OFF)
#else
#define HCI_BLE_DISCONNECT_REASON_CODE_FOR_POWEROFF      (HCI_ERROR_OETC_USER)
#endif

#endif  //BREDR_SCAN_MANAGER_H


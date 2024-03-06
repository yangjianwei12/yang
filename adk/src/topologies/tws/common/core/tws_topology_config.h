/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Configuration / parameters used by the topology.
*/

#ifndef TWS_TOPOLOGY_CONFIG_H
#define TWS_TOPOLOGY_CONFIG_H

#include "bredr_scan_manager.h"
#include "le_advertising_manager.h"

/*! Inquiry scan parameter set */
extern const bredr_scan_manager_parameters_t inquiry_scan_params;

/*! Page scan parameter set */
extern const bredr_scan_manager_parameters_t page_scan_params;

/*! LE Advertising parameter set */
extern const le_adv_parameters_t le_adv_params;

/*! LE Advertisement parameter set type */
typedef enum
{
    LE_ADVERTISING_PARAMS_SET_TYPE_FAST,
    LE_ADVERTISING_PARAMS_SET_TYPE_FAST_FALLBACK,
    LE_ADVERTISING_PARAMS_SET_TYPE_SLOW,
    LE_ADVERTISING_PARAMS_SET_TYPE_UNSET,
}tws_topology_le_adv_params_set_type_t;

// ----------------------------------------------------------------------------------------------------------------------------
// MASH TODO - revisit this lot as requires more work in tws_topology_procedure_pri_connect_peer_profiles
/*! Configure accessor for dynamic handover support */
#ifdef ENABLE_DYNAMIC_HANDOVER
#define TwsTopologyConfig_DynamicHandoverSupported()    (TRUE)
#else
#define TwsTopologyConfig_DynamicHandoverSupported()    (FALSE)
#endif

#ifdef INCLUDE_BTDBG
#define TwsTopologyConfig_BtdbgMask() (DEVICE_PROFILE_BTDBG)
#else
#define TwsTopologyConfig_BtdbgMask() (0)
#endif

#ifdef INCLUDE_SENSOR_PROFILE
#define TwsTopologyConfig_SensorMask() (DEVICE_PROFILE_SENSOR)
#else
#define TwsTopologyConfig_SensorMask() (0)
#endif

#define TwsTopologyConfig_PeerProfiles() (DEVICE_PROFILE_PEERSIG            | \
                                            DEVICE_PROFILE_HANDOVER         | \
                                            DEVICE_PROFILE_MIRROR           | \
                                            TwsTopologyConfig_BtdbgMask()   | \
                                            TwsTopologyConfig_SensorMask())


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
// ----------------------------------------------------------------------------------------------------------------------------

#endif /*TWS_TOPOLOGY_CONFIG_H*/

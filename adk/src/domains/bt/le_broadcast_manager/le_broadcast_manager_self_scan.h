/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup leabm
    \brief      Header for the LE Audio Broadcast Self-Scan module
    @{
*/

#ifndef LE_BROADCAST_MANAGER_SELF_SCAN_H
#define LE_BROADCAST_MANAGER_SELF_SCAN_H

#include <bdaddr.h>


/*! \brief Notifications sent to the client Task */
typedef enum le_broadcast_manager_self_scan_message
{
    /*! CFM of scan started by calling #LeBroadcastManager_SelfScanStart. */
    LE_BROADCAST_MANAGER_SELF_SCAN_START_CFM,

    /*! Confirmation of scan stopped by calling #LeBroadcastManager_SelfScanStop. */
    LE_BROADCAST_MANAGER_SELF_SCAN_STOP_CFM,

    /*! Notification of the current scan status. */
    LE_BROADCAST_MANAGER_SELF_SCAN_STATUS_IND,

    /*! Notification of a discovered Broadcast Source. */
    LE_BROADCAST_MANAGER_SELF_SCAN_DISCOVERED_SOURCE_IND
} le_broadcast_manager_self_scan_message_t;

typedef enum {
    lebmss_success = 0,
    lebmss_fail,
    lebmss_bad_parameters,
    lebmss_timeout,
    lebmss_in_progress,
    lebmss_stopped,
    
    lebmss_max
} le_broadcast_manager_self_scan_status_t;

/*! \brief Confirmation of scan start status. */
typedef struct {
    le_broadcast_manager_self_scan_status_t status;
} LE_BROADCAST_MANAGER_SELF_SCAN_START_CFM_T;

/*! \brief Confirmation of scan stop status. */
typedef LE_BROADCAST_MANAGER_SELF_SCAN_START_CFM_T LE_BROADCAST_MANAGER_SELF_SCAN_STOP_CFM_T;

/*! \brief Notification of the current scan status. */
typedef LE_BROADCAST_MANAGER_SELF_SCAN_START_CFM_T LE_BROADCAST_MANAGER_SELF_SCAN_STATUS_IND_T;

/*! \brief Data for a subgroup in the BIG */
typedef struct
{
    uint32 bis_sync;
    uint8 metadata_length;
    uint8 *metadata;
} big_subgroup_t;

/*! \brief Notification of a discovered Broadcast Source. */
typedef struct {
    /* --- Stuff in the EA --- */

    uint32 broadcast_id;
    uint8 adv_sid;

    /*! RSSI of the received advert

        signed integer (-127 to 20 dBm)
        127 - RSSI not available.
    */
    int8 rssi;

    /*! Address of the Broadcast Source */
    tp_bdaddr source_tpaddr;

    /*! The "Broadcast Name" AD Type is defined in the PBP spec. */
    uint8 broadcast_name_len;
    uint8 *broadcast_name;

    /* --- Stuff in the PA --- */

    /*! Flag to say whether the BISes in the BIG are encrypted. (Determined from the BIGInfo) */
    bool encryption_required;

    /*! Periodic Advertisiment interval used by the Broadcast Source. */
    uint16 pa_interval;

    /*! BIG subgroup data */
    uint8 num_subgroups;
    big_subgroup_t *subgroups;
} LE_BROADCAST_MANAGER_SELF_SCAN_DISCOVERED_SOURCE_IND_T;

/*! \brief Parameters for self-scan filter */
typedef struct {
    /*! Exact Broadcast Id to match */
    uint32 broadcast_id;

    /*! Minimum RSSI of received adverts to match */
    uint8 rssi_threshold;
} filter_params_t;

/*! \brief Parameters for starting a scan */
typedef struct {
    /*! Scan will automatically stop after this timeout period (ms)

        The timeout has a minimum of #LE_BROADCAST_SELF_SCAN_TIMEOUT_MINIMUM_MS,
        and a maximum of #LE_BROADCAST_SELF_SCAN_TIMEOUT_MAXIMUM_MS.

        If the timeout is set to 0 then the default timeout of
        #LE_BROADCAST_SELF_SCAN_TIMEOUT_DEFAULT_MS is used.

        If the timeout is outside the above values then the start scan request
        will fail with the error lebmss_bad_parameters.
    */
    uint32 timeout;

    /*! Should the scan sync to the periodic train of each source before notifying the client. */
    bool sync_to_pa;

    /*! Parameters used to filter received adverts. */
    filter_params_t filter;
} self_scan_params_t;


#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)

/*! \brief Initialise the Broadcast Self-Scan module. */
bool LeBroadcastManager_SelfScanInit(Task init_task);

/*! \brief Start a Self-Scan for nearby Broadcast Sources.

    LE_BROADCAST_MANAGER_SELF_SCAN_START_CFM will be sent to the client Task
    to give the result of starting the self-scan.

    Scanning for nearby Broadcast Sources is a two-step process.
    
    The first step is scanning for Extended Adverts and looking for adverts
    that contain the BASS UUID. There is only basic data for the source
    available at this point: Broadcast Id, RSSI, Address.

    The second step is to sync to the Periodic Advert (PA) train of the
    broadcast source. From this more detailed information can be read, for 
    example: Program Info, available BISes.

    Each discovered Broadcast Source will be notified to the client in a
    LE_BROADCAST_MANAGER_SELF_SCAN_DISCOVERED_SOURCE_IND. There will be one
    notification per discovered source.

    If the scan is not stopped by the client by calling 
    #LeBroadcastManager_SelfScanStop then it will end after a timeout.

    \param[in] task Client task that the results of the scan will be sent to.
    \param[in] params Parameters to use for the self-scan.
*/
void LeBroadcastManager_SelfScanStart(Task task, const self_scan_params_t *params);

/*! \brief Stop a Self-Scan for Broadcast Sources.

    LE_BROADCAST_MANAGER_SELF_SCAN_STOP_CFM will be sent to the client Task to
    give the result of stopping the self-scan.

    \param[in] task Client task to stop the self-scan for.
*/
void LeBroadcastManager_SelfScanStop(Task task);

#else

#define LeBroadcastManager_SelfScanInit(init_task)  (UNUSED(init_task), FALSE)

#define LeBroadcastManager_SelfScanStart(client, params)    (UNUSED(client), UNUSED(params))

#define LeBroadcastManager_SelfScanStop(client) (UNUSED(client))

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN) */

#endif /* LE_BROADCAST_MANAGER_SELF_SCAN_H */
/*! @} */
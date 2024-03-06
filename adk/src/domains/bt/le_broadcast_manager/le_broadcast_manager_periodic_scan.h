/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup leabm
    \brief      Header for the LE Audio Broadcast Periodic Scan module
    @{
*/

#ifndef LE_BROADCAST_MANAGER_PERIODIC_SCAN_H
#define LE_BROADCAST_MANAGER_PERIODIC_SCAN_H


#include <connection.h>

#include "broadcast_sink_role.h"


typedef enum
{
    le_bm_periodic_scan_status_success = 0,
    le_bm_periodic_scan_status_fail = 1,
    le_bm_periodic_scan_status_timeout = 2
} le_bm_periodic_scan_status_t;

/*! \brief Opaque handle to and active periodic scan request. */
typedef uint32 le_bm_periodic_scan_handle;

/*! \brief Value for the invalid periodic scan handle. */
#define INVALID_LE_BM_PERIODIC_SCAN_HANDLE  0xFFFF

/*! \brief Callback interface */
typedef struct
{
    /*! \brief Called when an extended advertising report has been received */
    void (*EaReportReceived)(le_bm_periodic_scan_handle handle, uint32 broadcast_id, const CL_DM_BLE_EXT_SCAN_FILTERED_ADV_REPORT_IND_T* ind);

    /*! \brief Called when a start scan for periodic trains has returned a confirmation. */
    void (*StartScanPaSourceConfirm)(le_bm_periodic_scan_handle handle, le_bm_periodic_scan_status_t status);

    /*! \brief Called when a stop scan for periodic trains has returned a confirmation. */
    void (*StopScanPaSourceConfirm)(le_bm_periodic_scan_handle handle, le_bm_periodic_scan_status_t status);

    /*! \brief Called when a scan for periodic trains has timed out. */
    void (*ScanPaSourceTimeout)(le_bm_periodic_scan_handle handle);
} le_bm_periodic_scan_callback_interface_t;

typedef struct
{
    /* Client implementation of the callback interface. */
    le_bm_periodic_scan_callback_interface_t *interface;

    /* Periodic scan timeouts in ms. */
    uint32 timeout;
} le_bm_periodic_scan_params_t;

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)

void leBroadcastManager_PeriodicScanInit(void);

/*! \brief Request to start a periodic scan.

    The result of starting the scan will be returned in the
    #PeriodicScanStartScanPaSourceConfirm callback.

    Once the periodic scan is running any discovered periodic adverts will be
    returned in the #PeriodicScanEaReportReceived callback.

    If the timeout is set to 0 the scan will never timeout. The client must
    request to stop the scan by calling
    #leBroadcastManager_PeriodicScanStopRequest.

    \param[in] params Parameters to use for the periodic scan, including the
                      callback functions.
    \param[out] handle Handle to the periodic scan instance.

    \return TRUE if scan was started successfully; FALSE otherwise.
*/
bool leBroadcastManager_PeriodicScanStartRequest(le_bm_periodic_scan_params_t *params, le_bm_periodic_scan_handle *handle);

/*! \brief Request to stop an active periodic scan.

    The result of stopping the scan will be return in the
    #PeriodicScanStopScanPaSourceConfirm callback.

    \param[in] handle Handle of the periodic scan instance to stop.
*/
void leBroadcastManager_PeriodicScanStopRequest(le_bm_periodic_scan_handle handle);

/*! \brief Callback to handle Extended Advert reports.

    Implements #LeBapBroadcastSink_callback_interface_t::LeBapBroadcastSink_EaReportReceived.

    \param ind[in] Extended Advert report indication.
*/
void leBroadcastManager_PeriodicScanEaReportReceived(const CL_DM_BLE_EXT_SCAN_FILTERED_ADV_REPORT_IND_T* ind);

/*! \brief Callback to handle periodic scan start confirmation.

    Implements #LeBapBroadcastSink_callback_interface_t::LeBapBroadcastSink_StartScanPaSourceConfirm.

    \param[in] cfm Confirmation containg the status of the start periodic scan request.
*/
void leBroadcastManager_PeriodicScanStartScanPaSourceConfirm(const le_broadcast_sink_start_scan_pa_source_cfm_t* cfm);

/*! \brief Callback to handle periodic scan stop confirmation.

    Implements #LeBapBroadcastSink_callback_interface_t::LeBapBroadcastSink_StopScanPaSourceConfirm.

    \param[in] cfm Confirmation containg the status of the stop periodic scan request.
*/
void leBroadcastManager_PeriodicScanStopScanPaSourceConfirm(const le_broadcast_sink_stop_scan_pa_source_cfm_t* cfm);

/*! \brief Callback to handle periodic scan timeout.

    Implements #LeBapBroadcastSink_callback_interface_t::LeBapBroadcastSink_ScanPaSourceTimeout.
*/
void leBroadcastManager_PeriodicScanScanPaSourceTimeout(void);

#else

#define leBroadcastManager_PeriodicScanInit()

#define leBroadcastManager_PeriodicScanStartRequest(params, handle)     (UNUSED(params), UNUSED(handle), FALSE)

#define leBroadcastManager_PeriodicScanStopRequest(handle)  UNUSED(handle)

#define leBroadcastManager_PeriodicScanEaReportReceived(ind)    UNUSED(ind)

#define leBroadcastManager_PeriodicScanStartScanPaSourceConfirm(cfm)    UNUSED(cfm)

#define leBroadcastManager_PeriodicScanStopScanPaSourceConfirm(cfm) UNUSED(cfm)

#define leBroadcastManager_PeriodicScanScanPaSourceTimeout()

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN) */

#endif // LE_BROADCAST_MANAGER_PERIODIC_SCAN_H
/*! @} */
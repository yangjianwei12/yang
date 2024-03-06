/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_bap
    \brief
    @{
*/

#ifndef BROADCAST_SINK_ROLE_H_
#define BROADCAST_SINK_ROLE_H_

#include "connection.h"
#include "le_scan_manager.h"
#ifdef USE_SYNERGY
#include "bap_server_lib.h"
#else
#include "bap_server.h"
#endif


/*! \brief Status used for callbacks.
 */
typedef enum
{
    le_bap_broadcast_sink_status_success,
    le_bap_broadcast_sink_status_fail
} le_bap_broadcast_sink_status_t;

/*! \brief Data returned with the LeBapBroadcastSink_StartScanPaSourceConfirm callback;
 */
typedef struct
{
    le_bap_broadcast_sink_status_t status;
} le_broadcast_sink_start_scan_pa_source_cfm_t;

/*! \brief Data returned with the LeBapBroadcastSink_StopScanPaSourceConfirm callback;
 */
typedef struct
{
    le_bap_broadcast_sink_status_t status;
} le_broadcast_sink_stop_scan_pa_source_cfm_t;


/*! \brief Callback interface

    Implemented and registered by the application to receive and influence Broadcast Sink behaviour
*/
typedef struct
{
    /*! \brief Called when an extended advertising report has been received
     */
    void (*LeBapBroadcastSink_EaReportReceived)(const CL_DM_BLE_EXT_SCAN_FILTERED_ADV_REPORT_IND_T* message);
    /*! \brief Called when a periodic advertising report has been received
     */
    void (*LeBapBroadcastSink_PaReportReceived)(const CL_DM_BLE_PERIODIC_SCAN_SYNC_ADV_REPORT_IND_T* message);
    /*! \brief Called when a BIGInfo report has been received
     */
    void (*LeBapBroadcastSink_BigInfoReportReceived)(const CL_DM_BLE_BIGINFO_ADV_REPORT_IND_T* message);
    /*! \brief Called when the local Controller has sent a PA sync confirmation.
     */
    void (*LeBapBroadcastSink_PaSyncConfirmReceived)(const CL_DM_BLE_PERIODIC_SCAN_SYNC_TO_TRAIN_CFM_T* message);
    /*! \brief Called when the local Controller has notified us of PA sync loss.
     */
    void (*LeBapBroadcastSink_PaSyncLossIndicated)(const CL_DM_BLE_PERIODIC_SCAN_SYNC_LOST_IND_T* message);
    /*! \brief Called when a start scan for periodic trains has returned a confirmation.
     */
    void (*LeBapBroadcastSink_StartScanPaSourceConfirm)(const le_broadcast_sink_start_scan_pa_source_cfm_t* message);
    /*! \brief Called when a stop scan for periodic trains has returned a confirmation.
     */
    void (*LeBapBroadcastSink_StopScanPaSourceConfirm)(const le_broadcast_sink_stop_scan_pa_source_cfm_t* message);
    /*! \brief Called when a scan for periodic trains has timed out.
     */
    void (*LeBapBroadcastSink_ScanPaSourceTimeout)(void);
    /*! \brief Called when the local Controller has sent a PA sync cancel confirmation.
     */
    void (*LeBapBroadcastSink_PaSyncCancelConfirm)(const CL_DM_BLE_PERIODIC_SCAN_SYNC_CANCEL_CFM_T* message);
} LeBapBroadcastSink_callback_interface_t;


/*! \brief Filter to use when scanning for periodic trains.
 */
typedef struct
{
    typed_bdaddr* source_addr;
    uint8 *adv_sid;
} le_bap_broadcast_sink_scan_filter_t;


/*! \brief Initialises the BAP Broadcast Sink role

    Registers the manadatory PAC records and configures the LE Scan Manager.

    \param callbacks_to_register LeBapBroadcastSink_callback_interface_t to register
 */
void LeBapBroadcastSink_Init(const LeBapBroadcastSink_callback_interface_t * callbacks_to_register);


/*! \brief Handler for all connection library messages not sent directly.

    This function is called to handle any connection library messages sent to
    the application that the Broadcast Sink is interested in. If a message
    is processed then the function returns TRUE.

    \param  id              Identifier of the connection library message
    \param  message         The message content (if any)
    \param  already_handled Indication whether this message has been processed by
                            another module. The handler may choose to ignore certain
                            messages if they have already been handled.

    \returns TRUE if the message has been processed, otherwise FALSE.
*/
bool LeBapBroadcastSink_HandleConnectionLibraryMessages(MessageId id, Message message, bool already_handled);


/*! \brief Instructs the Controller to synchronise to a specific Periodic Train.

    Note that scanning will also be enabled to ensure that synchronisation occurs as
    quickly as possible. At a Controller level the 'LE Periodic Advertising Create Sync'
    operates independently to the 'LE Set Extended Scan Enable' command but both need
    to be called for sync to be possible.

    \param adv_sid        The Advertising SID subfield used to indentify the Periodic Train.
    \param source_addr    The typed Bluetooth Address of the source device.
 */
void LeBapBroadcastSink_SyncPaSource(typed_bdaddr* source_addr, uint8 adv_sid);

#ifdef USE_SYNERGY
void LeBapBroadcastSink_MessageHandleCmPrim(void* message);
#endif

/*! \brief Handler for all BAP Server messages.

    This function is called to handle any BAP server message sent to
    the application that the Broadcast Sink is interested in. 

    \param  message         The message content of BapServerBigInfoAdvReportInd
*/
void LeBapBroadcastSink_MessageHandleBapPrim(BapServerBigInfoAdvReportInd *message);

/*! \brief Instructs the Controller to stop synchronisation with Periodic Trains.

 */
void LeBapBroadcastSink_StopSyncPaSource(void);

/*! \brief Starts finding Periodic Trains.

    Starts a scan for periodic trains for a fixed period of time. Requests can be
    filtered based on address, the advertised SID, both or none.

    The outcome of the start request is reported via the registered callback 
    LeBapBroadcastSink_StartScanPaSourceConfirm. Multiple calls to the function
    will report a failure in the callback using #le_bap_broadcast_sink_status_fail.

    When the scanning time has completed, the registered callback 
    LeBapBroadcastSink_ScanPaSourceTimeout will be called.

    During the scan, any matching periodic trains will be reported using the 
    registered callback LeBapBroadcastSink_EaReportReceived

    The scan may be stopped early, using the function 
    LeBapBroadcastSink_StopScanPaSourceRequest().

    \param scan_filter   The scan filter to apply to the found periodic trains.
                         This can be NULL. The fields of the filter can also be NULL.
 */
void LeBapBroadcastSink_StartScanPaSourceRequest(uint32 timeout, le_bap_broadcast_sink_scan_filter_t* scan_filter);

/*! \brief Stops finding Periodic Trains.

    Stops a scan for periodic trains.

    The outcome of the stop request is reported via the registered callback 
    LeBapBroadcastSink_StopScanPaSourceConfirm. Multiple calls to the function
    will report a failure in the callback using #le_bap_broadcast_sink_status_fail.
 */
void LeBapBroadcastSink_StopScanPaSourceRequest(void);

/*! \brief Get the Sync to PA train timeout.

    \return The sync to train timeout in milliseconds.
*/
uint16 LeBapBroadcastSink_GetSyncToTrainTimeout(void);

/*! \brief Set the Sync to PA train timeout.

    \param timeout Timeout in milliseconds.
*/
void LeBapBroadcastSink_SetSyncToTrainTimeout(uint16 timeout);

#endif /* BROADCAST_SINK_ROLE_H_ */
/*! @} */
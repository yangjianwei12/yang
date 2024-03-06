/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    leabm
    \brief      LE Broadcast Manager interface with the BAP Broadcast Sink role.
*/

#if defined(INCLUDE_LE_AUDIO_BROADCAST)

#include "le_broadcast_manager_broadcast_sink.h"

#include "le_broadcast_manager.h"
#include "le_broadcast_manager_source.h"
#include "le_broadcast_manager_sync.h"

#ifdef INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN
#include "le_broadcast_manager_periodic_scan.h"
#endif

#include "broadcast_sink_role.h"

#include <logging.h>
#include <timestamp_event.h>
#include <audio_announcement_parser_lib.h>


#define BROADCAST_MANAGER_BROADCAST_SINK_LOG        DEBUG_LOG
#define BROADCAST_AUDIO_ANNOUNCEMENT_ADVERT_LENGTH  0x7


static void leBroadcastManager_EaReportReceived(const CL_DM_BLE_EXT_SCAN_FILTERED_ADV_REPORT_IND_T* message);
static void leBroadcastManager_PaReportReceived(const CL_DM_BLE_PERIODIC_SCAN_SYNC_ADV_REPORT_IND_T* message);
static void leBroadcastManager_BigInfoReportReceived(const CL_DM_BLE_BIGINFO_ADV_REPORT_IND_T* message);
static void leBroadcastManager_PaSyncConfirmReceived(const CL_DM_BLE_PERIODIC_SCAN_SYNC_TO_TRAIN_CFM_T* message);
static void leBroadcastManager_PaSyncCancelConfirm(const CL_DM_BLE_PERIODIC_SCAN_SYNC_CANCEL_CFM_T* message);
static void leBroadcastManager_PaSyncLossIndicated(const CL_DM_BLE_PERIODIC_SCAN_SYNC_LOST_IND_T* message);
static void leBroadcastManager_StartScanPaSourceConfirm(const le_broadcast_sink_start_scan_pa_source_cfm_t* message);
static void leBroadcastManager_StopScanPaSourceConfirm(const le_broadcast_sink_stop_scan_pa_source_cfm_t* message);
static void leBroadcastManager_ScanPaSourceTimeout(void);


#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
static void leBroadcastManager_BroadcastSinkEaReportReceived(le_bm_periodic_scan_handle handle, uint32 broadcast_id, const CL_DM_BLE_EXT_SCAN_FILTERED_ADV_REPORT_IND_T* message);
static void leBroadcastManager_BroadcastSinkStartScanPaSourceConfirm(le_bm_periodic_scan_handle handle, le_bm_periodic_scan_status_t status);
static void leBroadcastManager_BroadcastSinkStopScanPaSourceConfirm(le_bm_periodic_scan_handle handle, le_bm_periodic_scan_status_t status);
static void leBroadcastManager_BroadcastSinkScanPaSourceTimeout(le_bm_periodic_scan_handle handle);

static const LeBapBroadcastSink_callback_interface_t bap_broadcast_sink_interface =
{
    .LeBapBroadcastSink_EaReportReceived = leBroadcastManager_PeriodicScanEaReportReceived,
    .LeBapBroadcastSink_PaReportReceived = leBroadcastManager_PaReportReceived,
    .LeBapBroadcastSink_BigInfoReportReceived = leBroadcastManager_BigInfoReportReceived,
    .LeBapBroadcastSink_PaSyncConfirmReceived = leBroadcastManager_PaSyncConfirmReceived,
    .LeBapBroadcastSink_PaSyncLossIndicated = leBroadcastManager_PaSyncLossIndicated,
    .LeBapBroadcastSink_StartScanPaSourceConfirm = leBroadcastManager_PeriodicScanStartScanPaSourceConfirm,
    .LeBapBroadcastSink_StopScanPaSourceConfirm = leBroadcastManager_PeriodicScanStopScanPaSourceConfirm,
    .LeBapBroadcastSink_ScanPaSourceTimeout = leBroadcastManager_PeriodicScanScanPaSourceTimeout,
    .LeBapBroadcastSink_PaSyncCancelConfirm = leBroadcastManager_PaSyncCancelConfirm
};

static le_bm_periodic_scan_callback_interface_t periodic_scan_interface =
{
    .EaReportReceived = leBroadcastManager_BroadcastSinkEaReportReceived,
    .StartScanPaSourceConfirm = leBroadcastManager_BroadcastSinkStartScanPaSourceConfirm,
    .StopScanPaSourceConfirm = leBroadcastManager_BroadcastSinkStopScanPaSourceConfirm,
    .ScanPaSourceTimeout = leBroadcastManager_BroadcastSinkScanPaSourceTimeout
};
#else
static const LeBapBroadcastSink_callback_interface_t bap_broadcast_sink_interface =
{
    .LeBapBroadcastSink_EaReportReceived = leBroadcastManager_EaReportReceived,
    .LeBapBroadcastSink_PaReportReceived = leBroadcastManager_PaReportReceived,
    .LeBapBroadcastSink_BigInfoReportReceived = leBroadcastManager_BigInfoReportReceived,
    .LeBapBroadcastSink_PaSyncConfirmReceived = leBroadcastManager_PaSyncConfirmReceived,
    .LeBapBroadcastSink_PaSyncLossIndicated = leBroadcastManager_PaSyncLossIndicated,
    .LeBapBroadcastSink_StartScanPaSourceConfirm = leBroadcastManager_StartScanPaSourceConfirm,
    .LeBapBroadcastSink_StopScanPaSourceConfirm = leBroadcastManager_StopScanPaSourceConfirm,
    .LeBapBroadcastSink_ScanPaSourceTimeout = leBroadcastManager_ScanPaSourceTimeout,
    .LeBapBroadcastSink_PaSyncCancelConfirm = leBroadcastManager_PaSyncCancelConfirm
};
#endif


static void leBroadcastManager_EaReportReceived(const CL_DM_BLE_EXT_SCAN_FILTERED_ADV_REPORT_IND_T* message)
{
    uint32 broadcast_id = 0;
    AudioAnnouncementParserStatus statusAdvDataParsing = AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND;
    
    BROADCAST_MANAGER_BROADCAST_SINK_LOG("leBroadcastManager_EaReportReceived data_len=0x%x sid=0x%x", 
        message->adv_data_len,
        message->adv_sid
        );
    BROADCAST_MANAGER_BROADCAST_SINK_LOG("leBroadcastManager_EaReportReceived perm_addr=[%u %04x:%02x:%06x] curr_addr=[%u %04x:%02x:%06x]", 
        message->permanent_addr.type,
        message->permanent_addr.addr.nap,
        message->permanent_addr.addr.uap,
        message->permanent_addr.addr.lap,
        message->current_addr.type,
        message->current_addr.addr.nap,
        message->current_addr.addr.uap,
        message->current_addr.addr.lap
        );
        
    if (leBroadcastManager_GetSyncState() != broadcast_manager_sync_scanning_for_pa)
    {
        BROADCAST_MANAGER_BROADCAST_SINK_LOG("leBroadcastManager_EaReportReceived. No scanning");
        return;
    }

    statusAdvDataParsing = AudioAnnouncementParserBcastAudioAnnouncementParsing(message->adv_data_len,
                                                                                (uint8 *) message->adv_data,
                                                                                &broadcast_id);

    if (statusAdvDataParsing == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS)
    {
        bool match = FALSE;

        BROADCAST_MANAGER_BROADCAST_SINK_LOG("leBroadcastManager_EaReportReceived Found BAA Service Data UUID: Broadcast_ID:0x%x",
                                             broadcast_id);

        broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceById(leBroadcastManager_GetSyncStateSourceId());
        if (broadcast_source)
        {
            if (BdaddrTypedIsEmpty(&broadcast_source->source_match_address))
            {
                match = LeBroadcastManager_DoesSidAndBroadcastIdMatchSource(broadcast_source, message->adv_sid, broadcast_id);
            }
            else
            {
                match = LeBroadcastManager_DoesSidAndBdaddrMatchSource(broadcast_source, message->adv_sid, &message->current_addr);
            }
        }

        if (match)
        {
            /* Stop finding trains and sync to this train */
            LeBroadcastManager_StoreCurrentSourceAddr(&message->current_addr);
            LeBroadcastManager_StopScanForPaSource(broadcast_source->source_id, TRUE);
        }
    }
}

static void leBroadcastManager_PaReportReceived(const CL_DM_BLE_PERIODIC_SCAN_SYNC_ADV_REPORT_IND_T* message)
{
    UNUSED(message);
    BROADCAST_MANAGER_BROADCAST_SINK_LOG("leBroadcastManager_PaReportReceived");
    LeBroadcastManager_SourcePaReportReceived(message->sync_handle, message->adv_data_len, message->adv_data);
}

static void leBroadcastManager_BigInfoReportReceived(const CL_DM_BLE_BIGINFO_ADV_REPORT_IND_T* message)
{
    UNUSED(message);
    BROADCAST_MANAGER_BROADCAST_SINK_LOG("leBroadcastManager_BigInfoReportReceived");
    LeBroadcastManager_SourceBigInfoReportReceived(message->sync_handle, 
                                                   message->max_sdu,
                                                   message->encryption,
                                                   message->iso_interval);
}

static void leBroadcastManager_PaSyncConfirmReceived(const CL_DM_BLE_PERIODIC_SCAN_SYNC_TO_TRAIN_CFM_T* message)
{
    BROADCAST_MANAGER_BROADCAST_SINK_LOG("leBroadcastManager_PaSyncConfirmReceived.");
    if (message->status == 0xFFFF)
    {
        BROADCAST_MANAGER_BROADCAST_SINK_LOG("leBroadcastManager: Sync to PA train pending.");
    }
    else if (message->status == 0x0000)
    {
        scan_delegator_periodic_sync_t sync;
        sync.source_adv_sid = message->adv_sid;
        sync.sync_handle = message->sync_handle;
        sync.taddr_source = message->taddr;
        sync.status = scan_delegator_status_success;
        sync.service_data = 0;

        BROADCAST_MANAGER_BROADCAST_SINK_LOG("leBroadcastManager: Sync to PA train successful time from request:%u",
                              TimestampEvent_DeltaFrom(TIMESTAMP_EVENT_LE_BROADCAST_START_PA_SYNC));
        LeBroadcastManager_SourceSyncedToPA(&sync);
    }
    else if (   message->status == hci_error_max_nr_of_acl
             && leBroadcastManager_GetSyncState() == broadcast_manager_sync_syncing_to_pa)
    {
        /* The controller will return this error code if a PA to the source
           already exists. This can happen when the app PAST timeout triggers
           and the app requests a PA sync, but in the controller PAST has
           continued and synced to the PA anyway.

           In this case, ignore the PA sync error code and wait for the
           scan delegator periodic sync callback that will be generated by the
           successful PAST procedure. */
        BROADCAST_MANAGER_BROADCAST_SINK_LOG("leBroadcastManager: Sync to PA train failed but PAST has been successful.");
    }
    else
    {
        BROADCAST_MANAGER_BROADCAST_SINK_LOG("leBroadcastManager: Sync to PA train error. Status:%d. Time from request:%u", 
                              message->status,
                              TimestampEvent_DeltaFrom(TIMESTAMP_EVENT_LE_BROADCAST_START_PA_SYNC));

        LeBroadcastManager_SourceFailedSyncToPA();
    }
}

static void leBroadcastManager_PaSyncCancelConfirm(const CL_DM_BLE_PERIODIC_SCAN_SYNC_CANCEL_CFM_T* message)
{
    BROADCAST_MANAGER_BROADCAST_SINK_LOG("leBroadcastManager_PaSyncCancelConfirm enum:hci_status:%d", message->status);
    LeBroadcastManager_SourceCancelledSyncToPA(message->status == hci_success ? TRUE : FALSE);
}

static void leBroadcastManager_PaSyncLossIndicated(const CL_DM_BLE_PERIODIC_SCAN_SYNC_LOST_IND_T* message)
{
    BROADCAST_MANAGER_BROADCAST_SINK_LOG("leBroadcastManager_PaSyncLossIndicated");
    LeBroadcastManager_SourcePaSyncLoss(message->sync_handle);
}

static void leBroadcastManager_StartScanPaSourceConfirm(const le_broadcast_sink_start_scan_pa_source_cfm_t* message)
{
    BROADCAST_MANAGER_BROADCAST_SINK_LOG("leBroadcastManager_StartScanPaSourceConfirm status:0x%x", message->status);
}

static void leBroadcastManager_StopScanPaSourceConfirm(const le_broadcast_sink_stop_scan_pa_source_cfm_t* message)
{
    BROADCAST_MANAGER_BROADCAST_SINK_LOG("leBroadcastManager_StopScanPaSourceConfirm status:0x%x", message->status);
    
    switch (leBroadcastManager_GetSyncState())
    {
        case broadcast_manager_sync_cancelling_scan_for_pa:
            leBroadcastManager_SetSyncState(SCAN_DELEGATOR_SOURCE_ID_INVALID, broadcast_manager_sync_none);
            LeBroadcastManager_InitiateSyncCheckAllSources();
            break;

        case broadcast_manager_sync_cancelling_scan_sync_to_pa:
            LeBroadcastManager_StartSyncToPaSource(leBroadcastManager_GetSyncStateSourceId());
            break;
    
        default:
            break;
    }
}

static void leBroadcastManager_ScanPaSourceTimeout(void)
{
    BROADCAST_MANAGER_BROADCAST_SINK_LOG("leBroadcastManager_ScanPaSourceTimeout");
    
    LeBroadcastManager_SourceFailedSyncToPA();
}

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
static void leBroadcastManager_BroadcastSinkEaReportReceived(le_bm_periodic_scan_handle handle,
                                                             uint32 broadcast_id,
                                                             const CL_DM_BLE_EXT_SCAN_FILTERED_ADV_REPORT_IND_T* ind)
{
    DEBUG_LOG_FN_ENTRY("leBroadcastManager_BroadcastSinkEaReportReceived hdl 0x%x", handle);

    UNUSED(handle);
    UNUSED(broadcast_id);

    leBroadcastManager_EaReportReceived(ind);
}

static void leBroadcastManager_BroadcastSinkStartScanPaSourceConfirm(le_bm_periodic_scan_handle handle, le_bm_periodic_scan_status_t status)
{
    DEBUG_LOG_FN_ENTRY("leBroadcastManager_BroadcastSinkStartScanPaSourceConfirm hdl 0x%x status enum:le_bm_periodic_scan_status_t:%u",
                       handle, status);

    UNUSED(handle);

    le_broadcast_sink_start_scan_pa_source_cfm_t cfm = {0};
    cfm.status = (status == le_bm_periodic_scan_status_success) ? le_bap_broadcast_sink_status_success : le_bap_broadcast_sink_status_fail;
    leBroadcastManager_StartScanPaSourceConfirm(&cfm);
}

static void leBroadcastManager_BroadcastSinkStopScanPaSourceConfirm(le_bm_periodic_scan_handle handle, le_bm_periodic_scan_status_t status)
{
    DEBUG_LOG_FN_ENTRY("leBroadcastManager_BroadcastSinkStopScanPaSourceConfirm hdl 0x%x status enum:le_bm_periodic_scan_status_t:%u",
                       handle, status);

    UNUSED(handle);

    le_broadcast_sink_stop_scan_pa_source_cfm_t cfm = {0};
    cfm.status = (status == le_bm_periodic_scan_status_success) ? le_bap_broadcast_sink_status_success : le_bap_broadcast_sink_status_fail;
    leBroadcastManager_StopScanPaSourceConfirm(&cfm);
}

static void leBroadcastManager_BroadcastSinkScanPaSourceTimeout(le_bm_periodic_scan_handle handle)
{
    DEBUG_LOG_FN_ENTRY("leBroadcastManager_BroadcastSinkScanPaSourceTimeout hdl 0x%x", handle);

    UNUSED(handle);

    leBroadcastManager_ScanPaSourceTimeout();
}
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN) */


void LeBroadcastManager_BroadcastSinkInit(void)
{
    LeBapBroadcastSink_Init(&bap_broadcast_sink_interface);
}

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
le_bm_periodic_scan_callback_interface_t *leBroadcastManager_GetPeriodicScanInterface(void)
{
    return &periodic_scan_interface;
}
#endif

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST) */

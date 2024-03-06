/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_bap
    \brief
*/

#include "broadcast_sink_role.h"
#include "pacs_utilities.h"
#include "connection.h"
#include "le_scan_manager_periodic.h"

#include <bdaddr.h>
#include <logging.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>
#include <stdio.h>


#define BROADCAST_SINK_ROLE_LOG    DEBUG_LOG

#define MS_TO_SYNC_TIMEOUT(ms)                                      ((ms)/10)

#define BROADCAST_SINK_SYNC_TO_TRAIN_REPORT_PERIODIC_NO             0x00
#define BROADCAST_SINK_SYNC_TO_TRAIN_REPORT_PERIODIC_YES            0x01
#define BROADCAST_SINK_SYNC_TO_TRAIN_SKIP                           0x0000
#define BROADCAST_SINK_SYNC_TO_TRAIN_SYNC_TIMEOUT_MS                4000
#define BROADCAST_SINK_SYNC_TO_TRAIN_SYNC_CTE_TYPE                  0x00
#define BROADCAST_SINK_SYNC_TO_TRAIN_ATTEMPT_SYNC_SECONDS           5
#define BROADCAST_SINK_SYNC_TO_TRAIN_NUMBER_PERIODIC_TRAINS         1

#define BROADCAST_SINK_SCAN_START_ATTEMPTS  3
#define BROADCAST_SINK_SCAN_STOP_ATTEMPTS   5

/*! Scanning state. */
typedef enum
{
    broadcast_sink_scanning_none,
    broadcast_sink_starting_scan_pa,
    broadcast_sink_scanning_pa,
    broadcast_sink_stopping_scan_pa,
    broadcast_sink_scanning_timeout
} broadcast_sink_scanning_state_t;

/*! Broadcast Sink internal messages. */
typedef enum
{
    BROADCAST_SINK_INTERNAL_MESSAGE_PERIODIC_SCAN_FIND_TRAINS_TIMEOUT
} broadcast_sink_internal_message_t;

typedef struct
{
    TaskData sync_task;
    broadcast_sink_scanning_state_t scanning_state;
    TaskData scan_task;
    le_bap_broadcast_sink_scan_filter_t filter;
    uint8 le_scan_manager_attempts_remaining;

    uint16 sync_to_train_timeout;
} broadcast_sink_role_t;


static broadcast_sink_role_t broadcast_sink_role;

const LeBapBroadcastSink_callback_interface_t * broadcast_sink_registered_callbacks = NULL;
 
#ifdef USE_SYNERGY
void LeBapBroadcastSink_MessageHandleCmPrim(void* message);
#endif
static void leBapBroadcastSink_HandlePeriodicSyncLostInd(const CL_DM_BLE_PERIODIC_SCAN_SYNC_LOST_IND_T * ind);

/*! \brief Check whether callbacks have been registered

    When the callbacks are registered, individual callbacks are checked
    for being non-NULL so functions need only check that registration
    has occurred
*/
static void leBapBroadcastSink_VerifyCallbacksRegistered(void)
{
    if (!broadcast_sink_registered_callbacks)
    {
        DEBUG_LOG_ERROR("leBapBroadcastSink_VerifyCallbacksRegistered No callbacks registered");
        Panic();
    }
}

static void leBapBroadcastSink_VerifyCallbacksArePresent(const LeBapBroadcastSink_callback_interface_t * callbacks_to_verify)
{
    PanicNull((void *)callbacks_to_verify);
    PanicNull((void *)callbacks_to_verify->LeBapBroadcastSink_EaReportReceived);
    PanicNull((void *)callbacks_to_verify->LeBapBroadcastSink_PaReportReceived);
    PanicNull((void *)callbacks_to_verify->LeBapBroadcastSink_BigInfoReportReceived);
    PanicNull((void *)callbacks_to_verify->LeBapBroadcastSink_PaSyncConfirmReceived);
    PanicNull((void *)callbacks_to_verify->LeBapBroadcastSink_PaSyncLossIndicated);
    PanicNull((void *)callbacks_to_verify->LeBapBroadcastSink_StartScanPaSourceConfirm);
    PanicNull((void *)callbacks_to_verify->LeBapBroadcastSink_StopScanPaSourceConfirm);
    PanicNull((void *)callbacks_to_verify->LeBapBroadcastSink_ScanPaSourceTimeout);
    PanicNull((void *)callbacks_to_verify->LeBapBroadcastSink_PaSyncCancelConfirm);
}

static broadcast_sink_scanning_state_t leBapBroadcastSink_GetScanningState(void)
{
    return broadcast_sink_role.scanning_state;
}

static void leBapBroadcastSink_SetScanningState(broadcast_sink_scanning_state_t state)
{
    broadcast_sink_role.scanning_state = state;
}

static void leBapBroadcastSink_ClearFilters(void)
{
    if (broadcast_sink_role.filter.source_addr)
    {
        free(broadcast_sink_role.filter.source_addr);
        broadcast_sink_role.filter.source_addr = NULL;
    }
    if (broadcast_sink_role.filter.adv_sid)
    {
        free(broadcast_sink_role.filter.adv_sid);
        broadcast_sink_role.filter.adv_sid = NULL;
    }
}

void LeBapBroadcastSink_SyncPaSource(typed_bdaddr* source_addr, uint8 adv_sid)
{
    leBapBroadcastSink_VerifyCallbacksRegistered();

    /* Note that this function at present does not allow for the use of the
     * Periodic Advertisers List. Any request to sync to a PA train must be
     * accompanied by the typed source address and Avertising SID.
     *
     * Ditto for the 'options' bitfield to manage duplicate filtering.
     */
    CL_DM_ULP_PERIODIC_SCAN_TRAINS_T train_to_sync;
    train_to_sync.adv_sid = adv_sid;
    train_to_sync.taddr = *source_addr;

    ConnectionDmBlePeriodicScanSyncTrainReq(
            (Task) &broadcast_sink_role.sync_task,
            BROADCAST_SINK_SYNC_TO_TRAIN_REPORT_PERIODIC_YES,
            BROADCAST_SINK_SYNC_TO_TRAIN_SKIP,
            MS_TO_SYNC_TIMEOUT(broadcast_sink_role.sync_to_train_timeout),
            BROADCAST_SINK_SYNC_TO_TRAIN_SYNC_CTE_TYPE,
            BROADCAST_SINK_SYNC_TO_TRAIN_ATTEMPT_SYNC_SECONDS,
            BROADCAST_SINK_SYNC_TO_TRAIN_NUMBER_PERIODIC_TRAINS,
            &train_to_sync);
}

void LeBapBroadcastSink_StopSyncPaSource(void)
{
    leBapBroadcastSink_VerifyCallbacksRegistered();
#ifdef USE_SYNERGY
    CmPeriodicScanSyncToTrainCancelReq((Task) &broadcast_sink_role.sync_task);
#else
    ConnectionDmBlePeriodicScanSyncCancelReq();
#endif
}


static void leBapBroadcastSink_StartScanPaSourceRequest(void)
{
    uint16 size_ad_types = 1;
    uint8 ad_types[] = {ble_ad_type_service_16bit_uuid};
    le_periodic_advertising_filter_t periodic_advertising_filter;
    
    periodic_advertising_filter.size_ad_types = size_ad_types;
    periodic_advertising_filter.ad_types = ad_types;

    broadcast_sink_role.le_scan_manager_attempts_remaining--;

    LeScanManager_StartPeriodicScanFindTrains(&broadcast_sink_role.scan_task, &periodic_advertising_filter);
}

static void leBapBroadcastSink_StopScanPaSourceRequest(void)
{
    broadcast_sink_role.le_scan_manager_attempts_remaining--;

    LeScanManager_Stop(&broadcast_sink_role.scan_task);
}


static void leBapBroadcastSink_SendPeriodicFindTrainsCfm(le_bap_broadcast_sink_status_t status)
{
    le_broadcast_sink_start_scan_pa_source_cfm_t result = {status};

    broadcast_sink_registered_callbacks->LeBapBroadcastSink_StartScanPaSourceConfirm(&result);
}


static void leBapBroadcastSink_SendPeriodicStopFindTrainsCfm(le_bap_broadcast_sink_status_t status)
{
    le_broadcast_sink_stop_scan_pa_source_cfm_t result = {status};

    broadcast_sink_registered_callbacks->LeBapBroadcastSink_StopScanPaSourceConfirm(&result);
}


static void leBapBroadcastSink_HandleStartPeriodicScanFindTrainsCfm(const LE_SCAN_MANAGER_START_PERIODIC_SCAN_FIND_TRAINS_CFM_T *cfm)
{
    le_bap_broadcast_sink_status_t result = le_bap_broadcast_sink_status_success;

    DEBUG_LOG_FN_ENTRY("leBapBroadcastSink_HandleStartPeriodicScanFindTrainsCfm state enum:broadcast_sink_scanning_state_t:%d sts enum:le_scan_result_t:%d",
                        leBapBroadcastSink_GetScanningState(), cfm->status);

    if (leBapBroadcastSink_GetScanningState() == broadcast_sink_starting_scan_pa)
    {
        switch (cfm->status)
        {
            case LE_SCAN_MANAGER_RESULT_SUCCESS:
                leBapBroadcastSink_SetScanningState(broadcast_sink_scanning_pa);
                break;

            case LE_SCAN_MANAGER_RESULT_BUSY:
                /* In busy case, retry and do not call the callback */
                if (broadcast_sink_role.le_scan_manager_attempts_remaining)
                {
                    leBapBroadcastSink_StartScanPaSourceRequest();
                    return;
                }
                else
                {
                    DEBUG_LOG_ERROR("leBapBroadcastSink_HandleStartPeriodicScanFindTrainsCfm Le Scan Manager failed to start periodic find trains");
                }

                /* Fallthrough */

            case LE_SCAN_MANAGER_RESULT_FAILURE:
            default:
                result = le_bap_broadcast_sink_status_fail;
                leBapBroadcastSink_ClearFilters();
                leBapBroadcastSink_SetScanningState(broadcast_sink_scanning_none);
                break;
        }
        leBapBroadcastSink_SendPeriodicFindTrainsCfm(result);
    }
}


static void leBapBroadcastSink_HandleStopPeriodicScanFindTrainsCfm(const LE_SCAN_MANAGER_STOP_CFM_T *cfm)
{
    le_bap_broadcast_sink_status_t result = le_bap_broadcast_sink_status_success;

    DEBUG_LOG_FN_ENTRY("leBapBroadcastSink_HandleStopPeriodicScanFindTrainsCfm state enum:broadcast_sink_scanning_state_t:%d sts enum:le_scan_result_t:%d",
                        leBapBroadcastSink_GetScanningState(), cfm->status);

    if ((leBapBroadcastSink_GetScanningState() == broadcast_sink_stopping_scan_pa) ||
        (leBapBroadcastSink_GetScanningState() == broadcast_sink_scanning_timeout))
    {
        switch (cfm->status)
        {
            case LE_SCAN_MANAGER_RESULT_SUCCESS:
                MessageCancelFirst(&broadcast_sink_role.scan_task, BROADCAST_SINK_INTERNAL_MESSAGE_PERIODIC_SCAN_FIND_TRAINS_TIMEOUT);
                
                if (leBapBroadcastSink_GetScanningState() == broadcast_sink_scanning_timeout)
                {
                    broadcast_sink_registered_callbacks->LeBapBroadcastSink_ScanPaSourceTimeout();
                }
                break;

            case LE_SCAN_MANAGER_RESULT_BUSY:
                if (broadcast_sink_role.le_scan_manager_attempts_remaining)
                {
                    leBapBroadcastSink_StopScanPaSourceRequest();
                    return;
                }
                else
                {
                    DEBUG_LOG_ERROR("leBapBroadcastSink_HandleStartPeriodicScanFindTrainsCfm Le Scan Manager failed to start periodic find trains");
                }

            /* Fallthrough */

            case LE_SCAN_MANAGER_RESULT_FAILURE:
            default:
                result = le_bap_broadcast_sink_status_fail;
                break;
        }

        leBapBroadcastSink_SendPeriodicStopFindTrainsCfm(result);
        leBapBroadcastSink_ClearFilters();
        leBapBroadcastSink_SetScanningState(broadcast_sink_scanning_none);
    }
}


static void leBapBroadcastSink_HandlePeriodicScanFindTrainsTimeout(void)
{
    if (leBapBroadcastSink_GetScanningState() == broadcast_sink_scanning_pa)
    {
        leBapBroadcastSink_StopScanPaSourceRequest();
        leBapBroadcastSink_SetScanningState(broadcast_sink_scanning_timeout);
    }
}

COMPILE_TIME_ASSERT(   sizeof(LE_SCAN_MANAGER_PERIODIC_FIND_TRAINS_ADV_REPORT_IND_T) 
                    == sizeof(CL_DM_BLE_EXT_SCAN_FILTERED_ADV_REPORT_IND_T),
                    filtered_advert_size_diverged);

static void leBapBroadcastSink_HandlePeriodicFindTrainsAdvReportInd(const LE_SCAN_MANAGER_PERIODIC_FIND_TRAINS_ADV_REPORT_IND_T *ind)
{
    DEBUG_LOG_FN_ENTRY("leBapBroadcastSink_HandlePeriodicFindTrainsAdvReportInd. Seen SID:%d",ind->adv_sid);

    if (leBapBroadcastSink_GetScanningState() == broadcast_sink_scanning_pa)
    {
        if (   broadcast_sink_role.filter.adv_sid
            && ind->adv_sid != *broadcast_sink_role.filter.adv_sid)
        {
            DEBUG_LOG_INFO("leBapBroadcastSink_HandlePeriodicFindTrainsAdvReportInd filtered out on sid %d-%d",
                            broadcast_sink_role.filter.adv_sid, ind->adv_sid);
            return;
        }
        if (   broadcast_sink_role.filter.source_addr
            && !(   BdaddrTypedIsSame(broadcast_sink_role.filter.source_addr, &ind->permanent_addr)
                 || BdaddrTypedIsSame(broadcast_sink_role.filter.source_addr, &ind->current_addr)))
        {
            DEBUG_LOG_INFO("leBapBroadcastSink_HandlePeriodicFindTrainsAdvReportInd filtered out on address",
                            broadcast_sink_role.filter.adv_sid, ind->adv_sid);
            return;
        }

        broadcast_sink_registered_callbacks->LeBapBroadcastSink_EaReportReceived((const CL_DM_BLE_EXT_SCAN_FILTERED_ADV_REPORT_IND_T *)ind);
    }
}


static void leBapBroadcastSink_HandlePeriodicScanSyncConfirm(const CL_DM_BLE_PERIODIC_SCAN_SYNC_TO_TRAIN_CFM_T * cfm)
{
    broadcast_sink_registered_callbacks->LeBapBroadcastSink_PaSyncConfirmReceived(cfm);

    if (cfm->status == hci_success)
    {
        /*! \todo Need to enable routing of the reports in scan manager */
    }
}

static void leBapBroadcastSink_HandlePeriodicScanCancelConfirm(const CL_DM_BLE_PERIODIC_SCAN_SYNC_CANCEL_CFM_T *cfm)
{
    broadcast_sink_registered_callbacks->LeBapBroadcastSink_PaSyncCancelConfirm(cfm);
}

#ifdef USE_SYNERGY
void LeBapBroadcastSink_MessageHandleCmPrim(void* message)
{
    CsrBtCmPrim *primType = (CsrBtCmPrim *) message;
    switch (*primType)
    {
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CFM:
            {
                CL_DM_BLE_PERIODIC_SCAN_SYNC_TO_TRAIN_CFM_T cfm;
                DEBUG_LOG("LeBapBroadcastSink: Callback for CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CFM");
                cfm.status = ((CmPeriodicScanSyncToTrainCfm *) message)->resultCode;
                cfm.sync_handle = ((CmPeriodicScanSyncToTrainCfm *) message)->syncHandle;
                cfm.adv_sid= ((CmPeriodicScanSyncToTrainCfm *) message)->advSid;
                cfm.taddr.type = ((CmPeriodicScanSyncToTrainCfm *) message)->addrt.type;
                cfm.taddr.addr.uap = ((CmPeriodicScanSyncToTrainCfm *) message)->addrt.addr.uap;
                cfm.taddr.addr.nap = ((CmPeriodicScanSyncToTrainCfm *) message)->addrt.addr.nap;
                cfm.taddr.addr.lap = ((CmPeriodicScanSyncToTrainCfm *) message)->addrt.addr.lap;
                cfm.adv_phy = ((CmPeriodicScanSyncToTrainCfm *) message)->advPhy;
                cfm.periodic_adv_interval = ((CmPeriodicScanSyncToTrainCfm *) message)->periodicAdvInterval;
                cfm.adv_clock_accuracy = ((CmPeriodicScanSyncToTrainCfm *) message)->advClockAccuracy;
                leBapBroadcastSink_HandlePeriodicScanSyncConfirm(&cfm);
            }
            break;

        case CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CANCEL_CFM:
            {
                CL_DM_BLE_PERIODIC_SCAN_SYNC_CANCEL_CFM_T cfm;
                DEBUG_LOG("LeBapBroadcastSink: Callback for CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CANCEL_CFM");
                cfm.status = ((CmPeriodicScanSyncToTrainCancelCfm *) message)->resultCode;
                leBapBroadcastSink_HandlePeriodicScanCancelConfirm(&cfm);
            }
            break;

        case CSR_BT_CM_PERIODIC_SCAN_SYNC_LOST_IND:
            {
                CL_DM_BLE_PERIODIC_SCAN_SYNC_LOST_IND_T ind;
                DEBUG_LOG("LeBapBroadcastSink: Callback for CSR_BT_CM_PERIODIC_SCAN_SYNC_LOST_IND");
                ind.sync_handle = ((CmPeriodicScanSyncLostInd *) message)->syncHandle;
                leBapBroadcastSink_HandlePeriodicSyncLostInd(&ind);
            }
            break;

        case CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_IND:
            {
                CL_DM_BLE_PERIODIC_SCAN_SYNC_ADV_REPORT_IND_T ind;
                DEBUG_LOG("LeBapBroadcastSink: Callback for CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_IND");
                ind.cte_type = ((CmPeriodicScanSyncAdvReportInd *) message)->cteType;
                ind.adv_data = ((CmPeriodicScanSyncAdvReportInd *) message)->data;
                ind.adv_data_len = ((CmPeriodicScanSyncAdvReportInd *) message)->dataLength;
                ind.rssi = ((CmPeriodicScanSyncAdvReportInd *) message)->rssi;
                ind.sync_handle = ((CmPeriodicScanSyncAdvReportInd *) message)->syncHandle;
                ind.tx_power = ((CmPeriodicScanSyncAdvReportInd *) message)->txPower;
                broadcast_sink_registered_callbacks->LeBapBroadcastSink_PaReportReceived(&ind);
            }
            break;

        case CSR_BT_CM_BLE_BIGINFO_ADV_REPORT_IND:
            {
                CL_DM_BLE_BIGINFO_ADV_REPORT_IND_T ind;
                DEBUG_LOG("LeBapBroadcastSink: Callback for CSR_BT_CM_BLE_BIGINFO_ADV_REPORT_IND");
                ind.bn = ((CmBleBigInfoAdvReportInd *) message)->bigParams.bn;
                ind.encryption = ((CmBleBigInfoAdvReportInd *) message)->encryption;
                ind.framing = ((CmBleBigInfoAdvReportInd *) message)->framing;
                ind.irc = ((CmBleBigInfoAdvReportInd *) message)->bigParams.irc;
                ind.iso_interval = ((CmBleBigInfoAdvReportInd *) message)->bigParams.iso_interval;
                ind.max_pdu = ((CmBleBigInfoAdvReportInd *) message)->bigParams.max_pdu;
                ind.max_sdu = ((CmBleBigInfoAdvReportInd *) message)->maxSdu;
                ind.nse = ((CmBleBigInfoAdvReportInd *) message)->bigParams.nse;
                ind.num_bis = ((CmBleBigInfoAdvReportInd *) message)->numBis;
                ind.phy = ((CmBleBigInfoAdvReportInd *) message)->bigParams.phy;
                ind.pto = ((CmBleBigInfoAdvReportInd *) message)->bigParams.pto;
                ind.sdu_interval = ((CmBleBigInfoAdvReportInd *) message)->sduInterval;
                ind.sync_handle = ((CmBleBigInfoAdvReportInd *) message)->syncHandle;
                broadcast_sink_registered_callbacks->LeBapBroadcastSink_BigInfoReportReceived(&ind);
            }
            break;

        default:
            DEBUG_LOG("LeBapBroadcastSink: Unhandled CM message %d(0x%x)", *primType, *primType);
            break;
    }

    CmFreeUpstreamMessageContents(message);
}
#endif

static void LeBapBroadcastSink_SyncMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    DEBUG_LOG_FN_ENTRY("LeBapBroadcastSink_SyncMessageHandler MSG:0x%x", id);

    switch (id)
    {
#ifdef USE_SYNERGY
       case CM_PRIM:
            LeBapBroadcastSink_MessageHandleCmPrim((void *) message);
            break;
#else
        case CL_DM_BLE_PERIODIC_SCAN_SYNC_TO_TRAIN_CFM:
            leBapBroadcastSink_HandlePeriodicScanSyncConfirm((const CL_DM_BLE_PERIODIC_SCAN_SYNC_TO_TRAIN_CFM_T *)message);
            break;

        case CL_DM_BLE_PERIODIC_SCAN_SYNC_CANCEL_CFM:
            leBapBroadcastSink_HandlePeriodicScanCancelConfirm((const CL_DM_BLE_PERIODIC_SCAN_SYNC_CANCEL_CFM_T *)message);
            break;

        case CL_DM_BLE_PERIODIC_SCAN_SYNC_LOST_IND:
            DEBUG_LOG("LeBapBroadcastSink: Callback for CL_DM_BLE_PERIODIC_SCAN_SYNC_LOST_IND");
            leBapBroadcastSink_HandlePeriodicSyncLostInd((CL_DM_BLE_PERIODIC_SCAN_SYNC_LOST_IND_T *) message);
            break;

        case CL_DM_BLE_PERIODIC_SCAN_SYNC_ADV_REPORT_IND:
            DEBUG_LOG("LeBapBroadcastSink: Callback for LE_SCAN_MANAGER_PERIODIC_SYNC_ADV_REPORT_IND");
            broadcast_sink_registered_callbacks->LeBapBroadcastSink_PaReportReceived((const CL_DM_BLE_PERIODIC_SCAN_SYNC_ADV_REPORT_IND_T *) message);
            break;
#endif

        default:
            BROADCAST_SINK_ROLE_LOG("LeBapBroadcastSink_SyncMessageHandler: Unhandled message %d(0x%x)", id, id);
            break;
    }
}
static void LeBapBroadcastSink_ScanMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    BROADCAST_SINK_ROLE_LOG("LeBapBroadcastSink_ScanMessageHandler: MSG:0x%x", id);

    switch (id)
    {
        case LE_SCAN_MANAGER_START_PERIODIC_SCAN_FIND_TRAINS_CFM:
            leBapBroadcastSink_HandleStartPeriodicScanFindTrainsCfm((LE_SCAN_MANAGER_START_PERIODIC_SCAN_FIND_TRAINS_CFM_T *)message);
            break;

        case LE_SCAN_MANAGER_STOP_CFM:
            leBapBroadcastSink_HandleStopPeriodicScanFindTrainsCfm((LE_SCAN_MANAGER_STOP_CFM_T *)message);
            break;

        case LE_SCAN_MANAGER_PERIODIC_FIND_TRAINS_ADV_REPORT_IND:
            leBapBroadcastSink_HandlePeriodicFindTrainsAdvReportInd((LE_SCAN_MANAGER_PERIODIC_FIND_TRAINS_ADV_REPORT_IND_T *)message);
            break;

        case BROADCAST_SINK_INTERNAL_MESSAGE_PERIODIC_SCAN_FIND_TRAINS_TIMEOUT:
            leBapBroadcastSink_HandlePeriodicScanFindTrainsTimeout();
            break;

        default:
            DEBUG_LOG_VERBOSE("LeBapBroadcastSink_ScanMessageHandler: Unhandled MSG:0x%x", id);
            break;
    }
}

void LeBapBroadcastSink_Init(const LeBapBroadcastSink_callback_interface_t * callbacks_to_register)
{
    leBapBroadcastSink_VerifyCallbacksArePresent(callbacks_to_register);

    memset(&broadcast_sink_role, 0, sizeof(broadcast_sink_role_t));

    /* Set up the Broadcast Sink's own message handlers. */
    broadcast_sink_role.sync_task.handler = LeBapBroadcastSink_SyncMessageHandler;
    broadcast_sink_role.scan_task.handler = LeBapBroadcastSink_ScanMessageHandler;
    
    /* Ensure mandatory PAC records for a LE Broadcast Sink are registered. */
    LeBapPacsUtilities_Init();
    broadcast_sink_registered_callbacks = callbacks_to_register;

#ifdef INSTALL_LOCAL_PA_SYNC_TEST
    /* Just to test - see if we can sync to a PA train. */
    {
        typed_bdaddr test_addr;
        test_addr.addr.lap = 0x001680;
        test_addr.addr.nap = 0x0002;
        test_addr.addr.uap = 0x5b;
        test_addr.type = TYPED_BDADDR_PUBLIC;
        LeBapBroadcastSink_SyncPaSource(&test_addr, 0x00);
    }
#endif

    /* Setup default timeouts */
    broadcast_sink_role.sync_to_train_timeout = BROADCAST_SINK_SYNC_TO_TRAIN_SYNC_TIMEOUT_MS;
}

static void leBapBroadcastSink_HandlePeriodicSyncLostInd(const CL_DM_BLE_PERIODIC_SCAN_SYNC_LOST_IND_T * ind)
{
    BROADCAST_SINK_ROLE_LOG("LeBapBroadcastSink: Handling CL_DM_BLE_PERIODIC_SCAN_SYNC_LOST_IND_T");
    broadcast_sink_registered_callbacks->LeBapBroadcastSink_PaSyncLossIndicated((CL_DM_BLE_PERIODIC_SCAN_SYNC_LOST_IND_T *) ind);
    ConnectionDmBlePeriodicScanSyncLostRsp(ind->sync_handle);
}

bool LeBapBroadcastSink_HandleConnectionLibraryMessages(MessageId id, Message message, bool already_handled)
{
    UNUSED(already_handled);

    switch(id)
    {
        case CL_DM_BLE_PERIODIC_SCAN_SYNC_LOST_IND:
            BROADCAST_SINK_ROLE_LOG("LeBapBroadcastSink_CL: Callback for CL_DM_BLE_PERIODIC_SCAN_SYNC_LOST_IND");
            leBapBroadcastSink_HandlePeriodicSyncLostInd((CL_DM_BLE_PERIODIC_SCAN_SYNC_LOST_IND_T *) message);
            break;

        case CL_DM_BLE_PERIODIC_SCAN_SYNC_ADV_REPORT_IND:
            DEBUG_LOG("LeBapBroadcastSink: Callback for CL_DM_BLE_PERIODIC_SCAN_SYNC_ADV_REPORT_IND");
            broadcast_sink_registered_callbacks->LeBapBroadcastSink_PaReportReceived((const CL_DM_BLE_PERIODIC_SCAN_SYNC_ADV_REPORT_IND_T *) message);
            break;

        case CL_DM_BLE_BIGINFO_ADV_REPORT_IND:
            DEBUG_LOG("LeBapBroadcastSink: Callback for CL_DM_BLE_BIGINFO_ADV_REPORT_IND");
            broadcast_sink_registered_callbacks->LeBapBroadcastSink_BigInfoReportReceived((const CL_DM_BLE_BIGINFO_ADV_REPORT_IND_T *) message);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

void LeBapBroadcastSink_StartScanPaSourceRequest(uint32 timeout, le_bap_broadcast_sink_scan_filter_t* scan_filter)
{
    leBapBroadcastSink_VerifyCallbacksRegistered();

    if (leBapBroadcastSink_GetScanningState() == broadcast_sink_scanning_none)
    {
        broadcast_sink_role.le_scan_manager_attempts_remaining = BROADCAST_SINK_SCAN_START_ATTEMPTS;
        broadcast_sink_role.filter.source_addr = NULL;
        broadcast_sink_role.filter.adv_sid = NULL;

        if (scan_filter)
        {
            if (scan_filter->source_addr)
            {
                broadcast_sink_role.filter.source_addr = PanicUnlessMalloc(sizeof(typed_bdaddr));
                *broadcast_sink_role.filter.source_addr = *scan_filter->source_addr;
            }
            if (scan_filter->adv_sid)
            {
                broadcast_sink_role.filter.adv_sid = PanicUnlessMalloc(sizeof(uint8));
                *broadcast_sink_role.filter.adv_sid = *scan_filter->adv_sid;
            }
        }

        leBapBroadcastSink_SetScanningState(broadcast_sink_starting_scan_pa);

        leBapBroadcastSink_StartScanPaSourceRequest();


        if (timeout > 0)
        {
            MessageSendLater(&broadcast_sink_role.scan_task,
                             BROADCAST_SINK_INTERNAL_MESSAGE_PERIODIC_SCAN_FIND_TRAINS_TIMEOUT,
                             NULL,
                             timeout);
        }
    }
    else
    {
        leBapBroadcastSink_SendPeriodicFindTrainsCfm(le_bap_broadcast_sink_status_fail);
    }
}

void LeBapBroadcastSink_StopScanPaSourceRequest(void)
{
    leBapBroadcastSink_VerifyCallbacksRegistered();

    if (leBapBroadcastSink_GetScanningState() == broadcast_sink_scanning_pa)
    {
        broadcast_sink_role.le_scan_manager_attempts_remaining = BROADCAST_SINK_SCAN_STOP_ATTEMPTS;

        leBapBroadcastSink_StopScanPaSourceRequest();
        leBapBroadcastSink_SetScanningState(broadcast_sink_stopping_scan_pa);
    }
    else
    {
        leBapBroadcastSink_SendPeriodicStopFindTrainsCfm(le_bap_broadcast_sink_status_fail);
    }
}

void LeBapBroadcastSink_MessageHandleBapPrim(BapServerBigInfoAdvReportInd *message)
{
    CL_DM_BLE_BIGINFO_ADV_REPORT_IND_T ind;

    DEBUG_LOG("LeBapBroadcastSink_MessageHandleBapPrim");
    ind.bn = message->bn;
    ind.encryption = message->encryption;
    ind.framing = message->framing;
    ind.irc = message->irc;
    ind.iso_interval = message->isoInterval;
    ind.max_pdu = message->maxPdu;
    ind.max_sdu = message->maxSdu;
    ind.nse = message->nse;
    ind.num_bis = message->numBis;
    ind.phy = message->phy;
    ind.pto = message->pto;
    ind.sdu_interval = message->sduInterval;
    ind.sync_handle = message->syncHandle;
    broadcast_sink_registered_callbacks->LeBapBroadcastSink_BigInfoReportReceived(&ind);
}

#ifdef HOSTED_TEST_ENVIRONMENT

const TaskData * LeBapBroadcastSink_GetSyncMessageHandler(void)
{
    return &broadcast_sink_role.sync_task;
}

const TaskData * LeBapBroadcastSink_GetScanMessageHandler(void)
{
    return &broadcast_sink_role.scan_task;
}

#endif

uint16 LeBapBroadcastSink_GetSyncToTrainTimeout(void)
{
    return broadcast_sink_role.sync_to_train_timeout;
}

void LeBapBroadcastSink_SetSyncToTrainTimeout(uint16 timeout)
{
    broadcast_sink_role.sync_to_train_timeout = timeout;
}

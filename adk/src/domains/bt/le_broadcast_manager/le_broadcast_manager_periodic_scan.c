/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    leabm
    \brief      Header for the LE Audio Broadcast Periodic Scan module
*/

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)

#include <logging.h>
#include <audio_announcement_parser_lib.h>

#include "le_broadcast_manager_periodic_scan.h"


#define ARRAY_INDEX(i, a)  ((i) - ARRAY_BEGIN((a)))


/*! \brief The mximum number of periodic scan clients this module can handle. */
#define LE_BROADCAST_MANAGER_PERIODIC_SCAN_MAX_INSTANCES    3


typedef enum
{
    LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_DELAYED_START_CFM = INTERNAL_MESSAGE_BASE,
    LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_DELAYED_STOP_CFM,

    LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_TIMEOUT_BASE,
    LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_TIMEOUT_TOP = LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_TIMEOUT_BASE + LE_BROADCAST_MANAGER_PERIODIC_SCAN_MAX_INSTANCES
} le_bm_periodic_scan_internal_msg_t;

#define HANDLE_TO_TIMEOUT_MSG_ID(handle)    (LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_TIMEOUT_BASE + (handle))
#define TIMEOUT_MSG_ID_TO_HANDLE(msg_id)    ((msg_id) - LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_TIMEOUT_BASE)
#define IS_TIMEOUT_MSG_ID(msg_id)           (   (LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_TIMEOUT_BASE <= (msg_id)) \
                                             && ((msg_id) < LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_TIMEOUT_TOP))

typedef struct
{
    le_bm_periodic_scan_handle handle;
    le_bm_periodic_scan_status_t status;
} LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_DELAYED_START_CFM_T;

typedef LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_DELAYED_START_CFM_T LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_DELAYED_STOP_CFM_T;

typedef struct
{
    /*! Opaque handle for this scan instance. */
    le_bm_periodic_scan_handle handle;

    /*! Copy of the scan parameters. */
    le_bm_periodic_scan_params_t params;
} le_bm_periodic_scan_instance_t;

typedef struct
{
    TaskData task;

    bool is_scanning;

    /*! Array of periodic scan instances. */
    le_bm_periodic_scan_instance_t scan_instances[LE_BROADCAST_MANAGER_PERIODIC_SCAN_MAX_INSTANCES];
} le_bm_periodic_scan_task_data_t;


static le_bm_periodic_scan_task_data_t le_broadcast_manager_periodic_scan_data = {0};

#define leBroadcastManager_PeriodicScanGetTaskData() (&le_broadcast_manager_periodic_scan_data)

#define leBroadcastManager_PeriodicScanGetTask()    (&le_broadcast_manager_periodic_scan_data.task)


static le_bm_periodic_scan_status_t leBroadcastManager_PeriodicScanConvertBapStatus(le_bap_broadcast_sink_status_t bap_status)
{
    switch (bap_status)
    {
    case le_bap_broadcast_sink_status_success: return le_bm_periodic_scan_status_success;
    case le_bap_broadcast_sink_status_fail: return le_bm_periodic_scan_status_fail;
    default:
        DEBUG_LOG_WARN("leBroadcastManager_PeriodicScanConvertBapStatus unrecognised status enum:le_bap_broadcast_sink_status_t:%u", bap_status);
        break;
    }

    return le_bm_periodic_scan_status_fail;
}

static void leBroadcastManager_PeriodicScanVerifyInterface(le_bm_periodic_scan_callback_interface_t *interface)
{
    PanicNull(interface);
    PanicNull((void *)interface->StartScanPaSourceConfirm);
    PanicNull((void *)interface->StopScanPaSourceConfirm);
    PanicNull((void *)interface->EaReportReceived);
    PanicNull((void *)interface->ScanPaSourceTimeout);
}

static le_bm_periodic_scan_instance_t *leBroadcastManager_PeriodicScanGetInstanceByHandle(le_bm_periodic_scan_handle handle)
{
    le_bm_periodic_scan_task_data_t * ps_data = leBroadcastManager_PeriodicScanGetTaskData();
    le_bm_periodic_scan_instance_t *scan_instance = NULL;
    le_bm_periodic_scan_instance_t *found_instance = NULL;

    ARRAY_FOREACH(scan_instance, ps_data->scan_instances)
    {
        if (scan_instance->handle == handle)
        {
            found_instance = scan_instance;
            break;
        }
    }

    return found_instance;
}

static unsigned leBroadcastManager_PeriodicScanGetInstanceCount(void)
{
    le_bm_periodic_scan_task_data_t * ps_data = leBroadcastManager_PeriodicScanGetTaskData();
    le_bm_periodic_scan_instance_t *scan_instance = NULL;
    unsigned count = 0;

    ARRAY_FOREACH(scan_instance, ps_data->scan_instances)
    {
        if (scan_instance->handle != INVALID_LE_BM_PERIODIC_SCAN_HANDLE)
        {
            count++;
        }
    }

    return count;
}

static void leBroadcastManager_PeriodicScanTimeoutStart(le_bm_periodic_scan_handle handle, uint32 timeout)
{
    DEBUG_LOG_FN_ENTRY("leBroadcastManager_PeriodicScanTimeoutStart hdl 0x%x timeout %u", handle, timeout);

    MessageId id = HANDLE_TO_TIMEOUT_MSG_ID(handle);
    MessageCancelFirst(leBroadcastManager_PeriodicScanGetTask(), id);
    MessageSendLater(leBroadcastManager_PeriodicScanGetTask(), id, NULL, timeout);
}

static void leBroadcastManager_PeriodicScanTimeoutCancel(le_bm_periodic_scan_handle handle)
{
    DEBUG_LOG_FN_ENTRY("leBroadcastManager_PeriodicScanTimeoutCancel hdl 0x%x", handle);

    MessageId id = HANDLE_TO_TIMEOUT_MSG_ID(handle);
    MessageCancelFirst(leBroadcastManager_PeriodicScanGetTask(), id);
}

static void leBroadcastManager_PeriodicScanResetInstance(le_bm_periodic_scan_instance_t *instance)
{
    leBroadcastManager_PeriodicScanTimeoutCancel(instance->handle);

    memset(instance, 0, sizeof(*instance));
    instance->handle = INVALID_LE_BM_PERIODIC_SCAN_HANDLE;
}

static void leBroadcastManager_PeriodicScanSendDelayedStartCfm(le_bm_periodic_scan_handle handle, le_bm_periodic_scan_status_t status)
{
    MESSAGE_MAKE(cfm, LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_DELAYED_START_CFM_T);
    cfm->handle = handle;
    cfm->status = status;

    MessageSend(leBroadcastManager_PeriodicScanGetTask(),
                LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_DELAYED_START_CFM,
                cfm);
}

static void leBroadcastManager_PeriodicScanSendDelayedStopCfm(le_bm_periodic_scan_handle handle, le_bm_periodic_scan_status_t status)
{
    MESSAGE_MAKE(cfm, LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_DELAYED_STOP_CFM_T);
    cfm->handle = handle;
    cfm->status = status;

    MessageSend(leBroadcastManager_PeriodicScanGetTask(),
                LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_DELAYED_STOP_CFM,
                cfm);
}

static void leBroadcastManager_PeriodicScanHandleInternalSendStartCfm(const LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_DELAYED_START_CFM_T *cfm)
{
    le_bm_periodic_scan_instance_t *scan_instance = leBroadcastManager_PeriodicScanGetInstanceByHandle(cfm->handle);
    if (scan_instance)
    {
        scan_instance->params.interface->StartScanPaSourceConfirm(cfm->handle, cfm->status);
    }
}

static void leBroadcastManager_PeriodicScanHandleInternalSendStopCfm(const LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_DELAYED_STOP_CFM_T *cfm)
{
    le_bm_periodic_scan_instance_t *scan_instance = leBroadcastManager_PeriodicScanGetInstanceByHandle(cfm->handle);
    if (scan_instance)
    {
        scan_instance->params.interface->StopScanPaSourceConfirm(cfm->handle, cfm->status);
        leBroadcastManager_PeriodicScanResetInstance(scan_instance);
    }
}

static void leBroadcastManager_PeriodicScanHandleInternalTimeout(le_bm_periodic_scan_handle handle)
{
    DEBUG_LOG_FN_ENTRY("leBroadcastManager_PeriodicScanHandleInternalTimeout hdl 0x%x", handle);

    le_bm_periodic_scan_instance_t *scan_instance = leBroadcastManager_PeriodicScanGetInstanceByHandle(handle);
    if (scan_instance)
    {
        scan_instance->params.interface->ScanPaSourceTimeout(scan_instance->handle);
        leBroadcastManager_PeriodicScanResetInstance(scan_instance);
    }

    if (0 == leBroadcastManager_PeriodicScanGetInstanceCount())
    {
        /* If there are no other clients, request to stop the periodic scan. */
        LeBapBroadcastSink_StopScanPaSourceRequest();
    }
}

static void leBroadcastManager_PeriodicScanMessageHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);
    UNUSED(msg);

    DEBUG_LOG_FN_ENTRY("leBroadcastManager_PeriodicScanMessageHandler id 0x%x", id);

    switch (id)
    {
    case LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_DELAYED_START_CFM:
        leBroadcastManager_PeriodicScanHandleInternalSendStartCfm((const LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_DELAYED_START_CFM_T *)msg);
        break;

    case LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_DELAYED_STOP_CFM:
        leBroadcastManager_PeriodicScanHandleInternalSendStopCfm((const LE_BROADCAST_MANAGER_PERIODIC_SCAN_INTERNAL_DELAYED_STOP_CFM_T *)msg);
        break;

    default:
        if (IS_TIMEOUT_MSG_ID(id))
        {
            leBroadcastManager_PeriodicScanHandleInternalTimeout(TIMEOUT_MSG_ID_TO_HANDLE(id));
        }
        break;
    }
}

void leBroadcastManager_PeriodicScanInit(void)
{
    le_bm_periodic_scan_instance_t *scan_instance = NULL;

    memset(&le_broadcast_manager_periodic_scan_data, 0, sizeof(le_broadcast_manager_periodic_scan_data));

    le_broadcast_manager_periodic_scan_data.task.handler = leBroadcastManager_PeriodicScanMessageHandler;

    ARRAY_FOREACH(scan_instance, le_broadcast_manager_periodic_scan_data.scan_instances)
    {
        leBroadcastManager_PeriodicScanResetInstance(scan_instance);
    }
}

bool leBroadcastManager_PeriodicScanStartRequest(le_bm_periodic_scan_params_t *params, le_bm_periodic_scan_handle *handle)
{
    le_bm_periodic_scan_task_data_t *ps_data = leBroadcastManager_PeriodicScanGetTaskData();
    le_bm_periodic_scan_instance_t *new_scan_instance = NULL;
    bool started = FALSE;

    DEBUG_LOG_FN_ENTRY("leBroadcastManager_PeriodicScanStartRequest timeout %u", params->timeout);

    /* Allocate new instance */
    new_scan_instance = leBroadcastManager_PeriodicScanGetInstanceByHandle(INVALID_LE_BM_PERIODIC_SCAN_HANDLE);
    if (new_scan_instance)
    {
        /* Assign a new handle. Currently the handle is the array index. */
        new_scan_instance->handle = ARRAY_INDEX(new_scan_instance, ps_data->scan_instances);

        /* Store params */
        new_scan_instance->params = *params;
        leBroadcastManager_PeriodicScanVerifyInterface(new_scan_instance->params.interface);

        /* Start scan if not already started. */
        if (!ps_data->is_scanning)
        {
            LeBapBroadcastSink_StartScanPaSourceRequest(0, NULL);
            ps_data->is_scanning = TRUE;
        }
        else
        {
            /* Scan is already running so send a delayed start cfm to the client. */
            leBroadcastManager_PeriodicScanSendDelayedStartCfm(new_scan_instance->handle, le_bm_periodic_scan_status_success);

            if (new_scan_instance->params.timeout != 0)
            {
                leBroadcastManager_PeriodicScanTimeoutStart(new_scan_instance->handle, new_scan_instance->params.timeout);
            }
        }

        *handle = new_scan_instance->handle;
        started = TRUE;
    }

    DEBUG_LOG("leBroadcastManager_PeriodicScanStartRequest new_scan_instance %p hdl 0x%x",
              new_scan_instance, *handle);

    return started;
}

void leBroadcastManager_PeriodicScanStopRequest(le_bm_periodic_scan_handle handle)
{
    le_bm_periodic_scan_instance_t *scan_instance = NULL;

    DEBUG_LOG_FN_ENTRY("leBroadcastManager_PeriodicScanStopRequest hdl 0x%x", handle);

    /* Find scan instance */
    scan_instance = leBroadcastManager_PeriodicScanGetInstanceByHandle(handle);
    if (scan_instance)
    {
        leBroadcastManager_PeriodicScanTimeoutCancel(scan_instance->handle);

        if (1 == leBroadcastManager_PeriodicScanGetInstanceCount())
        {
            /* If this is the only client request then request to end the
               periodic scan and wait for the cfm. */
            LeBapBroadcastSink_StopScanPaSourceRequest();
        }
        else
        {
            /* If there would be one active client left after this one is
               stopped then send the cfm for this client immediately.
               Don't stop the in-progress periodic scan because it is still
               needed by the other client. */
            leBroadcastManager_PeriodicScanSendDelayedStopCfm(scan_instance->handle, le_bm_periodic_scan_status_success);
        }
    }
    else
    {
        DEBUG_LOG_WARN("leBroadcastManager_PeriodicScanStopRequest couldn't find handle 0x%x", handle);
    }
}

void leBroadcastManager_PeriodicScanEaReportReceived(const CL_DM_BLE_EXT_SCAN_FILTERED_ADV_REPORT_IND_T* ind)
{
    uint32 broadcast_id = 0;
    AudioAnnouncementParserStatus statusAdvDataParsing = AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND;

    DEBUG_LOG_FN_ENTRY("leBroadcastManager_PeriodicScanEaReportReceived data_len=0x%x sid=0x%x",
                       ind->adv_data_len, ind->adv_sid);
    DEBUG_LOG_FN_ENTRY("leBroadcastManager_PeriodicScanEaReportReceived perm_addr=[%u %04x:%02x:%06x] curr_addr=[%u %04x:%02x:%06x]",
                       ind->permanent_addr.type, ind->permanent_addr.addr.nap,
                       ind->permanent_addr.addr.uap, ind->permanent_addr.addr.lap,
                       ind->current_addr.type, ind->current_addr.addr.nap,
                       ind->current_addr.addr.uap, ind->current_addr.addr.lap);

    /* Check for BASS UUID in the advert. */
    statusAdvDataParsing = AudioAnnouncementParserBcastAudioAnnouncementParsing(ind->adv_data_len,
                                                                                (uint8 *)ind->adv_data,
                                                                                &broadcast_id);

    if (statusAdvDataParsing == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS)
    {
        le_bm_periodic_scan_task_data_t *ps_data = leBroadcastManager_PeriodicScanGetTaskData();
        le_bm_periodic_scan_instance_t *scan_instance = NULL;

        DEBUG_LOG("leBroadcastManager_PeriodicScanEaReportReceived Found BASS UUID: Broadcast_ID:0x%x",
                  broadcast_id);

        /* Notify clients of this advert. */
        ARRAY_FOREACH(scan_instance, ps_data->scan_instances)
        {
            if (scan_instance->handle != INVALID_LE_BM_PERIODIC_SCAN_HANDLE)
            {
                scan_instance->params.interface->EaReportReceived(scan_instance->handle, broadcast_id, ind);
            }
        }
    }
}

void leBroadcastManager_PeriodicScanStartScanPaSourceConfirm(const le_broadcast_sink_start_scan_pa_source_cfm_t* cfm)
{
    le_bm_periodic_scan_task_data_t *ps_data = leBroadcastManager_PeriodicScanGetTaskData();
    le_bm_periodic_scan_instance_t *scan_instance = NULL;

    DEBUG_LOG_FN_ENTRY("leBroadcastManager_PeriodicScanStartScanPaSourceConfirm status enum:le_bap_broadcast_sink_status_t:%u", cfm->status);

    /* Send notification to clients */
    le_bm_periodic_scan_status_t status = leBroadcastManager_PeriodicScanConvertBapStatus(cfm->status);

    ARRAY_FOREACH(scan_instance, ps_data->scan_instances)
    {
        if (scan_instance->handle != INVALID_LE_BM_PERIODIC_SCAN_HANDLE)
        {
            scan_instance->params.interface->StartScanPaSourceConfirm(scan_instance->handle, status);

            if (scan_instance->params.timeout != 0)
            {
                leBroadcastManager_PeriodicScanTimeoutStart(scan_instance->handle, scan_instance->params.timeout);
            }
        }
    }
}

void leBroadcastManager_PeriodicScanStopScanPaSourceConfirm(const le_broadcast_sink_stop_scan_pa_source_cfm_t* cfm)
{
    le_bm_periodic_scan_task_data_t *ps_data = leBroadcastManager_PeriodicScanGetTaskData();
    le_bm_periodic_scan_instance_t *scan_instance = NULL;

    DEBUG_LOG_FN_ENTRY("leBroadcastManager_PeriodicScanStopScanPaSourceConfirm status enum:le_bap_broadcast_sink_status_t:%u", cfm->status);

    /* Send notification to clients */
    le_bm_periodic_scan_status_t status = leBroadcastManager_PeriodicScanConvertBapStatus(cfm->status);

    ARRAY_FOREACH(scan_instance, ps_data->scan_instances)
    {
        if (scan_instance->handle != INVALID_LE_BM_PERIODIC_SCAN_HANDLE)
        {
            scan_instance->params.interface->StopScanPaSourceConfirm(scan_instance->handle, status);

            leBroadcastManager_PeriodicScanResetInstance(scan_instance);
        }
    }

    ps_data->is_scanning = FALSE;
}

void leBroadcastManager_PeriodicScanScanPaSourceTimeout(void)
{
    le_bm_periodic_scan_task_data_t *ps_data = leBroadcastManager_PeriodicScanGetTaskData();
    le_bm_periodic_scan_instance_t *scan_instance = NULL;

    DEBUG_LOG_FN_ENTRY("leBroadcastManager_PeriodicScanScanPaSourceTimeout");

    ARRAY_FOREACH(scan_instance, ps_data->scan_instances)
    {
        if (scan_instance->handle != INVALID_LE_BM_PERIODIC_SCAN_HANDLE)
        {
            scan_instance->params.interface->ScanPaSourceTimeout(scan_instance->handle);
        }
    }

    ps_data->is_scanning = FALSE;
}

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN) */

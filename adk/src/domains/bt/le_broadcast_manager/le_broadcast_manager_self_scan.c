/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    leabm
    \brief      LE Audio Broadcast Self-Scan module
*/

#ifdef INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN
#include "le_broadcast_manager.h"
#include "le_broadcast_manager_self_scan.h"
#include "le_broadcast_manager_self_scan_discovered_source_list.h"
#include "le_broadcast_manager_self_scan_private.h"

#include "le_broadcast_manager_periodic_scan.h"

#include <audio_announcement_parser_lib.h>
#include <logging.h>
#include <rtime.h>
#include <stdlib.h>
#include <system_clock.h>


/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_ENUM(le_broadcast_manager_self_scan_message)

/* State data for the self-scan module. */
le_bm_self_scan_task_data_t self_scan_data = {0};


static le_broadcast_manager_self_scan_status_t leBroadcastManager_SelfScanConvertPeriodicScanStatus(le_bm_periodic_scan_status_t status)
{
    switch (status)
    {
    case le_bm_periodic_scan_status_success: return lebmss_success;
    case le_bm_periodic_scan_status_fail: return lebmss_fail;
    case le_bm_periodic_scan_status_timeout: return lebmss_timeout;
    default: return lebmss_fail;
    }
}

static void leBroadcastManager_SelfScanSendScanStartCfm(Task client, le_broadcast_manager_self_scan_status_t status)
{
    MESSAGE_MAKE(cfm, LE_BROADCAST_MANAGER_SELF_SCAN_START_CFM_T);
    cfm->status = status;
    MessageSend(client, LE_BROADCAST_MANAGER_SELF_SCAN_START_CFM, cfm);
}

static void leBroadcastManager_SelfScanSendScanStopCfm(Task client, le_broadcast_manager_self_scan_status_t status)
{
    MESSAGE_MAKE(cfm, LE_BROADCAST_MANAGER_SELF_SCAN_STOP_CFM_T);
    cfm->status = status;
    MessageSend(client, LE_BROADCAST_MANAGER_SELF_SCAN_STOP_CFM, cfm);
}

static void leBroadcastManager_SelfScanSendScanStatusInd(Task client, le_broadcast_manager_self_scan_status_t status)
{
    MESSAGE_MAKE(cfm, LE_BROADCAST_MANAGER_SELF_SCAN_STATUS_IND_T);
    cfm->status = status;
    MessageSend(client, LE_BROADCAST_MANAGER_SELF_SCAN_STATUS_IND, cfm);
}

static void leBroadcastManager_SelfScanSendDiscoveredSourceInd(Task client, const le_bm_self_scan_discovered_source_t *source)
{
    LE_BROADCAST_MANAGER_SELF_SCAN_DISCOVERED_SOURCE_IND_T src_ind = {0};
    src_ind.broadcast_id = source->ea_data.broadcast_id;
    src_ind.adv_sid = source->ea_data.adv_sid;
    src_ind.rssi = source->ea_data.rssi;
    src_ind.source_tpaddr.transport = TRANSPORT_BLE_ACL;
    src_ind.source_tpaddr.taddr.type = source->source_tpaddr.taddr.type;
    src_ind.source_tpaddr.taddr.addr.lap = source->source_tpaddr.taddr.addr.lap;
    src_ind.source_tpaddr.taddr.addr.uap = source->source_tpaddr.taddr.addr.uap;
    src_ind.source_tpaddr.taddr.addr.nap = source->source_tpaddr.taddr.addr.nap;

    if (source->ea_data.broadcast_name_len > 0)
    {
        src_ind.broadcast_name_len = source->ea_data.broadcast_name_len;
        src_ind.broadcast_name = source->ea_data.broadcast_name;
    }

    /* The original buffer for the name is sent to the client on purpose.
       The message is sent directly (synchronously) to avoid having to
       allocate a new buffer. If the client wants to store the name it must
       allocate its own buffer and copy the name into that.
    */
    client->handler(client, LE_BROADCAST_MANAGER_SELF_SCAN_DISCOVERED_SOURCE_IND, &src_ind);
}

static inline bool leBroadcastManager_SelfScanValidateTimeoutParam(uint32 timeout)
{
    bool valid = (   (timeout == 0)
                  || (   (timeout >= LE_BROADCAST_SELF_SCAN_TIMEOUT_MINIMUM_MS)
                      && (timeout <= LE_BROADCAST_SELF_SCAN_TIMEOUT_MAXIMUM_MS)));

    return valid;
}

static bool leBroadcastManager_SelfScanValidateScanParams(const self_scan_params_t *params)
{
    bool valid = FALSE;

    if (   params
        && leBroadcastManager_SelfScanValidateTimeoutParam(params->timeout))
    {
        valid = TRUE;
    }

    return valid;
}

static void leBroadcastManager_SelfScanClientReset(le_bm_self_scan_client_t *client)
{
    memset(client, 0, sizeof(*client));
    client->state = LE_BM_SELF_SCAN_STATE_IDLE;
    client->periodic_handle = INVALID_LE_BM_PERIODIC_SCAN_HANDLE;
}

static le_bm_self_scan_client_t *leBroadcastManager_SelfScanClientGetByTask(Task task)
{
    le_bm_self_scan_task_data_t *self_scan = leBroadcastManager_SelfScanGetTaskData();
    le_bm_self_scan_client_t *client = NULL;
    le_bm_self_scan_client_t *found_client = NULL;

    ARRAY_FOREACH(client, self_scan->clients)
    {
        if (client->task == task)
        {
            found_client = client;
            break;
        }
    }

    return found_client;
}

static le_bm_self_scan_client_t *leBroadcastManager_SelfScanClientGetByPeriodicHandle(le_bm_periodic_scan_handle handle)
{
    le_bm_self_scan_task_data_t *self_scan = leBroadcastManager_SelfScanGetTaskData();
    le_bm_self_scan_client_t *client = NULL;
    le_bm_self_scan_client_t *found_client = NULL;

    ARRAY_FOREACH(client, self_scan->clients)
    {
        if (client->periodic_handle == handle)
        {
            found_client = client;
            break;
        }
    }

    return found_client;
}

le_bm_self_scan_client_t *LeBroadcastManager_SelfScanClientCreate(Task task, const self_scan_params_t *params)
{
    le_bm_self_scan_client_t *new_client = NULL;

    new_client = leBroadcastManager_SelfScanClientGetByTask((Task)NULL);
    if (new_client)
    {
        new_client->task = task;
        new_client->params.timeout = params->timeout;
        new_client->params.sync_to_pa = params->sync_to_pa;
        new_client->params.filter = params->filter;
    }

    return new_client;
}

void LeBroadcastManager_SelfScanClientDestroy(le_bm_self_scan_client_t *client_to_destroy)
{
    le_bm_self_scan_task_data_t *self_scan = leBroadcastManager_SelfScanGetTaskData();
    le_bm_self_scan_client_t *client = NULL;

    ARRAY_FOREACH(client, self_scan->clients)
    {
        if (client == client_to_destroy)
        {
            leBroadcastManager_SelfScanClientReset(client);
            break;
        }
    }

    if (!LeBroadcastManager_SelfScanIsAnyClientActive())
    {
        if (leBroadcastManager_SelfScanDiscoveredSourceCurrentCount())
        {
            leBroadcastManager_SelfScanDiscoveredSourceResetAll();
        }
    }
}
/*
static unsigned leBroadcastManager_SelfScanClientCount(void)
{
    le_bm_self_scan_task_data_t *self_scan = leBroadcastManager_SelfScanGetTaskData();
    le_bm_self_scan_client_t *client = NULL;
    unsigned count = 0;

    ARRAY_FOREACH(client, self_scan->clients)
    {
        if (client->task != (Task)NULL)
        {
            count++;
            break;
        }
    }

    return count;
}
*/

static unsigned leBroadcastManager_SelfScanTerminatingClientCount(void)
{
    le_bm_self_scan_task_data_t *self_scan = leBroadcastManager_SelfScanGetTaskData();
    le_bm_self_scan_client_t *client = NULL;
    unsigned count = 0;

    ARRAY_FOREACH(client, self_scan->clients)
    {
        if (client->state == LE_BM_SELF_SCAN_STATE_TERMINATING)
        {
            ++count;
        }
    }

    DEBUG_LOG_VERBOSE("leBroadcastManager_SelfScanTerminatingClientCount: %u", count);
    return count;
}

static void leBroadcastManager_SelfScanClientSetState(le_bm_self_scan_client_t *client, le_bm_self_scan_state_t state)
{
    DEBUG_LOG_FN_ENTRY("leBroadcastManager_SelfScanClientSetState client %p state enum:le_bm_self_scan_state_t:%u -> enum:le_bm_self_scan_state_t:%u",
                       client, client->state, state);

    client->state = state;
}

static void leBroadcastManager_SelfScanEaReportReceived(le_bm_periodic_scan_handle handle,
                                                        uint32 broadcast_id,
                                                        const CL_DM_BLE_EXT_SCAN_FILTERED_ADV_REPORT_IND_T* ind)
{
    le_bm_self_scan_client_t *client = leBroadcastManager_SelfScanClientGetByPeriodicHandle(handle);

    DEBUG_LOG_FN_ENTRY("leBroadcastManager_SelfScanEaReportReceived hdl 0x%x client %p", handle, client);

    if (client)
    {
        if (client->state == LE_BM_SELF_SCAN_STATE_SCANNING)
        {
            /* Add or update this source in the discovered sources list

               * Search for existing entry in discovered sources list by broadcast_id
                 * Broadcast Id on its own is not enough for a duplicate (!)
                   * Some customers are considering using the same broadcast_id with multiple broadcasters.
                   * RSSI is variable even from one source so that can't be used.
                 * Address?
                   * Should we use the source_tpaddr or the permanent_tpaddr
                     * What is the difference?
                   * Devices could be advertising multiple Adv sets (adv_sid) from the same address.
                     * Address on its own is not enough.
                 * Address + adv_sid?
            */
            le_bm_self_scan_discovered_source_t *discovered_source = NULL;
            discovered_source = leBroadcastManager_SelfScanDiscoveredSourceGetByAddress(&ind->current_addr);
            if (discovered_source)
            {
                /* TBD: update values, e.g. RSSI? */
            }
            else
            {
                discovered_source = leBroadcastManager_SelfScanDiscoveredSourceCreate(&ind->current_addr);
                le_bm_self_scan_ea_data_t ea_data = {0};

                ea_data.broadcast_id = broadcast_id;
                ea_data.adv_sid = ind->adv_sid;
                ea_data.rssi = ind->rssi;
                ea_data.broadcast_name = AudioAnnouncementParserBcastNameParsing(ind->adv_data_len,
                                                                                 ind->adv_data,
                                                                                 &ea_data.broadcast_name_len);

                leBroadcastManager_SelfScanDiscoveredSourceSetEaData(discovered_source, &ea_data);

                /* TBD: for now, always send the discovered source Ind.
                   Eventually should only send once all data groups are set.  */
                leBroadcastManager_SelfScanSendDiscoveredSourceInd(client->task, discovered_source);
            }
        }
    }
}

/*! \brief Calculate absolute local time (in us) from now for remaining time on a timer. */
static rtime_t leBroadcastManager_SelfScanAbsoluteTime(uint32 due_ms)
{
    rtime_t local_clock = SystemClockGetTimerTime();

    /* if timer is already past due, just use current local time,
     * by the time this is marshalled to the peer it will be in
     * the past and will fire immediately */
    if (due_ms == 0)
    {
        return local_clock;
    }
    else
    {
        return rtime_add(local_clock, MS_TO_US(due_ms));
    }
}

static void leBroadcastManager_SelfScanStartScanPaSourceConfirm(le_bm_periodic_scan_handle handle, le_bm_periodic_scan_status_t status)
{
    le_bm_self_scan_client_t *client = leBroadcastManager_SelfScanClientGetByPeriodicHandle(handle);

    DEBUG_LOG_FN_ENTRY("leBroadcastManager_SelfScanStartScanPaSourceConfirm hdl 0x%x status enum:le_bm_periodic_scan_status_t:%u client %p",
                       handle, status, client);

    if (client)
    {
        switch (client->state)
        {
        case LE_BM_SELF_SCAN_STATE_STARTING:
        case LE_BM_SELF_SCAN_STATE_SCANNING:
            {
                le_broadcast_manager_self_scan_status_t cfm_status = leBroadcastManager_SelfScanConvertPeriodicScanStatus(status);
                le_bm_self_scan_state_t next_state = (cfm_status == lebmss_success) ? LE_BM_SELF_SCAN_STATE_SCANNING
                                                                                    : LE_BM_SELF_SCAN_STATE_IDLE;

                if (next_state != client->state)
                {
                    leBroadcastManager_SelfScanSendScanStartCfm(client->task, cfm_status);
                }

                switch (next_state)
                {
                case LE_BM_SELF_SCAN_STATE_SCANNING:
                    {
                        if (next_state != client->state)
                        {
                            leBroadcastManager_SelfScanSendScanStatusInd(client->task, lebmss_in_progress);

                            /* The scan started ok so set the wallclock end time based on the timeout */
                            client->timeout_end = leBroadcastManager_SelfScanAbsoluteTime(client->params.timeout);
                        }
                    }
                    break;

                case LE_BM_SELF_SCAN_STATE_IDLE:
                    {
                        LeBroadcastManager_SelfScanClientDestroy(client);
                    }
                    break;

                default:
                    break;
                }

                leBroadcastManager_SelfScanClientSetState(client, next_state);
            }
            break;

        default:
            DEBUG_LOG_WARN("leBroadcastManager_SelfScanStartScanPaSourceConfirm unexpected state enum:le_bm_self_scan_state_t:%u", client->state);
            break;
        }
    }
}

static void leBroadcastManager_SelfScanStopScanPaSourceConfirm(le_bm_periodic_scan_handle handle, le_bm_periodic_scan_status_t status)
{
    le_bm_self_scan_client_t *client = leBroadcastManager_SelfScanClientGetByPeriodicHandle(handle);

    DEBUG_LOG_FN_ENTRY("leBroadcastManager_SelfScanStopScanPaSourceConfirm hdl 0x%x status enum:le_bm_periodic_scan_status_t:%u client %p",
                       handle, status, client);

    if (client)
    {
        switch (client->state)
        {
        case LE_BM_SELF_SCAN_STATE_TERMINATING:
            {
                le_broadcast_manager_self_scan_status_t cfm_status = leBroadcastManager_SelfScanConvertPeriodicScanStatus(status);

                if (self_scan_data.stop_task)
                {
                    if (leBroadcastManager_SelfScanTerminatingClientCount() == 1)
                    {
                        MessageSend(self_scan_data.stop_task, LE_BROADCAST_MANAGER_SELF_SCAN_STOP_ALL_CFM, NULL);
                        self_scan_data.stop_task = NULL;
                    }
                }
                else
                {
                    leBroadcastManager_SelfScanSendScanStopCfm(client->task, cfm_status);
                }

                leBroadcastManager_SelfScanSendScanStatusInd(client->task, lebmss_stopped);
                leBroadcastManager_SelfScanClientSetState(client, LE_BM_SELF_SCAN_STATE_IDLE);
                LeBroadcastManager_SelfScanClientDestroy(client);
            }
            break;

        default:
            DEBUG_LOG_WARN("leBroadcastManager_SelfScanStopScanPaSourceConfirm unexpected state enum:le_bm_self_scan_state_t:%u", client->state);
            break;
        }
    }
}

static void leBroadcastManager_SelfScanScanPaSourceTimeout(le_bm_periodic_scan_handle handle)
{
    le_bm_self_scan_client_t *client = leBroadcastManager_SelfScanClientGetByPeriodicHandle(handle);

    DEBUG_LOG_FN_ENTRY("leBroadcastManager_SelfScanScanPaSourceTimeout hdl 0x%x client %p", handle, client);

    if (client)
    {
        switch (client->state)
        {
        case LE_BM_SELF_SCAN_STATE_SCANNING:
            {
                leBroadcastManager_SelfScanClientSetState(client, LE_BM_SELF_SCAN_STATE_IDLE);
                leBroadcastManager_SelfScanSendScanStatusInd(client->task, lebmss_timeout);
                LeBroadcastManager_SelfScanClientDestroy(client);
            }
            break;

        default:
            DEBUG_LOG_WARN("leBroadcastManager_SelfScanScanPaSourceTimeout unexpected state enum:le_bm_self_scan_state_t:%u", client->state);
            break;
        }
    }
}

le_bm_periodic_scan_callback_interface_t self_scan_periodic_scan_interface =
{
    .EaReportReceived = leBroadcastManager_SelfScanEaReportReceived,
    .StartScanPaSourceConfirm = leBroadcastManager_SelfScanStartScanPaSourceConfirm,
    .StopScanPaSourceConfirm = leBroadcastManager_SelfScanStopScanPaSourceConfirm,
    .ScanPaSourceTimeout = leBroadcastManager_SelfScanScanPaSourceTimeout
};

static void leBroadcastManager_SelfScanHandleMessage(Task task, MessageId id, Message msg)
{
    UNUSED(task);
    UNUSED(id);
    UNUSED(msg);
}

bool LeBroadcastManager_SelfScanInit(Task init_task)
{
    le_bm_self_scan_task_data_t *self_scan = leBroadcastManager_SelfScanGetTaskData();
    le_bm_self_scan_client_t *client = NULL;

    UNUSED(init_task);

    DEBUG_LOG_FN_ENTRY("LeBroadcastManager_SelfScanInit");

    memset(self_scan, 0, sizeof(*self_scan));
    self_scan->task.handler = leBroadcastManager_SelfScanHandleMessage;

    ARRAY_FOREACH(client, self_scan->clients)
    {
        leBroadcastManager_SelfScanClientReset(client);
    }

    leBroadcastManager_SelfScanDiscoveredSourceInit();

    return TRUE;
}

void LeBroadcastManager_SelfScanStart(Task task, const self_scan_params_t *params)
{
    le_broadcast_manager_self_scan_status_t status = lebmss_fail;
    le_bm_self_scan_client_t *new_client = NULL;

    DEBUG_LOG_FN_ENTRY("LeBroadcastManager_SelfScanStart task %p", task);

    if (leBroadcastManager_SelfScanValidateScanParams(params))
    {
        new_client = LeBroadcastManager_SelfScanClientCreate(task, params);
        if (new_client)
        {
            le_bm_periodic_scan_params_t scan_params = {
                .timeout = new_client->params.timeout,
                .interface = leBroadcastManager_SelfScanGetPeriodicScanInterface()
            };
            le_bm_periodic_scan_handle handle = 0;

            /* Use the default timeout if the client has set the timeout to
               the special value of 0. */
            if (scan_params.timeout == 0)
            {
                scan_params.timeout = LE_BROADCAST_SELF_SCAN_TIMEOUT_DEFAULT_MS;
            }

            if (leBroadcastManager_PeriodicScanStartRequest(&scan_params, &handle))
            {
                DEBUG_LOG("LeBroadcastManager_SelfScanStart STARTED handle 0x%x", handle);

                new_client->periodic_handle = handle;
                leBroadcastManager_SelfScanClientSetState(new_client, LE_BM_SELF_SCAN_STATE_STARTING);

                status = lebmss_success;
            }
        }
        else
        {
            status = lebmss_fail;
        }
    }
    else
    {
        status = lebmss_bad_parameters;
    }

    if (status != lebmss_success)
    {
        leBroadcastManager_SelfScanSendScanStartCfm(task, status);
    }
}

static bool leBroadcastManager_SelfScanStopClient(le_bm_self_scan_client_t *client)
{
    bool stopped = TRUE;

    switch (client->state)
    {
    case LE_BM_SELF_SCAN_STATE_IDLE:
        /* The client is already stopped */
        break;

    case LE_BM_SELF_SCAN_STATE_STARTING:
    case LE_BM_SELF_SCAN_STATE_SCANNING:
        {
            leBroadcastManager_PeriodicScanStopRequest(client->periodic_handle);
            leBroadcastManager_SelfScanClientSetState(client, LE_BM_SELF_SCAN_STATE_TERMINATING);
            stopped = FALSE;
        }
        break;

    case LE_BM_SELF_SCAN_STATE_TERMINATING:
        stopped = FALSE;
        break;

    default:
        break;
    }

    DEBUG_LOG_VERBOSE("leBroadcastManager_SelfScanStopClient: client=%p state=enum:le_bm_self_scan_state_t:%d stopped=%u", client, client->state, stopped);
    return stopped;
}

void LeBroadcastManager_SelfScanStop(Task task)
{
    bool send_cfm = TRUE;

    DEBUG_LOG_FN_ENTRY("LeBroadcastManager_SelfScanStop task %p", task);

    le_bm_self_scan_client_t *client = leBroadcastManager_SelfScanClientGetByTask(task);
    if (client)
    {
        send_cfm = leBroadcastManager_SelfScanStopClient(client);
    }
    else
    {
        /* If client doesn't exist it may have already stopped, so send
           'success' in the stop cfm. */
    }

    if (send_cfm)
    {
        leBroadcastManager_SelfScanSendScanStopCfm(task, lebmss_success);
    }
}

void LeBroadcastManager_SelfScanStopAll(Task task)
{
    le_bm_self_scan_task_data_t *self_scan = leBroadcastManager_SelfScanGetTaskData();
    bool all_stopped = TRUE;

    DEBUG_LOG_FN_ENTRY("LeBroadcastManager_SelfScanStopAll");

    if (self_scan->stop_task == NULL)
    {
        le_bm_self_scan_client_t *client = NULL;

        ARRAY_FOREACH(client, self_scan->clients)
        {
            if (client->task && client->state != LE_BM_SELF_SCAN_STATE_IDLE)
            {
                if (!leBroadcastManager_SelfScanStopClient(client))
                {
                    all_stopped = FALSE;
                }
            }
        }

        if (!all_stopped)
        {
            self_scan->stop_task = task;
        }
    }

    if (all_stopped)
    {
        MessageSend(task, LE_BROADCAST_MANAGER_SELF_SCAN_STOP_ALL_CFM, NULL);
    }

}

bool LeBroadcastManager_SelfScanIsAnyClientActive(void)
{
    le_bm_self_scan_task_data_t *self_scan = leBroadcastManager_SelfScanGetTaskData();
    le_bm_self_scan_client_t *client = NULL;
    bool active = FALSE;

    ARRAY_FOREACH(client, self_scan->clients)
    {
        if (   (client->task != (Task)NULL)
            && (client->state != LE_BM_SELF_SCAN_STATE_IDLE))
        {
            active = TRUE;
            break;
        }
    }

    return active;
}

#endif /* INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN */

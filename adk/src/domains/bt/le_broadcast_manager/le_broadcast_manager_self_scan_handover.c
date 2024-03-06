/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      LE Audio Broadcast Self-Scan handover interface implementation.
*/

/*
    The self-scan state does not depend on any particular device but since the
    handover interface does not support connectionless handover it has to be
    written based on the connected devices.

    The self-scan state only needs to be marshalled once so an important part
    of this file is to make sure the data is marshalled once and only once,
    regardless of how many BR/EDR or LE ACLs are connected.

    The marshalled_state flag is used to ensure that the data is marshalled
    once and the commit operations are applied only once.
*/

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)

#include "domain_marshal_types.h"
#include "app_handover_if.h"
#include "le_broadcast_manager_self_scan.h"
#include "le_broadcast_manager_self_scan_private.h"

#include <logging.h>
#include <rtime.h>
#include <stdlib.h>


const marshal_type_info_t le_bm_self_scan_marshal_types[] = {
    MARSHAL_TYPE_INFO(le_bm_self_scan_task_data_t, MARSHAL_TYPE_CATEGORY_GENERIC)
};

const marshal_type_list_t le_bm_self_scan_marshal_types_list = {le_bm_self_scan_marshal_types, ARRAY_DIM(le_bm_self_scan_marshal_types)};


/* Flag used to track if state data has been marshalled. */
bool marshalled_state = FALSE;

#define leBroadcastManager_SelfScanGetMarshalledState()  (marshalled_state)

#define leBroadcastManager_SelfScanSetMarshalledState(marshalled)    (marshalled_state = (marshalled))



static void leBroadcastManager_SelfScanDebugLogState(le_bm_self_scan_task_data_t *self_scan)
{
    le_bm_self_scan_client_t *client = NULL;

    DEBUG_LOG_VERBOSE("  TaskData %p", self_scan->task);
    DEBUG_LOG_VERBOSE("  Clients:");

    ARRAY_FOREACH(client, self_scan->clients)
    {
        DEBUG_LOG_VERBOSE("    Task %p", client->task);
        DEBUG_LOG_VERBOSE("    state enum:le_bm_self_scan_state_t:%u", client->state);
        DEBUG_LOG_VERBOSE("    periodic_handle 0x%x", client->periodic_handle);
        DEBUG_LOG_VERBOSE("    timeout_end %u", client->timeout_end);
        DEBUG_LOG_VERBOSE("    timeout delay %u", RtimeTimeToMsDelay(client->timeout_end));
        DEBUG_LOG_VERBOSE("    params:");
        DEBUG_LOG_VERBOSE("      timeout %u", client->params.timeout);
        DEBUG_LOG_VERBOSE("      sync_to_pa %u", client->params.sync_to_pa);
    }
}


/*! \brief Do veto check during handovers.

    Reasons for self-scan to veto:
    * If there is and ongoing periodic train sync (aka PA sync).
        * Self-scan PA sync has not been implemented yet so no need to veto based on this.

    \return FALSE always.
*/
static bool leBroadcastManager_SelfScanVeto(void)
{
    return FALSE;
}

/*!
    \brief The function shall set marshal_obj to the address of the object to
           be marshalled.

    \param[in] type         Type of the data to be marshalled \ref marshal_type_t
    \param[out] marshal_obj Holds address of data to be marshalled.
    \return TRUE: Required data has been copied to the marshal_obj.
            FALSE: No data is required to be marshalled. marshal_obj is set to NULL.

*/
static bool leBroadcastManager_SelfScanMarshal(marshal_type_t type,
                                               void **marshal_obj)
{
    bool status = FALSE;
    *marshal_obj = NULL;

    if (leBroadcastManager_SelfScanGetMarshalledState())
    {
        return status;
    }

    switch (type)
    {
        case MARSHAL_TYPE(le_bm_self_scan_task_data_t):
        {
            *marshal_obj = leBroadcastManager_SelfScanGetTaskData();
            status = TRUE;
            leBroadcastManager_SelfScanSetMarshalledState(TRUE);

            leBroadcastManager_SelfScanDebugLogState(leBroadcastManager_SelfScanGetTaskData());
            break;
        }
        default:
            Panic();
            break;
    }

    DEBUG_LOG_FN_ENTRY("leBroadcastManager_SelfScanMarshal status %u marshal_obj %p type 0x%x",
                       status, *marshal_obj, type);

    return status;
}

/*!
    \brief The function shall copy the unmarshal_obj associated to specific
            marshal type \ref marshal_type_t

    \param[in] type         Type of the unmarshalled data \ref marshal_type_t
    \param[in] unmarshal_obj Address of the unmarshalled object.
    \return unmarshalling result. Based on this, caller decides whether to free
            the marshalling object or not.

*/
static app_unmarshal_status_t leBroadcastManager_SelfScanUnmarshal(
        marshal_type_t type, void *unmarshal_obj)
{
    app_unmarshal_status_t result = UNMARSHAL_FAILURE;

    switch (type)
    {
        case MARSHAL_TYPE(le_bm_self_scan_task_data_t):
        {
            le_bm_self_scan_task_data_t *unmarshalled_self_scan = (le_bm_self_scan_task_data_t *)unmarshal_obj;

            DEBUG_LOG_VERBOSE("Unmarshalled self-scan data:");
            leBroadcastManager_SelfScanDebugLogState(unmarshalled_self_scan);

            le_bm_self_scan_client_t *unmarshalled_client = NULL;

            ARRAY_FOREACH(unmarshalled_client, unmarshalled_self_scan->clients)
            {
                self_scan_params_t scan_params = {
                    .timeout = unmarshalled_client->params.timeout,
                    .sync_to_pa = unmarshalled_client->params.sync_to_pa,
                    .filter = {
                        .broadcast_id = unmarshalled_client->params.filter.broadcast_id,
                        .rssi_threshold = unmarshalled_client->params.filter.rssi_threshold
                    }
                };
                le_bm_self_scan_client_t *client = LeBroadcastManager_SelfScanClientCreate(unmarshalled_client->task,
                                                                                           &scan_params);

                if (client)
                {
                    client->state = unmarshalled_client->state;
                    client->timeout_end = unmarshalled_client->timeout_end;
                }
                else
                {
                    DEBUG_LOG_ERROR("leBroadcastManager_SelfScanUnmarshal couldn't create a self-scan client");
                    Panic();
                }
            }

            leBroadcastManager_SelfScanSetMarshalledState(TRUE);

            result = UNMARSHAL_SUCCESS_FREE_OBJECT;
            break;
        }

        default:
            /* Do nothing */
            break;
    }

    DEBUG_LOG_FN_ENTRY("leBroadcastManager_SelfScanUnmarshal result enum:app_unmarshal_status_t:%u type 0x%x",
                       result, type);

    return result;
}

/*! \brief Commit to the given role when a handover is comitted.
*/
static void leBroadcastManager_SelfScanCommit(bool is_primary)
{
    if (!leBroadcastManager_SelfScanGetMarshalledState())
    {
        /* If there are both BR/EDR and LE connections this commit function
           will get called twice. But we only want to run it once so use
           the marshalled state flag as a check. */
        return;
    }

    DEBUG_LOG_FN_ENTRY("leBroadcastManager_SelfScanCommit primary %u", is_primary);

    if (is_primary)
    {
        /* Primary: Re-start any scans that were in progress
           * Set the timeout to the time remaining.
        */
        le_bm_self_scan_task_data_t *self_scan = leBroadcastManager_SelfScanGetTaskData();
        le_bm_self_scan_client_t *client = NULL;

        ARRAY_FOREACH(client, self_scan->clients)
        {
            if (client->state == LE_BM_SELF_SCAN_STATE_SCANNING)
            {
                le_bm_periodic_scan_params_t scan_params = {
                    .timeout = RtimeTimeToMsDelay(client->timeout_end),
                    .interface = leBroadcastManager_SelfScanGetPeriodicScanInterface()
                };
                le_bm_periodic_scan_handle handle = 0;

                if (leBroadcastManager_PeriodicScanStartRequest(&scan_params, &handle))
                {
                    client->periodic_handle = handle;
                }
                else
                {
                    DEBUG_LOG_WARN("Failed to re-start scanning");
                }
            }
        }
    }
    else
    {
        /* Secondary: clean up the state of any clients by:
           * Requesting to stop any in-progress scan.
             * This will clean up the state of any in-progress scan in the
               lower modules.
           * Destroying the client instance.
             * Destroy it here so that when the cfm for any stop request
               is handled it will not generate an unexpected stop cfm for the
               client.
        */
        le_bm_self_scan_task_data_t *self_scan = leBroadcastManager_SelfScanGetTaskData();
        le_bm_self_scan_client_t *client = NULL;

        ARRAY_FOREACH(client, self_scan->clients)
        {
            if (client->task != (Task)NULL)
            {
                LeBroadcastManager_SelfScanStop(client->task);
                LeBroadcastManager_SelfScanClientDestroy(client);
            }
        }
    }

    leBroadcastManager_SelfScanSetMarshalledState(FALSE);
}


static bool leBroadcastManager_SelfScanMarshalBrEdr(const bdaddr *bd_addr,
                                                    marshal_type_t type,
                                                    void **marshal_obj)
{
    UNUSED(bd_addr);
    return leBroadcastManager_SelfScanMarshal(type, marshal_obj);
}

static app_unmarshal_status_t leBroadcastManager_SelfScanUnmarshalBrEdr(
        const bdaddr *bd_addr, marshal_type_t type, void *unmarshal_obj)
{
    UNUSED(bd_addr);
    return leBroadcastManager_SelfScanUnmarshal(type, unmarshal_obj);
}

REGISTER_HANDOVER_INTERFACE(LE_BROADCAST_MANAGER_SELF_SCAN,
                            &le_bm_self_scan_marshal_types_list,
                            leBroadcastManager_SelfScanVeto,
                            leBroadcastManager_SelfScanMarshalBrEdr,
                            leBroadcastManager_SelfScanUnmarshalBrEdr,
                            leBroadcastManager_SelfScanCommit);

#ifdef ENABLE_LE_HANDOVER
static bool leBroadcastManager_SelfScanMarshalLe(const typed_bdaddr *tbdaddr,
                               marshal_type_t type,
                               void **marshal_obj)
{
    UNUSED(tbdaddr);
    return leBroadcastManager_SelfScanMarshal(type, marshal_obj);
}

static app_unmarshal_status_t leBroadcastManager_SelfScanUnmarshalLe(const typed_bdaddr *tbdaddr,
                                                   marshal_type_t type,
                                                   void *unmarshal_obj)
{
    UNUSED(tbdaddr);
    return leBroadcastManager_SelfScanUnmarshal(type, unmarshal_obj);
}



REGISTER_HANDOVER_INTERFACE_LE(LE_BROADCAST_MANAGER_SELF_SCAN,
                               &le_bm_self_scan_marshal_types_list,
                               leBroadcastManager_SelfScanVeto,
                               leBroadcastManager_SelfScanMarshalLe,
                               leBroadcastManager_SelfScanUnmarshalLe,
                               leBroadcastManager_SelfScanCommit);

#endif /* ENABLE_LE_HANDOVER */

#else
#if 0
/* Use xmacro to expand type table as enumeration of marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum
{
    LAST_COMMON_MARSHAL_TYPE = NUMBER_OF_COMMON_MARSHAL_OBJECT_TYPES -1, /* Subtracting 1 to keep the marshal types contiguous */
    LE_BROADCAST_MANAGER_SELF_SCAN_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
};
#undef EXPAND_AS_ENUMERATION
#endif
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN) */

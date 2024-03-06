/*!
    \copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version        
    \file
    \ingroup    bredr_scan_manager
    \addtogroup bredr_scan_manager
    \brief	    Implementation of BREDR scan instance (page or inquiry)
    @{ 
        
    @startuml
    TITLE Internal state diagram (per instance)
    SCAN_ENABLING : Has clients and not disabled, priority >= SLOW\nOnEntry - Enable Scan
    SCAN_DISABLING : No clients or disabled\nOnEntry - Disable Scan
    SCAN_DISABLED : No clients or disabled\nOn BredrScanManager_ScanDisable() - DISABLE_CFM & PAUSED_IND\nOn BredrScanManager_ScanEnable() - RESUMED_IND
    SCAN_ENABLED : Has clients and not disabled, priority >= SLOW
    [*] --> SCAN_DISABLED : Init
    SCAN_DISABLED -up-> SCAN_ENABLING : ScanRequest()\nBredrScanManager_ScanEnable()
    SCAN_ENABLING -up-> SCAN_ENABLED : CSR_BT_CM_WRITE_SCAN_ENABLE_CFM\nCL_DM_WRITE_SCAN_ENABLE_CFM
    SCAN_ENABLED -down-> SCAN_DISABLING : ScanRelease()\nScanRequest()\nBredrScanManager_ScanDisable()
    SCAN_DISABLING -down-> SCAN_DISABLED : CSR_BT_CM_WRITE_SCAN_ENABLE_CFM\nCL_DM_WRITE_SCAN_ENABLE_CFM
    @enduml
*/

#include "bredr_scan_manager_private.h"

#include "connection.h"
#include "logging.h"
#include "message.h"
#include "bandwidth_manager.h"
#include "qualcomm_connection_manager.h"

typedef struct
{
    task_list_t * list_to_remove_from;
    bredr_scan_manager_scan_type_t type_to_match;

} remove_pending_release_clients_t;

/*! \brief Helper function to write the task's scan type to the task list data. */
static inline void bredrScanManager_ListDataSet(task_list_data_t *data,
                                                bredr_scan_manager_scan_type_t type)
{
    PanicFalse(type <= SCAN_MAN_PARAMS_TYPE_MAX);
    data->s8 = (int8)type;
}

static void bredrScanManager_MarkClientPendingRelease(task_list_t* clients, Task client)
{
    /* Mark the client as pending release. */
    task_list_data_t data;
    PanicFalse(TaskList_GetDataForTask(clients, client, &data));
    bredrScanManager_ListDataSet(&data, SCAN_MAN_PARAMS_TYPE_PENDING_RELEASE);
    PanicFalse(TaskList_SetDataForTask(clients, client, &data));
}

static bool bredrScanManager_DoScanParamsMatch(const bredr_scan_manager_scan_parameters_t * params_ref, const bredr_scan_manager_scan_parameters_t * params_test)
{
    bool result = TRUE;

    if( 0 != memcmp(params_ref, params_test, sizeof(bredr_scan_manager_scan_parameters_t)))
    {
        result = FALSE;
    }

    return result;
}

void bredrScanManager_InstanceUpdateScanActivity(bsm_scan_context_t *context, bredr_scan_manager_scan_type_t type)
{
    if (type > SCAN_MAN_PARAMS_TYPE_PENDING_RELEASE)
    {
        const bredr_scan_manager_scan_parameters_t *params;
        PanicNull((void*)context->params);
        params = (const bredr_scan_manager_scan_parameters_t *)&context->params->sets[context->params_index].set_type[type];
        if (!bredrScanManager_DoScanParamsMatch(params, context->curr_scan_params))
        {
            *context->curr_scan_params = *params;
            bredrScanManager_ConnectionWriteScanActivity(context);
        }
        else
        {
            DEBUG_LOG("bredrScanManager_InstanceUpdateScanActivity: params already set");
        }

        if(type == SCAN_MAN_PARAMS_TYPE_THROTTLE)
        {
            if(!context->throttled)
            {
                context->throttled = TRUE;
                bredrScanManager_InstanceSendScanThrottleInd(context, TRUE);
            }
        }
        else
        {
            if(context->throttled)
            {
                context->throttled = FALSE;
                bredrScanManager_InstanceSendScanThrottleInd(context, FALSE);
            }
        }
    }
}

bool bredrScanManager_IsScanTypeRegistered(bsm_scan_context_t *context,  bredr_scan_manager_scan_type_t type)
{
    bool result = FALSE;

    if (type <= SCAN_MAN_PARAMS_TYPE_MAX)
    {
        if(context && context->params)
        {
            const bredr_scan_manager_scan_parameters_set_t *params_set = &context->params->sets[context->params_index];

            if(params_set)
            {
                const bredr_scan_manager_scan_parameters_t *params = &params_set->set_type[type];

                if (params && params->interval && params->window)
                {
                    return TRUE;
                }
            }
        }
    }
    return result;
}

void bredrScanManager_InstanceInit(bsm_scan_context_t *context)
{
    SET_BSM_STATE(context->state, BSM_SCAN_DISABLED);
    TaskList_WithDataInitialise(&context->clients);
}

void bredrScanManager_InstanceParameterSetRegister(bsm_scan_context_t *context,
                                                   const bredr_scan_manager_parameters_t *params)
{
    PanicNull((void*)params);
    PanicNull((void*)params->sets);
    context->params = params;
}

void bredrScanManager_InstanceParameterSetSelect(bsm_scan_context_t *context, uint8 index)
{
    PanicFalse(index < context->params->len);
    context->params_index = index;
    if (!TEST_BSM_STATE(context->state, BSM_SCAN_DISABLED))
    {
        bredrScanManager_InstanceUpdateScanActivity(context, context->type);
    }
}

static bool bredrScanManager_IsInstancePaused(bsm_scan_context_t *context)
{
    return context->paused;
}

static bool bredrScanManager_DoesInstanceHaveClients(bsm_scan_context_t *context)
{
    return (TaskList_Size(TaskList_GetBaseTaskList(&context->clients)) != 0);
}

void bredrScanManager_InstanceClientAddOrUpdate(bsm_scan_context_t *context, Task client,
                                                bredr_scan_manager_scan_type_t type)
{
    task_list_data_t data;
    task_list_t *list = TaskList_GetBaseTaskList(&context->clients);

    bredrScanManager_ListDataSet(&data, type);

    if (TaskList_IsTaskOnList(list, client))
    {
        PanicFalse(TaskList_SetDataForTask(list, client, &data));
    }
    else
    {
        PanicFalse(TaskList_AddTaskWithData(list, client, &data));

        if (bredrScanManager_IsDisabled()
                && TEST_BSM_STATE(context->state, BSM_SCAN_DISABLED))
        {
            /* If the instance is not already paused, requesting client
             * is the only client for the instance,so update paused state */
            if(!bredrScanManager_IsInstancePaused(context))
            {
                context->paused = TRUE;
            }

            MessageSend(client, bredrScanManager_PausedMsgId(context), NULL);
        }

    }

    bredrScanManager_SetGoals();
}

static void bredrScanManager_InstanceSetFlagToCleanUp(bsm_scan_context_t *context, bool value)
{
    context->needs_cleanup = value;
}

void bredrScanManager_InstanceClientRemove(bsm_scan_context_t *context, Task client)
{
    PanicNull(client);

    task_list_t *clients = TaskList_GetBaseTaskList(&context->clients);
    if (TaskList_IsTaskOnList(clients, client))
    {
        bredrScanManager_MarkClientPendingRelease(clients, client);

        bredrScanManager_InstanceSetFlagToCleanUp(context, TRUE);

        bredrScanManager_SetGoals();
    }
    else
    {
        /* Attempt to remove a client that is not on the list
         * The panic here is not neccessary for the BREDR Scan Manager, but the client behaviour ought to be corrected
         */
        DEBUG_LOG_ERROR("bredrScanManager_InstanceClientRemove removing %p", client);
    }
}

bool bredrScanManager_InstanceIsScanEnabledForClient(bsm_scan_context_t *context, Task client)
{
    task_list_data_t *data;
    bool enabled = FALSE;
    task_list_t *clients = TaskList_GetBaseTaskList(&context->clients);

    if (TaskList_GetDataForTaskRaw(clients, client, &data))
    {
        bredr_scan_manager_scan_type_t type = bredrScanManager_ListDataGet(data);
        enabled = (type != SCAN_MAN_PARAMS_TYPE_PENDING_RELEASE && type < SCAN_MAN_PARAMS_TYPES_TOTAL);
    }

    return enabled;
}

static bool bredrScanManager_RemoveClientsWithDataMatching(Task task, task_list_data_t *data, void *arg)
{
    remove_pending_release_clients_t *settings = (remove_pending_release_clients_t *)arg;
    bredr_scan_manager_scan_type_t this_type = bredrScanManager_ListDataGet(data);

    if (this_type == settings->type_to_match)
    {
        DEBUG_LOG_VERBOSE("bredrScanManager_RemovePendingReleaseClients removing %p", task);
        TaskList_RemoveTask(settings->list_to_remove_from, task);
    }

    return TRUE;
}

static void bredrScanManager_RemovePendingReleaseClients(task_list_t *list)
{
    remove_pending_release_clients_t settings = { 0 };
    settings.list_to_remove_from = list;
    settings.type_to_match = SCAN_MAN_PARAMS_TYPE_PENDING_RELEASE;

    TaskList_IterateWithDataRawFunction(list, bredrScanManager_RemoveClientsWithDataMatching, (void *) &settings);
}

bool bredrScanManager_InstanceCompleteTransition(bsm_scan_context_t *context)
{
    DEBUG_LOG("bredrScanManager_InstanceCompleteTransition, context is %s", (context == bredrScanManager_PageScanContext())? "PS" : (context == bredrScanManager_InquiryScanContext())? "IS" : "TPS");
    bool state_changed = FALSE;
    if (TEST_BSM_STATE(context->state, BSM_SCAN_DISABLING))
    {
        DEBUG_LOG("bredrScanManager_InstanceCompleteTransition, scan now disabled");
        SET_BSM_STATE(context->state, BSM_SCAN_DISABLED);
        state_changed = TRUE;

        if(context != bredrScanManager_InquiryScanContext())
        {
            DEBUG_LOG("bredrScanManager_InstanceCompleteTransition, feature to be stopped");

            BandwidthManager_FeatureStop(BANDWIDTH_MGR_FEATURE_PAGE_SCAN);
        }

        task_list_t *clients = TaskList_GetBaseTaskList(&context->clients);

        bredrScanManager_RemovePendingReleaseClients(clients);

        bool have_clients = bredrScanManager_DoesInstanceHaveClients(context);

        DEBUG_LOG("bredrScanManager_InstanceCompleteTransition, does instance have clients %d", have_clients);

        /* send out pause indication to the clients if we are transitioning to disabled state */
        if (TEST_BSM_STATE(context->state, BSM_SCAN_DISABLED)
                && have_clients)
        {
            context->paused = TRUE;
            TaskList_MessageSendId(clients, bredrScanManager_PausedMsgId(context));
        }
    }
    else if (TEST_BSM_STATE(context->state, BSM_SCAN_ENABLING))
    {
        DEBUG_LOG("bredrScanManager_InstanceCompleteTransition, scan now enabled");
        SET_BSM_STATE(context->state, BSM_SCAN_ENABLED);
        state_changed = TRUE;

        if(context != bredrScanManager_InquiryScanContext())
        {
            DEBUG_LOG("bredrScanManager_InstanceCompleteTransition, feature to be started");

            BandwidthManager_FeatureStart(BANDWIDTH_MGR_FEATURE_PAGE_SCAN);
        }

        task_list_t *clients = TaskList_GetBaseTaskList(&context->clients);

        /* send out resume indication to the clients if we are transitioning to enabled state */
        if (TEST_BSM_STATE(context->state, BSM_SCAN_ENABLED)
                && bredrScanManager_DoesInstanceHaveClients(context)
                && bredrScanManager_IsInstancePaused(context))
        {
            TaskList_MessageSendId(clients, bredrScanManager_ResumedMsgId(context));
            context->paused = FALSE;
        }

    }
    if (state_changed)
    {
        /* kick the state machine */
        bredrScanManager_RunStateMachine();
        /* send out disable confirmation if we have just transitioned to stable disabled state */
        if (bredrScanManager_IsDisabled() && BredrScanManager_IsScanDisabled())
        {
            bredrScanManager_SendDisableCfm(TRUE);
        }
    }
    return state_changed;
}

bredr_scan_manager_scan_type_t bredrScanManager_InstanceFindMaxScanType(bsm_scan_context_t *context, bool throttle)
{
    Task next_task = NULL;
    task_list_data_t data = {0};

    bredr_scan_manager_scan_type_t max_type = SCAN_MAN_PARAMS_TYPE_PENDING_RELEASE;

    while (TaskList_IterateWithData(TaskList_GetBaseTaskList(&context->clients), &next_task, &data))
    {
        bredr_scan_manager_scan_type_t this_type = bredrScanManager_ListDataGet(&data);

        if (this_type > max_type)
        {
            max_type = this_type;
        }
    }

    if(throttle
        && bredrScanManager_IsScanTypeRegistered(context, SCAN_MAN_PARAMS_TYPE_THROTTLE)
        && (max_type != SCAN_MAN_PARAMS_TYPE_PENDING_RELEASE))
    {
        max_type = SCAN_MAN_PARAMS_TYPE_THROTTLE;
    }

    if (max_type != SCAN_MAN_PARAMS_TYPE_PENDING_RELEASE && !bredrScanManager_IsScanTypeRegistered(context, max_type))
    {
        Panic();
    }

    return max_type;
}

void bredrScanManager_InstanceSendScanThrottleInd(bsm_scan_context_t *context, bool throttle_required)
{
    if(bredrScanManager_IsScanTypeRegistered(context, SCAN_MAN_PARAMS_TYPE_THROTTLE))
    {
        MessageId id;
        task_list_t *clients = TaskList_GetBaseTaskList(&context->clients);

        if (throttle_required)
        {
            id = BREDR_SCAN_MANAGER_PAGE_SCAN_THROTTLED_IND;
        }
        else
        {
            id = BREDR_SCAN_MANAGER_PAGE_SCAN_UNTHROTTLED_IND;
        }

        TaskList_MessageSendId(clients, id);
    }
}

void bredrScanManager_InstanceCleanUp(bsm_scan_context_t *context)
{
    DEBUG_LOG_VERBOSE("bredrScanManager_InstanceCleanUp context is 0x%x", context);

    if(context->needs_cleanup)
    {
        bredrScanManager_InstanceSetFlagToCleanUp(context, FALSE);

        if(bredrScanManager_DoesInstanceHaveClients(context))
        {
            DEBUG_LOG_VERBOSE("bredrScanManager_InstanceCleanUp instance has clients, remove clients pending release");

            task_list_t *clients = TaskList_GetBaseTaskList(&context->clients);
            bredrScanManager_RemovePendingReleaseClients(clients);
        }

        if(!bredrScanManager_DoesInstanceHaveClients(context))
        {
            DEBUG_LOG_VERBOSE("bredrScanManager_InstanceCleanUp instance has no clients, reset fields in instance");

            context->paused = FALSE;
            context->throttled = FALSE;
            context->state = BSM_SCAN_DISABLED;
            context->type = SCAN_MAN_PARAMS_TYPE_PENDING_RELEASE;
        }
    }
}

/*! @} */
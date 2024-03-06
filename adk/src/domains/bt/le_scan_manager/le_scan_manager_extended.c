/*!
    \copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_scan_manager
    \brief      Implementation of module managing le extended scanning.
*/

#ifdef ENABLE_LE_EXTENDED_SCANNING

#include "le_scan_manager_extended.h"

#include <logging.h>
#include <panic.h>
#include <vmtypes.h>
#include <stdlib.h>

/*! Function to handle extended scan related messages from synergy library */
static void leScanManager_HandleExtendedMessages(Task task, MessageId id, Message message);

const TaskData extended_task = {.handler = leScanManager_HandleExtendedMessages};

#define Lesme_GetTask() ((Task)&extended_task)

/*! AD Type for incomplete List of UUID16 */
#define AD_TYPE_INCOMPLETE_LIST_UUID16           0x02

/*! AD Type for complete List of UUID16 */
#define AD_TYPE_COMPLETE_LIST_UUID16             0x03

/*! AD Type for list of 16 bit solicitations UUIDs */
#define AD_TYPE_SOLICITATIONS_LIST_UUID16        0x14

/*! AD Type for service data of type UUID16 */
#define AD_TYPE_SERVICE_DATA_UUID16              0x16

/*! Scan Interval to be used for LE Extended Scanning */
#define LE_EXTENDED_SCAN_INTERVAL                0x64 /* (100 * 0.625 = 62.5ms) */

/*! Scan Window to be used for LE Extended Scanning */
#define LE_EXTENDED_SCAN_WINDOW                  0x64 /* (100 * 0.625 = 62.5ms) */

/*! Get the AD Element Data Length from the provided Advert payload */
#define leScanManager_GetAdvElemLength(data)     (data[0])

/*! Get the AD Element Ad Type provided Advert payload */
#define leScanManager_GetAdvElemAdType(data)     (data[1])

/*! Get the AD Element Data from the provided Advert payload */
#define leScanManager_GetAdvElemData(data)       (&data[2])

/*! Extract the 16bit Service UUID from the AD Element Data */
#define leScanManager_ExtractServiceUUID16(data)  (((uint16) data[1] << 8) | data[0])

/*! LE Extended States */
typedef enum le_scan_manger_extended_states
{
    /*! Extended module has not yet been initialised */
    LE_SCAN_MANAGER_EXTENDED_STATE_UNINITIALISED,

    /*! Extended module is enabled */
    LE_SCAN_MANAGER_EXTENDED_STATE_ENABLED,

    /*! Extended module is setting global scan parameters */
    LE_SCAN_MANAGER_EXTENDED_STATE_SETTING_GLOBAL_SCAN_PARAMS,

    /*! Extended module is registering scanner */
    LE_SCAN_MANAGER_EXTENDED_STATE_REGISTERING_SCANNER,

    /*! Extended module is enabling scanner */
    LE_SCAN_MANAGER_EXTENDED_STATE_ENABLING_SCANNER,

    /*! Extended module is disabling scanner */
    LE_SCAN_MANAGER_EXTENDED_STATE_DISABLING_SCANNER,

    /*! Extended module is unregistering scanner */
    LE_SCAN_MANAGER_EXTENDED_STATE_UNREGISTERING_SCANNER,

    /*! Extended module is disabled */
    LE_SCAN_MANAGER_EXTENDED_STATE_DISABLED,

    /*! Extended module is paused */
    LE_SCAN_MANAGER_EXTENDED_STATE_PAUSED,

    /*! Extended module is scanning */
    LE_SCAN_MANAGER_EXTENDED_STATE_SCANNING
} extendedScanState;

/*! Current LE Extended Command*/
typedef enum le_scan_manager_extended_commands
{
    /*! Extended module No Command */
    LE_SCAN_MANAGER_CMD_EXTENDED_NONE,

    /*! Extended module find extended adverts command */
    LE_SCAN_MANAGER_CMD_EXTENDED_START,

    /*! Extended module Stop Command */
    LE_SCAN_MANAGER_CMD_EXTENDED_STOP,

    /*! Extended module Disable Command */
    LE_SCAN_MANAGER_CMD_EXTENDED_DISABLE,

    /*! Extended module Enable Command */
    LE_SCAN_MANAGER_CMD_EXTENDED_ENABLE
} extendedScanCommand;

/* \brief LE Extended Scan settings. */
typedef struct
{
    /*! Advertising filter for parsing received extended advert reports */
    le_extended_advertising_filter_t filter;

    /*! Scan Handle for extended Scanning */
    uint8 scan_handle;

    /*! Scan Procedure for extended Scanning */
    extendedScanCommand scan_procedure;

    /*! Scan Task for extended Scanning*/
    Task scan_task;
} le_extended_scan_settings_t;

/*! \brief LE extended scan manager task and state machine */
typedef struct
{
   /*! Task for extended Scanning message handling */
   TaskData task;

   /*! State for extended Scanning */
   extendedScanState state;

   /*! Current command for extended Scanning */
   extendedScanCommand command;

   /*! Settings used for the Busy lock */
   le_extended_scan_settings_t* is_busy;

   /*! Busy lock */
   uint16 busy_lock;

   /*! Current Task Requester for receiving extended messages */
   Task  requester;

   /*! Active settings */
   le_extended_scan_settings_t* active_settings[MAX_ACTIVE_SCANS];

   /*! List Of tasks which to get response of filtered extended adverts*/
   task_list_t ext_scan_filtered_adv_report_client_list;
} le_scan_manager_extended_data_t;

le_scan_manager_extended_data_t  le_scan_manager_extended_data;

/*! Get extended task data */
#define LeScanManagerGetExtendedTaskData()          (&le_scan_manager_extended_data)

/*! Get extended task */
#define LeScanManagerGetExtendedTask()              (&le_scan_manager_extended_data.task)

/*! Get extended task state*/
#define LeScanManagerGetExtendedState()             (le_scan_manager_extended_data.state)

/*! Set extended current command to execute */
#define LeScanManagerSetExtendedCurrentCommand(s)   (le_scan_manager_extended_data.command = s)

/*! Get extended current command that is executing now */
#define LeScanManagerGetExtendedCurrentCommand()    (le_scan_manager_extended_data.command)

static le_extended_scan_settings_t* leScanManager_GetExtendedBusySettings(void);
static void leScanManager_SetExtendedGlobalParams(le_extended_scan_settings_t* scan_settings);
static bool leScanManager_IsExtendedBusy(void);
static void leScanManager_ClearExtendedBusy(void);
static void leScanManager_HandleExtendedDisable(void);
static void leScanManager_SendExtendedStopReq(le_extended_scan_settings_t* scan_settings);

/*! Returns TRUE if the provided GATT Service UUID is present in the received Extended advertising report 
 *
 *  Advertisment Report is of the following format:
 *
 *  ADV_DATA
 *     |
 *     |
 *     |---AD_ELEM
 *     |     |
 *     |     |
 *     |      --Length :   Length of the AD Element (Length of AD Type(1 Byte) + Length of Data)
 *     |      --AD Type:   AD Type such as Flag, Complete Service UUID list etc...
 *     |      --Data   :   Adv Elem Data.
 *     |
 *     |---AD_ELEM
 *     |     |
 *     |     |
 *     |      --Length :   Length of the AD Element (Length of AD Type(1 Byte) + Length of Data)
 *     |      --AD Type:   AD Type such as Flag, Complete Service UUID list etc...
 *     |      --Data   :   Adv Elem Data.
 *
 *   This function parses through all the ADV Element received in the advertisement report and checks whether 
 *   AD_TYPE_SERVICE_DATA_UUID16 or AD_TYPE_COMPLETE_LIST_UUID16 is present in any of the ADV Element.
 *   Filtering also considers solicitations list as some GAP Peripheral devices may show interest in GAP Central
 *   devices with certain GATT services and includes them in a solicitation list. If present it extracts the
     16 bit Service UUID(s) and compares with the Filter UUID supplied and returns TRUE if a match is found.
*/
static bool leScanManager_FilterAdvReportByUuid16(const CsrUint8 *data, CsrUint16 data_length, CsrUint16 filter_uuid)
{
    uint16 service_uuid;
    uint8 adv_elem_length;
    uint8 adv_elem_ad_type;
    const uint8 *adv_elem_data;
    const uint8 *end_data;

    if (data == NULL)
    {
        return FALSE;
    }

    end_data = data + data_length;

    /* Loop through the advert data, and extract each AD Element-type to parse */
    while (data < (end_data - 1))
    {
        adv_elem_length = leScanManager_GetAdvElemLength(data);

        if ((data + adv_elem_length + 1) > end_data)
        {
            return FALSE;
        }

        adv_elem_ad_type = leScanManager_GetAdvElemAdType(data);
        adv_elem_data = leScanManager_GetAdvElemData(data);

        switch (adv_elem_ad_type)
        {
            case AD_TYPE_SERVICE_DATA_UUID16:
            {
                /* Extract 16 bit Service UUID */
                service_uuid = leScanManager_ExtractServiceUUID16(adv_elem_data);

                if (service_uuid == filter_uuid)
                {
                    DEBUG_LOG("leScanManager_FilterAdvReportByUuid16 Filter Service UUID Found 0x%x", filter_uuid);
                    return TRUE;
                }
            }
            break;

            case AD_TYPE_SOLICITATIONS_LIST_UUID16:
            case AD_TYPE_INCOMPLETE_LIST_UUID16:
            case AD_TYPE_COMPLETE_LIST_UUID16:
            {
                /* Find the number of UUIDs present in the list */
                uint8 uuid_count;
                uint8 num_of_uuids = (adv_elem_length - 1) / sizeof(uint16);

                /* Iterate through the UUID list and compare */
                for (uuid_count = 0; uuid_count < num_of_uuids; uuid_count++)
                {
                    service_uuid = leScanManager_ExtractServiceUUID16(adv_elem_data);

                    if (service_uuid == filter_uuid)
                    {
                        DEBUG_LOG("leScanManager_FilterAdvReportByUuid16 Filter Service UUID Found 0x%x", filter_uuid);
                        return TRUE;
                    }

                    adv_elem_data += sizeof(uint16);
                }
            }
            break;

            default:
            break;
        }

        /* Increment the data by the total length of current AD Element */
        data += (adv_elem_length + 1);
    }

    return FALSE;
}

/*! Updates the extended scan state */
static void leScanManager_SetExtendedState(extendedScanState state)
{
    DEBUG_LOG("leScanManager_SetExtendedState %d->%d", le_scan_manager_extended_data.state, state);
    le_scan_manager_extended_data.state = state;
}

/*! Sends LE_SCAN_MANAGER_EXTENDED_STOP_CFM to registered clients */
static void leScanManager_SendExtendedStopCfm(Task req_task, Task scan_task, le_scan_manager_status_t scan_status)
{
    MAKE_MESSAGE(LE_SCAN_MANAGER_EXTENDED_STOP_CFM);
    message->result = scan_status;
    message->scan_task = scan_task;
    MessageSend(req_task, LE_SCAN_MANAGER_EXTENDED_STOP_CFM, message);
}

/*! Sends LE_SCAN_MANAGER_EXTENDED_DISABLE_CFM to registered clients */
static void leScanManager_SendExtendedDisableCfm(Task req_task, le_scan_manager_status_t scan_status)
{
    MAKE_MESSAGE(LE_SCAN_MANAGER_EXTENDED_DISABLE_CFM);
    message->result = scan_status;
    MessageSend(req_task, LE_SCAN_MANAGER_EXTENDED_DISABLE_CFM, message);
}

/*! Sends LE_SCAN_MANAGER_EXTENDED_ENABLE_CFM to registered clients */
static void leScanManager_SendExtendedEnableCfm(Task req_task, le_scan_manager_status_t scan_status)
{
    MAKE_MESSAGE(LE_SCAN_MANAGER_EXTENDED_ENABLE_CFM);
    message->result = scan_status;
    MessageSend(req_task, LE_SCAN_MANAGER_EXTENDED_ENABLE_CFM, message);
}

/*! Sends LE_SCAN_MANAGER_START_EXTENDED_SCAN_CFM to LE Scan Manager */
static void leScanManager_SendExtendedScanStartCfm(Task task, le_scan_result_t scan_status)
{
    MAKE_MESSAGE(LE_SCAN_MANAGER_START_EXTENDED_SCAN_CFM);
    message->status = scan_status;
    MessageSend(task, LE_SCAN_MANAGER_START_EXTENDED_SCAN_CFM, message);
}

/*! Returns a empty slot index which could be used to store the extended scan settings */
static uint8 leScanManager_GetExtendedScanEmptySlotIndex(void)
{
    uint8 settings_index;
    le_scan_manager_extended_data_t* extended_data = LeScanManagerGetExtendedTaskData();

    for (settings_index = 0; settings_index < MAX_ACTIVE_SCANS; settings_index++)
    {
        if (extended_data->active_settings[settings_index] == NULL)
        {
            break;
        }
    }

    return settings_index;
}

/*! Preserve the extended scan settings provided by the client */
static le_extended_scan_settings_t* leScanManager_StoreExtendedScan(le_extended_advertising_filter_t* filter, Task task)
{
    le_extended_scan_settings_t *scan_settings;
    le_scan_manager_extended_data_t* extended_data = LeScanManagerGetExtendedTaskData();
    uint8 settings_index = leScanManager_GetExtendedScanEmptySlotIndex();

    PanicNull(filter);

    if (settings_index < MAX_ACTIVE_SCANS)
    {
        DEBUG_LOG("leScanManager_StoreExtendedScan scan settings available.");
        extended_data->active_settings[settings_index] = PanicUnlessMalloc(sizeof(*scan_settings));

        extended_data->active_settings[settings_index]->filter.size_ad_types = filter->size_ad_types;
        extended_data->active_settings[settings_index]->filter.ad_types = PanicUnlessMalloc(filter->size_ad_types);
        memcpy(extended_data->active_settings[settings_index]->filter.ad_types , filter->ad_types , sizeof(uint8)*(filter->size_ad_types));

        extended_data->active_settings[settings_index]->filter.uuid_list_size = filter->uuid_list_size;
        extended_data->active_settings[settings_index]->filter.uuid_list = PanicUnlessMalloc(sizeof(le_scan_manager_uuid_t) * filter->uuid_list_size);
        memcpy(extended_data->active_settings[settings_index]->filter.uuid_list, filter->uuid_list, sizeof(le_scan_manager_uuid_t) * (filter->uuid_list_size));

        extended_data->active_settings[settings_index]->scan_procedure = LE_SCAN_MANAGER_CMD_EXTENDED_START;
        extended_data->active_settings[settings_index]->scan_task = task;
        scan_settings = extended_data->active_settings[settings_index];
    }
    else
    {
        DEBUG_LOG("leScanManager_StoreExtendedScan scan settings unavailable.");
        scan_settings = NULL;
    }

    return scan_settings;
}

/*! Get the slot in which the extended scan settings are stored for a client task */
static uint8 leScanManager_GetExtendedIndexFromTask(Task task)
{
    le_scan_manager_extended_data_t* extended_data = LeScanManagerGetExtendedTaskData();
    uint8 settings_index;

    for (settings_index = 0; settings_index < MAX_ACTIVE_SCANS; settings_index++)
    {
        le_extended_scan_settings_t *scan_settings = extended_data->active_settings[settings_index];

        if (scan_settings && scan_settings->scan_task == task)
        {
            break;
        }
    }

    return settings_index;
}

/*! Free the extended scan settings */
static void leScanManager_FreeExtendedScanSettings(le_extended_scan_settings_t *scan_settings)
{
    if (scan_settings->filter.ad_types)
    {
        free(scan_settings->filter.ad_types);
    }

    if (scan_settings->filter.uuid_list)
    {
        free(scan_settings->filter.uuid_list);
    }

    free(scan_settings);
}

/*! Find and clear the extended scan settings stored for the client */
static bool leScanManager_ClearExtendedScanOnTask(Task requester)
{
    le_scan_manager_extended_data_t* extended_data = LeScanManagerGetExtendedTaskData();
    int settings_index;

    for (settings_index = 0; settings_index < MAX_ACTIVE_SCANS; settings_index++)
    {
        if ((extended_data->active_settings[settings_index] != NULL) && (extended_data->active_settings[settings_index]->scan_task == requester))
        {
            leScanManager_FreeExtendedScanSettings(extended_data->active_settings[settings_index]);
            extended_data->active_settings[settings_index] = NULL;

            return TRUE;
        }
    }

    return FALSE;
}

/*! Release the extended scan settings */
static bool leScanManager_ReleaseExtendedScan(Task task)
{
    bool released = FALSE;
    le_scan_manager_extended_data_t* extended_data = LeScanManagerGetExtendedTaskData();
    uint8 settings_index = leScanManager_GetExtendedIndexFromTask(task);

    if (settings_index < MAX_ACTIVE_SCANS)
    {
        le_extended_scan_settings_t *scan_settings = extended_data->active_settings[settings_index];

        DEBUG_LOG("leScanManager_ReleaseScan scan settings released index %u", settings_index);
        leScanManager_FreeExtendedScanSettings(scan_settings);
        extended_data->active_settings[settings_index] = NULL;
        released = TRUE;
    }

    return released;
}

/*! Is any extended scan active? */
static bool leScanManager_AnyActiveExtendedScan(void)
{
    int settings_index;
    bool scan_active = FALSE;
    le_scan_manager_extended_data_t* extended_data = LeScanManagerGetExtendedTaskData();

    for (settings_index = 0; settings_index < MAX_ACTIVE_SCANS; settings_index++)
    {
        if ((extended_data->active_settings[settings_index] != NULL)&&
            (extended_data->active_settings[settings_index]->scan_procedure == LE_SCAN_MANAGER_CMD_EXTENDED_START))
        {
            scan_active = TRUE;
        }
    }

    return scan_active;
}

/*! Add the client to the extended advertising report client list */
static bool leScanManager_addExtendedScanClient(Task client)
{
    le_scan_manager_extended_data_t * extended_data = LeScanManagerGetExtendedTaskData();
    return TaskList_AddTask(&extended_data->ext_scan_filtered_adv_report_client_list, client);
}

/*! Removes the client from the extended advertising report client list */
static bool leScanManager_removeExtendedScanClient(Task client)
{
    le_scan_manager_extended_data_t * extended_data = LeScanManagerGetExtendedTaskData();
    return TaskList_RemoveTask(&extended_data->ext_scan_filtered_adv_report_client_list, client);
}

/*! Send a extended scan start failure message to LE Scan Manager */
static void leScanManager_handleExtendedScanFailure(extendedScanCommand cmd, Task req)
{
    DEBUG_LOG("leScanManager_handleExtendedScanFailure for command %d and client %d", cmd, req);

    switch(cmd)
    {
        case LE_SCAN_MANAGER_CMD_EXTENDED_START:
        {
            leScanManager_SendExtendedScanStartCfm(req, LE_SCAN_MANAGER_RESULT_FAILURE);
        }
        break;

        default:
            Panic();
        break;
    }

    leScanManager_ClearExtendedScanOnTask(req);
}

/*! Set the extended scan settings current being used for scanning and set the busy lock */
static void leScanManager_SetExtendedBusy(le_extended_scan_settings_t *scan_settings)
{
    le_scan_manager_extended_data_t *extended_data = LeScanManagerGetExtendedTaskData();

    extended_data->is_busy = scan_settings;
    extended_data->busy_lock = 1;
}

/*! Execute procedures needed to start extended scan */
static void leScanManager_SendExtendedScanReq(le_extended_scan_settings_t *scan_settings)
{
    DEBUG_LOG("leScanManager_SendExtendedScanReq handles scan:%d procedure:enum:extendedScanCommand:%d",
               scan_settings->scan_handle,
               scan_settings->scan_procedure);

    leScanManager_SetExtendedBusy(scan_settings);

    /* Set the extended global parameters */
    leScanManager_SetExtendedGlobalParams(scan_settings);
}

/*! Is the task requesting the extended scan already present? */
static bool leScanManager_isExtendedDuplicate(Task requester)
{
    le_scan_manager_extended_data_t * extended_data = LeScanManagerGetExtendedTaskData();
    int settings_index;

    for (settings_index = 0; settings_index < MAX_ACTIVE_SCANS; settings_index++)
    {
        if (extended_data->active_settings[settings_index]!= NULL &&
            extended_data->active_settings[settings_index]->scan_task == requester)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/*! Handle the extended scan start request */
static void leScanManager_HandleExtendedStart(Task task, le_extended_advertising_filter_t *filter)
{
    extendedScanState current_state = LeScanManagerGetExtendedState();
    bool respond = FALSE;
    le_extended_scan_settings_t *scan_settings = NULL;
    le_scan_manager_status_t scan_result = {LE_SCAN_MANAGER_RESULT_FAILURE};

    DEBUG_LOG("leScanManager_HandleExtendedStart Current State is:: %d", current_state);

    if (leScanManager_isExtendedDuplicate(task))
    {
        DEBUG_LOG("Found Duplicate for Task %d",task);
        scan_result.status = LE_SCAN_MANAGER_RESULT_FAILURE;
        respond = TRUE;
    }
    else if (leScanManager_IsExtendedBusy())
    {
        DEBUG_LOG("CL is Busy!");
        scan_result.status = LE_SCAN_MANAGER_RESULT_BUSY;
        respond = TRUE;
    }
    else
    {
        switch (current_state)
        {
            case LE_SCAN_MANAGER_EXTENDED_STATE_DISABLED:
            case LE_SCAN_MANAGER_EXTENDED_STATE_PAUSED:
            {
                /* Save the scan parameters and respond. Scan shall start on resume/enable */
                DEBUG_LOG("leScanManager_HandleExtendedStart Cannot start scanning in state %d!", current_state);
                scan_settings = leScanManager_StoreExtendedScan(filter, task);

                if(scan_settings)
                {
                    DEBUG_LOG("leScanManager_HandleExtendedStart new scan settings created.");
                    scan_result.status = LE_SCAN_MANAGER_RESULT_SUCCESS;
                }
                else
                {
                    scan_result.status = LE_SCAN_MANAGER_RESULT_FAILURE;
                }
                respond = TRUE;
            }
            break;

            case LE_SCAN_MANAGER_EXTENDED_STATE_ENABLED:
            case LE_SCAN_MANAGER_EXTENDED_STATE_SCANNING:
            {
                /* Acquire the scan and save the filter details in local structure */
                scan_settings = leScanManager_StoreExtendedScan(filter, task);

                if (scan_settings)
                {
                    LeScanManagerSetExtendedCurrentCommand(LE_SCAN_MANAGER_CMD_EXTENDED_START);
                    DEBUG_LOG("leScanManager_HandleExtendedStart new scan settings created.");
                    leScanManager_SendExtendedScanReq(scan_settings);
                }
                else
                {
                    scan_result.status = LE_SCAN_MANAGER_RESULT_FAILURE;
                    respond = TRUE;
                }
            }
            break;

            default:
                break;
        }
    }

    if(respond)
    {
        leScanManager_SendExtendedScanStartCfm(task,scan_result.status);
    }
}

/*! Gets the next extended task in start state to the provided task */
static Task leScanManager_GetNextExtendedScanTaskAfterSpecifiedTask(Task task)
{
    int settings_index;
    le_scan_manager_extended_data_t *extended_data = LeScanManagerGetExtendedTaskData();

    for (settings_index = 0; settings_index < MAX_ACTIVE_SCANS; settings_index++)
    {
        if ((extended_data->active_settings[settings_index] != NULL) && (extended_data->active_settings[settings_index]->scan_task == task))
        {
            settings_index++;
            for (; settings_index < MAX_ACTIVE_SCANS; settings_index++)
            {
                if (extended_data->active_settings[settings_index] != NULL)
                {
                    switch (extended_data->active_settings[settings_index]->scan_procedure)
                    {
                        case LE_SCAN_MANAGER_CMD_EXTENDED_START:
                        {
                            return extended_data->active_settings[settings_index]->scan_task;
                        }

                        default:
                        break;
                    }
                }
            }
        }
    }

    return NULL;
}

/* Starts the extended scan for a specified task */
static bool leScanManager_StartExtendedScanByTask(Task task)
{
    int settings_index;
    le_scan_manager_extended_data_t *extended_data = LeScanManagerGetExtendedTaskData();

    for (settings_index = 0; settings_index < MAX_ACTIVE_SCANS; settings_index++)
    {
        if (extended_data->active_settings[settings_index]!= NULL)
        {
            if (extended_data->active_settings[settings_index]->scan_task == task)
            {
                switch (extended_data->active_settings[settings_index]->scan_procedure)
                {
                    case LE_SCAN_MANAGER_CMD_EXTENDED_START:
                    {
                        leScanManager_SendExtendedScanReq(extended_data->active_settings[settings_index]);
                        return FALSE;
                    }

                    default:
                        Panic();
                    break;
                }
            }
        }
    }

    return TRUE;
}

/* Handle extended enable request for a task */
static void leScanManager_HandleExtendedEnable(Task current_task)
{
    Task extended_scan_task = leScanManager_GetNextExtendedScanTaskAfterSpecifiedTask(current_task);

    if (extended_scan_task)
    {
        leScanManager_StartExtendedScanByTask(extended_scan_task);
    }

    leScanManager_ClearExtendedBusy();
}

/* Handle and send extended advertising report to registered clients */
static void leScanManager_handleConnectionDmBleExtScanFilteredAdvReportInd(const CL_DM_BLE_EXT_SCAN_FILTERED_ADV_REPORT_IND_T *ind)
{
    Task next_client = NULL;
    le_scan_manager_extended_data_t *extended_data = LeScanManagerGetExtendedTaskData();

    switch(LeScanManagerGetExtendedState())
    {
        case LE_SCAN_MANAGER_EXTENDED_STATE_SCANNING:
        {
            while (TaskList_Iterate(&extended_data->ext_scan_filtered_adv_report_client_list, &next_client))
            {
                next_client->handler(next_client, LE_SCAN_MANAGER_EXT_SCAN_FILTERED_ADV_REPORT_IND, ind);
            }
        }
        break;

        default:
        break;
    }
}

/* Get the first extended scan task in start state */
static Task leScanManager_GetFirstExtendedScanTask(void)
{
    int settings_index;
    le_scan_manager_extended_data_t *extended_data = LeScanManagerGetExtendedTaskData();

    for (settings_index = 0; settings_index < MAX_ACTIVE_SCANS; settings_index++)
    {
        if (extended_data->active_settings[settings_index] != NULL)
        {
            switch (extended_data->active_settings[settings_index]->scan_procedure)
            {
                case LE_SCAN_MANAGER_CMD_EXTENDED_START:
                {
                    return extended_data->active_settings[settings_index]->scan_task;
                }
                default:
                break;
            }
        }
    }

    return NULL;
}

/* Handle the extended scan stop confirmation */
static void leScanManager_handleExtendedScanStopCfm(const CmExtScanUnregisterScannerCfm *cfm)
{
    le_scan_manager_extended_data_t *extended_data = LeScanManagerGetExtendedTaskData();
    le_extended_scan_settings_t *scan_settings = leScanManager_GetExtendedBusySettings();
    extendedScanCommand scan_command = LeScanManagerGetExtendedCurrentCommand();
    le_scan_manager_status_t scan_result = {LE_SCAN_MANAGER_RESULT_FAILURE};
    Task scan_task;

    DEBUG_LOG("leScanManager_handleExtendedScanStopCfm resultCode 0x%04x", cfm->resultCode);

    if (cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
    {
        switch (LeScanManagerGetExtendedState())
        {
            case LE_SCAN_MANAGER_EXTENDED_STATE_UNREGISTERING_SCANNER:
            {
                if (scan_command == LE_SCAN_MANAGER_CMD_EXTENDED_STOP)
                {
                    scan_task = scan_settings->scan_task;
                    scan_result.status = LE_SCAN_MANAGER_RESULT_SUCCESS;

                    leScanManager_ClearExtendedBusy();
                    leScanManager_removeExtendedScanClient(scan_task);
                    leScanManager_ReleaseExtendedScan(scan_task);
                    leScanManager_SendExtendedStopCfm(extended_data->requester, scan_task, scan_result);

                    /* update state if nothing scanning */
                    if (!leScanManager_AnyActiveExtendedScan())
                    {
                        leScanManager_SetExtendedState(LE_SCAN_MANAGER_EXTENDED_STATE_ENABLED);
                    }
                }
                else if(scan_command == LE_SCAN_MANAGER_CMD_EXTENDED_DISABLE)
                {
                    scan_task = scan_settings->scan_task;
                    leScanManager_removeExtendedScanClient(scan_task);
                    leScanManager_ReleaseExtendedScan(scan_task);
                    leScanManager_HandleExtendedDisable();
                }
            }
            break;

            default:
            break;
        }
    }
    else
    {
        Panic();
    }
}

/* Is Extended scan busy */
static bool leScanManager_IsExtendedBusy(void)
{
    le_scan_manager_extended_data_t *extended_data = LeScanManagerGetExtendedTaskData();

    return extended_data->busy_lock;
}

/* Get extended scan settings for the ongoing scan */
static le_extended_scan_settings_t* leScanManager_GetExtendedBusySettings(void)
{
    le_scan_manager_extended_data_t *extended_data = LeScanManagerGetExtendedTaskData();

    return extended_data->is_busy;
}

/* Clear busy settings */
static void leScanManager_ClearExtendedBusy(void)
{
    le_scan_manager_extended_data_t *extended_data = LeScanManagerGetExtendedTaskData();

    extended_data->busy_lock = 0;
}

/* Function to set the extended global parameter */
static void leScanManager_SetExtendedGlobalParams(le_extended_scan_settings_t* scan_settings)
{
    CmScanningPhy phys[2] = {0};

    UNUSED(scan_settings);

    phys[0].scan_type = CSR_BT_CM_LE_SCANTYPE_ACTIVE;
    phys[0].scan_interval = LE_EXTENDED_SCAN_INTERVAL;
    phys[0].scan_window = LE_EXTENDED_SCAN_WINDOW;

    leScanManager_SetExtendedState(LE_SCAN_MANAGER_EXTENDED_STATE_SETTING_GLOBAL_SCAN_PARAMS);

    /* Set the global Parameters */
    CmExtScanSetGlobalParamsRequest(Lesme_GetTask(),
                                    0, /* No Flags */
                                    0, /* Address type is public */
                                    0, /* No Filter policy applied */
                                    0, /* Duplicate filtering is disabled */
                                    1, /* Scan on one PHY */
                                    phys);
}

/* Function to stop the ongoing extended scan for a task */
static bool leScanManager_StopExtendedScanByTask(Task task)
{
    int settings_index;
    le_scan_manager_extended_data_t *extended_data = LeScanManagerGetExtendedTaskData();

    for (settings_index = 0; settings_index < MAX_ACTIVE_SCANS; settings_index++)
    {
        if (extended_data->active_settings[settings_index]!= NULL)
        {
            if (extended_data->active_settings[settings_index]->scan_task == task)
            {
                switch (extended_data->active_settings[settings_index]->scan_procedure)
                {
                    case LE_SCAN_MANAGER_CMD_EXTENDED_START:
                    {
                        leScanManager_SendExtendedStopReq(extended_data->active_settings[settings_index]);
                        return FALSE;
                    }

                    default:
                        Panic();
                    break;
                }
            }
        }
    }

    return TRUE;
}

/*! Handle the extended scan disable request */
static void leScanManager_HandleExtendedDisable(void)
{
    Task extended_scan_task = leScanManager_GetFirstExtendedScanTask();
    le_scan_manager_extended_data_t *extended_data = LeScanManagerGetExtendedTaskData();
    le_scan_manager_status_t scan_result = {LE_SCAN_MANAGER_RESULT_SUCCESS};
    bool respond = FALSE;

    if (extended_scan_task)
    {
        respond = leScanManager_StopExtendedScanByTask(extended_scan_task);
    }
    else
    {
        respond = TRUE;
    }

    if (respond)
    {
        leScanManager_ClearExtendedBusy();
        leScanManager_SetExtendedState(LE_SCAN_MANAGER_EXTENDED_STATE_DISABLED);
        leScanManager_SendExtendedDisableCfm(extended_data->requester, scan_result);
    }
}

/*! Sends the CM command to enable the extended scanner for receiving adv reports */
static void leScanManager_EnableExtendedScanner(void)
{
    le_extended_scan_settings_t *ext_scan_settings = leScanManager_GetExtendedBusySettings();
    CsrUint8 numOfScanners = 1;
    CmScanners scanners[1];
    scanners[0].scan_handle = ext_scan_settings->scan_handle;
    scanners[0].duration = 0;  /* Currently, it should be set to 0 only */

    leScanManager_SetExtendedState(LE_SCAN_MANAGER_EXTENDED_STATE_ENABLING_SCANNER);

    CmExtScanEnableScannersReq(Lesme_GetTask(),
                              CSR_BT_CM_LE_MODE_ON,
                              numOfScanners,
                              scanners);
}

/*! Sends the CM command to stop the extended scanner */
static void leScanManager_SendExtendedStopReq(le_extended_scan_settings_t *scan_settings)
{
    CsrUint8 numOfScanners = 1;
    CmScanners scanners[1];

    DEBUG_LOG("leScanManager_SendExtendedStopReq");

    scanners[0].scan_handle = scan_settings->scan_handle;
    scanners[0].duration = 0;  /* Currently, it should be set to 0 only */

    leScanManager_SetExtendedBusy(scan_settings);
    leScanManager_SetExtendedState(LE_SCAN_MANAGER_EXTENDED_STATE_DISABLING_SCANNER);

    CmExtScanEnableScannersReq(Lesme_GetTask(),
                               CSR_BT_CM_LE_MODE_OFF,
                               numOfScanners,
                               scanners);
}

/*! Post disabling an active extended scanner, unregister the scanner */
static void leScanManager_UnregisterExtendedScanner(void)
{
    le_extended_scan_settings_t* ext_scan_settings = leScanManager_GetExtendedBusySettings();

    DEBUG_LOG("leScanManager_UnregisterExtendedScanner");

    leScanManager_SetExtendedState(LE_SCAN_MANAGER_EXTENDED_STATE_UNREGISTERING_SCANNER);
    CmExtScanUnregisterScannerReq(Lesme_GetTask(), ext_scan_settings->scan_handle);
}

/*! Register a extended scanner by sending a CM register scanner request */
static void leScanManager_RegisterExtendedScanner(le_extended_scan_settings_t *scan_settings)
{
    CsrUint32 flags = 0;
    CsrUint16 advFilter = CM_EXT_SCAN_ADV_FILTER_NONE;
    CsrUint16 advFilterSubField1 = CM_EXT_SCAN_SUB_FIELD_INVALID;
    CsrUint32 advFilterSubField2 = CM_EXT_SCAN_SUB_FIELD_INVALID;
    CsrUint16 adStructFilter = CM_EXT_SCAN_AD_STRUCT_FILTER_NONE;
    CsrUint16 adStructFilterSubField1 = 0;
    CsrUint32 adStructFilterSubField2 = 0;

    leScanManager_SetExtendedState(LE_SCAN_MANAGER_EXTENDED_STATE_REGISTERING_SCANNER);

    CmExtScanRegisterScannerReq(Lesme_GetTask(),
                                flags,
                                advFilter,
                                advFilterSubField1,
                                advFilterSubField2,
                                adStructFilter,
                                adStructFilterSubField1,
                                adStructFilterSubField2,
                                0, /* Filter based on ad types not working now */
                                scan_settings->filter.ad_types);
}

/*! handle extended scan start confirmation */
static void leScanManager_HandleCmExtendedScanStartCfm(CmExtScanEnableScannersCfm *cfm)
{
    le_extended_scan_settings_t *scan_settings = leScanManager_GetExtendedBusySettings();
    Task scan_task = scan_settings->scan_task;
    extendedScanCommand scan_command = LeScanManagerGetExtendedCurrentCommand();

    DEBUG_LOG("leScanManager_HandleExtendedScanStartCfm : Result 0x%x\n" , cfm->resultCode);

    if (cfm->resultCode != CSR_BT_RESULT_CODE_CM_SUCCESS)
    {
        leScanManager_handleExtendedScanFailure(scan_command, scan_task);
        leScanManager_ClearExtendedBusy();
        leScanManager_SetExtendedState(LE_SCAN_MANAGER_EXTENDED_STATE_ENABLED);
    }
    else if (cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
    {
        switch (LeScanManagerGetExtendedState())
        {
            case LE_SCAN_MANAGER_EXTENDED_STATE_ENABLING_SCANNER:
            {
                if (scan_command == LE_SCAN_MANAGER_CMD_EXTENDED_START)
                {
                    leScanManager_ClearExtendedBusy();
                    leScanManager_SetExtendedState(LE_SCAN_MANAGER_EXTENDED_STATE_SCANNING);
                    leScanManager_addExtendedScanClient(scan_task);
                    leScanManager_SendExtendedScanStartCfm(scan_task, LE_SCAN_MANAGER_RESULT_SUCCESS);
                }
            }
            break;

            case LE_SCAN_MANAGER_EXTENDED_STATE_DISABLING_SCANNER:
            {
                leScanManager_UnregisterExtendedScanner();
            }
            break;

            case LE_SCAN_MANAGER_EXTENDED_STATE_DISABLED:
            {
                if(scan_command == LE_SCAN_MANAGER_CMD_EXTENDED_START)
                {
                    leScanManager_addExtendedScanClient(scan_task);
                    leScanManager_HandleExtendedEnable(scan_task);
                }
            }
            break;

            default:
            {
                Panic();
            }
            break;
        }
    }
}

/*! handle extended scan unregister confirmation */
static void leScanManager_HandleCmExtendedScanUnregisterCfm(CmExtScanUnregisterScannerCfm *msg)
{
    if (msg->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
    {
        leScanManager_handleExtendedScanStopCfm(msg);
    }
}

/*! handle extended scan global parameters confirmation */
static void leScanManager_HandleCmExtendedScannerGlobalParamsCfm(CmExtScanSetGlobalParamsCfm *cfm)
{
    extendedScanState state = LeScanManagerGetExtendedState();

    DEBUG_LOG("CSR_BT_CM_EXT_SCAN_SET_GLOBAL_PARAMS_CFM Result 0x%x\n", cfm->resultCode);

    if (cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
        state == LE_SCAN_MANAGER_EXTENDED_STATE_SETTING_GLOBAL_SCAN_PARAMS)
    {
        le_extended_scan_settings_t * extended_scan_settings = leScanManager_GetExtendedBusySettings();

        /* Register the Scanner */
        leScanManager_RegisterExtendedScanner(extended_scan_settings);
    }
}

/*! handle extended scan register confirmation */
static void leScanManager_HandleCmExtendedScannerRegisterCfm(CmExtScanRegisterScannerCfm *cfm)
{
    extendedScanState state = LeScanManagerGetExtendedState();

    DEBUG_LOG("CSR_BT_CM_EXT_SCAN_REGISTER_SCANNER_CFM Result 0x%x, Scan Hndl 0x%x\n", cfm->resultCode, cfm->scan_handle);

    if (cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
        state == LE_SCAN_MANAGER_EXTENDED_STATE_REGISTERING_SCANNER)
    {
        /* Store the Scan handle*/
        le_extended_scan_settings_t * extended_scan_settings = leScanManager_GetExtendedBusySettings();

        if (extended_scan_settings != NULL)
        {
            extended_scan_settings->scan_handle = cfm->scan_handle;
            leScanManager_EnableExtendedScanner();
        }
        else
        {
            DEBUG_LOG("CSR_BT_CM_EXT_SCAN_REGISTER_SCANNER_CFM Invalid Scan Settings");
        }
    }
}

/*! Handle extended advert report indication */
static void leScanManager_HandleCmExtendedAdvertReportInd(CmExtScanFilteredAdvReportInd *cmMessage)
{
    uint8 filter_count = 0;
    bool valid_adv_report = FALSE;
    le_extended_scan_settings_t * scan_config = leScanManager_GetExtendedBusySettings();
    extendedScanState state = LeScanManagerGetExtendedState();
    CL_DM_BLE_EXT_SCAN_FILTERED_ADV_REPORT_IND_T clMessage;

    if (state != LE_SCAN_MANAGER_EXTENDED_STATE_SCANNING)
    {
        return;
    }

    for (filter_count = 0; filter_count < scan_config->filter.uuid_list_size; filter_count++)
    {
        le_scan_manager_uuid_t uuid_filter = scan_config->filter.uuid_list[filter_count];

        if (uuid_filter.type == UUID_TYPE_16)
        {
            valid_adv_report = leScanManager_FilterAdvReportByUuid16(cmMessage->data,
                                                                     cmMessage->dataLength,
                                                                     uuid_filter.uuid[0]);
            if (valid_adv_report)
            {
                /* If atleast one UUID is present, then consider it as a valid adv report */
                break;
            }
        }
    }

    if (scan_config->filter.uuid_list_size && !valid_adv_report)
    {
        /* Ignore the advertisement if filter list is present but adv doesn't contain UUID */
        return;
    }

    clMessage.event_type = cmMessage->eventType;
    clMessage.primary_phy = cmMessage->primaryPhy;
    clMessage.secondary_phy = cmMessage->secondaryPhy;
    clMessage.adv_sid = cmMessage->advSid;
    clMessage.current_addr.type = cmMessage->currentAddrt.type;
    clMessage.current_addr.addr.lap = cmMessage->currentAddrt.addr.lap;
    clMessage.current_addr.addr.uap = cmMessage->currentAddrt.addr.uap;
    clMessage.current_addr.addr.nap = cmMessage->currentAddrt.addr.nap;
    clMessage.permanent_addr.type = cmMessage->permanentAddrt.type;
    clMessage.permanent_addr.addr.lap = cmMessage->permanentAddrt.addr.lap;
    clMessage.permanent_addr.addr.uap = cmMessage->permanentAddrt.addr.uap;
    clMessage.permanent_addr.addr.nap = cmMessage->permanentAddrt.addr.nap;
    clMessage.direct_addr.type = cmMessage->directAddrt.type;
    clMessage.direct_addr.addr.lap = cmMessage->directAddrt.addr.lap;
    clMessage.direct_addr.addr.uap = cmMessage->directAddrt.addr.uap;
    clMessage.direct_addr.addr.nap = cmMessage->directAddrt.addr.nap;
    clMessage.tx_power = cmMessage->txPower;
    clMessage.rssi = cmMessage->rssi;
    clMessage.periodic_adv_interval = cmMessage->periodicAdvInterval;
    clMessage.adv_data_info = cmMessage->advDataInfo;
    clMessage.ad_flags = cmMessage->adFlags;
    clMessage.adv_data_len = cmMessage->dataLength;
    clMessage.adv_data = cmMessage->data;

    leScanManager_handleConnectionDmBleExtScanFilteredAdvReportInd(&clMessage);
}

/*! handle extended messages from cm */
static bool leScanManager_HandleExtendedCmMessages(void *msg)
{
    CsrBtCmPrim *primType = (CsrBtCmPrim *) msg;
    bool status = TRUE;

    switch (*primType)
    {
        case CSR_BT_CM_EXT_SCAN_SET_GLOBAL_PARAMS_CFM:
            leScanManager_HandleCmExtendedScannerGlobalParamsCfm((CmExtScanSetGlobalParamsCfm*) msg);
        break;

        case CSR_BT_CM_EXT_SCAN_REGISTER_SCANNER_CFM:
            leScanManager_HandleCmExtendedScannerRegisterCfm((CmExtScanRegisterScannerCfm*) msg);
        break;

        case CSR_BT_CM_EXT_SCAN_ENABLE_SCANNERS_CFM:
            leScanManager_HandleCmExtendedScanStartCfm((CmExtScanEnableScannersCfm*) msg);
        break;

        case CSR_BT_CM_EXT_SCAN_UNREGISTER_SCANNER_CFM:
            leScanManager_HandleCmExtendedScanUnregisterCfm((CmExtScanUnregisterScannerCfm*) msg);
        break;

        case CSR_BT_CM_EXT_SCAN_FILTERED_ADV_REPORT_IND:
            leScanManager_HandleCmExtendedAdvertReportInd((CmExtScanFilteredAdvReportInd*)msg);
        break;

        default:
        {
            status = FALSE;
        }
        break;
    }

    CmFreeUpstreamMessageContents((void *) msg);

    return status;
}

static void leScanManager_HandleExtendedMessages(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(id);

    leScanManager_HandleExtendedCmMessages((void *) message);
}

/************************** Public API *************************************************/

void LeScanManager_ExtendedScanInit(void)
{
    le_scan_manager_extended_data_t *scanTask = LeScanManagerGetExtendedTaskData();

    memset(scanTask, 0, sizeof(*scanTask));
    scanTask->task.handler = leScanManager_HandleExtendedMessages;
    TaskList_Initialise(&scanTask->ext_scan_filtered_adv_report_client_list);
    leScanManager_SetExtendedState(LE_SCAN_MANAGER_EXTENDED_STATE_ENABLED);
}

void LeScanManager_ExtendedScanStart(Task task, le_extended_advertising_filter_t* filter)
{
    DEBUG_LOG("leScanManager_StartExtendedScan from Requester %d",task);
    PanicNull(task);
    PanicFalse(LeScanManagerGetExtendedState() > LE_SCAN_MANAGER_EXTENDED_STATE_UNINITIALISED);

    leScanManager_HandleExtendedStart(task, filter);
}

bool LeScanManager_ExtendedScanStop(Task req_task, Task scan_task)
{
    le_scan_manager_extended_data_t *extended_data = LeScanManagerGetExtendedTaskData();
    le_scan_manager_status_t scan_result = {LE_SCAN_MANAGER_RESULT_FAILURE};
    bool respond = FALSE;
    uint8 settings_index = leScanManager_GetExtendedIndexFromTask(scan_task);

    if (settings_index >= MAX_ACTIVE_SCANS)
    {
        return FALSE;
    }

    if (leScanManager_IsExtendedBusy())
    {
        DEBUG_LOG("CL is Busy!");
        scan_result.status = LE_SCAN_MANAGER_RESULT_BUSY;
        respond = TRUE;
    }
    else
    {
        switch (LeScanManagerGetExtendedState())
        {
            case LE_SCAN_MANAGER_EXTENDED_STATE_SCANNING:
            {
                if (settings_index < MAX_ACTIVE_SCANS)
                {
                    extended_data->requester = req_task;
                    LeScanManagerSetExtendedCurrentCommand(LE_SCAN_MANAGER_CMD_EXTENDED_STOP);
                    respond = leScanManager_StopExtendedScanByTask(scan_task);
                    scan_result.status = LE_SCAN_MANAGER_RESULT_SUCCESS;
                }
                else
                {
                    /* Settings not found */
                    DEBUG_LOG("leScanManager_ExtendedScanStop cannot release scan settings");
                    scan_result.status = LE_SCAN_MANAGER_RESULT_SUCCESS;
                    respond = TRUE;
                }
            }
            break;

            default:
            {
                scan_result.status = LE_SCAN_MANAGER_RESULT_SUCCESS;
                respond = TRUE;
            }
            break;
        }
    }

    if (respond)
    {
        leScanManager_SendExtendedStopCfm(req_task, scan_task, scan_result);
    }

    return TRUE;
}

bool LeScanManager_IsExtendedTaskScanning(Task task)
{
    return leScanManager_isExtendedDuplicate(task);
}

bool leScanManager_ExtendedScanDisable(Task req_task)
{
    le_scan_manager_extended_data_t *extended_data = LeScanManagerGetExtendedTaskData();
    le_scan_manager_status_t scan_result = {LE_SCAN_MANAGER_RESULT_FAILURE};
    bool respond = FALSE;
    Task extended_scan_task = leScanManager_GetFirstExtendedScanTask();

    if (extended_scan_task == NULL)
    {
        if (LeScanManagerGetExtendedState() == LE_SCAN_MANAGER_EXTENDED_STATE_ENABLED)
        {
            leScanManager_SetExtendedState(LE_SCAN_MANAGER_EXTENDED_STATE_DISABLED);
        }
        return FALSE;
    }

    if (leScanManager_IsExtendedBusy())
    {
        DEBUG_LOG("CL is Busy!");
        scan_result.status = LE_SCAN_MANAGER_RESULT_BUSY;
        respond = TRUE;
    }
    else
    {
        switch (LeScanManagerGetExtendedState())
        {
            case LE_SCAN_MANAGER_EXTENDED_STATE_SCANNING:
            {
                extended_data->requester = req_task;
                LeScanManagerSetExtendedCurrentCommand(LE_SCAN_MANAGER_CMD_EXTENDED_DISABLE);
                respond = leScanManager_StopExtendedScanByTask(extended_scan_task);
                scan_result.status = LE_SCAN_MANAGER_RESULT_SUCCESS;
            }
            break;

            default:
            {
                leScanManager_SetExtendedState(LE_SCAN_MANAGER_EXTENDED_STATE_DISABLED);
                scan_result.status = LE_SCAN_MANAGER_RESULT_SUCCESS;
                respond = TRUE;
            }
            break;
        }
    }

    if(respond)
    {
        leScanManager_SendExtendedDisableCfm(req_task, scan_result);
    }

    return TRUE;
}

bool leScanManager_ExtendedScanEnable(Task req_task)
{
    le_scan_manager_extended_data_t *extended_data = LeScanManagerGetExtendedTaskData();
    le_scan_manager_status_t scan_result = {LE_SCAN_MANAGER_RESULT_FAILURE};
    bool respond = FALSE;
    Task extended_scan_task = leScanManager_GetFirstExtendedScanTask();

    if (extended_scan_task == NULL)
    {
        if (LeScanManagerGetExtendedState() == LE_SCAN_MANAGER_EXTENDED_STATE_DISABLED)
        {
            leScanManager_SetExtendedState(LE_SCAN_MANAGER_EXTENDED_STATE_ENABLED);
        }
        return FALSE;
    }

    if(leScanManager_IsExtendedBusy())
    {
        DEBUG_LOG("CL is Busy!");
        scan_result.status = LE_SCAN_MANAGER_RESULT_BUSY;
        respond = TRUE;
    }
    else
    {
        switch(LeScanManagerGetExtendedState())
        {
            case LE_SCAN_MANAGER_EXTENDED_STATE_DISABLED:
            {
                extended_data->requester = req_task;
                LeScanManagerSetExtendedCurrentCommand(LE_SCAN_MANAGER_CMD_EXTENDED_ENABLE);
                respond = leScanManager_StartExtendedScanByTask(extended_scan_task);
                scan_result.status = LE_SCAN_MANAGER_RESULT_SUCCESS;
                if (respond)
                {
                    leScanManager_SetExtendedState(LE_SCAN_MANAGER_EXTENDED_STATE_ENABLED);
                }
            }
            break;

            case LE_SCAN_MANAGER_EXTENDED_STATE_ENABLED:
            case LE_SCAN_MANAGER_EXTENDED_STATE_SCANNING:
            {
                DEBUG_LOG("leScanManager_ExtendedScanEnable no action in (%d) state", LeScanManagerGetExtendedState());
                return FALSE;
            }

            case LE_SCAN_MANAGER_EXTENDED_STATE_PAUSED:
            break;

            default:
            {
                Panic();
            }
            break;
        }
    }

    if (respond)
    {
        leScanManager_SendExtendedEnableCfm(req_task, scan_result);
    }

    return TRUE;
}

#endif


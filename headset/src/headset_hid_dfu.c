/*!
\copyright  Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Headset application HID DFU related routines and message handling.

*/

#ifdef INCLUDE_HID_DFU

#include "headset_log.h"

#include "headset_hid_dfu.h"

/* local includes */
#include "headset_config.h"
#include "headset_sm.h"
#include "ui.h"

/* framework includes */
#include <connection_manager.h>
#include <dfu.h>
#include <power_manager.h>
#include <system_state.h>
#include <unexpected_message.h>
#include <usb_hid_datalink.h>
/* system includes */
#include <hid_upgrade.h>
#include <message.h>
#include <panic.h>
#include <vmtypes.h>


/*! How long to wait for clean up actions (e.g. sink disconnect) to complete,
    before rebooting into a newly downloaded DFU image anyway, as a fail-safe. */
#define DFU_REBOOT_TIMEOUT_MS (D_SEC(10))

/*! \brief Application DFU module internal message IDs */
enum headset_dfu_internal_message_ids
{
    DFU_INTERNAL_REBOOT_TIMEOUT,            /*!< Cleanup actions took too long */
};

/*! \brief headset DFU task data structure */
typedef struct
{
    TaskData task;                      /*!< Application DFU task */
    bool reboot_pending;                /*!< Waiting to reboot flag */
    bool performance_profile_requested; /*!< Request sent to power manager */
} headset_dfu_task_data_t;

/*! \brief headset DFU task data instance */
headset_dfu_task_data_t headset_dfu;

#define headsetDFUTaskData() (&headset_dfu)
#define headsetDFU_GetTask() &(headsetDFUTaskData()->task)


static void headsetDFU_HandleHidDatalinkReport(uint8 report_id,
                                                 const uint8 *data, uint16 size)
{
    if (size < HID_REPORTID_COMMAND_SIZE)
    {
        /* Smallest incoming report supported by HID Upgrade protocol */
        DEBUG_LOG_DEBUG("headsetDFU_HandleHidDatalinkReport, unrecognised"
                        " report 0x%02X, length too short, %u bytes",
                        report_id, size);
        return;
    }
    DEBUG_LOG_V_VERBOSE("headsetDFU_HandleHidDatalinkReport, report_id 0x%02X"
                        ", %u bytes [%02x %02x %02x %02x %02x %02x %02x %02x"
                        " ... %02x]", report_id, size-1, data[1], data[2],
                        data[3], data[4], data[5], data[6], data[7], data[8],
                        data[size-1]);

    if (data && report_id)
    {
        /* data[0] = report_id, data[1] = size, so only need to forward data[2]
           onwards to the HID Upgrade libray. */
        uint16 queue_len = HidUpgradeHandleReport(report_id, size-2, &data[2]);
        DEBUG_LOG_VERBOSE("headsetDFU_HandleHidDatalinkReport, queue length %u",
                          queue_len);

        if (queue_len == 0)
        {
            /* Report not queued */
            DEBUG_LOG_DEBUG("headsetDFU_HandleHidDatalinkReport, unrecognised"
                            " report 0x%02X, report not queued", report_id);
        }
    }
}

static void headsetDFU_SendHidUpgradeReport(uint8 report_id,
                                              const uint8 *data, uint16 size)
{
    if (size < HID_REPORTID_UPGRADE_RESPONSE_SIZE)
    {
        /* Smallest outgoing report supported by HID Upgrade protocol */
        DEBUG_LOG_ERROR("headsetDFU_SendHidUpgradeReport, tried to send HID"
                        " upgrade report 0x%02X, too short, %u bytes",
                        report_id, size);
        return;
    }
    DEBUG_LOG_V_VERBOSE("headsetDFU_SendHidUpgradeReport, report_id 0x%02X"
                        ", %u bytes [%02x %02x %02x %02x %02x %02x %02x %02x"
                        " ... %02x]", report_id, size, data[0], data[1],
                        data[2], data[3], data[4], data[5], data[6], data[7],
                        data[size-1]);

    usb_result_t result = UsbHid_Datalink_SendReport(report_id, data, size);

    if (result != USB_RESULT_OK)
    {
        DEBUG_LOG_ERROR("headsetDFU_SendHidUpgradeReport, failed to send"
                        " report, enum:usb_result_t:0x%x", result);
    }
}

/*! \brief Handle pre-reboot clean up action timeout. */
static void headsetDFU_HandleInternalRebootTimeout(void)
{
    DEBUG_LOG_FN_ENTRY("headsetDFU_HandleInternalRebootTimeout");

    if (headsetDFUTaskData()->reboot_pending)
    {
        DEBUG_LOG_INFO("headsetDFU_HandleInternalRebootTimeout, disconnect timed out,"
                       " rebooting to complete DFU");
        headsetDFUTaskData()->reboot_pending = FALSE;
        Dfu_RebootConfirm();
    }
}

/*! \brief Perform pre-DFU file transfer actions. */
static void headsetDFU_HandleDfuStarted(void)
{
    DEBUG_LOG_FN_ENTRY("headsetDFU_HandleDfuStarted");

    /* Make sure we stay in high performance mode during DFU file transfer.
       This prevents stalls if sink disconnects whilst USB audio in progress. */
    if (!headsetDFUTaskData()->performance_profile_requested)
    {
        appPowerPerformanceProfileRequest();
        headsetDFUTaskData()->performance_profile_requested = TRUE;
    }

    DEBUG_LOG_INFO("headsetDFU_HandleDfuStarted, DFU in progress");
}

/*! \brief Perform post-DFU clean up actions. */
static void headsetDFU_HandleDfuCompleted(bool upgrade_successful)
{
    DEBUG_LOG_FN_ENTRY("headsetDFU_HandleDfuCompleted");

    /* Release our lock on high performance mode, if applicable. */
    if (headsetDFUTaskData()->performance_profile_requested)
    {
        appPowerPerformanceProfileRelinquish();
        headsetDFUTaskData()->performance_profile_requested = FALSE;
    }

    Dfu_SetRebootReason(REBOOT_REASON_NONE);

    DEBUG_LOG_INFO("headsetDFU_HandleDfuCompleted, DFU complete, success %u", upgrade_successful);
}

/*! \brief Perform pre-reboot clean up actions. */
static void headsetDFU_HandleDfuReadyToReboot(void)
{
    DEBUG_LOG_INFO("headsetDFU_HandleDfuReadyToReboot, DFU data transfer complete, max HID report queue"
                   " length reached %u", HidUpgradeGetStatsMaxReportQueueLen());


    /* Attempt to disconnect any links gracefully */
    if(headsetSmDisconnectLink())
    {
        headsetDFUTaskData()->reboot_pending = TRUE;

        /* Set a timeout as a fail-safe */
        MessageSendLater(headsetDFU_GetTask(), DFU_INTERNAL_REBOOT_TIMEOUT,
                         NULL, DFU_REBOOT_TIMEOUT_MS);
        DEBUG_LOG_INFO("headsetDFU_HandleDfuReadyToReboot, DFU reboot requested, wait for link disconnect");
    }
    else
    {

        /* Not connected, can reboot immediately */
        DEBUG_LOG_INFO("headsetDFU_HandleDfuReadyToReboot, DFU reboot requested, no connections, reboot now");
        Dfu_RebootConfirm();
    }

}

static void headsetDFU_HandleConManagerConnectionInd(CON_MANAGER_CONNECTION_IND_T *msg)
{
    DEBUG_LOG_FN_ENTRY("headsetDFU_HandleConManagerConnectionInd");

    if (msg->ble || !appDeviceTypeIsSink(&msg->bd_addr))
    {
        /* Only interested in sink device disconnections */
        return;
    }

    if (headsetDFUTaskData()->reboot_pending && !msg->connected)
    {
        /* Sink device disconnected, ok to reboot now. */
        DEBUG_LOG_INFO("headsetDFU_HandleDfuReadyToReboot, reboot now to complete DFU");
        MessageCancelAll(headsetDFU_GetTask(), DFU_INTERNAL_REBOOT_TIMEOUT);
        headsetDFUTaskData()->reboot_pending = FALSE;
        Dfu_RebootConfirm();
    }
}

/*! \brief Handles system state changes the DFU module is interested in */
static void headsetDFU_HandleSystemStateChange(SYSTEM_STATE_STATE_CHANGE_T *msg)
{
    DEBUG_LOG_FN_ENTRY("headsetDFU_HandleSystemStateChange");

    switch (msg->new_state)
    {
        case system_state_starting_up:
            Dfu_RequireRebootPermission(TRUE);
            Dfu_AllowUpgrades(TRUE);
            break;

        case system_state_active:
            if (Dfu_IsUpgradeInProgress())
            {
                headsetDFU_HandleDfuStarted();
            }
            break;

        default:
            break;
    }
}

/*! \brief Application DFU module message handler.
    \param task The application DFU task.
    \param id The message ID to handle.
    \param message The message content (if any).
*/
static void headsetDFU_HandleMessage(Task task, MessageId id, Message message)
{
    DEBUG_LOG_VERBOSE("headsetDFU_HandleMessage");

    UNUSED(task);

    switch (id)
    {
        case DFU_INTERNAL_REBOOT_TIMEOUT:
            headsetDFU_HandleInternalRebootTimeout();
            break;

        case DFU_STARTED:
            headsetDFU_HandleDfuStarted();
            break;

        case DFU_COMPLETED:
            headsetDFU_HandleDfuCompleted(TRUE);
            break;

        case DFU_ABORTED:
            headsetDFU_HandleDfuCompleted(FALSE);
            break;

        case DFU_READY_TO_REBOOT:
            headsetDFU_HandleDfuReadyToReboot();
            break;

        case CON_MANAGER_CONNECTION_IND:
            headsetDFU_HandleConManagerConnectionInd((CON_MANAGER_CONNECTION_IND_T *)message);
            break;

        case SYSTEM_STATE_STATE_CHANGE:
            headsetDFU_HandleSystemStateChange((SYSTEM_STATE_STATE_CHANGE_T *)message);
            break;

        default:
            UnexpectedMessage_HandleMessage(id);
            break;
    }
}

bool headsetHIDDFU_Init(Task init_task)
{
    headsetDFUTaskData()->task.handler = headsetDFU_HandleMessage;
    headsetDFUTaskData()->reboot_pending = FALSE;
    headsetDFUTaskData()->performance_profile_requested = FALSE;

    /* Register with connection manager for notification of (dis)connections */
    ConManagerRegisterConnectionsClient(headsetDFU_GetTask());

    /* Register for system state change indications */
    SystemState_RegisterForStateChanges(headsetDFU_GetTask());

    /* Register as DFU client */
    Dfu_ClientRegister(headsetDFU_GetTask());

    /* Register to receive report data from host via USB HID Datalink class */
    UsbHid_Datalink_RegisterHandler(headsetDFU_HandleHidDatalinkReport);

    /* Register report data to be sent to host via USB HID Datalink class */
    HidUpgradeRegisterInputReportCb(headsetDFU_SendHidUpgradeReport);

    UNUSED(init_task);
    return TRUE;
}

bool headsetDFU_UpgradeInProgress(void)
{
    return Dfu_IsUpgradeInProgress();
}

#endif /* INCLUDE_HID_DFU */

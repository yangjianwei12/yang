/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Charger case application DFU related routines and message handling.

*/

#ifdef INCLUDE_DFU

#include "charger_case_dfu.h"

/* local includes */
#include "charger_case_config.h"
#include "charger_case_sm.h"
#include "charger_case_sm_private.h"

/* framework includes */
#include <adk_log.h>
#include <connection_manager.h>
#include <dfu.h>
#include <gatt_server_gatt.h>
#include <handset_service.h>
#include <pairing.h>
#include <system_state.h>
#include <ui.h>
#include <unexpected_message.h>
/* system includes */
#include <message.h>
#include <panic.h>
#include <vmtypes.h>


/*! How long to wait for clean up actions (e.g. sink disconnect) to complete,
    before rebooting into a newly downloaded DFU image anyway, as a fail-safe. */
#define DFU_REBOOT_TIMEOUT_MS (D_SEC(10))

/*! \brief Application DFU module internal message IDs */
enum charger_case_dfu_internal_message_ids
{
    DFU_INTERNAL_HANDSET_PAIRING_ENABLE,    /*!< Allow handset to pair/connect */
    DFU_INTERNAL_HANDSET_PAIRING_DISABLE,   /*!< Disallow handset connections */
    DFU_INTERNAL_REBOOT_TIMEOUT,            /*!< Cleanup actions took too long */
};

/*! \brief UI Inputs the DFU module is interested in. */
const message_group_t charger_case_dfu_ui_inputs[] =
{
    UI_INPUTS_HANDSET_MESSAGE_GROUP,
};

/*! \brief Charger case DFU task data structure */
typedef struct
{
    TaskData task;                          /*!< Application DFU task */
    bool pairing_active;                    /*!< Pairing in progress flag */
    bool reboot_pending;                    /*!< Waiting to reboot flag */

} charger_case_dfu_task_data_t;

/*! \brief Charger case DFU task data instance */
charger_case_dfu_task_data_t charger_case_dfu;

#define chargerCaseDfuTaskData() (&charger_case_dfu)
#define chargerCaseDfu_GetTask() &(chargerCaseDfuTaskData()->task)


/*! \brief Handle request to enable DFU handset pairing. */
static void chargerCaseDfu_HandleInternalHandsetPairingEnable(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseDfu_HandleInternalHandsetPairingEnable");

    /* Only enable handset pairing if sink pairing is not in progress */
    if (ChargerCaseSm_GetState() == CHARGER_CASE_STATE_PAIRING)
    {
        DEBUG_LOG_INFO("ChargerCase: Ignoring DFU handset pair request"
                       " - sink pairing in progress");
        return;
    }

    /* Allow pairing */
    if (!chargerCaseDfuTaskData()->pairing_active)
    {
        Pairing_Pair(chargerCaseDfu_GetTask(), TRUE);
        chargerCaseDfuTaskData()->pairing_active = TRUE;
    }

    /* Go connectable to allow a handset to actually connect */
    HandsetService_ConnectableRequest(chargerCaseDfu_GetTask());

    /* Set a timeout to disable pairing/connectable again later */
    MessageCancelAll(chargerCaseDfu_GetTask(), DFU_INTERNAL_HANDSET_PAIRING_DISABLE);
    MessageSendLater(chargerCaseDfu_GetTask(), DFU_INTERNAL_HANDSET_PAIRING_DISABLE,
                     NULL, appConfigDfuHandsetPairingModeTimeoutMs());

    DEBUG_LOG_INFO("ChargerCase: DFU handset pairing started");
}

/*! \brief Handle request to disable DFU handset pairing. */
static void chargerCaseDfu_HandleInternalHandsetPairingDisable(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseDfu_HandleInternalHandsetPairingDisable");

    /* Stop pairing if still active */
    if (chargerCaseDfuTaskData()->pairing_active)
    {
        Pairing_PairStop(chargerCaseDfu_GetTask());
        chargerCaseDfuTaskData()->pairing_active = FALSE;
    }

    /* Cancel connectable and stop LE advertising */
    HandsetService_CancelConnectableRequest(chargerCaseDfu_GetTask());
    HandsetService_SetBleConnectable(FALSE);

    DEBUG_LOG_INFO("ChargerCase: DFU handset pairing stopped");
}

/*! \brief Handle pre-reboot clean up action timeout. */
static void chargerCaseDfu_HandleInternalRebootTimeout(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseDfu_HandleInternalRebootTimeout");

    if (chargerCaseDfuTaskData()->reboot_pending)
    {
        chargerCaseDfuTaskData()->reboot_pending = FALSE;
        Dfu_RebootConfirm();
    }
}

/*! \brief Handle confirmation of pairing complete. */
static void chargerCaseDfu_HandlePairingPairCfm(PAIRING_PAIR_CFM_T *msg)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseDfu_HandlePairingPairCfm");
    UNUSED(msg);
    chargerCaseDfuTaskData()->pairing_active = FALSE;
}

/*! \brief Perform post-DFU clean up actions. */
static void chargerCaseDfu_HandleDfuCompleted(bool upgrade_successful)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseDfu_HandleDfuCompleted");

    if (upgrade_successful)
    {
        /* Gatt database may have changed. */
        GattServerGatt_SetGattDbChanged();
    }
    
    Dfu_SetRebootReason(REBOOT_REASON_NONE);

    /* No longer need to stay connectable */
    HandsetService_CancelConnectableRequest(chargerCaseDfu_GetTask());
    HandsetService_SetBleConnectable(FALSE);

    DEBUG_LOG_INFO("ChargerCase: DFU complete, success:%u", upgrade_successful);
}

/*! \brief Perform pre-reboot clean up actions. */
static void chargerCaseDfu_HandleDfuReadyToReboot(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseDfu_HandleDfuReadyToReboot");

    switch (ChargerCaseSm_GetState())
    {
        case CHARGER_CASE_STATE_CONNECTING:
        case CHARGER_CASE_STATE_CONNECTED:
        case CHARGER_CASE_STATE_AUDIO_STARTING:
        case CHARGER_CASE_STATE_AUDIO_STREAMING:
        case CHARGER_CASE_STATE_AUDIO_STOPPING:
            /* Disconnect from sink device gracefully */
            MessageSend(ChargerCaseSmGetTask(), SM_INTERNAL_DISCONNECT_SINK_DEVICE, NULL);
            /* Fall through */
        case CHARGER_CASE_STATE_DISCONNECTING:
            /* Wait for disconnect confirmation before calling Dfu_RebootConfirm(). */
            chargerCaseDfuTaskData()->reboot_pending = TRUE;
            /* Set a timeout as a fail-safe */
            MessageSendLater(chargerCaseDfu_GetTask(), DFU_INTERNAL_REBOOT_TIMEOUT,
                             NULL, DFU_REBOOT_TIMEOUT_MS);
            break;

        default:
            /* Not connected, can reboot immediately */
            Dfu_RebootConfirm();
            break;
    }
}

static void chargerCaseDfu_HandleConManagerConnectionInd(CON_MANAGER_CONNECTION_IND_T *msg)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseDfu_HandleConManagerConnectionInd");

    if (msg->ble || !appDeviceTypeIsSink(&msg->bd_addr))
    {
        /* Only interested in sink device disconnections */
        return;
    }

    if (chargerCaseDfuTaskData()->reboot_pending && !msg->connected)
    {
        /* Sink device disconnected, ok to reboot now. */
        MessageCancelAll(chargerCaseDfu_GetTask(), DFU_INTERNAL_REBOOT_TIMEOUT);
        chargerCaseDfuTaskData()->reboot_pending = FALSE;
        Dfu_RebootConfirm();
    }
}

/*! \brief Handles DFU module specific ui inputs

    Invokes routines based on ui input received from ui module.

    \param[in] id - ui input
    \returns void
 */
static void chargerCaseDfu_HandleUiInput(MessageId ui_input)
{
    switch (ui_input)
    {
        case ui_input_sm_pair_handset:
            ChargerCaseDfu_HandsetPairingStart();
            break;

        default:
            break;
    }
}

/*! \brief Handles system state changes the DFU module is interested in */
static void chargerCaseDfu_HandleSystemStateChange(SYSTEM_STATE_STATE_CHANGE_T *msg)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseDfu_HandleSystemStateChange");

    switch (msg->new_state)
    {
        case system_state_starting_up:
            Dfu_RequireRebootPermission(TRUE);
            Dfu_AllowUpgrades(TRUE);
            break;

        case system_state_active:
            if (Dfu_IsUpgradeInProgress())
            {
                /* Allow previous handset to reconnect to complete upgrade */
                DEBUG_LOG_INFO("ChargerCase: DFU in progress, enable connectable");
                HandsetService_ConnectableRequest(chargerCaseDfu_GetTask());
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
static void chargerCaseDfu_HandleMessage(Task task, MessageId id, Message message)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseDfu_HandleMessage");

    UNUSED(task);

    if (isMessageUiInput(id))
    {
       chargerCaseDfu_HandleUiInput(id);
       return;
    }
    switch (id)
    {
        case PAIRING_PAIR_CFM:
            chargerCaseDfu_HandlePairingPairCfm((PAIRING_PAIR_CFM_T *)message);
            break;

        case DFU_COMPLETED:
            chargerCaseDfu_HandleDfuCompleted(TRUE);
            break;

        case DFU_ABORTED:
            chargerCaseDfu_HandleDfuCompleted(FALSE);
            break;

        case DFU_READY_TO_REBOOT:
            chargerCaseDfu_HandleDfuReadyToReboot();
            break;

        case CON_MANAGER_CONNECTION_IND:
            chargerCaseDfu_HandleConManagerConnectionInd((CON_MANAGER_CONNECTION_IND_T *)message);
            break;

        case SYSTEM_STATE_STATE_CHANGE:
            chargerCaseDfu_HandleSystemStateChange((SYSTEM_STATE_STATE_CHANGE_T *)message);
            break;

        case DFU_INTERNAL_HANDSET_PAIRING_ENABLE:
            chargerCaseDfu_HandleInternalHandsetPairingEnable();
            break;

        case DFU_INTERNAL_HANDSET_PAIRING_DISABLE:
            chargerCaseDfu_HandleInternalHandsetPairingDisable();
            break;

        case DFU_INTERNAL_REBOOT_TIMEOUT:
            chargerCaseDfu_HandleInternalRebootTimeout();
            break;

        default:
            UnexpectedMessage_HandleMessage(id);
            break;
    }
}

bool ChargerCaseDfu_Init(Task init_task)
{
    chargerCaseDfuTaskData()->task.handler = chargerCaseDfu_HandleMessage;
    chargerCaseDfuTaskData()->pairing_active = FALSE;
    chargerCaseDfuTaskData()->reboot_pending = FALSE;

    /* Register with connection manager for notification of (dis)connections */
    ConManagerRegisterConnectionsClient(chargerCaseDfu_GetTask());

    /* Register for system state change indications */
    SystemState_RegisterForStateChanges(chargerCaseDfu_GetTask());

    /* Register DFU module as a UI input consumer */
    Ui_RegisterUiInputConsumer(chargerCaseDfu_GetTask(),
                               charger_case_dfu_ui_inputs,
                               ARRAY_DIM(charger_case_dfu_ui_inputs));

    /* Register as DFU client */
    Dfu_ClientRegister(chargerCaseDfu_GetTask());

    /* Set the QoS as low latency for better DFU performance over LE Transport.
       This will come at the cost of high power consumption. */
    ConManagerRequestDefaultQos(cm_transport_ble, cm_qos_low_latency);

    UNUSED(init_task);
    return TRUE;
}

void ChargerCaseDfu_HandsetPairingStart(void)
{
    DEBUG_LOG_FN_ENTRY("ChargerCaseDfu_HandsetPairingStart");
    MessageSend(chargerCaseDfu_GetTask(), DFU_INTERNAL_HANDSET_PAIRING_ENABLE, NULL);
}

void ChargerCaseDfu_HandsetPairingCancel(void)
{
    DEBUG_LOG_FN_ENTRY("ChargerCaseDfu_HandsetPairingCancel");
    MessageCancelAll(ChargerCaseSmGetTask(), DFU_INTERNAL_HANDSET_PAIRING_DISABLE);
    MessageSend(ChargerCaseSmGetTask(), DFU_INTERNAL_HANDSET_PAIRING_DISABLE, NULL);
}

bool ChargerCaseDfu_UpgradeInProgress(void)
{
    return (Dfu_IsUpgradeInProgress() || chargerCaseDfuTaskData()->pairing_active);
}

#endif /* INCLUDE_DFU */

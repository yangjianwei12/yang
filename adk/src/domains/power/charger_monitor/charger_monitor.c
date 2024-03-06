/*!
    \copyright  Copyright (c) 2008 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       charger_monitor.c
    \ingroup    charger_monitor
    \brief      Charger monitoring
*/

#include "charger_monitor.h"
#include "charger_monitor_config.h"
#include "battery_monitor_config.h"
#include "battery_region.h"
#include "usb_device.h"

#include "panic.h"

#include "temperature.h"

#include <logging.h>
#include <battery_monitor.h>

#include "charger_detect.h"
#include "charger_data.h"

#ifdef INCLUDE_CHARGER_DETECT
#include "led_manager_protected.h"
#endif

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(chargerMessages)

#ifndef HOSTED_TEST_ENVIRONMENT

/*! There is checking that the messages assigned by this module do
not overrun into the next module's message ID allocation */
ASSERT_MESSAGE_GROUP_NOT_OVERFLOWED(CHARGER, CHARGER_MESSAGE_END)

#endif

/*! \brief Charger component data structure. */
chargerTaskData app_charger_data;

#ifdef INCLUDE_CHARGER

/*! List of client tasks */
static task_list_t *charger_client_tasks;

/*! Broadcast message to the clients */
static void Charger_NotifyClients(MessageId id);


/*! Enable the charger */
static void Charger_Enable(void);
/*! Disable the charger */
static void Charger_Disable(void);

LOGGING_PRESERVE_MESSAGE_ENUM(charger_monitor_internal_messages)
LOGGING_PRESERVE_MESSAGE_TYPE(chargerMessages)

ASSERT_INTERNAL_MESSAGES_NOT_OVERFLOWED(CHARGER_INTERNAL_MESSAGE_END)

#ifdef ENABLE_CHARGER_HYSTERESIS_SW
/*!
 * \brief Check if the charger has completed a charge cycle and entered
 * STANDBY since power was attached to it.
 *
 * \return TRUE if the charger has completed, FALSE otherwise.
 */
static bool charger_HasCompleted(void)
{
    chargerTaskData *theCharger = appGetCharger();

    /* The charger must be exclusively disabled by completion. */
    return theCharger->disable_reason == CHARGER_DISABLE_REASON_CHARGED;
}
#endif

/*! \brief provides charger module current context to ui module

    \return     ui_provider_ctxt_t - current context of charger module.
*/
static unsigned appChargerCurrentContext(void)
{
    charger_provider_context_t context = BAD_CONTEXT;
    if (Charger_IsConnected())
    {
        if (Charger_IsEnabled())
        {
            chargerTaskData *theCharger = appGetCharger();

            switch (theCharger->status)
            {
            case TRICKLE_CHARGE:
            case PRE_CHARGE:
                context = context_charger_low;
                break;
            case FAST_CHARGE:
                context = context_charger_fast;
                break;
            case STANDBY:
                context = context_charger_completed;
                break;
            case VBAT_OVERVOLT_ERROR:
            case HEADROOM_ERROR:
            case DISABLED_ERROR:
                context = context_charger_error;
                break;
            default:
                break;
            }
        }
        else
        {
#ifdef ENABLE_CHARGER_HYSTERESIS_SW
            if (charger_HasCompleted())
            {
                context = context_charger_completed;
            }
            else
#endif
            {
                context = context_charger_disabled;
            }
        }
    }
    else
    {
        context = context_charger_detached;
    }
    return (unsigned)context;
}

/*! Handle charger error
 *
 * On error we first disable the charger (which is usually enough to
 * clear an error) and then send a message to self to re-enable it later. */
static void Charger_Error(void)
{
    chargerTaskData *theCharger = appGetCharger();

    Charger_DisableReasonAdd(CHARGER_DISABLE_REASON_ERROR);

    MessageCancelAll(&theCharger->task, CHARGER_INTERNAL_RE_ENABLE_TIMEOUT);
    MessageSendLater(&theCharger->task, CHARGER_INTERNAL_RE_ENABLE_TIMEOUT,
                     NULL, appConfigChargerReEnableTimeoutMs());
}

static MessageId charger_StatusMessageId(charger_status status)
{
    switch (status)
    {
    case TRICKLE_CHARGE:
    case PRE_CHARGE:
        return CHARGER_MESSAGE_CHARGING_LOW;
    case FAST_CHARGE:
        return CHARGER_MESSAGE_CHARGING_OK;
    case STANDBY:
        return CHARGER_MESSAGE_COMPLETED;
    case VBAT_OVERVOLT_ERROR:
    case HEADROOM_ERROR:
        return CHARGER_MESSAGE_ERROR;
    default:
        break;
    }
    return CHARGER_MESSAGE_DISABLED;
}

/*! Handle ChargerChanged message */
static void Charger_StatusChanged(MessageChargerStatus *message)
{
    chargerTaskData *theCharger = appGetCharger();

    charger_status old_status = theCharger->status;

    theCharger->status = message->chg_status;

    uint32 charge_timeout_ms = 0;

    MessageCancelAll(&theCharger->task, CHARGER_INTERNAL_DISABLE_AFTER_CHARGED);

    switch (theCharger->status)
    {
        case TRICKLE_CHARGE:
            break;
        case FAST_CHARGE:
            charge_timeout_ms = appConfigChargerFastChargeTimeoutMs();
            break;
        case DISABLED_ERROR:
#ifdef ENABLE_CHARGER_HYSTERESIS_SW
            /* If charging has completed, the HW is disabled however it is still reported
             * as if was enabled to keep state consistent. */
            if (charger_HasCompleted())
            {
                theCharger->status = STANDBY;
            }
#endif
            break;
        case STANDBY:
#ifdef ENABLE_CHARGER_HYSTERESIS_SW
            MessageSendLater(&theCharger->task, CHARGER_INTERNAL_DISABLE_AFTER_CHARGED,
                             NULL, appConfigChargerDisableAfterStandbyTimeoutMs());
#endif
            break;
        case PRE_CHARGE:
            charge_timeout_ms = appConfigChargerPreChargeTimeoutMs();
            break;
        case NO_POWER:
            break;
        case HEADROOM_ERROR:
            Charger_Error();
            break;
        case VBAT_OVERVOLT_ERROR:
            Charger_Error();
            break;
        case CONFIG_FAIL_VALUES_OUT_OF_RANGE:
            if (theCharger->fast_current > appConfigChargerInternalMaxCurrent())
            {
                /* Current is too high: ChargerConfigure() or ChargerEnable()
                 * requested current that can not be supported. */
                DEBUG_LOG_INFO("Charger: current too high %d", theCharger->fast_current);

                /* reduce supported current and re-try */
                theCharger->max_supported_current = theCharger->fast_current - 50;
                Charger_UpdateCurrent();

                /* If the error triggered by ChargerEnable() the charger stays disabled.
                 * Toggle CHARGER_DISABLE_REASON_INTERNAL to re-enable it
                 * (this time with reduced current) */
                Charger_DisableReasonAdd(CHARGER_DISABLE_REASON_INTERNAL);
                Charger_DisableReasonClear(CHARGER_DISABLE_REASON_INTERNAL);
            }
            else
            {
                Charger_Error();
            }
            break;

        default:
            break;
    }

    if (old_status != theCharger->status)
    {
        DEBUG_LOG_INFO("Charger: enum:charger_status:%d", theCharger->status);
        Charger_NotifyClients(charger_StatusMessageId(theCharger->status));
        Ui_InformContextChange(ui_provider_charger, appChargerCurrentContext());
    }    

    MessageCancelAll(&theCharger->task, CHARGER_INTERNAL_CHARGE_TIMEOUT);
    if (charge_timeout_ms)
    {
        MessageSendLater(&theCharger->task, CHARGER_INTERNAL_CHARGE_TIMEOUT,
                         NULL, charge_timeout_ms);
    }
}

static void charger_SetFastCurrent(uint16 fast_current)
{
    chargerTaskData *theCharger = appGetCharger();

    if (fast_current == 0)
    {
        /* setting "0" current would trigger CONFIG_FAIL_CURRENTS_ZERO,
         * so make sure charger is disabled first */
        Charger_DisableReasonAdd(CHARGER_DISABLE_REASON_ZERO_CURRENT);
    }

    if (fast_current > appConfigChargerInternalMaxCurrent())
    {
#if FAST_CHARGE_EXTERNAL_RESISTOR
        if (!theCharger->ext_mode_enabled)
        {
            /* turn off the charger to enable external mode */
            Charger_DisableReasonAdd(CHARGER_DISABLE_REASON_INTERNAL);

            DEBUG_LOG_INFO("Charger: enable external mode");
            PanicFalse(ChargerConfigure(CHARGER_USE_EXTERNAL_RESISTOR_FOR_FAST_CHARGE, 1));
            theCharger->ext_mode_enabled = 1;
        }
#else
        DEBUG_LOG_WARN("Charger: %dmA current too high for internal mode",
                       fast_current);
        /* External charging mode not supported */
        fast_current = appConfigChargerInternalMaxCurrent();
#endif
    }
#if FAST_CHARGE_EXTERNAL_RESISTOR
    /* fast_current <= appConfigChargerInternalMaxCurrent() */
    else if (theCharger->ext_mode_enabled)
    {
        /* turn off charger to disable external mode */
        Charger_DisableReasonAdd(CHARGER_DISABLE_REASON_INTERNAL);

        DEBUG_LOG_INFO("Charger: disable external mode");
        PanicFalse(ChargerConfigure(CHARGER_USE_EXTERNAL_RESISTOR_FOR_FAST_CHARGE, 0));
        theCharger->ext_mode_enabled = 0;
    }
#endif

    DEBUG_LOG_INFO("Charger: set fast current %dmA", fast_current);

    PanicFalse(ChargerConfigure(CHARGER_FAST_CURRENT, fast_current));
    theCharger->fast_current = fast_current;

#if FAST_CHARGE_EXTERNAL_RESISTOR
    Charger_DisableReasonClear(CHARGER_DISABLE_REASON_INTERNAL);
#endif

    if (fast_current != 0)
    {
        Charger_DisableReasonClear(CHARGER_DISABLE_REASON_ZERO_CURRENT);
    }
}

void Charger_UpdateCurrent(void)
{
    chargerTaskData *theCharger = appGetCharger();
    unsigned fast_current = theCharger->max_supported_current;

#ifdef INCLUDE_CHARGER_DETECT
    fast_current = MIN(ChargerDetect_Current(), fast_current);
#endif
    fast_current = MIN(BatteryRegion_GetCurrent(), fast_current);

    if (theCharger->fast_current != fast_current)
    {
        charger_SetFastCurrent(fast_current);
#ifdef INCLUDE_CHARGER_DETECT
        ChargerDetect_NotifyCurrentChanged();
#endif
    }
}

unsigned Charger_GetFastCurrent(void)
{
    chargerTaskData *theCharger = appGetCharger();
    return theCharger->fast_current;
}

void Charger_HandleChange(void)
{
    chargerTaskData *theCharger = appGetCharger();

    if (!Charger_IsConnected())
    {
        /* When charger is detached, power source switches to VBAT
         * automatically, so nothing else to do here except to record
         * the current state. */
        theCharger->power_source_vbat = 1;
        return;
    }

    /* By default application attempts to run from VCHG when it is present
     * (unless USB suspend rules apply). */
    unsigned power_source_vbat = 0;

#ifdef INCLUDE_CHARGER_DETECT
    bool usb_is_suspend = ChargerDetect_UsbIsSuspend();

    if (usb_is_suspend)
    {
        /* USB suspend, force PSU to VBAT, but only if the battery is good.
         * This is to make sure we don't switch to VBAT when the battery is
         * not there (e.g. USB Dongle). */
        power_source_vbat = BatteryMonitor_IsGood() ? 1 : 0;
    }

    unsigned usb_leds_forced_off = usb_is_suspend ? 1 : 0;

    if (theCharger->usb_leds_forced_off != usb_leds_forced_off)
    {
        theCharger->usb_leds_forced_off = usb_leds_forced_off;
        if (usb_leds_forced_off)
        {
            DEBUG_LOG_ALWAYS("Charger: USB suspend, force LEDs off");
            LedManager_ForceLeds(0);
        }
        else
        {
            DEBUG_LOG_ALWAYS("Charger: USB not suspend, enable LEDS");
            LedManager_ForceLedsStop();
        }
    }
#endif

    if (theCharger->power_source_vbat != power_source_vbat)
    {
        theCharger->power_source_vbat = power_source_vbat;
        if (power_source_vbat)
            DEBUG_LOG_ALWAYS("Charger: switch PSU to VBAT");
        else
            DEBUG_LOG_ALWAYS("Charger: switch PSU to VCHG");

        /* Connected to a charger, switch PSU VBAT->VCHG */
        if (!PsuConfigure(PSU_ALL, PSU_SMPS_INPUT_SEL_VBAT, power_source_vbat))
        {
            /* Switch failed, this can only be because VCHG is not OK (< 4v4).
             * Possibly a glitch, we could re-try but for now just ignore. */
            DEBUG_LOG_WARN("Charger: PSU switch failed, leave in current state");
        }
    }
}

void Charger_UpdateConnected(bool charger_is_connected)
{
    chargerTaskData *theCharger = appGetCharger();
    unsigned new_is_connected = charger_is_connected ? 1 : 0;

    if (theCharger->is_connected == new_is_connected)
    {
        return;
    }

    theCharger->is_connected = new_is_connected;

    if (theCharger->is_connected)
    {
        DEBUG_LOG_INFO("Charger: CONNECTED");

        Charger_UpdateCurrent();

        Charger_DisableReasonClear(CHARGER_DISABLE_REASON_NOT_CONNECTED);
    }
    else
    {
        DEBUG_LOG_INFO("Charger: DETACHED");

        Charger_DisableReasonAdd(CHARGER_DISABLE_REASON_NOT_CONNECTED);
        /* Allow the battery to charge after timeout if the charger is disconnected */
        Charger_DisableReasonClear(CHARGER_DISABLE_REASON_TIMEOUT);


#ifdef ENABLE_CHARGER_HYSTERESIS_SW
        MessageCancelAll(&theCharger->task, CHARGER_INTERNAL_DISABLE_AFTER_CHARGED);
        Charger_DisableReasonClear(CHARGER_DISABLE_REASON_CHARGED);
#endif
    }

    Charger_HandleChange();

    if (theCharger->is_connected)
        DEBUG_LOG_INFO("CHARGER_MESSAGE_ATTACHED");
    else
        DEBUG_LOG_INFO("CHARGER_MESSAGE_DETACHED");

    Charger_NotifyClients(theCharger->is_connected ?
                             CHARGER_MESSAGE_ATTACHED:
                             CHARGER_MESSAGE_DETACHED);
}

#ifdef ENABLE_CHARGER_HYSTERESIS_SW
/**
 * \brief Charger actions to perform when the battery voltage changes.
 * \param battery_voltage The new battery voltage level.
 */
static void Charger_HandleBatteryLevelUpdate(uint16 battery_voltage)
{
    DEBUG_LOG("Charger: battery level update voltage: %d", battery_voltage);

    /* If the battery voltage has fallen below the termination voltage
     * hysteresis, we should re-enable the charger. */
    if ((int32)(appConfigChargerTerminationVoltage() - battery_voltage)
        >= appConfigChargerStandbyFastVoltageHysteresis())
    {
        Charger_DisableReasonClear(CHARGER_DISABLE_REASON_CHARGED);
    }
}
#endif

/**************************************************************************/
static void charger_HandleMessage(Task task, MessageId id, Message message)
{
    chargerTaskData *theCharger = appGetCharger();

    UNUSED(task);

    switch (id)
    {
        case CHARGER_INTERNAL_CHARGE_TIMEOUT:
            DEBUG_LOG_INFO("Charger: timeout, status %d", theCharger->status);

            Charger_DisableReasonAdd(CHARGER_DISABLE_REASON_TIMEOUT);
            break;

        case CHARGER_INTERNAL_RE_ENABLE_TIMEOUT:
            DEBUG_LOG_INFO("Charger: re-enable after an error, status %d", theCharger->status);

            Charger_DisableReasonClear(CHARGER_DISABLE_REASON_ERROR);
            break;

        case MESSAGE_CHARGER_DETECTED:
#ifdef INCLUDE_CHARGER_DETECT
            if (!theCharger->test_mode)
            {
                ChargerDetect_Detected((MessageChargerDetected *)message);
            }
#endif
            break;

        case MESSAGE_CHARGER_CHANGED:
            if (!theCharger->test_mode)
            {
#ifdef INCLUDE_CHARGER_DETECT
                ChargerDetect_Changed((MessageChargerChanged *)message);
#else
                Charger_UpdateConnected(((MessageChargerChanged *)message)->charger_connected);
#endif
            }
            break;

        case MESSAGE_CHARGER_STATUS:
            if (!theCharger->test_mode)
            {
                Charger_StatusChanged((MessageChargerStatus *)message);
            }
            break;

#ifdef INCLUDE_CHARGER_DETECT
        case USB_DEVICE_ENUMERATED:
        case USB_DEVICE_DECONFIGURED:
        case USB_DEVICE_SUSPEND:
        case USB_DEVICE_RESUME:
            ChargerDetect_UpdateUsbStatus(id);
            break;

        case CHARGER_INTERNAL_VCHG_MEASUREMENT:
            Charger_VchgMonitorPeriodic(theCharger);
            break;

        case MESSAGE_ADC_RESULT:
            Charger_VChgMonitorReading(theCharger, (MessageAdcResult*)message);
            break;

#endif /* INCLUDE_CHARGER_DETECT */

#ifdef ENABLE_CHARGER_HYSTERESIS_SW
        case MESSAGE_BATTERY_LEVEL_UPDATE_VOLTAGE:
            Charger_HandleBatteryLevelUpdate(((const MESSAGE_BATTERY_LEVEL_UPDATE_VOLTAGE_T*)message)->voltage_mv);
            break;


        case CHARGER_INTERNAL_DISABLE_AFTER_CHARGED:
            DEBUG_LOG_INFO("Charger: disable after charging complete");

            Charger_DisableReasonAdd(CHARGER_DISABLE_REASON_CHARGED);
            break;
#endif

        default:
            break;
    }
}

static void Charger_Enable(void)
{
    chargerTaskData *theCharger = appGetCharger();

    DEBUG_LOG_WARN("Charger: enable");

    theCharger->is_enabled = 1;
    PanicFalse(ChargerConfigure(CHARGER_ENABLE, 1));
}

static void Charger_Disable(void)
{
    chargerTaskData *theCharger = appGetCharger();

    DEBUG_LOG_WARN("Charger: disable");

    theCharger->is_enabled = 0;
    PanicFalse(ChargerConfigure(CHARGER_ENABLE, 0));

    MessageCancelAll(&theCharger->task, CHARGER_INTERNAL_CHARGE_TIMEOUT);
}

/* Set the configuration of the charger.
 */
static void charger_ConfigureCharger(void)
{
    /* Currents */
    PanicFalse(ChargerConfigure(CHARGER_TRICKLE_CURRENT, appConfigChargerTrickleCurrent()));
    PanicFalse(ChargerConfigure(CHARGER_PRE_CURRENT, appConfigChargerPreCurrent()));
    PanicFalse(ChargerConfigure(CHARGER_ITERM_CTRL, appConfigChargerTerminationCurrent()));

#if FAST_CHARGE_EXTERNAL_RESISTOR
    DEBUG_LOG_INFO("Charger: external resistor value %u", FAST_CHARGE_EXTERNAL_RESISTOR);
    PanicFalse(ChargerConfigure(CHARGER_EXTERNAL_RESISTOR, FAST_CHARGE_EXTERNAL_RESISTOR));
#endif

    /* Reset fast current, it is set just before enabling the charger */
    charger_SetFastCurrent(0);

    /* Voltages */
    PanicFalse(ChargerConfigure(CHARGER_PRE_FAST_THRESHOLD, appConfigChargerPreFastThresholdVoltage()));
    PanicFalse(ChargerConfigure(CHARGER_TERM_VOLTAGE, appConfigChargerTerminationVoltage()));

    /* Thresholds/timing */
#ifdef ENABLE_CHARGER_HYSTERESIS_SW
    /* Set the HW hysteresis to the maximum value to ensure the HW does not re-enable the charger before
     * the SW can disable it.*/
    PanicFalse(ChargerConfigure(CHARGER_STANDBY_FAST_HYSTERESIS, CHARGER_STANDBY_FAST_HYSTERESIS_MILLIVOLT_250));
#else
    PanicFalse(ChargerConfigure(CHARGER_STANDBY_FAST_HYSTERESIS, appConfigChargerStandbyFastVoltageHysteresis()));
#endif

    PanicFalse(ChargerConfigure(CHARGER_STATE_CHANGE_DEBOUNCE, appConfigChargerStateChangeDebounce()));
}

void Charger_DisableReasonAdd(ChargerDisableReason reason)
{
    chargerTaskData *theCharger = appGetCharger();

    if ((theCharger->disable_reason & reason) == 0)
    {
        switch (reason)
        {
            case CHARGER_DISABLE_REASON_TIMEOUT:
                DEBUG_LOG_INFO("Charger: set DISABLE reason TIMEOUT"); break;
            case CHARGER_DISABLE_REASON_REQUEST:
                DEBUG_LOG_INFO("Charger: set DISABLE reason REQUEST"); break;
            case CHARGER_DISABLE_REASON_ERROR:
                DEBUG_LOG_INFO("Charger: set DISABLE reason ERROR"); break;
            case CHARGER_DISABLE_REASON_ZERO_CURRENT:
                DEBUG_LOG_INFO("Charger: set DISABLE reason ZERO CURRENT"); break;
            case CHARGER_DISABLE_REASON_NOT_CONNECTED:
                DEBUG_LOG_INFO("Charger: set DISABLE reason NOT CONNECTED"); break;
            default:
                break;
        }
    }

    if (reason)
    {
        if (!theCharger->disable_reason)
        {
            Charger_Disable();
        }
        theCharger->disable_reason |= reason;
    }

#ifdef ENABLE_CHARGER_HYSTERESIS_SW
    if (reason != CHARGER_DISABLE_REASON_CHARGED)
    {
        /* If the charger was already disabled because it had completed we should stop
         * taking that reason into account. */
        Charger_DisableReasonClear(CHARGER_DISABLE_REASON_CHARGED);
    }
#endif
}

void Charger_DisableReasonClear(ChargerDisableReason reason)
{
    chargerTaskData *theCharger = appGetCharger();

    if (theCharger->disable_reason & reason)
    {
        switch (reason)
        {
            case CHARGER_DISABLE_REASON_TIMEOUT:
                DEBUG_LOG_ALWAYS("Charger: clear DISABLE reason TIMEOUT"); break;
            case CHARGER_DISABLE_REASON_REQUEST:
                DEBUG_LOG_ALWAYS("Charger: clear DISABLE reason REQUEST"); break;
            case CHARGER_DISABLE_REASON_ERROR:
                DEBUG_LOG_ALWAYS("Charger: clear DISABLE reason ERROR"); break;
            case CHARGER_DISABLE_REASON_ZERO_CURRENT:
                DEBUG_LOG_ALWAYS("Charger: clear DISABLE reason ZERO CURRENT"); break;
            case CHARGER_DISABLE_REASON_NOT_CONNECTED:
                DEBUG_LOG_ALWAYS("Charger: clear DISABLE reason NOT CONNECTED"); break;
            default:
                break;
        }
    }

    if (reason && theCharger->disable_reason)
    {
        theCharger->disable_reason &= ~reason;
        if (!theCharger->disable_reason)
        {
            Charger_Enable();
        }
    }
}

void Charger_ForceDisable(void)
{
    Charger_DisableReasonAdd(CHARGER_DISABLE_REASON_REQUEST);
}

void Charger_RestoreState(void)
{
    Charger_DisableReasonClear(CHARGER_DISABLE_REASON_REQUEST);
}

bool Charger_IsEnabled(void)
{
    chargerTaskData *theCharger = appGetCharger();

    return theCharger->is_enabled != 0;
}

bool Charger_IsCharging(void)
{
    if (Charger_IsConnected() &&
            Charger_IsEnabled())
    {
        chargerTaskData *theCharger = appGetCharger();

        switch (theCharger->status)
        {
            case TRICKLE_CHARGE:
            case FAST_CHARGE:
            case PRE_CHARGE:
                return TRUE;

            case STANDBY:
            default:
                break;
        }
    }
    return FALSE;
}

#ifdef ENABLE_SKIP_PFR
bool Charger_IsChargerConnectedAtBoot(void)
{
    chargerTaskData *theCharger = appGetCharger();

    return theCharger->charger_connected_at_boot;
}

void Charger_ClearChargerConnectedAtBootFlag(void)
{
    chargerTaskData *theCharger = appGetCharger();

    theCharger->charger_connected_at_boot = FALSE;
}
#endif

bool Charger_Init(Task init_task)
{
    chargerTaskData *theCharger = appGetCharger();

    /* Set up task handler & record current charger status */
    theCharger->task.handler = charger_HandleMessage;
    /* no last status yet */
    theCharger->status = ENABLE_FAIL_UNKNOWN;
    /* connection status not resolved yet */
    theCharger->is_connected = CHARGER_CONNECTION_UNKNOWN;
    /* this will be adjusted down if Curator reports an error */
    theCharger->max_supported_current = appConfigChargerFastCurrent();

    /* Make sure PSU is switched to VCHG when booted with charger attached */
    theCharger->power_source_vbat = 1;

    /* Register for charger messages */
    MessageChargerTask(&theCharger->task);

    /* Assume not connected by default (this also disables the charger,
     * just in case) */
    Charger_DisableReasonAdd(CHARGER_DISABLE_REASON_NOT_CONNECTED);

    /* Configure charger parameters */
    charger_ConfigureCharger();

#ifdef INCLUDE_CHARGER_DETECT
    /* Init charger detection */
    ChargerDetect_Init(&theCharger->task);
#else
    Charger_UpdateConnected(Charger_Status() != NO_POWER);
#endif

#ifdef ENABLE_SKIP_PFR
    theCharger->charger_connected_at_boot = (Charger_Status() != NO_POWER) ? TRUE : FALSE;
#endif

    /* Register charger module as ui provider*/
    Ui_RegisterUiProvider(ui_provider_charger, appChargerCurrentContext);

#ifdef ENABLE_CHARGER_HYSTERESIS_SW
    /* Register for battery voltage */
    batteryRegistrationForm form;
    form.task = &theCharger->task;
    form.representation = battery_level_repres_voltage;
    form.hysteresis = 5;
    (void)appBatteryRegister(&form);
#endif

    UNUSED(init_task);
    return TRUE;
}


static void Charger_NotifyClients(MessageId id)
{
    if (charger_client_tasks)
    {
        TaskList_MessageSendId(charger_client_tasks, id);
    }
}

bool Charger_ClientRegister(Task client_task)
{
    if (!charger_client_tasks)
    {
        charger_client_tasks = TaskList_Create();
    }

    if (TaskList_AddTask(charger_client_tasks, client_task))
    {
        /* Send initial state if charger is attached */
        MessageSend(client_task, Charger_IsConnected() ?
                                 CHARGER_MESSAGE_ATTACHED :
                                 CHARGER_MESSAGE_DETACHED, NULL);

        /* Send charger status update */
        chargerTaskData *theCharger = appGetCharger();

        MessageSend(client_task, charger_StatusMessageId(theCharger->status), NULL);

        return TRUE;
    }
    return FALSE;
}

void Charger_ClientUnregister(Task client_task)
{
    TaskList_RemoveTask(charger_client_tasks, client_task);
}

void Charger_TestModeControl(bool enabled)
{
    chargerTaskData *theCharger = appGetCharger();

    if (enabled)
    {
        theCharger->test_mode = 1;
    }
    else if (theCharger->test_mode)
    {
        theCharger->test_mode = 0;

        /* restore normal operation */

#ifdef INCLUDE_CHARGER_DETECT
        MessageChargerChanged msg_changed;
        msg_changed.charger_connected = Charger_Status() != NO_POWER;
        msg_changed.vreg_en_high = 0;
        ChargerDetect_Changed(&msg_changed);

        MessageChargerDetected msg_detected;
        msg_detected.attached_status = Charger_AttachedStatus();
        msg_detected.charger_dp_millivolts = 0;
        msg_detected.charger_dm_millivolts = 0;
        msg_detected.cc_status = CC_CURRENT_DEFAULT;
        ChargerDetect_Detected(&msg_detected);
#else
        Charger_UpdateConnected(Charger_Status() != NO_POWER);
#endif
    }
}

void Charger_TestChargerConnected(bool is_connected)
{
    chargerTaskData *theCharger = appGetCharger();
    if (!theCharger->test_mode)
    {
        DEBUG_LOG_ERROR("Charger_TestChargerConnected: test mode not enabled");
        return;
    }

#ifdef INCLUDE_CHARGER_DETECT
    MessageChargerChanged message;
    message.charger_connected = is_connected;
    ChargerDetect_Changed(&message);
#else
    Charger_UpdateConnected(is_connected);
#endif
}

void Charger_TestChargerDetected(usb_attached_status attached_status,
                                       uint16 charger_dp_millivolts,
                                       uint16 charger_dm_millivolts,
                                       usb_type_c_advertisement cc_status)
{
    chargerTaskData *theCharger = appGetCharger();
    if (!theCharger->test_mode)
    {
        DEBUG_LOG_ERROR("Charger_TestChargerDetected: test mode not enabled");
        return;
    }
    theCharger->test_attached_status = attached_status;

#ifdef INCLUDE_CHARGER_DETECT
    MessageChargerDetected message;
    message.attached_status = attached_status;
    message.charger_dp_millivolts = charger_dp_millivolts;
    message.charger_dm_millivolts = charger_dm_millivolts;
    message.cc_status = cc_status;
    ChargerDetect_Detected(&message);
#else
    UNUSED(charger_dp_millivolts);
    UNUSED(charger_dm_millivolts);
    UNUSED(cc_status);
    DEBUG_LOG_INFO("Charger_TestChargerDetected: charger detect is not enabled");
#endif
}

void Charger_TestChargerStatus(charger_status chg_status)
{
    chargerTaskData *theCharger = appGetCharger();
    if (!theCharger->test_mode)
    {
        DEBUG_LOG_ERROR("Charger_TestChargerStatus: test mode not enabled");
        return;
    }

    MessageChargerStatus message;
    message.chg_status = chg_status;
    Charger_StatusChanged(&message);
}

#endif /* INCLUDE_CHARGER */


charger_status Charger_Status(void)
{
    chargerTaskData *theCharger = appGetCharger();

    if (theCharger->test_mode)
    {
        return theCharger->status;
    }

    return ChargerStatus();
}

usb_attached_status Charger_AttachedStatus(void)
{
    chargerTaskData *theCharger = appGetCharger();

    if (theCharger->test_mode)
    {
        return theCharger->test_attached_status;
    }

    return UsbAttachedStatus();
}

bool Charger_AttachedStatusPending(void)
{
    return Charger_AttachedStatus() == UNKNOWN_STATUS;
}

bool Charger_IsConnected(void)
{
#ifdef INCLUDE_CHARGER
    chargerTaskData *theCharger = appGetCharger();

    return theCharger->is_connected != 0;
#else
    return Charger_Status() != NO_POWER;
#endif
}

void Charger_ForceAllowPowerOff(bool force_allow_power_off)
{
    chargerTaskData *theCharger = appGetCharger();
    theCharger->force_allow_power_off = force_allow_power_off ? 1 : 0;
}

void Charger_DisallowDormant(bool disallow_dormant)
{
    chargerTaskData *theCharger = appGetCharger();
    theCharger->disallow_dormant = disallow_dormant ? 1 : 0;
}

bool Charger_CanPowerOff(void)
{
    chargerTaskData *theCharger = appGetCharger();

    if(theCharger->force_allow_power_off)
    {
        return TRUE;
    }
#ifdef INCLUDE_CHARGER
    return !Charger_IsConnected();
#else
    return FALSE;
#endif
}

bool Charger_CanEnterDormant(void)
{
    chargerTaskData *theCharger = appGetCharger();
    if(theCharger->disallow_dormant)
    {
        return FALSE;
    }
    if(theCharger->force_allow_power_off)
    {
        return TRUE;
    }
#ifdef INCLUDE_CHARGER
    if (Charger_IsCharging())
    {
    return FALSE;
    }
    return TRUE;
#else
    return FALSE;
#endif
}

static void chargerMonitor_RegisterMessageGroup(Task task, message_group_t group)
{
    PanicFalse(group == CHARGER_MESSAGE_GROUP);
#ifdef INCLUDE_CHARGER
    (void)Charger_ClientRegister(task);
#else
    UNUSED(task);
#endif
}

MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(CHARGER, chargerMonitor_RegisterMessageGroup, NULL);

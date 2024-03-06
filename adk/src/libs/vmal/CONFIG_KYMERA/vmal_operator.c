/*******************************************************************************
Copyright (c) 2016-2022 Qualcomm Technologies International, Ltd.

FILE NAME
    vmal_operator.c

DESCRIPTION
    Operator shim
*/

#include <vmal.h>
#include <operator.h>
#include <panic.h>
#include <stdlib.h>
#include "logging.h"

#ifndef UNUSED
#define UNUSED(x) ((void)x)
#endif

static void vmalMsgHandler(Task task, MessageId id, Message msg);
static const TaskData vmal_handler = {vmalMsgHandler};
#define MY_TASK ((Task)&vmal_handler)

typedef enum
{
    processor_enable = 0,
    processor_disable,
} vmal_function_id;

/*! Registration array for all available observers */
typedef struct
{
    unsigned nr_entries;
    const vmal_registry_per_observer_t* *entry;
} vmal_registry_t;

typedef struct
{
    uint16 framework_on[max_number_processors];
    bool low_power_mode;
    bool locked;
    int16 framework_counter_while_locked[max_number_processors];
} vmal_track_t;

static struct
{
    vmal_registry_t registry;
    vmal_track_t track;
} state = {0};

static void vmalInformObserversAboutTurnedFrameworkOn(void)
{
    state.track.locked = TRUE;
    for(unsigned i = 0; i < state.registry.nr_entries; i++)
    {
        if(state.registry.entry[i]->callbacks->TurnedFrameworkOn)
        {
                DEBUG_LOG_V_VERBOSE("vmalInformObserversAboutTurnedFrameworkOn");
                state.registry.entry[i]->callbacks->TurnedFrameworkOn(state.track.low_power_mode);
        }
    }
    state.track.locked = FALSE;
}

static void vmalInformObserversAboutTurningFrameworkOff(void)
{
    state.track.locked = TRUE;
    for(unsigned i = 0; i < state.registry.nr_entries; i++)
    {
        if(state.registry.entry[i]->callbacks->TurningFrameworkOff)
        {
                DEBUG_LOG_V_VERBOSE("vmalInformObserversAboutTurningFrameworkOff");
                state.registry.entry[i]->callbacks->TurningFrameworkOff(state.track.low_power_mode);
        }
    }
    state.track.locked = FALSE;
}

void VmalRegisterAudioFrameworkObserver(const vmal_registry_per_observer_t * const info)
{
    state.registry.entry = PanicNull(realloc(state.registry.entry, (state.registry.nr_entries + 1) * sizeof(*state.registry.entry)));

    PanicNull((void*)info->callbacks);

    state.registry.entry[state.registry.nr_entries] = info;
    state.registry.nr_entries++;
}

static bool vmalOperatorIsStateLocked(void)
{
    return state.track.locked;
}

static bool vmalOperatorIsFrameworkOn(void)
{
    return (state.track.framework_on[main_processor] > 0);
}

static void vmalOperatorTrackState(uint8 processor, vmal_function_id function)
{
    PanicFalse(processor < max_number_processors);
    PanicFalse(!vmalOperatorIsStateLocked());

    switch(function)
    {
        case processor_enable:
            if((processor == main_processor) && (state.track.framework_on[main_processor] == 0))
            {
                vmalInformObserversAboutTurnedFrameworkOn();
            }
            state.track.framework_on[processor] ++;
            DEBUG_LOG_V_VERBOSE("vmalOperatorTrackState: Proc[%d] On %d", processor, state.track.framework_on[processor]);
        break;

        case processor_disable:
            PanicFalse(state.track.framework_on[processor] > 0);

            state.track.framework_on[processor] --;
            if((processor == main_processor) && (state.track.framework_on[main_processor] == 0))
            {
                vmalInformObserversAboutTurningFrameworkOff();
            }
            DEBUG_LOG_V_VERBOSE("vmalOperatorTrackState: Proc[%d] Off %d", processor, state.track.framework_on[processor]);
        break;
    }
}

static void vmalOperatorHandleLockedCalls(vmal_function_id function)
{
    switch(function)
    {
        case processor_enable:
        if(state.track.framework_counter_while_locked[main_processor] < 0)
        {
            DEBUG_LOG_PANIC("vmalOperatorHandleLockedCalls: Received turn off framework while turning on");
        }
        else
        {
            while(state.track.framework_counter_while_locked[main_processor] > 0)
            {
                vmalOperatorTrackState(main_processor, processor_enable);
                PanicFalse(OperatorFrameworkEnable(MAIN_PROCESSOR_ON));
                state.track.framework_counter_while_locked[main_processor] --;
            }
        }

        if(state.track.framework_counter_while_locked[second_processor] < 0)
        {
            DEBUG_LOG_PANIC("vmalOperatorHandleLockedCalls: Received turn off framework while turning on");
        }
        else
        {
            while(state.track.framework_counter_while_locked[second_processor] > 0)
            {
                vmalOperatorTrackState(second_processor, processor_enable);
                PanicFalse(OperatorFrameworkEnable(SECOND_PROCESSOR_ON));
                state.track.framework_counter_while_locked[second_processor] --;
            }
        }
        break;

        case processor_disable:
        if(state.track.framework_counter_while_locked[second_processor] > 0)
        {
            DEBUG_LOG_PANIC("vmalOperatorHandleLockedCalls: Received turn on framework while turning off");
        }
        else
        {
            while(state.track.framework_counter_while_locked[second_processor] < 0)
            {
                vmalOperatorTrackState(second_processor, processor_disable);
                PanicFalse(OperatorFrameworkEnable(SECOND_PROCESSOR_OFF));
                state.track.framework_counter_while_locked[second_processor] ++;
            }
        }

        if(state.track.framework_counter_while_locked[main_processor] > 0)
        {
            DEBUG_LOG_PANIC("vmalOperatorHandleLockedCalls: Received turn on framework while turning off");
        }
        else
        {
            while(state.track.framework_counter_while_locked[main_processor] < 0)
            {
                vmalOperatorTrackState(main_processor, processor_disable);
                PanicFalse(OperatorFrameworkEnable(MAIN_PROCESSOR_OFF));
                state.track.framework_counter_while_locked[main_processor] ++;
            }
        }
        break;
        default:
            DEBUG_LOG_PANIC("vmalOperatorHandleLockedCalls: Received unknown function %d", function);
        break;
    }
}

static void vmalEnableMainProcessor(void)
{
    PanicFalse(OperatorFrameworkEnable(MAIN_PROCESSOR_ON));
    vmalOperatorTrackState(main_processor, processor_enable);
    vmalOperatorHandleLockedCalls(processor_enable);
}

static void vmalDisableMainProcessor(void)
{
    vmalOperatorTrackState(main_processor, processor_disable);
    vmalOperatorHandleLockedCalls(processor_disable);
    PanicFalse(OperatorFrameworkEnable(MAIN_PROCESSOR_OFF));
}

static void vmalEnableSecondProcessor(void)
{
    PanicFalse(OperatorFrameworkEnable(SECOND_PROCESSOR_ON));
    vmalOperatorTrackState(second_processor, processor_enable);
    vmalOperatorHandleLockedCalls(processor_enable);
}

static void vmalDisableSecondProcessor(void)
{
    vmalOperatorTrackState(second_processor, processor_disable);
    vmalOperatorHandleLockedCalls(processor_disable);
    PanicFalse(OperatorFrameworkEnable(SECOND_PROCESSOR_OFF));
}

static void vmalOperatorTrackEnteringLpMode(void)
{
    state.track.low_power_mode = TRUE;
}

static void vmalOperatorTrackExitedLpMode(void)
{
    state.track.low_power_mode = FALSE;
}

Operator VmalOperatorCreate(uint16 cap_id)
{
    return OperatorCreate(cap_id, 0, NULL);
}

Operator VmalOperatorCreateWithKeys(uint16 capability_id, vmal_operator_keys_t* keys, uint16 num_keys)
{
    return OperatorCreate(capability_id, num_keys, (OperatorCreateKeys *)keys);
}

void VmalOperatorFrameworkEnableMainProcessor(void)
{
    if(!vmalOperatorIsStateLocked())
    {
        vmalEnableMainProcessor();
    }
    else
    {
        state.track.framework_counter_while_locked[main_processor] ++;
        DEBUG_LOG_WARN("VmalOperatorFrameworkEnableMainProcessor: Entered in locked state: %d",
                       state.track.framework_counter_while_locked[main_processor]);
    }
}

bool VmalOperatorFrameworkEnableMainProcessorAsync(void)
{
    bool framework_is_on = vmalOperatorIsFrameworkOn();
    if(!vmalOperatorIsStateLocked())
    {
        if(framework_is_on)
        {
            DEBUG_LOG_V_VERBOSE("VmalOperatorFrameworkEnableMainProcessorAsync: Already running");
            vmalEnableMainProcessor();
            return TRUE;
        }
        PanicFalse(OperatorFrameworkEnableAsync(MAIN_PROCESSOR_ON, MY_TASK));
    }
    else
    {
        state.track.framework_counter_while_locked[main_processor] ++;
        DEBUG_LOG_WARN("VmalOperatorFrameworkEnableMainProcessorAsync: Entered in locked state: %d",
                       state.track.framework_counter_while_locked[main_processor]);
    }
    return (framework_is_on);
}

void VmalOperatorFrameworkDisableMainProcessor(void)
{
    if(!vmalOperatorIsStateLocked())
    {
        vmalDisableMainProcessor();
    }
    else
    {
        state.track.framework_counter_while_locked[main_processor] --;;
        DEBUG_LOG_WARN("VmalOperatorFrameworkDisableMainProcessor: Entered in locked state: %d",
                       state.track.framework_counter_while_locked[main_processor]);
    }
}

void VmalOperatorFrameworkEnableSecondProcessor(void)
{
    if(!vmalOperatorIsStateLocked())
    {
        vmalEnableSecondProcessor();
    }
    else
    {
        state.track.framework_counter_while_locked[second_processor] ++;
        DEBUG_LOG_WARN("VmalOperatorFrameworkEnableSecondProcessor: Entered in locked state: %d",
                       state.track.framework_counter_while_locked[second_processor]);
    }
}

void VmalOperatorFrameworkDisableSecondProcessor(void)
{
    if(!vmalOperatorIsStateLocked())
    {
        vmalDisableSecondProcessor();
    }
    else
    {
        state.track.framework_counter_while_locked[second_processor] --;
        DEBUG_LOG_WARN("VmalOperatorFrameworkDisableSecondProcessor: Entered in locked state: %d",
                       state.track.framework_counter_while_locked[second_processor]);
    }
}

void VmalOperatorFrameworkTriggerNotificationStart(OperatorFrameworkTriggerType type, Operator opid)
{
    vmalOperatorTrackEnteringLpMode();
    PanicFalse(OperatorFrameworkTriggerNotificationStart(type, opid));
}

void VmalOperatorFrameworkTriggerNotificationStop(void)
{
    PanicFalse(OperatorFrameworkTriggerNotificationStop());
    vmalOperatorTrackExitedLpMode();
}

static void vmalMsgHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);

    switch (id)
    {
        case MESSAGE_OPERATOR_FRAMEWORK_ENABLE_RESULT:
            DEBUG_LOG_V_VERBOSE("vmalMsgHandler: MESSAGE_OPERATOR_FRAMEWORK_ENABLE_RESULT %d", ((MessageOperatorFrameworkEnableResult *)msg)->success);
            vmalOperatorTrackState(main_processor, processor_enable);
            vmalOperatorHandleLockedCalls(processor_enable);
        break;
        default:
            DEBUG_LOG_PANIC("vmalMsgHandler: Unknown message ID received: %d", id);
        break;
    }
}

bool vmalOperatorIsProcessorFrameworkEnabled(uint16 processor_id)
{
    if (processor_id < max_number_processors)
    {
        return (state.track.framework_on[processor_id] > 0);
    }

    return FALSE;
}

#ifdef HOSTED_TEST_ENVIRONMENT
void VmalGetState(uint16 *framework_counter_main, uint16 *framework_counter_second, bool *low_power_mode, bool *locked)
{
    *framework_counter_main = state.track.framework_on[main_processor];
    *framework_counter_second = state.track.framework_on[second_processor];
    *low_power_mode = state.track.low_power_mode;
    *locked = state.track.locked;
}

void VmalClearState(void)
{
    if (state.registry.entry)
    {
        free(state.registry.entry);
        state.registry.entry = NULL;
        state.registry.nr_entries = 0;
    }
    memset(&state.track, 0, sizeof(state.track));
}

void VmalSendAsyncMessageToTask(void)
{
    MessageOperatorFrameworkEnableResult msg;
    msg.success = TRUE;
    vmalMsgHandler(MY_TASK, MESSAGE_OPERATOR_FRAMEWORK_ENABLE_RESULT, (void*)&msg);
}
#endif

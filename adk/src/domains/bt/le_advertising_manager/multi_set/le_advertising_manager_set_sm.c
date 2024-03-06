/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_advertising_manager_multi_set
    \brief      Header file for management of Bluetooth Low Energy advertising sets.
*/

#include "le_advertising_manager_set_sm.h"
#include "le_advertising_manager_set_sm_msg_handler.h"
#include "le_advertising_manager_multi_set_private.h"

#include "local_addr.h"

#include <bdaddr.h>
#include <connection.h>
#include <logging.h>
#include <panic.h>
#include <stdlib.h>
#include "util.h"

/* Maximum number of advertising set state machines that can be created. */
#define LE_ADV_SET_SM_MAX 5

static le_advertising_manager_set_state_machine_t le_ext_adv_sm_list[LE_ADV_SET_SM_MAX] = {0};

static void leAdvertisingManager_SetSmSetAdvertData(le_advertising_manager_set_state_machine_t *sm)
{
    le_advertising_manager_data_packet_t *packet = LeAdvertisingManager_SetSmGetAdvertDataPacket(sm);
    uint8 size_advert = packet->data_size;
    uint8* advert_start[MAX_EXT_AD_DATA_BUFFER_COUNT] = {0};

    if (size_advert)
    {
        for (uint8 i = 0; i < ARRAY_DIM(advert_start); i++)
        {
            advert_start[i] = packet->data[i];
        }
    }

    DEBUG_LOG_VERBOSE("LEAM SetSmSetAdvertData, Size is %d data:%p", size_advert, advert_start[0]);

    LeAdvertisingManager_DataPacketDebugLogData(packet);

    ConnectionDmBleExtAdvSetDataReq(LeAdvertisingManager_SetSmGetTask(sm),
                                    LeAdvertisingManager_SetSmGetAdvHandle(sm),
                                    complete_data,
                                    size_advert,
                                    advert_start);

    /* ConnectionDmBleExtAdvSetDataReq takes ownership of the buffers passed
       in via advert_start, so set the pointers to NULL in the packet data. */
    LeAdvertisingManager_DataPacketReset(packet);
}

static void leAdvertisingManager_SetSmSetScanResponseData(le_advertising_manager_set_state_machine_t *sm)
{
    le_advertising_manager_data_packet_t *packet = LeAdvertisingManager_SetSmGetScanResponseDataPacket(sm);
    uint8 size_scan_resp = packet->data_size;
    uint8* scan_resp_start[MAX_EXT_AD_DATA_BUFFER_COUNT] = {0};

    if (size_scan_resp)
    {
        for (uint8 i = 0; i < ARRAY_DIM(scan_resp_start); i++)
        {
            scan_resp_start[i] = packet->data[i];
        }
    }

    DEBUG_LOG_VERBOSE("LEAM SetSmSetScanResponseData, Size is %d data:%p", size_scan_resp, scan_resp_start[0]);

    LeAdvertisingManager_DataPacketDebugLogData(packet);

    ConnectionDmBleExtAdvSetScanRespDataReq(LeAdvertisingManager_SetSmGetTask(sm),
                                            LeAdvertisingManager_SetSmGetAdvHandle(sm),
                                            complete_data,
                                            size_scan_resp,
                                            scan_resp_start);

    /* ConnectionDmBleExtAdvSetDataReq takes ownership of the buffers passed
       in via advert_start, so set the pointers to NULL in the packet data. */
    LeAdvertisingManager_DataPacketReset(packet);
}

static void leAdvertisingManager_SetSmEnterRegistering(le_advertising_manager_set_state_machine_t *sm)
{
    ConnectionDmBleExtAdvRegisterAppAdvSetReq(LeAdvertisingManager_SetSmGetTask(sm),
                                              LeAdvertisingManager_SetSmGetAdvHandle(sm));
}

static void leAdvertisingManager_SetSmEnterConfiguringParams(le_advertising_manager_set_state_machine_t *sm)
{
    Task confirm_task = LeAdvertisingManager_SetSmGetTask(sm);
    le_advertising_manager_set_params_t *params = LeAdvertisingManager_SetSmGetParams(sm);

    DEBUG_LOG("LEAM Sm Enter Configuring Params adv_filter_policy 0x%x", params->adv_filter_policy);

    ConnectionDmBleExtAdvSetParamsReq(confirm_task,
                                      LeAdvertisingManager_SetSmGetAdvHandle(sm),
                                      params->adv_event_properties,
                                      params->primary_adv_interval_min,
                                      params->primary_adv_interval_max,
                                      params->primary_adv_channel_map,
                                      params->own_addr_type,
                                      params->peer_tpaddr,
                                      params->adv_filter_policy,
                                      params->primary_adv_phy,
                                      params->secondary_adv_max_skip,
                                      params->secondary_adv_phy,
                                      params->adv_sid);
}

static void leAdvertisingManager_SetSmEnterConfiguringAddress(le_advertising_manager_set_state_machine_t *sm)
{
    le_advertising_manager_set_params_t *params = LeAdvertisingManager_SetSmGetParams(sm);

    ConnectionDmBleExtAdvSetRandomAddressReq(LeAdvertisingManager_SetSmGetTask(sm),
                                             LeAdvertisingManager_SetSmGetAdvHandle(sm),
                                             params->random_addr_type,
                                             params->random_addr.addr);

    sm->extended_advert_rpa_retries = 2;
}

static void leAdvertisingManager_SetSmEnterConfiguringData(le_advertising_manager_set_state_machine_t *sm)
{
    leAdvertisingManager_SetSmSetAdvertData(sm);
}

static void leAdvertisingManager_SetSmEnterConfiguringScanResponseData(le_advertising_manager_set_state_machine_t *sm)
{
    leAdvertisingManager_SetSmSetScanResponseData(sm);
}

static void leAdvertisingManager_SetSmEnterEnabling(le_advertising_manager_set_state_machine_t *sm)
{
    ConnectionDmBleExtAdvertiseEnableReq(LeAdvertisingManager_SetSmGetTask(sm),
                                         TRUE,
                                         LeAdvertisingManager_SetSmGetAdvHandle(sm));
}

static void leAdvertisingManager_SetSmEnterSuspending(le_advertising_manager_set_state_machine_t *sm)
{
    ConnectionDmBleExtAdvertiseEnableReq(LeAdvertisingManager_SetSmGetTask(sm),
                                         FALSE,
                                         LeAdvertisingManager_SetSmGetAdvHandle(sm));
}

static void leAdvertisingManager_SetSmEnterUnregistering(le_advertising_manager_set_state_machine_t *sm)
{
    ConnectionDmBleExtAdvUnregisterAppAdvSetReq(LeAdvertisingManager_SetSmGetTask(sm),
                                                sm->adv_handle);
}

void LeAdvertisingManager_SetSmSetState(le_advertising_manager_set_state_machine_t *sm, le_advertising_manager_set_state_t state)
{
    le_advertising_manager_set_state_t old_state = sm->state;

    /* It is not valid to re-enter the same state */
    PanicFalse(old_state != state);

    DEBUG_LOG_STATE("LEAM SetSmSetState 0x%p enum:le_advertising_manager_set_state_t:%d -> enum:le_advertising_manager_set_state_t:%d", sm, old_state, state);

    /* Handle state exit functions */
    switch (sm->state)
    {
    default:
        break;
    }

    /* Set new state */
    sm->state = state;

    /* Update the internal message lock */
    if (LeAdvertisingManager_SetSmIsSteadyState(state))
    {
        sm->internal_msg_lock = 0;
    }
    else
    {
        sm->internal_msg_lock = 1;
    }

    /* Handle state entry functions */
    switch (sm->state)
    {
    case LE_ADVERTISING_MANAGER_SET_STATE_REGISTERING:
        leAdvertisingManager_SetSmEnterRegistering(sm);
        break;

    case LE_ADVERTISING_MANAGER_SET_STATE_CONFIGURING_SET_PARAMS:
        leAdvertisingManager_SetSmEnterConfiguringParams(sm);
        break;

    case LE_ADVERTISING_MANAGER_SET_STATE_CONFIGURING_SET_ADDRESS:
        leAdvertisingManager_SetSmEnterConfiguringAddress(sm);
        break;

    case LE_ADVERTISING_MANAGER_SET_STATE_CONFIGURING_SET_DATA:
        leAdvertisingManager_SetSmEnterConfiguringData(sm);
        break;

    case LE_ADVERTISING_MANAGER_SET_STATE_CONFIGURING_SET_SCAN_RESP_DATA:
        leAdvertisingManager_SetSmEnterConfiguringScanResponseData(sm);
        break;

    case LE_ADVERTISING_MANAGER_SET_STATE_ENABLING:
        leAdvertisingManager_SetSmEnterEnabling(sm);
        break;

    case LE_ADVERTISING_MANAGER_SET_STATE_SUSPENDING:
        leAdvertisingManager_SetSmEnterSuspending(sm);
        break;

    case LE_ADVERTISING_MANAGER_SET_STATE_UNREGISTERING:
        leAdvertisingManager_SetSmEnterUnregistering(sm);
        break;

    default:
        break;
    }
}

void LeAdvertisingManager_SetSmSetTargetState(le_advertising_manager_set_state_machine_t *sm, le_advertising_manager_set_state_t target_state)
{
    DEBUG_LOG_FN_ENTRY("LEAM SetSmSetTargetState target_state enum:le_advertising_manager_set_state_t:%u", target_state);

    sm->target_state = target_state;
}

static void leAdvertisingManager_SetSmReset(le_advertising_manager_set_state_machine_t *sm)
{
    memset(sm, 0, sizeof(*sm));

    sm->task_data.handler = LeAdvertisingManager_SetSmMessageHandler;
    sm->state = LE_ADVERTISING_MANAGER_SET_STATE_NULL;
    sm->target_state = LE_ADVERTISING_MANAGER_SET_STATE_IDLE;
}

void LeAdvertisingManager_SetSmInit(void)
{
    le_advertising_manager_set_state_machine_t *sm = NULL;

    ARRAY_FOREACH(sm, le_ext_adv_sm_list)
    {
        leAdvertisingManager_SetSmReset(sm);
    }
}

static void leAdvertisingManager_SetSmValidateClientInterface(le_advertising_manager_set_client_interface_t *client_if)
{
    PanicNull(client_if);
    PanicNull((void *)client_if->RegisterCfm);
    PanicNull((void *)client_if->UpdateParamsCfm);
    PanicNull((void *)client_if->UpdateDataCfm);
    PanicNull((void *)client_if->EnableCfm);
    PanicNull((void *)client_if->DisableCfm);
    PanicNull((void *)client_if->UnregisterCfm);
    PanicNull((void *)client_if->RandomAddressRotateTimeout);
}

le_advertising_manager_set_state_machine_t *LeAdvertisingManager_SetSmCreate(le_advertising_manager_set_client_interface_t *client_if, uint8 adv_handle)
{
    le_advertising_manager_set_state_machine_t *sm = NULL;
    le_advertising_manager_set_state_machine_t *new_sm = NULL;

    /* Try to find an unused state machine. */
    ARRAY_FOREACH(sm, le_ext_adv_sm_list)
    {
        if (LE_ADVERTISING_MANAGER_SET_STATE_NULL == sm->state)
        {
            new_sm = sm;
            break;
        }
    }

    if (new_sm)
    {
        leAdvertisingManager_SetSmValidateClientInterface(client_if);

        new_sm->client_if = client_if;
        new_sm->adv_handle = adv_handle;
        LeAdvertisingManager_SetSmSetState(new_sm, LE_ADVERTISING_MANAGER_SET_STATE_UNREGISTERED);
        sm->target_state = LE_ADVERTISING_MANAGER_SET_STATE_IDLE;
    }
    else
    {
        Panic();
    }
    return new_sm;
}

/* Destroy an existing advertising set state machine */
void LeAdvertisingManager_SetSmDestroy(le_advertising_manager_set_state_machine_t *sm)
{
    /* Can only destroy the state machine when in the unregistered state.
       It is the clients responsibility to unregsiter the set before
       destroying it. */
    PanicFalse(LE_ADVERTISING_MANAGER_SET_STATE_UNREGISTERED == LeAdvertisingManager_SetSmGetState(sm));

    leAdvertisingManager_SetSmReset(sm);
}

le_advertising_manager_set_state_machine_t *LeAdvertisingManager_SetSmGetByAdvHandle(uint8 adv_handle)
{
    le_advertising_manager_set_state_machine_t *found_sm = NULL;
    le_advertising_manager_set_state_machine_t *sm = NULL;

    ARRAY_FOREACH(sm, le_ext_adv_sm_list)
    {
        if (adv_handle == sm->adv_handle)
        {
            found_sm = sm;
            break;
        }
    }

    return found_sm;
}

void LeAdvertisingManager_SetSmRegister(le_advertising_manager_set_state_machine_t *sm)
{
    MESSAGE_MAKE(msg, LE_ADVERTISING_MANAGER_SET_INTERNAL_REGISTER_T);
    msg->sm = sm;
    MessageSendConditionally(LeAdvertisingManager_SetSmGetTask(sm), LE_ADVERTISING_MANAGER_SET_INTERNAL_REGISTER, msg, &sm->internal_msg_lock);
}

void LeAdvertisingManager_SetSmUpdateParams(le_advertising_manager_set_state_machine_t *sm, const le_advertising_manager_set_params_t *params)
{
    MESSAGE_MAKE(msg, LE_ADVERTISING_MANAGER_SET_INTERNAL_UPDATE_PARAMS_T);
    msg->sm = sm;
    msg->params = *params;
    MessageSendConditionally(LeAdvertisingManager_SetSmGetTask(sm), LE_ADVERTISING_MANAGER_SET_INTERNAL_UPDATE_PARAMS_REQ, msg, &sm->internal_msg_lock);
}

void LeAdvertisingManager_SetSmUpdateData(le_advertising_manager_set_state_machine_t *sm, const le_advertising_manager_set_adv_data_t* data)
{
    MESSAGE_MAKE(msg, LE_ADVERTISING_MANAGER_SET_INTERNAL_UPDATE_DATA_T);
    msg->sm = sm;
    msg->data = *data;
    MessageSendConditionally(LeAdvertisingManager_SetSmGetTask(sm), LE_ADVERTISING_MANAGER_SET_INTERNAL_UPDATE_DATA_REQ, msg, &sm->internal_msg_lock);
}

void LeAdvertisingManager_SetSmEnable(le_advertising_manager_set_state_machine_t *sm)
{
    MESSAGE_MAKE(msg, LE_ADVERTISING_MANAGER_SET_INTERNAL_ENABLE_T);
    msg->sm = sm;
    MessageSendConditionally(LeAdvertisingManager_SetSmGetTask(sm), LE_ADVERTISING_MANAGER_SET_INTERNAL_ENABLE, msg, &sm->internal_msg_lock);
}

void LeAdvertisingManager_SetSmDisable(le_advertising_manager_set_state_machine_t *sm)
{
    MESSAGE_MAKE(msg, LE_ADVERTISING_MANAGER_SET_INTERNAL_DISABLE_T);
    msg->sm = sm;
    MessageSendConditionally(LeAdvertisingManager_SetSmGetTask(sm), LE_ADVERTISING_MANAGER_SET_INTERNAL_DISABLE, msg, &sm->internal_msg_lock);
}

void LeAdvertisingManager_SetSmUnregister(le_advertising_manager_set_state_machine_t *sm)
{
    MESSAGE_MAKE(msg, LE_ADVERTISING_MANAGER_SET_INTERNAL_UNREGISTER_T);
    msg->sm = sm;
    MessageSendConditionally(LeAdvertisingManager_SetSmGetTask(sm), LE_ADVERTISING_MANAGER_SET_INTERNAL_UNREGISTER, msg, &sm->internal_msg_lock);
}

bool LeAdvertisingManager_SetSmIsActive(le_advertising_manager_set_state_machine_t *sm)
{
    return (LE_ADVERTISING_MANAGER_SET_STATE_ACTIVE == LeAdvertisingManager_SetSmGetState(sm));
}

bool LeAdvertisingManager_SetSmIsStarting(le_advertising_manager_set_state_machine_t *sm)
{
    return (LE_ADVERTISING_MANAGER_SET_STATE_ENABLING == LeAdvertisingManager_SetSmGetState(sm));
}

bool LeAdvertisingManager_SetSmIsInactive(le_advertising_manager_set_state_machine_t *sm)
{
    return (LE_ADVERTISING_MANAGER_SET_STATE_IDLE == LeAdvertisingManager_SetSmGetState(sm));
}

bool LeAdvertisingManager_SetSmIsSuspending(le_advertising_manager_set_state_machine_t *sm)
{
    return (LE_ADVERTISING_MANAGER_SET_STATE_SUSPENDING == LeAdvertisingManager_SetSmGetState(sm));
}

uint8 LeAdvertisingManager_SetSmGetNumberOfSupportedSets(void)
{
    return LE_ADV_SET_SM_MAX;
}

uint8 LeAdvertisingManager_SetSmGetNextUnusedAdvHandle(void)
{
    uint8 next_adv_handle = 1;

    while (LeAdvertisingManager_SetSmGetByAdvHandle(next_adv_handle))
    {
        next_adv_handle++;

        /* If next_adv_handle becomes 0 it means the unsinged integer has
           wrapped around. This also means all the valid handles have been
           tried and failed - this should never happen. */
        if (next_adv_handle == 0)
        {
            Panic();
        }
    }

    return next_adv_handle;
}

static uint16 leAdvertisingManager_AggregatorGenerateRandomOffsetTimeInMilisecondsForRange(uint16 range)
{
    if(range)
    {
        uint16 random = UtilRandom();
        return (random < range) ? random : (random % range);
    }

    return 0;
}

void leAdvertisingManager_SetSmEnableAddressRotateTimer(le_advertising_manager_set_state_machine_t * sm, bool enable)
{
    if(enable &&
       sm->params.random_addr_generate_rotation_timeout_maximum_in_minutes > sm->params.random_addr_generate_rotation_timeout_minimum_in_minutes)
    {
        uint16 range_in_minutes = sm->params.random_addr_generate_rotation_timeout_maximum_in_minutes - sm->params.random_addr_generate_rotation_timeout_minimum_in_minutes;

        uint32 timeout_in_miliseconds = (sm->params.random_addr_generate_rotation_timeout_minimum_in_minutes * 60 * 1000)
                + (leAdvertisingManager_AggregatorGenerateRandomOffsetTimeInMilisecondsForRange(range_in_minutes * 60 * 1000));

        DEBUG_LOG("LEAM leAdvertisingManager_StartAddressRotateTimer, timeout in miliseconds %d", timeout_in_miliseconds);

        if(timeout_in_miliseconds)
        {
            MESSAGE_MAKE(msg, LE_ADVERTISING_MANAGER_SET_INTERNAL_RANDOM_ADDRESS_ROTATE_TIMER_T);
            msg->sm = sm;

            MessageSendLater(LeAdvertisingManager_SetSmGetTask(sm),
                         LE_ADVERTISING_MANAGER_SET_INTERNAL_RANDOM_ADDRESS_ROTATE_TIMER,
                         msg,
                         timeout_in_miliseconds);
        }
    }
    else
    {
        MessageCancelFirst(LeAdvertisingManager_SetSmGetTask(sm),LE_ADVERTISING_MANAGER_SET_INTERNAL_RANDOM_ADDRESS_ROTATE_TIMER);
    }
}

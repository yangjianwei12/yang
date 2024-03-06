/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    call_control_client
    \brief      Call control Profile SM - Client Role  state machine Implementation.
*/

#include "call_control_client_private.h"

#define CCP_SM_LOG     DEBUG_LOG

static void ccpSm_EnterIdle(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source);
static void ccpSm_ExitIdle(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source);
static void ccpSm_EnterOutgoingDialing(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source);
static void ccpSm_ExitOutgoingDialing(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source);
static void ccpSm_EnterOutgoingAlerting(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source);
static void ccpSm_ExitOutgoingAlerting(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source);
static void ccpSm_EnterActive(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source);
static void ccpSm_ExitActive(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source);
static void ccpSm_EnterLocallyHeld(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source);
static void ccpSm_ExitLocallyHeld(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source);
static void ccpSm_EnterRemotelyHeld(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source);
static void ccpSm_ExitRemotelyHeld(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source);
static void ccpSm_EnterLocallyRemotelyHeld(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source);
static void ccpSm_ExitLocallyRemotelyHeld(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source);
static void ccpSm_EnterIncoming(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source);
static void ccpSm_ExitIncoming(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source);

const ccp_call_state_handler_t call_state_handlers[] =
{
    /*TBS_CALL_STATE_INVALID                   */ {ccpSm_EnterIdle, ccpSm_ExitIdle},
    /*TBS_CALL_STATE_INCOMING                  */ {ccpSm_EnterIncoming, ccpSm_ExitIncoming},
    /*TBS_CALL_STATE_DIALLING                  */ {ccpSm_EnterOutgoingDialing, ccpSm_ExitOutgoingDialing},
    /*TBS_CALL_STATE_ALERTING                  */ {ccpSm_EnterOutgoingAlerting, ccpSm_ExitOutgoingAlerting},
    /*TBS_CALL_STATE_ACTIVE                    */ {ccpSm_EnterActive, ccpSm_ExitActive},
    /*TBS_CALL_STATE_LOCALLY_HELD              */ {ccpSm_EnterLocallyHeld, ccpSm_ExitLocallyHeld},
    /*TBS_CALL_STATE_REMOTELY_HELD             */ {ccpSm_EnterRemotelyHeld, ccpSm_ExitRemotelyHeld},
    /*TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD */ {ccpSm_EnterLocallyRemotelyHeld, ccpSm_ExitLocallyRemotelyHeld},
};

static void ccpSm_EnterIdle(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source)
{
    UNUSED(tbs_call_state);

    call_info->state = CCP_CALL_STATE_IDLE;
    call_info->tbs_call_id = 0;
    Telephony_NotifyMicrophoneUnmuted(source);
    Telephony_NotifyCallEnded(source);
}

static void ccpSm_ExitIdle(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source)
{
    UNUSED(call_info);
    UNUSED(tbs_call_state);
    UNUSED(source);
}

static void ccpSm_EnterOutgoingDialing(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source)
{
    UNUSED(source);

    call_info->tbs_call_id = tbs_call_state->callId;
    call_info->state = CCP_CALL_STATE_OUTGOING_DIALING;
}

static void ccpSm_ExitOutgoingDialing(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source)
{
    UNUSED(call_info);
    UNUSED(tbs_call_state);
    UNUSED(source);
}

static void ccpSm_EnterOutgoingAlerting(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source)
{
    UNUSED(source);

    call_info->tbs_call_id = tbs_call_state->callId;
    call_info->state = CCP_CALL_STATE_OUTGOING_ALERTING;
}

static void ccpSm_ExitOutgoingAlerting(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source)
{
    UNUSED(call_info);
    UNUSED(tbs_call_state);
    UNUSED(source);
}

static void ccpSm_EnterActive(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source)
{
    UNUSED(source);

    call_info->tbs_call_id = tbs_call_state->callId;
    call_info->state = CCP_CALL_STATE_ACTIVE;
}

static void ccpSm_ExitActive(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source)
{
    UNUSED(call_info);
    UNUSED(tbs_call_state);
    UNUSED(source);
}

static void ccpSm_EnterLocallyHeld(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source)
{
    UNUSED(source);
    call_info->tbs_call_id = tbs_call_state->callId;
    call_info->state = CCP_CALL_STATE_LOCALLY_HELD;
}

static void ccpSm_ExitLocallyHeld(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source)
{
    UNUSED(call_info);
    UNUSED(source);
    UNUSED(tbs_call_state);
}

static void ccpSm_EnterRemotelyHeld(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source)
{
    UNUSED(source);
    call_info->tbs_call_id = tbs_call_state->callId;
    call_info->state = CCP_CALL_STATE_REMOTELY_HELD;
}

static void ccpSm_ExitRemotelyHeld(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source)
{
    UNUSED(call_info);
    UNUSED(source);
    UNUSED(tbs_call_state);
}

static void ccpSm_EnterLocallyRemotelyHeld(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source)
{
    UNUSED(source);
    call_info->tbs_call_id = tbs_call_state->callId;
    call_info->state = CCP_CALL_STATE_LOCALLY_REMOTELY_HELD;
}

static void ccpSm_ExitLocallyRemotelyHeld(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source)
{
    UNUSED(call_info);
    UNUSED(tbs_call_state);
    UNUSED(source);
}

/*! \brief Enter Connected Incoming */
static void ccpSm_EnterIncoming(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source)
{
    call_info->tbs_call_id = tbs_call_state->callId;
    call_info->state = CCP_CALL_STATE_INCOMING;
    Telephony_NotifyCallIncoming(source);

    CCP_SM_LOG("ccpSm_EnterConnectedIncoming Incoming Call callId:%d", tbs_call_state->callId);
}

static void ccpSm_ExitIncoming(ccp_call_info_t *call_info, const TbsCallState *tbs_call_state, voice_source_t source)
{
    UNUSED(call_info);
    UNUSED(tbs_call_state);
    MessageCancelFirst(CallControlClient_GetTask(), CCP_INTERNAL_OUT_OF_BAND_RINGTONE_REQ);
    Telephony_NotifyCallIncomingEnded(source);
}

ccp_call_state_t CcpSm_GetCallStateForMatchingCallId(call_control_client_instance_t *instance, uint8 call_id)
{
    for (uint8 idx = 0; idx < MAX_ACTIVE_CALLS_SUPPORTED; idx++)
    {
        if ( instance->call_state[idx].tbs_call_id == call_id)
        {
            return instance->call_state[idx].state;
        }
    }
    return CCP_CALL_STATE_IDLE;
}

void CcpSm_SetCallState(gatt_cid_t cid, ccp_call_info_t *call_info, const TbsCallState *tbs_call_state)
{
    ccp_call_state_t new_state;
    ccp_call_state_t old_state;

    voice_source_t source = CallControlClient_GetVoiceSourceForCid(cid);
    voice_source_t focused_source = voice_source_none;

    new_state = tbs_call_state->callState + 1;
    old_state = call_info->state;

    if (old_state == new_state)
    {
        return;
    }

    CCP_SM_LOG("CcpSm_SetCallState( enum:ccp_call_state_t:%d -> enum:ccp_call_state_t:%d)", old_state, new_state);

    if (new_state >= CCP_CALL_STATE_IDLE && new_state <= CCP_CALL_STATE_LOCALLY_REMOTELY_HELD)
    {
        /* Exit from previous state*/
        call_state_handlers[old_state].exit(call_info, tbs_call_state, source);

        /* Enter new state*/
        call_state_handlers[new_state].enter(call_info, tbs_call_state, source);
    }

    /* Inform the context changes to the UI*/
    Focus_GetVoiceSourceForContext(ui_provider_telephony, &focused_source);
    if (focused_source == source)
    {
        Ui_InformContextChange(ui_provider_telephony, VoiceSources_GetSourceContext(source));
    }
    else
    {
        DEBUG_LOG_VERBOSE("CcpSm_SetCallState didn't push context for unfocused enum:voice_source_t:%d", source);
    }
}

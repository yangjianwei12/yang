/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Application domain AGHFP component abstraction layer that creates a common api for 
*           both the legacy api and synergy APIs.
*/
#include <panic.h>
#include <logging.h>
#include <device_properties.h>
#include <device_list.h>
#include "aghfp_profile_config.h"
#include "aghfp_profile_private.h"
#include "aghfp_profile_abstraction.h"
#include "aghfp_profile_instance.h"
#include "hfg_lib.h"


/***************************** Macros ****************************************/

/*! \brief Hfp Indicator Support */
#define AGHFP_ENHANCED_SAFETY_HF_IND_INDEX (0)
#define AGHFP_BATTERY_LEVEL_HF_IND_INDEX   (1)
#define AGHFP_INDICATORS_COUNT             (2)

/***************************** function defines ****************************************/
void    AghfpProfileAbstract_Activate(uint16 supported_qce_codecs, uint16 supported_features)
{
    UNUSED(supported_qce_codecs);

    CsrBtHfgHfIndicator *hfgIndicators = NULL;

    hfgIndicators = (CsrBtHfgHfIndicator *) PanicUnlessMalloc(AGHFP_INDICATORS_COUNT * sizeof(CsrBtHfgHfIndicator));

    hfgIndicators[AGHFP_ENHANCED_SAFETY_HF_IND_INDEX].hfIndicatorID     = CSR_BT_HFP_ENHANCED_SAFETY_HF_IND;
    hfgIndicators[AGHFP_ENHANCED_SAFETY_HF_IND_INDEX].status            = CSR_BT_HFP_HF_INDICATOR_STATE_DISABLE;
    hfgIndicators[AGHFP_ENHANCED_SAFETY_HF_IND_INDEX].valueMax          = 1;
    hfgIndicators[AGHFP_ENHANCED_SAFETY_HF_IND_INDEX].valueMin          = 0;

    hfgIndicators[AGHFP_BATTERY_LEVEL_HF_IND_INDEX].hfIndicatorID     = CSR_BT_HFP_BATTERY_LEVEL_HF_IND;
    hfgIndicators[AGHFP_BATTERY_LEVEL_HF_IND_INDEX].status            = CSR_BT_HFP_HF_INDICATOR_STATE_ENABLE;
    hfgIndicators[AGHFP_BATTERY_LEVEL_HF_IND_INDEX].valueMax          = 100;
    hfgIndicators[AGHFP_BATTERY_LEVEL_HF_IND_INDEX].valueMin          = 0;
    
    HfgActivateReqSend(&aghfp_profile_task_data.task,
                       CSR_BT_HFG_AT_MODE_SEMI,
                       AGHFP_MAX_HF_CONNECTION,
                       NULL,
                       supported_features,
                       AGHFP_CALL_CONFIG,
                       AGHFG_PROFILE_SETUP,
                       hfgIndicators,
                       AGHFP_INDICATORS_COUNT);

}

void AghfpProfileAbstract_SendOk(aghfpInstanceTaskData *ins, bool ignore)
{
    UNUSED(ins);
    if(!ignore)
     {
        char* command = malloc(20);
        memset(command, 0, 20);
        memcpy(command, AGHFG_STR_OK, AGHFG_STR_OK_LENGTH);
        HfgAtCmdReqSend(ins->connection_id, command);
     }
}

void    AghfpProfileAbstract_SendError(aghfpInstanceTaskData *ins)
{
    UNUSED(ins);
}

void AghfpProfileAbstract_ConfigureAudioSettings(aghfpInstanceTaskData * instance)
{
    uint8* current_audio_settings = NULL;
    const CsrBtHfgAudioLinkParameterListConfig *default_audio_settings = NULL;

    if(instance->audio_params == NULL)
    {
        return;
    }

    default_audio_settings = instance->audio_params;
    current_audio_settings = (uint8*) PanicUnlessMalloc(sizeof(default_audio_settings->packetType));

    memcpy(current_audio_settings, &default_audio_settings->packetType, sizeof(default_audio_settings->packetType));
    HfgConfigAudioReqSend(instance->connection_id, CSR_BT_HFG_AUDIO_SUP_PACKETS, current_audio_settings, sizeof(default_audio_settings->packetType));

    current_audio_settings = (uint8*) PanicUnlessMalloc(sizeof(default_audio_settings->txBandwidth));
    memcpy(current_audio_settings, &default_audio_settings->txBandwidth, sizeof(default_audio_settings->txBandwidth));
    HfgConfigAudioReqSend(instance->connection_id, CSR_BT_HFG_AUDIO_TX_BANDWIDTH, current_audio_settings, sizeof(default_audio_settings->txBandwidth));

    current_audio_settings = (uint8*) PanicUnlessMalloc(sizeof(default_audio_settings->rxBandwidth));
    memcpy(current_audio_settings, &default_audio_settings->rxBandwidth, sizeof(default_audio_settings->rxBandwidth));
    HfgConfigAudioReqSend(instance->connection_id, CSR_BT_HFG_AUDIO_RX_BANDWIDTH, current_audio_settings, sizeof(default_audio_settings->rxBandwidth));

    current_audio_settings =  (uint8*) PanicUnlessMalloc(sizeof(default_audio_settings->maxLatency));
    memcpy(current_audio_settings, &default_audio_settings->maxLatency, sizeof(default_audio_settings->maxLatency));
    HfgConfigAudioReqSend(instance->connection_id, CSR_BT_HFG_AUDIO_MAX_LATENCY, current_audio_settings, sizeof(default_audio_settings->maxLatency));

    current_audio_settings = (uint8*) PanicUnlessMalloc(sizeof(default_audio_settings->reTxEffort));
    memcpy(current_audio_settings, &default_audio_settings->reTxEffort, sizeof(default_audio_settings->reTxEffort));
    HfgConfigAudioReqSend(instance->connection_id, CSR_BT_HFG_AUDIO_RETRANSMISSION, current_audio_settings, sizeof(default_audio_settings->reTxEffort));

    current_audio_settings = (uint8*) PanicUnlessMalloc(sizeof(default_audio_settings->voiceSettings));
    memcpy(current_audio_settings, &default_audio_settings->voiceSettings, sizeof(default_audio_settings->voiceSettings));
    HfgConfigAudioReqSend(instance->connection_id, CSR_BT_HFG_AUDIO_VOICE_SETTINGS, current_audio_settings, sizeof(default_audio_settings->voiceSettings));
}

void AghfpProfileAbstract_EstablishSlcConnect(aghfpInstanceTaskData * instance)
{
    CsrBtDeviceAddr deviceAddr;
    /* Retreive the BD address of AG device */
    BdaddrConvertVmToBluestack(&deviceAddr, &(instance->hf_bd_addr));
    HfgServiceConnectReqSend(deviceAddr, CSR_BT_HFG_CONNECTION_UNKNOWN);
}

void AghfpProfileAbstract_SlcDisconnect(aghfpInstanceTaskData * instance)
{
    HfgDisconnectReqSend(instance->connection_id);
}

void AghfpProfileAbstract_AudioConnect(aghfpInstanceTaskData * instance, const aghfp_audio_params *audio_param, uint16 qceCodecId)
{
    HfgAudioConnectExtReqSend(instance->connection_id, PCM_SLOT, PCM_SLOT_REALLOC, qceCodecId);
    UNUSED(audio_param);
}

void AghfpProfileAbstract_AudioDisconnect(aghfpInstanceTaskData * instance)
{
    HfgAudioDisconnectReqSend(instance->connection_id);
}

void AghfpProfileAbstract_AudioConnectResponse(aghfpInstanceTaskData * instance, bool accept, sync_pkt_type packet_type, const aghfp_audio_params *audio_param)
{
    hci_error_t hci = accept? HCI_SUCCESS : HCI_ERROR_REJ_BY_REMOTE_NO_RES;
    CsrBtHfgAudioIncomingAcceptParameters *param = NULL;
    if(audio_param != NULL)
    {
        param = (CsrBtHfgAudioIncomingAcceptParameters*) PanicUnlessMalloc(sizeof(CsrBtHfgAudioIncomingAcceptParameters));
        param->packetTypes = audio_param->packetType;   /* Specifies which SCO/eSCO packet types to accept */
        param->txBandwidth = audio_param->txBandwidth;   /* Specifies the maximum Transmission bandwidth to accept */
        param->rxBandwidth = audio_param->rxBandwidth;   /* Specifies the maximum Receive bandwidth to accept */
        param->maxLatency = audio_param->maxLatency;    /* Specifies the maximum Latency to accept */
        param->contentFormat = audio_param->voiceSettings; /* Specifies which SCO/eSCO content format to accept */
        param->reTxEffort = audio_param->reTxEffort;    /* Specifies the Retransmission setting(s) to accept */
    }

    HfgAudioAcceptConnectResSend(instance->connection_id, hci,
                                 param, PCM_SLOT, PCM_SLOT_REALLOC);
    UNUSED(packet_type);
}

void AghfpProfileAbstract_SetRemoteMicrophoneGain(aghfpInstanceTaskData * instance, uint8 gain)
{
    HfgMicGainReqSend(instance->connection_id, gain);
}

void AghfpProfileAbstract_SetRemoteSpeakerVolume(aghfpInstanceTaskData * instance, int gain)
{
    HfgSpeakerGainReqSend(instance->connection_id, gain);
}

void AghfpProfileAbstract_SendCallHeldIndication(aghfpInstanceTaskData * instance)
{
    HfgStatusIndicatorSetReqSend(instance->connection_id, CSR_BT_CALL_HELD_INDICATOR, instance->bitfields.call_hold);
}

void AghfpProfileAbstract_SendCallSetupIndication(aghfpInstanceTaskData * instance)
{
    HfgStatusIndicatorSetReqSend(instance->connection_id, CSR_BT_CALL_SETUP_STATUS_INDICATOR, instance->bitfields.call_setup);
}

void AghfpProfileAbstract_SendCallStatusIndication(aghfpInstanceTaskData * instance)
{
    HfgStatusIndicatorSetReqSend(instance->connection_id, CSR_BT_CALL_STATUS_INDICATOR, instance->bitfields.call_status);
}

void AghfpProfileAbstract_EnableInbandRingTone(aghfpInstanceTaskData * instance)
{
    HfgInbandRingingReqSend(instance->connection_id, instance->bitfields.in_band_ring);
}


void AghfpProfileAbstract_SendRingAlert(aghfpInstanceTaskData * instance, bool caller_id_host, bool caller_id_remote)
{
    CsrCharString* name = NULL;
    CsrCharString* num = NULL;

    name = (CsrCharString*) PanicUnlessMalloc(2);
    strcpy(name, "");

    if (caller_id_host)
    {
        num = (CsrCharString*) PanicUnlessMalloc(instance->clip.size_clip_number + 1);
        if(instance->clip.size_clip_number > 0)
        {
            memcpy(num, instance->clip.clip_number, instance->clip.size_clip_number);
        }
        memcpy(num+instance->clip.size_clip_number, "\0", 1);    
    }
    HfgRingReqSend(instance->connection_id, 2, 2, num, name, instance->clip.clip_type);
    UNUSED(caller_id_remote);
}

void AghfpProfileAbstract_SendCallerId(aghfpInstanceTaskData * instance)
{
    /* Do nothing */
    UNUSED(instance);
}

void AghfpProfileAbstract_SendServiceIndication(aghfpInstanceTaskData * instance, aghfp_service_availability avail)
{
    HfgStatusIndicatorSetReqSend(instance->connection_id, CSR_BT_SERVICE_INDICATOR, avail)
}

void AghfpProfileAbstract_SendSignalStrengthIndication(aghfpInstanceTaskData * instance, uint16 level)
{
    HfgStatusIndicatorSetReqSend(instance->connection_id, CSR_BT_SIGNAL_STRENGTH_INDICATOR, level)
}

void AghfpProfileAbstract_SendDialRes(aghfpInstanceTaskData * instance, bool resp, bool ignore)
{
    uint16 cmee_code = ((resp == TRUE)? CSR_BT_CME_SUCCESS:CSR_BT_CME_AG_FAILURE);
    UNUSED (ignore);
    HfgDialResSend(instance->connection_id, cmee_code);
}

void AghfpProfileAbstract_SendNetworkOperator(aghfpInstanceTaskData * instance)
{
    HfgOperatorResSend(instance->connection_id, 3, CsrStrDup((char*)instance->network_operator), CSR_BT_CME_SUCCESS); 
}

void AghfpProfileAbstract_SendSubscriberNumber(aghfpInstanceTaskData * instance)
{
    HfgSubscriberNumberResSend(instance->connection_id, TRUE, 
                               (CsrCharString *) CsrStrDup(""),
                               145, 0, CSR_BT_CME_SUCCESS);
}


void AghfpProfileAbstract_SendCallWaiting(aghfpInstanceTaskData * instance, bool caller_id_host, bool caller_id_remote)
{
    if (caller_id_host)
    {
        CsrCharString* clip = NULL;
        clip = (CsrCharString*) PanicUnlessMalloc(instance->clip.size_clip_number + 1);
        if(instance->clip.size_clip_number > 0)
        {
            memcpy(clip, instance->clip.clip_number, instance->clip.size_clip_number);
        }
        memcpy(clip+instance->clip.size_clip_number, "\0", 1);
        HfgCallWaitingReqSend(instance->connection_id, clip, NULL, instance->clip.clip_type);
    }
    else
    {
        CsrBtHfgCallWaitingReqSend(instance->connection_id, NULL, NULL, 0);
    }
    UNUSED(caller_id_remote);
}

void AghfpProfileAbstract_SendCallList(aghfpInstanceTaskData * instance, uint16 index)
{
    call_list_element_t *call;

    UNUSED(index);
    for_each_call(instance->call_list, call)
    {
        DEBUG_LOG_ALWAYS("call->idx %d", call->call.idx);
        CsrCharString* number = NULL;
        number = (CsrCharString*) PanicUnlessMalloc(call->call.size_number + 1);
        if(call->call.size_number > 0)
        {
            memcpy(number, call->call.number, call->call.size_number);
        }
        memcpy(number+call->call.size_number, "\0", 1);
    
        CsrBtHfgCallListResSend(instance->connection_id, FALSE, call->call.idx,
            call->call.dir, call->call.status, call->call.mode, call->call.mpty, 
            number, call->call.type, CSR_BT_CME_SUCCESS);
    }
    
     CsrBtHfgCallListResSend(instance->connection_id,TRUE, 0, 0, 0, 0, 0, NULL, 0, CSR_BT_CME_SUCCESS);
}


void AghfpProfileAbstract_SendBattChgIndicator(aghfpInstanceTaskData * instance, uint16 level)
{
    HfgStatusIndicatorSetReqSend(instance->connection_id, CSR_BT_BATTERY_CHARGE_INDICATOR, level);
}

void AghfpProfileAbstract_IndicatorsStatusResponse(aghfpInstanceTaskData * instance, 
                                                                aghfp_service_availability availability,
                                                                uint16 signal, 
                                                                aghfp_roam_status roam_status, 
                                                                uint16 batt)
{
    CsrBtHfgManualIndicatorResSend(instance->connection_id, 
                                   availability, 
                                   instance->bitfields.call_status, 
                                   instance->bitfields.call_setup, 
                                   instance->bitfields.call_hold, 
                                   signal, 
                                   roam_status, 
                                   batt);
}






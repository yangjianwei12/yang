/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_unicast_manager
    \brief      Implementations for the LE Unicast Manager message task.
*/

#if defined(INCLUDE_LE_AUDIO_UNICAST)

#include "le_unicast_manager_instance.h"
#include "le_unicast_manager_task.h"
#include "call_control_client.h"
#include "media_control_client.h"
#include "pacs_utilities.h"
#include "gatt_connect.h"
#include <timestamp_event.h>
#include <panic.h>
#include "gatt_tmas_server.h"
#include "tmap_profile.h"
#include "micp_server.h"
#include "le_unicast_music_source.h"
#include "telephony_messages.h"
#include "mirror_profile.h"
#include "kymera.h"
#include "kymera_le_mic_chain.h"
#include "device_properties.h"
#include "le_unicast_music_source.h"
#include "le_unicast_voice_source.h"

/*! Offset of client configuration within the structure. */
#define leUnicastManager_GetCCCDOffset()  (offsetof(BapAscsConfig, aseControlPointCharClientCfg))
#define leUnicastManager_GetClientCfgOffset()  (offsetof(BapAscsConfig, aseCharClientCfg))

/*! Minimum GATT MTU needed to support LE unicast. */
#define leUnicastManagerConfig_GattMtuMinimum() 100

/*! Maximum number of active ASEs supported per Gatt connection. */
#define NUMBER_OF_ASES_REQUIRED    6

static void leUnicastManager_OnGattConnect(gatt_cid_t cid);
static void leUnicastManager_OnGattDisconnect(gatt_cid_t cid);
static void leUnicastManager_OnEncryptionChanged(gatt_cid_t cid, bool encrypted);

static const gatt_connect_observer_callback_t leUnicastManager_connect_callbacks =
{
    .OnConnection = leUnicastManager_OnGattConnect,
    .OnDisconnection = leUnicastManager_OnGattDisconnect,
    .OnEncryptionChanged = leUnicastManager_OnEncryptionChanged
};

static bool leUnicastManager_SampleRateLookupOnPacs(bool is_source, uint16 audiocontext, uint16 sample_rate, uint16 codec_type)
{
    bool match_found = FALSE;
    uint16 no_of_pacs;
    uint8 pacs_index;

    switch(codec_type)
    {
        case KYMERA_LE_AUDIO_CODEC_LC3:
        {
            const GattPacsServerRecordType* pacs_record = LeBapPacsUtilities_GetPacs(is_source, &no_of_pacs);

            for (pacs_index = 0; pacs_index < no_of_pacs; pacs_index++)
            {
                AudioContextType pacs_audiocontext = LeBapPacsUtilities_GetPreferredAudioContext(pacs_record[pacs_index].metadata, pacs_record[pacs_index].metadataLength);

                if ((audiocontext & pacs_audiocontext) && (sample_rate & pacs_record[pacs_index].supportedSamplingFrequencies))
                {
                    match_found = TRUE;
                    break;
                }
            }
        }
        break;
#if defined (INCLUDE_LE_APTX_ADAPTIVE) || defined (INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE)
        case KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE:
        case KYMERA_LE_AUDIO_CODEC_APTX_LITE:
        {
            const GattPacsServerVSPacRecord* pacs_record = LeBapPacsUtilities_GetPacsVS(is_source, &no_of_pacs, codec_type);
            PacsSamplingFrequencyType pacs_sample_rate;

            for (pacs_index = 0; pacs_index < no_of_pacs; pacs_index++)
            {
                AudioContextType pacs_audiocontext = LeBapPacsUtilities_GetPreferredAudioContext(pacs_record[pacs_index].metadata, pacs_record[pacs_index].metadataLength);
                pacs_sample_rate = LeBapPacsUtilities_GetSampleRateFromVSPac(&pacs_record[pacs_index]);

                if ((audiocontext & pacs_audiocontext) && (sample_rate & pacs_sample_rate))
                {
                    match_found = TRUE;
                    break;
                }
            }
        }
        break;
#endif
        default:
            break;
    }

    return match_found;
}

static bool leUnicastManager_ValidateSamplingRateForASE(gatt_cid_t cid, uint16 ase_id, uint16 audiocontext)
{
    bool sampling_rate_matched = FALSE;
    uint8 direction = LeBapUnicastServer_GetAseDirection(cid, ase_id);
    BapServerAseCodecInfo *codec_info = LeBapUnicastServer_GetCodecParameters(cid, ase_id);
    PacsSamplingFrequencyType sample_rate = codec_info == NULL ? 0 :
                    LeBapPacsUtilities_GetPacsSamplingFreqBitMaskFromFreq(LeUnicastManager_GetSampleRate(codec_info));
    appKymeraLeAudioCodec codec_type = codec_info == NULL ? KYMERA_LE_AUDIO_CODEC_LC3 :
                    leUnicastManager_GetCodecType(codec_info);

    switch (direction)
    {
        case GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK:
            sampling_rate_matched = leUnicastManager_SampleRateLookupOnPacs(FALSE, audiocontext, sample_rate, codec_type);
        break;

        case GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE:
            sampling_rate_matched =  leUnicastManager_SampleRateLookupOnPacs(TRUE, audiocontext, sample_rate, codec_type);
        break;

        default:
        break;
    }

    return sampling_rate_matched;
}

static bool leUnicastManager_IsConversationalContextReqd(AudioContextType current_audio_context)
{
    /* Check if CCP call state is not Idle and ensure that CCP notification arrived before ASE enable, by checking the UM context is unknown.
     * Or if ASE were enabled first (sequential ase enable) for which the determined
     * audio context was conversational, which means that the audio is currently routed through LE voice, in that case return TRUE.
     */
    if (((CallClientControl_GetContext(voice_source_le_audio_unicast_1) > context_voice_connected &&
          CallClientControl_GetContext(voice_source_le_audio_unicast_1) < max_voice_contexts) &&
          current_audio_context == AUDIO_CONTEXT_TYPE_UNKNOWN) ||
          current_audio_context == AUDIO_CONTEXT_TYPE_COVERSATIONAL)
    {
        return TRUE;
    }

    return FALSE;
}

static AudioContextType leUnicastManager_DetermineAudioContext(gatt_cid_t cid, uint8 ase_count, BapServerEnableIndInfo *p_enable_info)
{
    AudioContextType audio_context;
    AudioContextType determined_audio_context;
    AudioContextType current_audio_context;
    uint8 scan;
    le_um_instance_t *inst = LeUnicastManager_GetInstance();

    PanicNull(inst);
    current_audio_context = inst->audio_context;

    audio_context = BapServerLtvUtilitiesGetStreamingAudioContext(p_enable_info[0].metadata, p_enable_info[0].metadataLength);
    determined_audio_context = audio_context;

    /* The audio context is determined when the audio context for the ASE is unknown or has unspecified bit set in it or audio context contains more than 1 context in it */
    if (LeUnicastManager_IsContextOfTypeUnknown(audio_context) ||
        LeUnicastManager_AudioContextHasUnspecifiedContext(audio_context) ||
        LeUnicastManager_AudioContextHasMoreThanOneContext(audio_context))
    {
        uint8 src_ase_count = 0;
        uint8 snk_ase_count = 0;
        uint16 src_sample_rate = 0;
        uint16 snk_sample_rate = 0;
        uint16 sample_rate;

        for (scan = 0; scan < ase_count; scan++)
        {
           const BapServerAseCodecInfo *codec_info = LeBapUnicastServer_GetCodecParameters(cid, p_enable_info[scan].aseId);

           PanicNull((void *) codec_info);
           sample_rate = LeUnicastManager_GetSampleRate(codec_info);

           if ((LeBapUnicastServer_GetAseDirection(cid, p_enable_info[scan].aseId)) == ASE_DIRECTION_AUDIO_SINK)
           {
               snk_ase_count++;
               snk_sample_rate = sample_rate;
           }
           else
           {
               src_ase_count++;
               src_sample_rate = sample_rate;
           }
        }

        if (src_ase_count == 0)
        {
            /* For Sink ASE only, check if CCP context is not idle and the audio is not routed through LE Voice previously else determine context as MEDIA */
            determined_audio_context = leUnicastManager_IsConversationalContextReqd(current_audio_context) ? AUDIO_CONTEXT_TYPE_COVERSATIONAL : AUDIO_CONTEXT_TYPE_MEDIA;
        }
        else if (snk_ase_count == 0)
        {
            /* For Source ASE only, check if CCP context is not idle and the audio is not routed through LE Voice previously else determine context as LIVE */
            determined_audio_context = leUnicastManager_IsConversationalContextReqd(current_audio_context) ? AUDIO_CONTEXT_TYPE_COVERSATIONAL : AUDIO_CONTEXT_TYPE_LIVE;
        }
        else
        {
            determined_audio_context = src_sample_rate == snk_sample_rate ? AUDIO_CONTEXT_TYPE_COVERSATIONAL : AUDIO_CONTEXT_TYPE_GAME;
        }
    }
    else
    {
        switch (audio_context)
        {
            case AUDIO_CONTEXT_TYPE_RINGTONE:
            {
                /* Treat the Ringtone Audio context as Conversational to route it through LE Voice */
                determined_audio_context = AUDIO_CONTEXT_TYPE_COVERSATIONAL;
            }
            break;

            case AUDIO_CONTEXT_TYPE_SOUND_EFFECTS:
            case AUDIO_CONTEXT_TYPE_NOTIFICATIONS:
            case AUDIO_CONTEXT_TYPE_ALERTS:
            case AUDIO_CONTEXT_TYPE_EMERGENCY_ALARM:
            {
                /* Treat the Audio contexts as Media to route it through LE Audio */
                determined_audio_context = AUDIO_CONTEXT_TYPE_MEDIA;
            }
            break;

            default:
            {
                /* No need for determining the audio context */
                determined_audio_context = audio_context;
            }
            break;
        }
    }

    if (audio_context == (AUDIO_CONTEXT_TYPE_COVERSATIONAL | AUDIO_CONTEXT_TYPE_MEDIA))
    {
        const BapServerAseCodecInfo *codec_info = LeBapUnicastServer_GetCodecParameters(cid, p_enable_info[0].aseId);

        if (codec_info)
        {
            uint16 sample_rate = LeUnicastManager_GetSampleRate(codec_info);

            audio_context = sample_rate == SAMPLE_RATE_48000 ? AUDIO_CONTEXT_TYPE_MEDIA : AUDIO_CONTEXT_TYPE_COVERSATIONAL;
            DEBUG_LOG("leUnicastManager_DetermineAudioContext: sample_rate=%u audio_context=0x%04X", sample_rate, audio_context);
        }
    }

    UNICAST_MANAGER_LOG("leUnicastManager_DetermineAudioContext LeUnicastManager_AudioContext: 0x%x, rx from Remote audio_context: 0x%x, determined_audio_context:0x%x", current_audio_context, audio_context, determined_audio_context);

    return determined_audio_context;
}

/*!< Check if any change ase1 is routed and ase2 is not in ready to be routed state */
static bool leUnicastManager_IsAnyChangeInBetweenAseRouteState(le_um_ase_t *ase1, le_um_ase_t *ase2)
{
    bool reconfig = FALSE;

    if ((ase1->state == le_um_ase_state_routed || ase1->state == le_um_ase_state_streaming) &&
        LeUnicastManager_IsAseActive(ase2) &&
        ase2->cis_data != NULL &&
        LeUnicastManager_IsCisEstablished(ase2->cis_data->state))
    {
        reconfig = TRUE;
    }

    return reconfig;
}

/*! \brief Check and move newly Enabled Sink ASEes to streaming state */
static bool leUnicastManager_CheckAndMoveAsesToStreamingState(le_um_instance_t *inst)
{
    bool found = FALSE;
    le_um_ase_t *ase;

    ARRAY_FOREACH(ase, inst->ase)
    {
        if (ase->state == le_um_ase_state_enabling &&
            ase->cis_data != NULL &&
            LeUnicastManager_IsCisEstablished(ase->cis_data->state))
        {
            UNICAST_MANAGER_LOG("leUnicastManager_CheckAndMoveAsesToStreamingState: ASE ID %d", ase->ase_id);

            if (ase->direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK)
            {
                ase->state = le_um_ase_state_streaming;
                leUnicastManager_ExecuteReadyToReceiveIfReady(inst, ase);
            }

            found = TRUE;
        }
    }

    return found;
}

#ifdef USE_SYNERGY

/*! \brief Handle Config Change ind for ASCS */
static void leUnicastManager_HandleAscsConfigChangeInd(BapServerConfigChangeInd *ind)
{
    BapAscsConfig stored_config = {0};
    BapAscsConfig *service_config = NULL;
    uint8 size = 0;
    uint8 *data;
    bool needs_update = FALSE;

    service_config = (BapAscsConfig *) LeBapUnicastServer_GetServiceConfig(ind->connectionId, ind->configType);
    data = (uint8 *) leUnicastManager_RetrieveClientConfig(ind->connectionId);

    if (data != NULL)
    {
        stored_config = *(BapAscsConfig *) data;
        stored_config.aseCharClientCfg = (ClientConfig *) &data[offsetof(BapAscsConfig, aseCharClientCfg)];
    }

    if (service_config != NULL && service_config->aseCharClientCfg != NULL)
    {
        if (service_config->numAses == stored_config.numAses &&
            service_config->aseControlPointCharClientCfg == stored_config.aseControlPointCharClientCfg)
        {
            for (int scan = 0; scan < service_config->numAses; scan++)
            {
                if (service_config->aseCharClientCfg[scan] != stored_config.aseCharClientCfg[scan])
                {
                    needs_update = TRUE;
                    break;
                }
            }
        }
        else
        {
            needs_update = TRUE;
        }

        if (needs_update)
        {
            size = offsetof(BapAscsConfig, aseCharClientCfg) + service_config->numAses * sizeof(ClientConfig);
            data = (uint8 *) PanicUnlessMalloc(size);
            PanicNull(data);

            memcpy(data, &service_config->numAses, sizeof(service_config->numAses));

            memcpy(data + offsetof(BapAscsConfig, aseControlPointCharClientCfg),
                   &service_config->aseControlPointCharClientCfg,
                   sizeof(service_config->aseControlPointCharClientCfg));

            memcpy(data + offsetof(BapAscsConfig, aseCharClientCfg),
                   service_config->aseCharClientCfg,
                   service_config->numAses * sizeof(ClientConfig));

            leUnicastManager_StoreClientConfig(ind->connectionId, data, size);

            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler storing configType %d, size %d", ind->configType, size);
            pfree(data);
        }
        pfree (service_config->aseCharClientCfg);
    }
    pfree (service_config);
}

/*! \brief Handle Config Change ind for Pacs */
static void leUnicastManager_HandlePacsConfigChangeInd(BapServerConfigChangeInd *ind)
{
    BapPacsConfig *stored_pacs_config = NULL;
    BapPacsConfig *service_pacs_config = NULL;

    service_pacs_config = (BapPacsConfig *)LeBapUnicastServer_GetServiceConfig(ind->connectionId, ind->configType);
    stored_pacs_config = (BapPacsConfig *)LeBapPacsUtilities_RetrieveClientConfig(ind->connectionId);

    if (service_pacs_config != NULL)
    {
        if (stored_pacs_config == NULL ||
            service_pacs_config->sinkPacClientCfg1 != stored_pacs_config->sinkPacClientCfg1 ||
            service_pacs_config->sinkPacClientCfg2 != stored_pacs_config->sinkPacClientCfg2 ||
            service_pacs_config->sinkPacClientCfg3 != stored_pacs_config->sinkPacClientCfg3 ||
            service_pacs_config->sourcePacClientCfg1 != stored_pacs_config->sourcePacClientCfg1 ||
            service_pacs_config->sourcePacClientCfg2 != stored_pacs_config->sourcePacClientCfg2 ||
            service_pacs_config->sourcePacClientCfg3 != stored_pacs_config->sourcePacClientCfg3 ||
            service_pacs_config->sinkAudioLocationsClientCfg != stored_pacs_config->sinkAudioLocationsClientCfg ||
            service_pacs_config->sourceAudioLocationsClientCfg != stored_pacs_config->sourceAudioLocationsClientCfg ||
            service_pacs_config->availableAudioContextsClientCfg != stored_pacs_config->availableAudioContextsClientCfg ||
            service_pacs_config->supportedAudioContextsClientCfg != stored_pacs_config->supportedAudioContextsClientCfg ||
            service_pacs_config->vsAptXSinkPacClientCfg != stored_pacs_config->vsAptXSinkPacClientCfg ||
            service_pacs_config->vsAptXSourcePacClientCfg != stored_pacs_config->vsAptXSourcePacClientCfg)
        {
            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler storing configType %d", ind->configType);
            LeBapPacsUtilities_StoreClientConfig(ind->connectionId, (void *)service_pacs_config, sizeof(BapPacsConfig));
        }
        pfree (service_pacs_config);
    }
}
#endif

static void leUnicastManager_BapMessageHandler(Message message)
{
    BapServerPrim id;

    PanicNull((void*) message);

#ifdef USE_SYNERGY
    id = *(BapServerPrim *) message;
#else
    id = 0xFFFF;
#endif

    switch(id)
    {
        case BAP_SERVER_ASE_CODEC_CONFIGURED_IND:
        {
            BapServerAseCodecConfiguredInd *ind = (BapServerAseCodecConfiguredInd *) message;

            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler : BAP_SERVER_ASE_CODEC_CONFIGURED_IND cid=0x%x ase=0x%x direction: %d",
                           ind->connectionId, ind->aseCodecConfig.aseId, ind->aseCodecConfig.direction);
        }
        break;

        case BAP_SERVER_ASE_QOS_CONFIGURED_IND:
        {
            BapServerAseQosConfiguredInd *ind = (BapServerAseQosConfiguredInd *) message;

            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler : BAP_SERVER_ASE_QOS_CONFIGURED_IND cid=0x%x ase=0x%x cis: 0x%x",
                           ind->connectionId, ind->aseQosConfig.aseId, ind->aseQosConfig.cisId);
            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler : BAP_SERVER_ASE_QOS_CONFIGURED_IND sampleRate=%d sduSize: %d, frameDuration: %d, presentationDelay: %d",
                           ind->aseQosConfig.sampleRate, ind->aseQosConfig.sduSize, ind->aseQosConfig.frameDuration, ind->aseQosConfig.presentationDelay);
        }
        break;

        case BAP_SERVER_ASE_ENABLED_IND:
        {
            BapServerAseEnabledInd *ind = (BapServerAseEnabledInd *) message;

            if (ind->connectionId && ind->numAses)
            {
                uint8 i;
                le_um_instance_t *inst = LeUnicastManager_InstanceGetByCidOrCreate(ind->connectionId);
                GattAscsAseResultValue result = inst != NULL ? GATT_ASCS_ASE_RESULT_SUCCESS : GATT_ASCS_ASE_RESULT_UNSPECIFIED_ERROR;
                AudioContextType audio_context;
                BapServerAseResult *aseResult =  PanicUnlessMalloc(ind->numAses * sizeof(BapServerAseResult));

                audio_context = leUnicastManager_DetermineAudioContext(ind->connectionId, ind->numAses, ind->bapServerEnableIndInfo);

                for (i = 0; i < ind->numAses; i++)
                {
                    aseResult[i].aseId = ind->bapServerEnableIndInfo[i].aseId;
                    aseResult[i].additionalInfo = 0;

                    if (result == GATT_ASCS_ASE_RESULT_SUCCESS)
                    {
                        result = BapServerValidateMetadataLtvs(ind->bapServerEnableIndInfo[i].metadata,
                                                                           ind->bapServerEnableIndInfo[i].metadataLength,
                                                                           &aseResult[i].additionalInfo);
                    }

                    if (result == GATT_ASCS_ASE_RESULT_SUCCESS)
                    {
                        if (BapServerValidateStreamingContext(bapUnicastServiceHandle, ind->connectionId,
                                                              ind->bapServerEnableIndInfo[i].aseId,
                                                              ind->bapServerEnableIndInfo[i].metadata,
                                                              &ind->bapServerEnableIndInfo[i].metadataLength) &&
                            leUnicastManager_ValidateSamplingRateForASE(ind->connectionId,
                                                                        ind->bapServerEnableIndInfo[i].aseId,
                                                                        audio_context))
                        {
                            AseMetadataType metadata;

                            metadata.aseId = ind->bapServerEnableIndInfo[i].aseId;
                            metadata.metadataLength = ind->bapServerEnableIndInfo[i].metadata == NULL ? 0 : ind->bapServerEnableIndInfo[i].metadataLength;
                            metadata.metadata = ind->bapServerEnableIndInfo[i].metadata;

                            if (!leUnicastManager_AseEnabled(ind->connectionId, &metadata, audio_context))
                            {
                                result = GATT_ASCS_ASE_RESULT_UNSPECIFIED_ERROR;
                                aseResult[i].additionalInfo = 0;
                            }
                        }
                        else
                        {
                            result = GATT_ASCS_ASE_RESULT_REJECTED_METADATA;
                            aseResult[i].additionalInfo = BAP_METADATA_LTV_TYPE_STREAMING_AUDIO_CONTEXTS;
                        }
                    }

                    aseResult[i].value = result;
                    DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler : BAP_SERVER_ASE_ENABLED_IND cid=0x%x ase=0x%x cis: 0x%x, audio_context: 0x%x, result=0x%x, additional_info: 0x%x",
                                   ind->connectionId, ind->bapServerEnableIndInfo[i].aseId, ind->bapServerEnableIndInfo[i].cisId, audio_context, aseResult[i].value, aseResult[i].additionalInfo);
                }

#ifdef INCLUDE_MIRRORING
                if (result == GATT_ASCS_ASE_RESULT_SUCCESS && !leUnicastManager_UpdateMirrorType(inst))
                {
                    result = GATT_ASCS_ASE_RESULT_UNSPECIFIED_ERROR;
                }
#endif

                if (result != GATT_ASCS_ASE_RESULT_SUCCESS)
                {
                    for (i = 0; i < ind->numAses; i++)
                    {
                        if (aseResult[i].value == GATT_ASCS_ASE_RESULT_SUCCESS)
                        {
                            aseResult[i].value = result;
                            leUnicastManager_AseDisabled(ind->connectionId, ind->bapServerEnableIndInfo[i].aseId);
                        }
                    }
                }

                BapServerUnicastAseEnableRsp(bapUnicastServiceHandle, ind->connectionId, ind->numAses, aseResult);

                if (result == GATT_ASCS_ASE_RESULT_SUCCESS && inst != NULL)
                {
                    /* Send a LE audio enable message to registered clients */
                    LeAudioMessages_SendUnicastEnabled(Multidevice_GetPairSide());

                    /* Check and move newly enabled ASEes to Streaming state */
                    leUnicastManager_CheckAndMoveAsesToStreamingState(inst);

                    /* Check if both CIS and data path are ready for this ASE earlier along with previoulsy enabled ASE */
                    if (LeUnicastManager_IsReconfigRequired(inst, Multidevice_GetSide()) ||
                        LeUnicastManager_IsReconfigRequired(inst, Multidevice_GetPairSide()))
                    {
                        /* Need to reconfigure the graph as new ASEs in CIS are enabled here */
                        if (LeUnicastManager_IsContextTypeConversational(audio_context))
                        {                          
                            LeUnicastVoiceSource_Reconfig(inst);
                        }
                        else
                        {
                            LeUnicastMusicSource_Reconfig(inst);
                        }
                    }
                    else
                    {
                        /* If all CISes were established earlier, then data path for them */
                        leUnicastManager_CheckAndEstablishDataPath(inst, TRUE);
                    }
                }

                pfree(aseResult);
            }
        }
        break;

        case BAP_SERVER_CIS_ESTABLISHED_IND:
        {
            BapServerCisEstablishedInd *ind = (BapServerCisEstablishedInd *) message;
            uint8 dir;

            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler BAP_SERVER_CIS_ESTABLISHED_IND cid: 0x%x cis: 0x%x, handle: 0x%x",
                           ind->connectionId, ind->cisId, ind->cisHandle);

            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler CIS Params cigSyncDelay: 0x%x, cisSyncDelay: 0x%x, isoInt: 0x%x, nse: 0x%x",
                           ind->cisParams.cigSyncDelay, ind->cisParams.cisSyncDelay, ind->cisParams.isoInterval, ind->cisParams.nse);
            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler CIS Params MtoS transLat: 0x%x, maxPDU: 0x%x, phy: 0x%x, bn: 0x%x, ft: 0x%x",
                           ind->cisParams.transportLatencyMtoS, ind->cisParams.maxPduMtoS, ind->cisParams.phyMtoS, ind->cisParams.bnMtoS,
                           ind->cisParams.ftMtoS);
            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler CIS Params StoM transLat: 0x%x, maxPDU: 0x%x, phy: 0x%x, bn: 0x%x, ft: 0x%x",
                           ind->cisParams.transportLatencyStoM, ind->cisParams.maxPduStoM, ind->cisParams.phyStoM, ind->cisParams.bnStoM,
                           ind->cisParams.ftStoM);

            dir = ind->cisParams.bnMtoS != 0 ? LE_UM_CIS_DIRECTION_DL : 0;
            dir |= ind->cisParams.bnStoM != 0 ? LE_UM_CIS_DIRECTION_UL : 0;
            leUnicastManager_CisEstablished(ind->connectionId, ind->cisId, ind->cisHandle, dir);
        }
        break;

        case BAP_SERVER_SETUP_DATA_PATH_CFM:
        {
            BapServerSetupDataPathCfm *cfm = (BapServerSetupDataPathCfm *) message;

            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler BAP_SERVER_SETUP_DATA_PATH_CFM handle: 0x%x, status: 0x%x",
                           cfm->isoHandle, cfm->status);
            if (cfm->status == BAP_SERVER_STATUS_SUCCESS)
            {
                leUnicastManager_DataPathCreated(cfm->isoHandle);
            }
        }
        break;

        case BAP_SERVER_CIS_DISCONNECTED_IND:
        {
            BapServerCisDisconnectedInd *ind = (BapServerCisDisconnectedInd *) message;
            le_um_instance_t *inst = LeUnicastManager_InstanceGetByCisHandle(ind->cisHandle);

            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler BAP_SERVER_CIS_DISCONNECTED_IND CIS handle: 0x%x, reason: 0x%x",
                           ind->cisHandle, ind->reason);
            if (inst != NULL &&
               (ind->reason == hci_error_conn_timeout || ind->reason == hci_error_lmp_response_timeout))
            {
                le_um_internal_msg_t msg = LeUnicastManager_GetCisLinklossMessageForInst(inst);
                MessageCancelFirst(LeUnicastManager_GetTask(), msg);
                MessageSendLater(LeUnicastManager_GetTask(), msg, NULL,
                                 LeUnicastManager_CisLinklossConfirmationTimeout());
            }

            leUnicastManager_CisDisconnected(ind->cisHandle);
        }
        break;

        case BAP_SERVER_CIS_DISCONNECTED_CFM:
        {
            BapServerCisDisconnectedCfm *cfm = (BapServerCisDisconnectedCfm *) message;

            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler BAP_SERVER_CIS_DISCONNECTED_CFM CIS handle: 0x%x, reason: 0x%x",
                           cfm->cisHandle, cfm->status);
            leUnicastManager_CisDisconnected(cfm->cisHandle);
        }
        break;

        case BAP_SERVER_ASE_RECEIVER_START_READY_IND:
        {
            BapServerAseReceiverStartReadyInd *ind = (BapServerAseReceiverStartReadyInd *)message;

            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler : BAP_SERVER_ASE_RECEIVER_START_READY_IND cid=0x%x ase=0x%x",
                           ind->connectionId, ind->aseId);
            leUnicastManager_AseReceiverReady(ind->connectionId, ind->aseId);
        }
        break;

        case BAP_SERVER_ASE_UPDATE_METADATA_IND:
        {
            BapServerAseUpdateMetadataInd *ind = (BapServerAseUpdateMetadataInd *) message;
            AudioContextType audioContext;
            GattAscsAseResultValue result = GATT_ASCS_ASE_RESULT_SUCCESS;
            uint8 i;

            if (ind->connectionId && ind->numAses)
            {
                BapServerAseResult *aseResult =  PanicUnlessMalloc(ind->numAses * sizeof(BapServerAseResult));
                for(i = 0; i < ind->numAses; i++)
                {
                    aseResult[i].aseId = ind->bapServerUpdateMetadataInfo[i].aseId;
                    aseResult[i].additionalInfo = 0;

                    result = BapServerValidateMetadataLtvs(ind->bapServerUpdateMetadataInfo[i].metadata,
                                                           ind->bapServerUpdateMetadataInfo[i].metadataLength,
                                                           &aseResult[i].additionalInfo);
                    audioContext = BapServerLtvUtilitiesGetStreamingAudioContext(ind->bapServerUpdateMetadataInfo[i].metadata, ind->bapServerUpdateMetadataInfo[i].metadataLength);

                    if (result == GATT_ASCS_ASE_RESULT_SUCCESS)
                    {
                        if (!BapServerValidateStreamingContext(bapUnicastServiceHandle, ind->connectionId,
                                                              ind->bapServerUpdateMetadataInfo[i].aseId,
                                                              ind->bapServerUpdateMetadataInfo[i].metadata,
                                                              &ind->bapServerUpdateMetadataInfo[i].metadataLength))
                        {
                            result = GATT_ASCS_ASE_RESULT_REJECTED_METADATA;
                            aseResult[i].additionalInfo = BAP_METADATA_LTV_TYPE_STREAMING_AUDIO_CONTEXTS;
                        }
                    }

                    /* Accept the valid metadata update */
                    aseResult[i].value = result;

                    DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler : BAP_SERVER_ASE_UPDATE_METADATA_IND ase=0x%x audio_context = 0x%x, result = 0x%x, additional_info: 0x%x",
                                    ind->bapServerUpdateMetadataInfo[i].aseId, audioContext, aseResult[i].value, aseResult[i].additionalInfo);
                }
                BapServerUnicastAseUpdateMetadataRsp(bapUnicastServiceHandle, ind->connectionId, ind->numAses, aseResult);
                pfree(aseResult);
            }
        }
        break;

        case BAP_SERVER_ASE_DISABLED_IND:
        {
            BapServerAseDisabledInd *ind = (BapServerAseDisabledInd *) message;

            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler : BAP_SERVER_ASE_DISABLED_IND cid=0x%x ase=0x%x",
                           ind->connectionId, ind->aseId);
            leUnicastManager_AseDisabled(ind->connectionId, ind->aseId);
        }
        break;

        case BAP_SERVER_ASE_RECEIVER_STOP_READY_IND:
        {
            BapServerAseReceiverStopReadyInd *ind = (BapServerAseReceiverStopReadyInd *) message;

            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler : BAP_SERVER_ASE_RECEIVER_STOP_READY_IND cid=0x%x ase=0x%x",
                           ind->connectionId, ind->aseId);
            leUnicastManager_AseReceiverStop(ind->connectionId, ind->aseId);
        }
        break;

        case BAP_SERVER_ASE_RELEASED_IND:
        {
            BapServerAseReleasedInd *ind = (BapServerAseReleasedInd *) message;

            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler : BAP_SERVER_ASE_RELEASED_IND cid=0x%x ase=0x%x",
                           ind->connectionId, ind->aseId);
            leUnicastManager_AseReleased(ind->connectionId, ind->aseId);
        }
        break;

        case BAP_SERVER_REMOVE_DATA_PATH_CFM:
        {
            BapServerRemoveDataPathCfm *cfm = (BapServerRemoveDataPathCfm *) message;

            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler BAP_SERVER_REMOVE_DATA_PATH_CFM handle: 0x%x, status: 0x%x",
                           cfm->isoHandle, cfm->status);

            if (cfm->status == BAP_SERVER_STATUS_SUCCESS)
            {
                leUnicastManager_DataPathRemoved(cfm->isoHandle);
            }
        }
        break;

#ifdef USE_SYNERGY
        case BAP_SERVER_CONFIG_CHANGE_IND:
        {
            BapServerConfigChangeInd *ind = (BapServerConfigChangeInd *) message;
            le_um_instance_t *inst = LeUnicastManager_InstanceGetByCidOrCreate(ind->connectionId);
#ifdef ENABLE_LEA_TARGETED_ANNOUNCEMENT
            device_t device = GattConnect_GetBtDevice(ind->connectionId);
#endif
            UNUSED(inst);
            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler BAP_SERVER_CONFIG_CHANGE_IND cid: 0x%x, configType %d, changeCmplt: %d, inst: %d",
                           ind->connectionId, ind->configType, ind->configChangeComplete, inst);

            if (ind->configChangeComplete)
            {
                if (ind->configType == BAP_SERVER_CONFIG_ASCS)
                {
                    leUnicastManager_HandleAscsConfigChangeInd(ind);
                }
                else if (ind->configType == BAP_SERVER_CONFIG_PACS)
                {
                    leUnicastManager_HandlePacsConfigChangeInd(ind);
                }

#ifdef ENABLE_LEA_TARGETED_ANNOUNCEMENT
                /* Mark this device as LEA capable */
                if (device != NULL)
                {
                    BtDevice_AddSupportedProfilesToDevice(device, DEVICE_PROFILE_LE_AUDIO);
                }
#endif
            }
        }
        break;
#endif

        default:
            DEBUG_LOG_INFO("leUnicastManager_BapMessageHandler: unhandled message 0x%04X", id);
        break;
    }
}

static void leUnicastManager_HandleMuteInd(MICP_SERVER_MUTE_IND_T *ind)
{
    uint8 mute_state = ind->mute_state;
    /* TBD: Add the GATT cid to the MICP_SERVER_MUTE_IND_T and use that to get the context. */
    le_um_instance_t *inst = LeUnicastManager_GetInstance();
    AudioContextType audio_context = inst->audio_context;
    voice_source_t source = voice_source_le_audio_unicast_1;
    bool mic_active = FALSE;

    if (!LeUnicastManager_IsStreamingActive())
    {
        return;
    }

    if (LeUnicastManager_IsContextTypeConversational(audio_context))
    {
        if (VoiceSources_IsAudioRouted(source))
        {
            /* Notify Telephony mute state is active /inactive
               TODO : Expand this message as a generic for Gaming Mode + VBC and SREC use case */
            if (mute_state)
            {
                Telephony_NotifyMicrophoneMuted(source);
            }
            else
            {
                Telephony_NotifyMicrophoneUnmuted(source);
            }

            mic_active = TRUE;
        }
    }
    else if (LeUnicastManager_IsSourceAseActive(inst))
    {
        mic_active = TRUE;
    }

    DEBUG_LOG("leUnicastManager_HandleMuteInd mute-state:=%d, mic-active: %d", ind->mute_state, mic_active);
    if (mic_active)
    {
        KymeraLeAudioVoice_SetMicMuteState(mute_state);

#ifdef ENABLE_LEA_CIS_DELEGATION
        MirrorProfile_SendMicMute(mute_state);
#endif
    }
}

static void leUnicastManager_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
        case BAP_SRVR_PRIM:
            leUnicastManager_BapMessageHandler(message);
            break;

        case MICP_SERVER_MUTE_IND:
            leUnicastManager_HandleMuteInd((MICP_SERVER_MUTE_IND_T *)message);
            break;

        default:
            break;
    }
}

static void leUnicastManager_OnGattConnect(gatt_cid_t cid)
{
    UNUSED(cid);

    TimestampEvent(TIMESTAMP_EVENT_LE_UNICAST_ACL_CONNECT);
}

static void leUnicastManager_OnGattDisconnect(gatt_cid_t cid)
{
    BapAscsConfig *config = BapServerUnicastRemoveAscsConfig(bapUnicastServiceHandle, cid);

    UNICAST_MANAGER_LOG("leUnicastManager_OnGattDisconnect cid=0x%x", cid);

    if(config)
    {
#ifdef USE_SYNERGY
        uint8 size = offsetof(BapAscsConfig, aseCharClientCfg) + config->numAses * sizeof(ClientConfig);
        uint8 *data = (uint8 *) PanicUnlessMalloc(size);

        memcpy(data, &config->numAses, sizeof(config->numAses));
        memcpy(data + offsetof(BapAscsConfig, aseControlPointCharClientCfg),
               &config->aseControlPointCharClientCfg,
               sizeof(config->aseControlPointCharClientCfg));
        memcpy(data + offsetof(BapAscsConfig, aseCharClientCfg),
               config->aseCharClientCfg,
               config->numAses * sizeof(ClientConfig));

        leUnicastManager_StoreClientConfig(cid, data, size);
        pfree(data);

        pfree(config->aseCharClientCfg);
#endif
        pfree(config);
    }

    leUnicastManager_CidDisconnected(cid);
}

static void leUnicastManager_OnEncryptionChanged(gatt_cid_t cid, bool encrypted)
{
    if (encrypted && !GattConnect_IsDeviceTypeOfPeer(cid))
    {
        uint8 *data = NULL;
        BapAscsConfig config = {0};
        bapStatus status;

#ifdef USE_SYNERGY
        data = (uint8 *)leUnicastManager_RetrieveClientConfig(cid);

        if (data != NULL)
        {
            memcpy(&config.numAses, data, sizeof(config.numAses));
            memcpy(&config.aseControlPointCharClientCfg,
                   data + leUnicastManager_GetCCCDOffset(),
                   sizeof(config.aseControlPointCharClientCfg));
            config.aseCharClientCfg = (ClientConfig*) &data[leUnicastManager_GetClientCfgOffset()];
        }
#else
        (void) config;
#endif
        UNICAST_MANAGER_LOG("leUnicastManager_OnEncryptionChanged add ASCS config handle=0x%p cid=0x%x config=0x%x",
                            bapUnicastServiceHandle, cid, data);

        status = BapServerUnicastAddAscsConfig(bapUnicastServiceHandle, cid, data != NULL ? &config : NULL);
        if (status != BAP_SERVER_STATUS_SUCCESS)
        {
            UNICAST_MANAGER_LOG("leUnicastManager_OnEncryptionChanged add ASCS config failed status=%d", status);
            Panic();
        }
    }
}

void LeUnicastManagerTask_Init(void)
{
    LeUnicastManager_TaskReset();

    LeBapUnicastServer_Init(NUMBER_OF_ASES_REQUIRED, LeUnicastManager_GetTask());
#ifdef ENABLE_TMAP_PROFILE
    TmapProfile_Init();
#endif
    MicpServer_Init();
    MicpServer_ClientRegister(LeUnicastManager_GetTask());
    GattConnect_RegisterObserver(&leUnicastManager_connect_callbacks);
    GattConnect_UpdateMinAcceptableMtu(leUnicastManagerConfig_GattMtuMinimum());
}

void LeUnicastManager_TaskReset(void)
{
    le_um_task_data_t *task_data = LeUnicastManager_GetTaskData();
    le_um_instance_t *inst = NULL;

    memset(task_data, 0, sizeof(*task_data));
    LeUnicastManager_GetTask()->handler = leUnicastManager_MessageHandler;

    ARRAY_FOREACH(inst, task_data->le_unicast_instances)
    {
        LeUnicastManager_InstanceReset(inst);
    }

    task_data->audio_interface_state = source_state_invalid;
}

bool LeUnicastManager_IsReconfigRequired(le_um_instance_t *inst, multidevice_side_t side)
{
    bool is_reconfig_needed = FALSE;
    le_um_ase_t *sink_ase = NULL;
    le_um_ase_t *source_ase = NULL;

    leUnicastManager_GetAseFromSide(inst, side, &sink_ase, &source_ase);

    if(leUnicastManager_IsAnyChangeInBetweenAseRouteState(source_ase, sink_ase) ||
       leUnicastManager_IsAnyChangeInBetweenAseRouteState(sink_ase, source_ase))
    {
        is_reconfig_needed = TRUE;
    }

    DEBUG_LOG_INFO("LeUnicastManager_IsReconfigRequired side: %d, is_reconfig_needed: %d", side, is_reconfig_needed);

    return is_reconfig_needed;
}

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST) */

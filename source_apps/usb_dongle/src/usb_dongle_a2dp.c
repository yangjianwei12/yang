/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      application A2DP source module
*/

#include "usb_dongle_logging.h"

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
#include "usb_dongle_config.h"
#include "wired_audio_source.h"
#include "unexpected_message.h"
#include "usb_dongle_a2dp.h"
#include "usb_dongle_audio.h"
#include "chain.h"
#include "chain_input_wired_sbc_encode.h"
#include "chain_input_wired_aptx_adaptive_encode.h"
#include "chain_input_wired_aptxhd_encode.h"
#include "chain_input_wired_aptx_classic_encode.h"
#include "kymera_chain_roles.h"
#include "av.h"
#include "avrcp_profile.h"
#include "device_list.h"
#include "pairing.h"
#include <ui.h>
#include "bt_device.h"
#include "bt_device_class.h"
#include "device_properties.h"
#include <app/bluestack/hci.h>
#include "connection_manager.h"
#include <power_manager.h>
#include "usb_dongle_volume_observer.h"
#include "usb_dongle_hfp.h"
#include "sink_service.h"

#define LATENCY_TARGET_DEFAULT  150
#define LATENCY_OFFSET_MINIMUM  10

/* The min/max offset of latency is maximum of LATENCY_MIN_MAX_OFFSET_MINIMUM and 10% of Target Latency */
#define LATENCY_MIN_MAX_OFFSET(target_latency) MAX(LATENCY_OFFSET_MINIMUM, target_latency / 10)

usb_dongle_a2dp_source_data_t a2dp_source_data;

static unsigned usbDongleA2dp_GetMediaStateContext(void)
{
    audio_source_provider_context_t context = BAD_CONTEXT;

    switch(a2dp_source_data.state)
    {
        case APP_A2DP_STATE_DISCONNECTED:
        case APP_A2DP_STATE_CONNECTING:
        case APP_A2DP_STATE_CONNECTED:
            context = context_audio_disconnected;
            break;

        case APP_A2DP_STATE_CONNECTED_MEDIA:
            context = context_audio_connected;
            break;

        case APP_A2DP_STATE_STREAMING:
            context = context_audio_is_streaming;
            break;

        default:
            break;
     }

    return (unsigned)context;
}

static void usbDongle_HandleAudioStopComplete(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleAudioStopComplete, enum:usb_dongle_a2dp_source_state_t:%d", a2dp_source_data.state);

    if (a2dp_source_data.state == APP_A2DP_STATE_STREAMING)
    {
        /* Move out of USB_DONGLE_A2DP_SOURCE_STATE_STREAMING */
        a2dp_source_data.state = APP_A2DP_STATE_CONNECTED_MEDIA;
        Ui_InformContextChange(ui_provider_media_player, usbDongleA2dp_GetMediaStateContext());
    }
}

static void usbDongle_HandleAvA2dpConnectedIndication(AV_A2DP_CONNECTED_IND_T *msg)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleAvA2dpConnectedIndication, enum:usb_dongle_a2dp_source_state_t:%d", a2dp_source_data.state);
    a2dp_source_data.active_av_instance = msg->av_instance;
    a2dp_source_data.state = APP_A2DP_STATE_CONNECTED;
    a2dp_source_data.connected_cb();
    UsbDongle_A2dpUpdatePreferredSampleRate();
}

static void usbDongle_HandleAvA2dpDisonnectedIndication(AV_A2DP_DISCONNECTED_IND_T *ind)
{
    /* AV will normally disconnect AVRCP of its own accord after a couple of
       seconds. However, to speed the process up, we explicitly request an
       immediate disconnection of AVRCP here, so that we can accept new
       connections again as quickly as possible. */

    DEBUG_LOG_FN_ENTRY("usbDongle_HandleAvA2dpDisonnectedIndication, enum:usb_dongle_a2dp_source_state_t:%d", a2dp_source_data.state);

    if (appAvrcpIsConnected(ind->av_instance))
    {
        /* Since AV was the initiator of the original connection, AVRCP expects
           AV's task to be the one to request disconnection. */
        Task client_task = &ind->av_instance->av_task;

        if (!appAvAvrcpDisconnectRequest(client_task, ind->av_instance))
        {
            DEBUG_LOG_ERROR("usbDongle_HandleAvA2dpDisonnectedIndication, appAvAvrcpDisconnectRequest() failed");
        }
    }

    a2dp_source_data.active_av_instance = NULL;
    a2dp_source_data.state = APP_A2DP_STATE_DISCONNECTED;
    /* Update UI context in case we just dropped out of streaming */
    Ui_InformContextChange(ui_provider_media_player, usbDongleA2dp_GetMediaStateContext());
    a2dp_source_data.disconnected_cb();
}

static void usbDongle_HandleAvA2dpAudioConnected(AV_A2DP_AUDIO_CONNECT_MESSAGE_T *msg)
{
   	audio_source_t source_type = UsbDongle_AudioDetermineNewSource();

    DEBUG_LOG_FN_ENTRY("usbDongle_HandleAvA2dpAudioConnected, enum:audio_source_t:%d", msg->audio_source);

    PanicFalse(msg->audio_source == Av_GetSourceForInstance(a2dp_source_data.active_av_instance));

    source_defined_params_t a2dp_audio_source_paramaters;
    AudioSources_GetConnectParameters(msg->audio_source,&a2dp_audio_source_paramaters);
    a2dp_connect_parameters_t * a2dp_connect_params = a2dp_audio_source_paramaters.data;

    /* Get the a2dp_codec_settings from the a2dp_connect_parameters */
    a2dp_source_data.a2dp_settings.channel_mode = (a2dp_channel_mode)a2dp_connect_params->channel_mode;
    a2dp_source_data.a2dp_settings.remote_mtu = a2dp_connect_params->remote_mtu;
    a2dp_source_data.a2dp_settings.codecData.bitpool = a2dp_connect_params->bitpool;
    a2dp_source_data.a2dp_settings.codecData.format = a2dp_connect_params->format;
    a2dp_source_data.a2dp_settings.codecData.packet_size = a2dp_connect_params->packet_size;
    a2dp_source_data.a2dp_settings.codecData.content_protection = a2dp_connect_params->content_protection;
    a2dp_source_data.a2dp_settings.codecData.aptx_ad_params.q2q_enabled = a2dp_connect_params->q2q_mode;
    a2dp_source_data.a2dp_settings.codecData.aptx_ad_params.nq2q_ttp = a2dp_connect_params->nq2q_ttp;
    a2dp_source_data.a2dp_settings.codecData.aptx_ad_params.version = a2dp_connect_params->aptx_version;
    a2dp_source_data.a2dp_settings.codecData.aptx_ad_params.features = a2dp_connect_params->aptx_features;

    a2dp_source_data.a2dp_settings.rate = a2dp_connect_params->rate;
    a2dp_source_data.a2dp_settings.seid = a2dp_connect_params->seid;
    a2dp_source_data.a2dp_settings.sink = a2dp_connect_params->sink;

    AudioSources_ReleaseConnectParameters(msg->audio_source,
                                            &a2dp_audio_source_paramaters);

    wired_audio_config_t wired_audio_config;

    DEBUG_LOG_INFO("usbDongle_HandleAvA2dpAudioConnected, audio connect parameters:");
    switch (a2dp_source_data.a2dp_settings.seid)
    {
        case AV_SEID_SBC_SRC:
            DEBUG_LOG_INFO("  codec SBC");
            DEBUG_LOG_INFO("  packet_size %u", a2dp_source_data.a2dp_settings.codecData.packet_size);
            DEBUG_LOG_INFO("  bitpool %u", a2dp_source_data.a2dp_settings.codecData.bitpool);
            DEBUG_LOG_INFO("  format %u", a2dp_source_data.a2dp_settings.codecData.format);
            break;

        case AV_SEID_APTX_CLASSIC_SRC:
            DEBUG_LOG_INFO("  codec aptX Classic");
            DEBUG_LOG_INFO("  packet_size %u", a2dp_source_data.a2dp_settings.codecData.packet_size);
            break;

        case AV_SEID_APTXHD_SRC:
            DEBUG_LOG_INFO("  codec aptX HD");
            DEBUG_LOG_INFO("  packet_size %u", a2dp_source_data.a2dp_settings.codecData.packet_size);
            break;

        case AV_SEID_APTX_ADAPTIVE_SRC:
            DEBUG_LOG_INFO("  codec aptX Adaptive");
            DEBUG_LOG_INFO("  mtu %u", a2dp_source_data.a2dp_settings.remote_mtu);
            break;

        default:
            DEBUG_LOG_INFO("  codec UNKNOWN");
            DEBUG_LOG_INFO("  packet_size %u", a2dp_source_data.a2dp_settings.codecData.packet_size);
            break;
    }
    DEBUG_LOG_INFO("  rate %u", a2dp_source_data.a2dp_settings.rate);
    DEBUG_LOG_INFO("  channel_mode enum:a2dp_channel_mode:%d", a2dp_source_data.a2dp_settings.channel_mode);
    DEBUG_LOG_INFO("  qhs active %d", ConManagerGetQhsConnectStatus(&a2dp_source_data.active_av_instance->bd_addr ));
    DEBUG_LOG_INFO("  source enum:audio_source_t:%d", source_type );

    /*
    ** Configure the latencies based on the CODEC, source, and sink; or
    ** if a special test mode is being used
    */
    if (a2dp_source_data.a2dp_settings.seid == AV_SEID_APTX_ADAPTIVE_SRC)  /* aptX Adaptive */
    {
        bool is_headset = !(a2dp_source_data.a2dp_settings.codecData.aptx_ad_params.features & aptx_ad_twm_support);

        if ( is_headset )
        {
            DEBUG_LOG_INFO("  sink headset");
        }
        else
        {
            DEBUG_LOG_INFO("  sink twm");
        }

        /* Set the TTP target latency based on the desired aptX adaptive quality
           mode. Note that LL mode will only be engaged if the sample rate is
           exactly 48kHz, hence the need for an explicit check. At other sample
           rates, HQ mode will be forced, overriding any user preference. */
        if ((Kymera_AptxAdEncoderGetDesiredQualityMode() == aptxad_mode_low_latency) &&
            (a2dp_connect_params->rate == APTXAD_REQUIRED_SAMPLE_RATE_LL_MODE))
        {
            /* In LL mode, determine TTP target latency based on source and sink types. */
            switch (source_type)
            {
                case audio_source_line_in:
                    if (is_headset)
                    {
                        wired_audio_config.target_latency = APP_CONFIG_LATENCY_TARGET_APTX_AD_LL_ANALOG_TO_HS;
                    }
                    else
                    {
                        wired_audio_config.target_latency = APP_CONFIG_LATENCY_TARGET_APTX_AD_LL_ANALOG_TO_TWM;
                    }
                    break;

                case audio_source_usb:
                    if (is_headset)
                    {
                        wired_audio_config.target_latency = APP_CONFIG_LATENCY_TARGET_APTX_AD_LL_USB_TO_HS;
                    }
                    else
                    {
                        wired_audio_config.target_latency = APP_CONFIG_LATENCY_TARGET_APTX_AD_LL_USB_TO_TWM;
                    }
                    break;

                default:
                    wired_audio_config.target_latency = LATENCY_TARGET_DEFAULT;
                    break;
            }
        }
        else
        {
            /* In HQ mode, TTP target latency is fixed regardless of sink/source types. */
            wired_audio_config.target_latency = APP_CONFIG_LATENCY_TARGET_APTX_AD_HQ_MODE;
        }
    }
    else    /* SBC, aptX Classic */
    {
        wired_audio_config.target_latency = LATENCY_TARGET_DEFAULT;
    }

    /*
     * The use_saved flag is used for either:
     * (a) replacing the calculated target latency with a test value set by automated test, or
     * (b) saving the calculated value so automated test can retrieve it
     */
    if ( a2dp_source_data.target_latency.use_saved ) /* test override */
    {
        wired_audio_config.target_latency = a2dp_source_data.target_latency.latency;
    }
    else    /* use the calculated value, but save it for retrieval */
    {
        a2dp_source_data.target_latency.latency = wired_audio_config.target_latency;
    }

    /* min/max are relative to the Target Latency, so calculate them last.*/
    wired_audio_config.min_latency    = wired_audio_config.target_latency - LATENCY_MIN_MAX_OFFSET(wired_audio_config.target_latency);
    wired_audio_config.max_latency    = wired_audio_config.target_latency + LATENCY_MIN_MAX_OFFSET(wired_audio_config.target_latency);


    /* set the rate */
    wired_audio_config.rate = a2dp_source_data.a2dp_settings.rate;

    /* Set A2DP parameters */
    Kymera_SetA2dpOutputParams(&a2dp_source_data.a2dp_settings);

    /* Clear the audio lockbits */
    appA2dpClearAudioStartLockBit(a2dp_source_data.active_av_instance);


    /* Start audio source if the voice source is not present */
    if ((UsbDongle_VoiceDetermineNewSource() == voice_source_none) &&
        (!UsbDongle_AudioStart(&wired_audio_config)))
    {
        DEBUG_LOG_INFO("usbDongle_HandleAvA2dpAudioConnected, failed to start audio, suspending");
        UsbDongle_A2dpSourceSuspend();
        return;
    }

    a2dp_source_data.state = APP_A2DP_STATE_STREAMING;
    Ui_InformContextChange(ui_provider_media_player, usbDongleA2dp_GetMediaStateContext());
}

static void usbDongle_HandleAvA2dpAudioDisconnected(AV_A2DP_AUDIO_DISCONNECT_MESSAGE_T *msg)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleAvA2dpAudioDisconnected, audioSource %d, enum:usb_dongle_a2dp_source_state_t:%d", msg->audio_source, a2dp_source_data.state);

    /* Stop audio, if not already stopped */
    if (UsbDongle_AudioIsActive())
    {
        UsbDongle_AudioStop(usbDongle_HandleAudioStopComplete);
    }
    else
    {
        a2dp_source_data.state = APP_A2DP_STATE_CONNECTED_MEDIA;
        Ui_InformContextChange(ui_provider_media_player, usbDongleA2dp_GetMediaStateContext());
    }
}

static void usbDongle_HandleAvStreamingActive(const AV_STREAMING_ACTIVE_IND_T *msg)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleAvStreamingActive, enum:usb_dongle_a2dp_source_state_t:%d", a2dp_source_data.state);

    PanicFalse(msg->audio_source == Av_GetSourceForInstance(a2dp_source_data.active_av_instance));

    /* The sink has accepted our AVDTP Start Request, so we can now start
     * streaming media packets over the air. */
    if (!Kymera_A2dpSourceAllowMediaStreaming(TRUE))
    {
        DEBUG_LOG_FN_ENTRY("usbDongle_HandleAvStreamingActive, failed to start media stream");
    }

    /* Notify the sink that we're now PLAYING.
     * Make sure someone else hasn't already sent the same notification */
    avrcp_play_status status = a2dp_source_data.active_av_instance->avrcp.play_status;

    if (status != avrcp_play_status_playing)
    {
        appAvAvrcpPlayStatusNotification(a2dp_source_data.active_av_instance, avrcp_play_status_playing);
    }

    /* In case streaming somehow started without audio,
     * make sure to suspend */
    if (a2dp_source_data.state != APP_A2DP_STATE_STREAMING)
    {
        UsbDongle_A2dpSourceSuspend();
    }
}

static void usbDongle_HandleAvStreamingInactive(const AV_STREAMING_INACTIVE_IND_T *msg)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleAvStreamingInactive");

    PanicFalse(msg->audio_source == Av_GetSourceForInstance(a2dp_source_data.active_av_instance));

    avrcp_play_status status = a2dp_source_data.active_av_instance->avrcp.play_status;

    /* A2DP is no longer in streaming state. */
    Kymera_A2dpSourceAllowMediaStreaming(FALSE);

    /* Notify the sink that we're now PAUSED/STOPPED.
     * Make sure someone else hasn't already sent the same notification */
    if (status != avrcp_play_status_paused && status != avrcp_play_status_stopped)
    {
        status = UsbDongle_AudioInputIsConnected(usb_dongle_audio_input_usb) ?
                                        avrcp_play_status_paused : avrcp_play_status_stopped;

        appAvAvrcpPlayStatusNotification(a2dp_source_data.active_av_instance, status);
    }
}

static void usbDongle_HandleAvA2dpMediaConnected(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleAvA2dpMediaConnected");

    a2dp_source_data.state = APP_A2DP_STATE_CONNECTED_MEDIA;
    Ui_InformContextChange(ui_provider_media_player, usbDongleA2dp_GetMediaStateContext());
}

static void usbDongle_HandleAvA2dpConnectedConfirm(AV_A2DP_CONNECT_CFM_T *msg)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleAvA2dpConnectedConfirm");
    if (msg->successful)
    {
        a2dp_source_data.connected_cb();
    }
}

static void usbDongle_HandleSourceMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    /* Handle Connection Library messages that are not sent directly to
       the requestor */
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleSourceMessage, enum:av_status_messages:%d", id);

    switch (id)
    {
        case AV_A2DP_CONNECTED_IND:
            usbDongle_HandleAvA2dpConnectedIndication((AV_A2DP_CONNECTED_IND_T *) message);
        break;
        case AV_A2DP_DISCONNECTED_IND:
            usbDongle_HandleAvA2dpDisonnectedIndication((AV_A2DP_DISCONNECTED_IND_T *) message);
        break;
        case AV_A2DP_AUDIO_CONNECTED:
            usbDongle_HandleAvA2dpAudioConnected((AV_A2DP_AUDIO_CONNECT_MESSAGE_T *) message);
        break;
        case AV_A2DP_AUDIO_DISCONNECTED:
            usbDongle_HandleAvA2dpAudioDisconnected((AV_A2DP_AUDIO_DISCONNECT_MESSAGE_T *) message);
        break;
        case AV_STREAMING_ACTIVE_IND:
            usbDongle_HandleAvStreamingActive((AV_STREAMING_ACTIVE_IND_T *) message);
        break;
        case AV_STREAMING_INACTIVE_IND:
            usbDongle_HandleAvStreamingInactive((AV_STREAMING_INACTIVE_IND_T *) message);
        break;
        case AV_A2DP_MEDIA_CONNECTED:
            usbDongle_HandleAvA2dpMediaConnected();
        break;
        case AV_A2DP_CONNECT_CFM:
            usbDongle_HandleAvA2dpConnectedConfirm((AV_A2DP_CONNECT_CFM_T *) message);
        break;
        default:
            UnexpectedMessage_HandleMessage(id);
        break;
    }
}

bool UsbDongle_A2dpSourceConnect(void)
{
    avInstanceTaskData * inst = a2dp_source_data.active_av_instance;

    if (inst)
    {
        DEBUG_LOG_INFO("UsbDongle_A2dpSourceConnect, IsConnected:%d, IsConnectedMedia:%d", appA2dpIsConnected(inst), appA2dpIsConnectedMedia(inst));
        if(appA2dpIsConnected(inst) && (!appA2dpIsConnectedMedia(inst)))
        {
            return appAvA2dpMediaConnectRequest(inst);
        }
    }

    return FALSE;
}

bool UsbDongle_A2dpSourceSuspend(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_A2dpSourceSuspend");

    avInstanceTaskData * inst = a2dp_source_data.active_av_instance;
    if (inst)
    {
        if (A2dpProfile_IsMediaSourceConnected(inst))
        {
            DEBUG_LOG_INFO("UsbDongle_A2dpSourceSuspend, suspending AV streaming");
            A2dpProfile_SuspendMedia(inst);

            if(a2dp_source_data.state == APP_A2DP_STATE_CONNECTED_MEDIA &&
                    !appA2dpIsKymeraOnInState((inst)->a2dp.state))
            {
                /* Even if there were pending Resume request, it shall be cancelled by above Suspend request. */
                DEBUG_LOG_INFO("UsbDongle_A2dpSourceSuspend, Streaming not initiated, a2dp state enum:avA2dpState:0x%x", (inst)->a2dp.state);
                Ui_InformContextChange(ui_provider_media_player, usbDongleA2dp_GetMediaStateContext());
            }

            return TRUE;
        }

        DEBUG_LOG_INFO("UsbDongle_A2dpSourceSuspend, Unexpected State enum:usb_dongle_a2dp_source_state_t:%d IsConnectedMedia:%d IsSourceCodec:%d",
                       a2dp_source_data.state,
                       appA2dpIsConnectedMedia(inst),
                       appA2dpIsSourceCodec(inst));
    }

    Ui_InformContextChange(ui_provider_media_player, usbDongleA2dp_GetMediaStateContext());

    return FALSE;
}

bool UsbDongle_A2dpSourceResume(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_A2dpSourceResume");

    avInstanceTaskData * inst = a2dp_source_data.active_av_instance;
    if (inst && A2dpProfile_IsMediaSourceConnected(inst))
    {
        DEBUG_LOG_INFO("UsbDongle_A2dpSourceResume, resuming AV streaming");

        /* Check if negotiated sample rate needs updating first. */
        UsbDongle_A2dpUpdatePreferredSampleRate();

        /* Resume A2DP media channel. */
        A2dpProfile_ResumeMedia(inst);
        return TRUE;
    }
    return FALSE;
}

void UsbDongle_A2dpSourceInit(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_A2dpSourceInit");

    a2dp_source_data.task.handler = usbDongle_HandleSourceMessage;
    a2dp_source_data.state = APP_A2DP_STATE_DISCONNECTED;
    a2dp_source_data.active_av_instance = NULL;
    a2dp_source_data.connected_cb = NULL;
    a2dp_source_data.disconnected_cb = NULL;
    a2dp_source_data.target_latency.use_saved = FALSE;
    a2dp_source_data.aptxad_hq_sample_rate.use_saved = FALSE;
    a2dp_source_data.aptxad_hq_sample_rate.rate = 0;

    /* Register as ui provider */
    Ui_RegisterUiProvider(ui_provider_media_player, usbDongleA2dp_GetMediaStateContext);

    appAvStatusClientRegister(&a2dp_source_data.task);
}

void UsbDongle_A2dpRegisterConnectionCallbacks(void (*connected_cb)(void),
                                               void (*disconnected_cb)(void))
{
    a2dp_source_data.connected_cb = connected_cb;
    a2dp_source_data.disconnected_cb = disconnected_cb;
}

bool UsbDongle_A2dpSourceInConnectedState(void)
{
    return (a2dp_source_data.state >= APP_A2DP_STATE_CONNECTED);
}

void UsbDongle_A2dpLatencyTargetSet(uint16 target_latency)
{
    a2dp_source_data.target_latency.use_saved = TRUE;
    a2dp_source_data.target_latency.latency = target_latency;
}

uint16 UsbDongle_A2dpLatencyTargetGet(void)
{
    return a2dp_source_data.target_latency.latency;
}

void UsbDongle_A2dpLatencyTargetDefault(void)
{
    a2dp_source_data.target_latency.use_saved = FALSE;
    a2dp_source_data.target_latency.latency = 0;
}

void UsbDongle_A2dpAptxAdHqSampleRateSet(uint32 sample_rate)
{
    a2dp_source_data.aptxad_hq_sample_rate.use_saved = TRUE;
    a2dp_source_data.aptxad_hq_sample_rate.rate = sample_rate;

    switch (sample_rate)
    {
        case SAMPLE_RATE_96000:
            DEBUG_LOG_INFO("UsbDongle_A2dpAptxAdHqSampleRateSet, aptX Adaptive will use 96kHz for HQ mode, if possible");
            break;

        case SAMPLE_RATE_48000:
            DEBUG_LOG_INFO("UsbDongle_A2dpAptxAdHqSampleRateSet, aptX Adaptive will use 48kHz for HQ mode");
            break;

        case SAMPLE_RATE_44100:
            DEBUG_LOG_INFO("UsbDongle_A2dpAptxAdHqSampleRateSet, aptX Adaptive will use 44.1kHz Lossless for HQ mode");
            break;

        default:
            DEBUG_LOG_INFO("UsbDongle_A2dpAptxAdHqSampleRateSet, %lu Hz not supported, aptX Adaptive will use"
                           " highest sample rate available for HQ mode", sample_rate);
            break;
    }

    if (Kymera_AptxAdEncoderGetDesiredQualityMode() == aptxad_mode_high_quality)
    {
        UsbDongle_A2dpUpdatePreferredSampleRate();
    }
}

uint32 UsbDongle_A2dpAptxAdHqSampleRateGet(void)
{
    return a2dp_source_data.aptxad_hq_sample_rate.rate;
}

void UsbDongle_A2dpAptxAdHqSampleRateDefault(void)
{
    a2dp_source_data.aptxad_hq_sample_rate.use_saved = FALSE;
    a2dp_source_data.aptxad_hq_sample_rate.rate = 0;

    DEBUG_LOG_INFO("UsbDongle_A2dpAptxAdHqSampleRateDefault, aptX Adaptive will use default audio input rate for HQ mode");

    if (Kymera_AptxAdEncoderGetDesiredQualityMode() == aptxad_mode_high_quality)
    {
        UsbDongle_A2dpUpdatePreferredSampleRate();
    }
}

uint32 UsbDongle_A2dpGetCurrentSampleRate(void)
{
    uint32 rate = 0;

    avInstanceTaskData *inst = a2dp_source_data.active_av_instance;
    if (inst && A2dpProfile_IsMediaSourceConnected(inst))
    {
        /* Return the current A2DP media channel sample rate (live value). */
        source_defined_params_t source_params;
        a2dp_connect_parameters_t *a2dp_connect_params;

        AudioSources_GetConnectParameters(audio_source_a2dp_1, &source_params);
        a2dp_connect_params = source_params.data;
        rate = a2dp_connect_params->rate;
        AudioSources_ReleaseConnectParameters(audio_source_a2dp_1, &source_params);
    }

    return rate;
}

bool UsbDongle_A2dpUpdatePreferredSampleRate(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_A2dpUpdatePreferredSampleRate");

    bool renegotiating = FALSE;

    avInstanceTaskData *inst = a2dp_source_data.active_av_instance;
    if (inst)
    {
        uint32 new_rate;

        switch (Kymera_AptxAdEncoderGetDesiredQualityMode())
        {
            case aptxad_mode_low_latency:
                /* In LL mode, only 48kHz is supported. */
                new_rate = APTXAD_REQUIRED_SAMPLE_RATE_LL_MODE;
                break;

            case aptxad_mode_high_quality:
            default:
                if (a2dp_source_data.aptxad_hq_sample_rate.use_saved)
                {
                    /* Test override. */
                    new_rate = a2dp_source_data.aptxad_hq_sample_rate.rate;
                }
                else
                {
                    /* In HQ mode, application can request for preferred rate of the audio input. */
                    new_rate = UsbDongle_AudioGetCurrentPreferredSampleRate();

                    /* As per design, 44.1kHz lossless is preferred over 48kHz.
                     * If 44.1kHz lossless can not be achieved, 48kHz will be used for negotiation. */
                    new_rate = (new_rate == SAMPLE_RATE_48000) ? SAMPLE_RATE_44100 : new_rate;
                }
                break;
        }

        renegotiating = A2dpProfile_SetPreferredSampleRate(inst, new_rate);
    }

    return renegotiating;
}

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

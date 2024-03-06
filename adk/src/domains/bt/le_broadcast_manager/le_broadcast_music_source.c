/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    leabm
    \brief
*/

#if defined(INCLUDE_LE_AUDIO_BROADCAST)

#include "le_broadcast_music_source.h"
#include "le_broadcast_manager.h"
#include "le_broadcast_manager_data.h"
#include "le_broadcast_manager_source.h"

#include "le_audio_messages.h"
#include "audio_sources.h"
#include "bt_device.h"
#include "bandwidth_manager.h"
#include "gatt_connect.h"
#include "kymera_adaptation_audio_protected.h"
#include "kymera.h"
#include "link_policy.h"
#include "multidevice.h"
#include "pacs_utilities.h"

#include <panic.h>
#include <stdlib.h>
#include <logging.h>

static bool leBroadcastMusicSource_GetAudioConnectParameters(audio_source_t source, source_defined_params_t * source_params);
static void leBroadcastMusicSource_FreeAudioConnectParameters(audio_source_t source, source_defined_params_t * source_params);
static bool leBroadcastMusicSource_GetAudioDisconnectParameters(audio_source_t source, source_defined_params_t * source_params);
static void leBroadcastMusicSource_FreeAudioDisconnectParameters(audio_source_t source, source_defined_params_t * source_params);
static bool leBroadcastMusicSource_IsAudioRouted(audio_source_t source);
static bool leBroadcastMusicSource_IsAudioAvailable(audio_source_t source);
static source_status_t leBroadcastMusicSource_SetState(audio_source_t source, source_state_t state);
static device_t leBroadcastMusicSource_GetAudioSourceDevice(audio_source_t source);

static const audio_source_audio_interface_t music_source_audio_interface =
{
    .GetConnectParameters = leBroadcastMusicSource_GetAudioConnectParameters,
    .ReleaseConnectParameters = leBroadcastMusicSource_FreeAudioConnectParameters,
    .GetDisconnectParameters = leBroadcastMusicSource_GetAudioDisconnectParameters,
    .ReleaseDisconnectParameters = leBroadcastMusicSource_FreeAudioDisconnectParameters,
    .IsAudioRouted = leBroadcastMusicSource_IsAudioRouted,
    .IsAudioAvailable = leBroadcastMusicSource_IsAudioAvailable,
    .SetState = leBroadcastMusicSource_SetState,
    .GetState = NULL,
    .Device = leBroadcastMusicSource_GetAudioSourceDevice
};

static appKymeraLeStreamType leBroadcastMusicSource_GetStreamTypeAndHandle(uint16 *source_iso_handle, uint16 *source_iso_handle_r)
{
    appKymeraLeStreamType stream_type = KYMERA_LE_STREAM_MONO;
    broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceOfActiveBis();

    *source_iso_handle = LeBroadcastManager_SourceGetBisStreamHandle(broadcast_manager_bis_location_left_or_stereo);
    *source_iso_handle_r = 0;

    if (Multidevice_IsDeviceStereo())
    {
        *source_iso_handle_r = LeBroadcastManager_SourceGetBisStreamHandle(broadcast_manager_bis_location_right);
        if (*source_iso_handle_r != 0)
        {
            stream_type = KYMERA_LE_STREAM_DUAL_MONO;
        }
    }

    if (*source_iso_handle_r == 0)
    {
        *source_iso_handle_r = LE_AUDIO_INVALID_ISO_HANDLE;
    }

    if (stream_type == KYMERA_LE_STREAM_MONO && leBroadcastManager_IsStereoBis(broadcast_source))
    {
        switch (Multidevice_GetSide())
        {
            default:
            case multidevice_side_both:
                stream_type = KYMERA_LE_STREAM_STEREO_USE_BOTH;
                break;

            case multidevice_side_left:
                stream_type = KYMERA_LE_STREAM_STEREO_USE_LEFT;
                break;

            case multidevice_side_right:
                stream_type = KYMERA_LE_STREAM_STEREO_USE_RIGHT;
                break;
        }
    }

    return stream_type;
}

static bool leBroadcastMusicSource_GetAudioConnectParameters(audio_source_t source, source_defined_params_t * source_params)
{
    switch(source)
    {
        case audio_source_le_audio_broadcast:
        {
            le_audio_connect_parameters_t *conn_param = (le_audio_connect_parameters_t*)PanicUnlessMalloc(sizeof(le_audio_connect_parameters_t));
            memset(conn_param, 0, sizeof(le_audio_connect_parameters_t));  

            conn_param->media_present = TRUE;
            conn_param->media.volume = AudioSources_CalculateOutputVolume(audio_source_le_audio_broadcast);
            conn_param->media.sample_rate = LeBroadcastManager_SourceGetAudioStreamSampleRate();
            conn_param->media.frame_length = LeBroadcastManager_SourceGetAudioStreamOctetsPerFrame() * LeBroadcastManager_SourceGetAudioStreamCodecFrameBlocksPerSdu();
            conn_param->media.frame_duration = LeBroadcastManager_SourceGetAudioStreamFrameDuration();
            conn_param->media.start_muted = leBroadcastManager_GetStartMuted();
            conn_param->media.presentation_delay = LeBroadcastManager_SourceGetAudioStreamPresentationDelay();
            conn_param->media.codec_type = KYMERA_LE_AUDIO_CODEC_LC3;/* @TODO Supports only LC3 codec. */
            conn_param->media.stream_type = leBroadcastMusicSource_GetStreamTypeAndHandle(&conn_param->media.source_iso_handle,
                                                                                          &conn_param->media.source_iso_handle_right);
#ifdef ENABLE_LE_AUDIO_WBM
            /* Enabling VS LC3 based on availability of valid license */
            conn_param->media.codec_version = LeBapPacsUtilities_Lc3EpcLicenseCheck() ? 1 : 0;
#else
            conn_param->media.codec_version = 0x00;
#endif
            conn_param->media.codec_frame_blocks_per_sdu = LeBroadcastManager_SourceGetAudioStreamCodecFrameBlocksPerSdu();
            conn_param->microphone_present = FALSE;
            source_params->data = (void *)conn_param;
            source_params->data_length = sizeof(le_audio_connect_parameters_t);
        }
        break;
        
        default:
            return FALSE;
    }
   return TRUE;
}

static void leBroadcastMusicSource_FreeAudioConnectParameters(audio_source_t source, source_defined_params_t * source_params)
{
    switch(source)
    {
        case audio_source_le_audio_broadcast:
        {
            PanicNull(source_params);
            PanicFalse(source_params->data_length == sizeof(le_audio_connect_parameters_t));
            free(source_params->data);
            source_params->data = (void *)NULL;
            source_params->data_length = 0;
        }
        break;
        
        default:
            break;
    }
}

static bool leBroadcastMusicSource_GetAudioDisconnectParameters(audio_source_t source, source_defined_params_t * source_params)
{
    UNUSED(source_params);
    UNUSED(source);
    return TRUE;
}

static void leBroadcastMusicSource_FreeAudioDisconnectParameters(audio_source_t source, source_defined_params_t * source_params)
{
    UNUSED(source_params);
    UNUSED(source);
}

static bool leBroadcastMusicSource_IsAudioRouted(audio_source_t source)
{
    bool audio_available = FALSE;
    
    if (source == audio_source_le_audio_broadcast)
    {
        if (LeBroadcastManager_SourceIsBisSync())
        {
            audio_available = TRUE;
        }
    }
    
    return audio_available;
}

static bool leBroadcastMusicSource_IsAudioAvailable(audio_source_t source)
{
    bool audio_available = FALSE;

    if (source == audio_source_le_audio_broadcast)
    {
        if (LeBroadcastManager_SourceIsBisSync())
        {
            audio_available = TRUE;
        }
    }

    return audio_available;
}

static source_status_t leBroadcastMusicSource_SetState(audio_source_t source, source_state_t state)
{
    source_status_t status = source_status_ready;

    if (source == audio_source_le_audio_broadcast)
    {
        DEBUG_LOG("leBroadcastMusicSource_SetState enum:source_state_t:%d", state);
        switch (state)
        {
            case source_state_connecting:
            {
                /* Broadcast is being routed so set the Broadcast Assistant
                   as the MRU device. The intention is that any control type
                   events, e.g. volume, should be associated with the
                   Broadcast Assistant device.

                   If multipoint is disabled and a handset has to be
                   disconnected, setting the Broadcast Assistant as the MRU
                   device means the other handset will get disconnected.
                */
                broadcast_source_state_t * bss = LeBroadcastManager_GetSourceOfActiveBis();
                if (bss)
                {
                    typed_bdaddr taddr = {0};
                    if (BtDevice_GetPublicAddress(&LeBroadcastManager_GetAssistantAddress(bss), &taddr))
                    {
                        appDeviceUpdateMruDevice(&taddr.addr);
                    }
                }

                appLinkPolicyUpdatePowerTable(NULL);
            }
            break;

            case source_state_disconnecting:
            {
                /*! \todo In future, the audio router will use media to control
                to pause the source when another source interrupts this source.
                This code should then be moved to the media control pause interface.
                Also note, that currently there is no way to resume the broadcast.
                This should also be fixed by the audio router using the pause/play
                media control APIs to handle resuming interrupted sources. */

                if (LeBroadcastManager_SourceGetBisStreamHandle(broadcast_manager_bis_location_left_or_stereo))
                {
                    LeBroadcastManager_Pause(LeBroadcastManager_SourceGetTask());
                }
                else
                {
                    status = source_status_ready;
                }

                appLinkPolicyUpdatePowerTable(NULL);
            }
            break;

            case source_state_connected:
            {
                if (LeBroadcastManager_IsHighPriorityBandwidthUser())
                {
                    BandwidthManager_FeatureStart(BANDWIDTH_MGR_FEATURE_LE_BROADCAST);
                }
            }
            break;

            case source_state_disconnected:
            {
                /*! Broadcast audio was not routed or has become disconnected.
                    The Broadcast should pause if it is still actively synced to BIS. */

                if (LeBroadcastManager_SourceGetBisStreamHandle(broadcast_manager_bis_location_left_or_stereo))
                {
                    LeBroadcastManager_Pause(LeBroadcastManager_SourceGetTask());
                }
                else
                {
                    status = source_status_ready;
                }
                
                if (LeBroadcastManager_IsHighPriorityBandwidthUser())
                {
                    BandwidthManager_FeatureStop(BANDWIDTH_MGR_FEATURE_LE_BROADCAST);
                }
            }
            break;

            default:
                break;
        }
    }
    return status;
}

static device_t leBroadcastMusicSource_GetAudioSourceDevice(audio_source_t source)
{
    broadcast_source_state_t *broadcast = LeBroadcastManager_GetSourceOfActiveBis();

    UNUSED(source);

    if (broadcast)
    {
        typed_bdaddr active_media_typed_addr = {0};
        typed_bdaddr source_taddr = LeBroadcastManager_GetCurrentSourceAddr(broadcast);
        typed_bdaddr public_taddr = {0};

        if (BtDevice_GetPublicAddress(&source_taddr, &public_taddr))
        {
            active_media_typed_addr = public_taddr;
        }
        else
        {
            active_media_typed_addr = source_taddr;
        }

        return BtDevice_GetDeviceForBdAddr(&active_media_typed_addr.addr);
    }
    return NULL;
}

const audio_source_audio_interface_t * LeBroadcastMusicSource_GetAudioInterface(void)
{
    return &music_source_audio_interface;
}

#endif /* INCLUDE_LE_AUDIO_BROADCAST */

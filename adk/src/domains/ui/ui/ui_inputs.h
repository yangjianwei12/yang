/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file       ui_inputs.h
    \addtogroup ui
    \brief     Contains definitions required by ui config table.
    @{
*/

#ifndef _UI_INPUTS_H_
#define _UI_INPUTS_H_

#include "domain_message.h"

#ifdef INCLUDE_VOICE_UI
#define UI_INPUT_VOICE_UI(x) x
#else
#define UI_INPUT_VOICE_UI(x)
#endif

#define FOREACH_UI_INPUT(UI_INPUT) \
    UI_INPUT(ui_input_voice_call_hang_up = UI_INPUTS_TELEPHONY_MESSAGE_BASE)   \
    UI_INPUT(ui_input_voice_call_accept)  \
    UI_INPUT(ui_input_voice_transfer)   \
    UI_INPUT(ui_input_voice_transfer_to_ag)   \
    UI_INPUT(ui_input_voice_transfer_to_headset)   \
    UI_INPUT(ui_input_voice_call_reject)  \
    UI_INPUT(ui_input_voice_dial) \
    UI_INPUT(ui_input_voice_call_last_dialed) \
    UI_INPUT(ui_input_mic_mute_toggle) \
    UI_INPUT(ui_input_voice_call_cycle) \
    UI_INPUT(ui_input_voice_call_join_calls) \
    UI_INPUT(ui_input_voice_call_join_calls_and_hang_up) \
    UI_INPUT(ui_input_toggle_play_pause = UI_INPUTS_MEDIA_PLAYER_MESSAGE_BASE)  \
    UI_INPUT(ui_input_play)  \
    UI_INPUT(ui_input_pause)  \
    UI_INPUT(ui_input_pause_all)  \
    UI_INPUT(ui_input_stop_av_connection)  \
    UI_INPUT(ui_input_av_forward)  \
    UI_INPUT(ui_input_av_backward)  \
    UI_INPUT(ui_input_av_fast_forward_start) \
    UI_INPUT(ui_input_fast_forward_stop) \
    UI_INPUT(ui_input_av_rewind_start) \
    UI_INPUT(ui_input_rewind_stop) \
    UI_INPUT(ui_input_switch_active_audio_source) \
    UI_INPUT(ui_input_factory_reset_request = UI_INPUTS_DEVICE_STATE_MESSAGE_BASE) \
    UI_INPUT(ui_input_dfu_active_when_in_case_request) \
    UI_INPUT(ui_input_force_reset) \
    UI_INPUT(ui_input_production_test_mode) \
    UI_INPUT(ui_input_production_test_mode_request) \
    UI_INPUT(ui_input_dts_mode) \
    UI_INPUT(ui_input_dts_mode_idle) \
    UI_INPUT(ui_input_dts_mode_dut) \
    UI_INPUT(ui_input_sm_power_on) \
    UI_INPUT(ui_input_sm_power_off) \
    UI_INPUT(ui_input_disable_touchpad) \
    UI_INPUT(ui_input_volume_stop = UI_INPUTS_VOLUME_MESSAGE_BASE) \
    UI_INPUT(ui_input_volume_down_start) \
    UI_INPUT(ui_input_volume_up_start) \
    UI_INPUT(ui_input_volume_up) \
    UI_INPUT(ui_input_volume_down) \
    UI_INPUT(ui_input_sm_pair_handset = UI_INPUTS_HANDSET_MESSAGE_BASE) \
    UI_INPUT(ui_input_sm_delete_handsets) \
    UI_INPUT(ui_input_connect_handset) \
    UI_INPUT(ui_input_connect_handset_link_loss) \
    UI_INPUT(ui_input_disconnect_lru_handset) \
    UI_INPUT(ui_input_enable_multipoint) \
    UI_INPUT(ui_input_disable_multipoint) \
    UI_INPUT(ui_input_enable_sass_connection_switch) \
    UI_INPUT(ui_input_disable_sass_connection_switch) \
    UI_INPUT(ui_input_enable_google_finder_location_tracking) \
    UI_INPUT(ui_input_disable_google_finder_location_tracking) \
    UI_INPUT(ui_input_connect_sink = UI_INPUTS_SINK_MESSAGE_BASE) \
    UI_INPUT(ui_input_disconnect_sink) \
    UI_INPUT(ui_input_pair_sink) \
    UI_INPUT(ui_input_anc_on = UI_INPUTS_AUDIO_CURATION_MESSAGE_BASE) \
    UI_INPUT(ui_input_anc_off) \
    UI_INPUT(ui_input_anc_toggle_on_off) \
    UI_INPUT(ui_input_anc_set_mode_1) \
    UI_INPUT(ui_input_anc_set_mode_2) \
    UI_INPUT(ui_input_anc_set_mode_3) \
    UI_INPUT(ui_input_anc_set_mode_4) \
    UI_INPUT(ui_input_anc_set_mode_5) \
    UI_INPUT(ui_input_anc_set_mode_6) \
    UI_INPUT(ui_input_anc_set_mode_7) \
    UI_INPUT(ui_input_anc_set_mode_8) \
    UI_INPUT(ui_input_anc_set_mode_9) \
    UI_INPUT(ui_input_anc_set_mode_10) \
    UI_INPUT(ui_input_anc_set_next_mode) \
    UI_INPUT(ui_input_anc_set_world_vol_up) \
    UI_INPUT(ui_input_anc_set_world_vol_down) \
    UI_INPUT(ui_input_anc_wind_detected) \
    UI_INPUT(ui_input_anc_wind_released) \
    UI_INPUT(ui_input_anc_wind_enable) \
    UI_INPUT(ui_input_anc_wind_disable) \
    UI_INPUT(ui_input_anc_auto_transparency_enable) \
    UI_INPUT(ui_input_anc_auto_transparency_disable) \
    UI_INPUT(ui_input_anc_auto_transparency_release_time) \
    UI_INPUT(ui_input_anc_anti_howling_enable) \
    UI_INPUT(ui_input_anc_anti_howling_disable) \
    UI_INPUT(ui_input_anc_noise_id_enable) \
    UI_INPUT(ui_input_anc_noise_id_disable) \
    UI_INPUT(ui_input_anc_adverse_acoustic_handler_enable) \
    UI_INPUT(ui_input_anc_adverse_acoustic_handler_disable) \
    UI_INPUT(ui_input_anc_enter_tuning_mode) \
    UI_INPUT(ui_input_anc_exit_tuning_mode) \
    UI_INPUT(ui_input_anc_enter_adaptive_anc_tuning_mode) \
    UI_INPUT(ui_input_anc_exit_adaptive_anc_tuning_mode) \
    UI_INPUT(ui_input_anc_set_leakthrough_gain) \
    UI_INPUT(ui_input_anc_adaptivity_toggle_on_off) \
    UI_INPUT(ui_input_anc_toggle_way) \
    UI_INPUT(ui_input_anc_set_world_volume_gain) \
    UI_INPUT(ui_input_anc_set_world_volume_balance) \
    UI_INPUT(ui_input_continuous_fit_test_start)\
    UI_INPUT(ui_input_continuous_fit_test_stop)\
    UI_INPUT(ui_input_continuous_fit_test_single_capture)\
    UI_INPUT(ui_input_anc_set_filter_topology_parallel) \
    UI_INPUT(ui_input_anc_set_filter_topology_dual) \
    UI_INPUT(ui_input_anc_toggle_diagnostic) \
    UI_INPUT(ui_input_leakthrough_on) \
    UI_INPUT(ui_input_leakthrough_off) \
    UI_INPUT(ui_input_leakthrough_toggle_on_off) \
    UI_INPUT(ui_input_leakthrough_set_mode_1) \
    UI_INPUT(ui_input_leakthrough_set_mode_2) \
    UI_INPUT(ui_input_leakthrough_set_mode_3) \
    UI_INPUT(ui_input_leakthrough_set_next_mode) \
    UI_INPUT(ui_input_fit_test_prepare_test)\
    UI_INPUT(ui_input_fit_test_start)\
    UI_INPUT(ui_input_fit_test_remote_result_ready)\
    UI_INPUT(ui_input_fit_test_abort)\
    UI_INPUT(ui_input_fit_test_disable)\
    UI_INPUT(ui_input_fit_test_enter_tuning_mode)\
    UI_INPUT(ui_input_fit_test_exit_tuning_mode)\
    UI_INPUT(ui_input_le_audio_disable_anc)\
    UI_INPUT(ui_input_le_audio_enable_anc)\
    UI_INPUT_VOICE_UI(UI_INPUT(ui_input_va_1 = UI_INPUTS_VOICE_UI_MESSAGE_BASE)) \
    UI_INPUT_VOICE_UI(UI_INPUT(ui_input_va_2)) \
    UI_INPUT_VOICE_UI(UI_INPUT(ui_input_va_3)) \
    UI_INPUT_VOICE_UI(UI_INPUT(ui_input_va_4)) \
    UI_INPUT_VOICE_UI(UI_INPUT(ui_input_va_5)) \
    UI_INPUT_VOICE_UI(UI_INPUT(ui_input_va_6)) \
    UI_INPUT(ui_input_gaming_mode_toggle = UI_INPUTS_GAMING_MODE_MESSAGE_BASE) \
    UI_INPUT(ui_input_set_dongle_mode) \
    UI_INPUT(ui_input_at_serial_number_lookup = UI_INPUTS_ACCESSORY_TRACKING_MESSAGE_BASE) \
    UI_INPUT(ui_input_app_peer_pair = UI_INPUTS_APP_MESSAGE_BASE) \
    UI_INPUT(ui_input_app_toggle_twm_standalone) \
    UI_INPUT(ui_input_app_toggle_party_mode) \
    UI_INPUT(ui_input_app_toggle_broadcast_media_sender) \
    UI_INPUT(ui_input_invalid = UI_INPUTS_BOUNDS_CHECK_MESSAGE_BASE) \

#define GENERATE_ENUM(ENUM) ENUM,

enum UI_INPUT_ENUM {
    FOREACH_UI_INPUT(GENERATE_ENUM)
};

/*! The first UI input, not in the enum, as that causes ui_input_string_debug
    test to fail */
#define ui_input_first UI_INPUTS_TELEPHONY_MESSAGE_BASE

/*! Macro to test if a message id is a UI input message */
#define isMessageUiInput(msg_id) (ui_input_first <= (msg_id) && (msg_id) < ui_input_invalid)

typedef enum UI_INPUT_ENUM ui_input_t;

/*! \brief ui providers */
typedef enum
{
    ui_provider_telephony,
    ui_provider_handset,
    ui_provider_media_player,
    ui_provider_sink_service,
    ui_provider_phy_state,
    ui_provider_ptm_state,
    ui_provider_app_sm,
    ui_provider_power,
    ui_provider_audio_curation,
    ui_provider_voice_ui,
    ui_provider_peer_pairing,
    ui_provider_handset_pairing,
    ui_provider_charger,
    ui_provider_battery,
    ui_provider_accessory_tracking,
    ui_providers_max
} ui_providers_t;

#endif /* _UI_INPUTS_H_ */

/*! @} */

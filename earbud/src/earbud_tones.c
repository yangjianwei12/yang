/*!
\copyright  Copyright (c) 2019-2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Source file for the Earbud Application user interface tone indications.
*/
#include "earbud_tones.h"

/*! At the end of every tone, add a short rest to make sure tone mxing in the DSP doens't truncate the tone */
#define RINGTONE_STOP  RINGTONE_NOTE(REST, HEMIDEMISEMIQUAVER), RINGTONE_END

const ringtone_note app_tone_button[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_button_2[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_button_3[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_button_4[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_double_tap[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A6, SEMIQUAVER),
    RINGTONE_NOTE(B6, SEMIQUAVER),
    RINGTONE_STOP
};

#ifdef INCLUDE_DFU
const ringtone_note app_tone_button_dfu[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(A7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};
#endif

const ringtone_note app_tone_button_factory_reset[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(A7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(C7, SEMIQUAVER),
    RINGTONE_NOTE(B7, SEMIQUAVER),
    RINGTONE_STOP
};

#ifdef INCLUDE_AV
const ringtone_note app_tone_av_connect[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_av_disconnect[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_av_remote_control[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_av_connected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D6,  SEMIQUAVER),
    RINGTONE_NOTE(A6,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_av_disconnected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A6,  SEMIQUAVER),
    RINGTONE_NOTE(D6,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_av_link_loss[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_a2dp_not_routed[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_gaming_mode_on[] =
{
    RINGTONE_TIMBRE(plucked), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D6, HEMIDEMISEMIQUAVER),
    RINGTONE_NOTE(G6, HEMIDEMISEMIQUAVER),
    RINGTONE_NOTE(B6, HEMIDEMISEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_gaming_mode_off[] =
{
    RINGTONE_TIMBRE(plucked), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B6, HEMIDEMISEMIQUAVER),
    RINGTONE_NOTE(G6, HEMIDEMISEMIQUAVER),
    RINGTONE_NOTE(D6, HEMIDEMISEMIQUAVER),
    RINGTONE_STOP
};
#endif

const ringtone_note app_tone_hfp_connect[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_connected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D6,  SEMIQUAVER),
    RINGTONE_NOTE(A6,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_disconnected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A6,  SEMIQUAVER),
    RINGTONE_NOTE(D6,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_link_loss[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_sco_connected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(AS5, DEMISEMIQUAVER),
    RINGTONE_NOTE(DS6, DEMISEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_sco_disconnected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(DS6, DEMISEMIQUAVER),
    RINGTONE_NOTE(AS5, DEMISEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_mute_reminder[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_sco_unencrypted_reminder[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_ring[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_NOTE(D7,   SEMIQUAVER),
    RINGTONE_NOTE(REST, SEMIQUAVER),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_NOTE(D7,   SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_ring_caller_id[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_voice_dial[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_voice_dial_disable[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_answer[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_hangup[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_mute_active[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(CS7, SEMIQUAVER),
    RINGTONE_NOTE(DS7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_mute_inactive[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(DS7, SEMIQUAVER),
    RINGTONE_NOTE(CS7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_talk_long_press[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_pairing[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_paired[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A6, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_pairing_deleted[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(A6, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_volume[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_volume_limit[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_error[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_battery_empty[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B6, SEMIQUAVER),
    RINGTONE_NOTE(B6, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_power_on[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(CS5, SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_power_off[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_NOTE(CS5, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_paging_reminder[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_peer_pairing[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_peer_pairing_error[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_peer_pairing_reminder[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_factory_reset[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_NOTE(B7, SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_STOP
};

#if defined INCLUDE_GAA && defined INCLUDE_WUW
const ringtone_note app_tone_doff[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(24),
    RINGTONE_NOTE(D6, DEMISEMIQUAVER),
    RINGTONE_NOTE(CS6, DEMISEMIQUAVER),
    RINGTONE_NOTE(B5, DEMISEMIQUAVER),
    RINGTONE_NOTE(A5, QUAVER),
    RINGTONE_STOP
};
#endif /* INCLUDE_GAA && INCLUDE_WUW */

#ifdef INCLUDE_DFU
const ringtone_note app_tone_dfu[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(CS5, SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_STOP
};
#endif

#ifdef PRODUCTION_TEST_MODE
const ringtone_note dut_mode_tone[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A5, CROTCHET),
    RINGTONE_NOTE(B5, CROTCHET),
    RINGTONE_NOTE(C5, CROTCHET),
    RINGTONE_STOP
};
#endif

#ifdef INCLUDE_TEST_TONES
const ringtone_note app_tone_test_continuous[] =
{
    /* Intended to last about 5 minutes. */
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(0), RINGTONE_TEMPO(1),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE(A5, WHOLENOTE),
    RINGTONE_STOP
};

const ringtone_note app_tone_test_indexed[] =
{
    /* Index tones are about 2 seconds apart. */
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(0), RINGTONE_TEMPO(120),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A6, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(AS6, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(B6, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(C7, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(CS7, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(D7, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(DS7, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(E7, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(F7, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(FS7, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(G7, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(GS7, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A7, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(AS7, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(B7, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(C8, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(CS8, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(D8, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(DS8, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(E8, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(F8, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(FS8, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(G8, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(GS8, SIXTYFOURTHNOTE),
    RINGTONE_NOTE_TIE(A5, WHOLENOTE),
    RINGTONE_NOTE_TIE(A8, SIXTYFOURTHNOTE),
    RINGTONE_NOTE(A5, WHOLENOTE),
    RINGTONE_STOP
};
#endif

#ifdef INCLUDE_AMA
#ifdef HAVE_RDP_UI

const ringtone_note app_tone_ama_unregistered[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(G5,  SEMIQUAVER),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_NOTE(F4,  SEMIQUAVER),
    RINGTONE_NOTE(F5,  SEMIQUAVER),
    RINGTONE_NOTE(C5,  QUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_ama_not_connected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(G5,  SEMIQUAVER),
    RINGTONE_NOTE(G5,  SEMIQUAVER),
    RINGTONE_NOTE(G5,  SEMIQUAVER),
    RINGTONE_NOTE(E5,  QUAVER),
    RINGTONE_STOP
};

#endif  /* HAVE_RDP_UI */

const ringtone_note app_tone_ama_privacy_mode_disabled[] =
{
    RINGTONE_TIMBRE(sine),
    RINGTONE_DECAY(16),
    RINGTONE_NOTE(G6, DEMISEMIQUAVER),
    RINGTONE_NOTE(G6, DEMISEMIQUAVER),
    RINGTONE_NOTE(C7, SEMIQUAVER),
    RINGTONE_NOTE(REST, HEMIDEMISEMIQUAVER),
    RINGTONE_END
};

const ringtone_note app_tone_ama_privacy_mode_enabled[] =
{
    RINGTONE_TIMBRE(sine),
    RINGTONE_DECAY(16),
    RINGTONE_NOTE(G6, DEMISEMIQUAVER),
    RINGTONE_NOTE(G6, DEMISEMIQUAVER),
    RINGTONE_NOTE(C6, SEMIQUAVER),
    RINGTONE_NOTE(REST, HEMIDEMISEMIQUAVER),
    RINGTONE_END
};
#endif  /* INCLUDE_AMA */

#ifdef INCLUDE_ACCESSORY_TRACKING
/* These are all defined as "loud" tones, which means they play at a base
 * volume much much louder than the normal tones. */
const ringtone_note app_tone_accessory_tracking[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),

    RINGTONE_VOLUME(64),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_NOTE(D7,   SEMIQUAVER),

    RINGTONE_NOTE(REST, SEMIQUAVER),

    RINGTONE_VOLUME(96),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_NOTE(D7,   SEMIQUAVER),

    RINGTONE_NOTE(REST, SEMIQUAVER),

    RINGTONE_VOLUME(128),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_NOTE(D7,   SEMIQUAVER),

    RINGTONE_NOTE(REST, SEMIQUAVER),

    RINGTONE_VOLUME(128),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_NOTE(D7,   SEMIQUAVER),

    RINGTONE_NOTE(REST, SEMIQUAVER),

    RINGTONE_VOLUME(160),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_NOTE(D7,   SEMIQUAVER),

    RINGTONE_NOTE(REST, SEMIQUAVER),

    RINGTONE_VOLUME(192),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_NOTE(D7,   SEMIQUAVER),

    RINGTONE_NOTE(REST, SEMIQUAVER),

    RINGTONE_VOLUME(224),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_NOTE(D7,   SEMIQUAVER),

    RINGTONE_NOTE(REST, SEMIQUAVER),

    RINGTONE_VOLUME(255),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_NOTE(D7,   SEMIQUAVER),

    RINGTONE_NOTE(REST, SEMIQUAVER),

    RINGTONE_VOLUME(255),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_NOTE(D7,   SEMIQUAVER),

    RINGTONE_NOTE(REST, SEMIQUAVER),

    RINGTONE_VOLUME(255),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_NOTE(D7,   SEMIQUAVER),

    RINGTONE_NOTE(REST, SEMIQUAVER),

    RINGTONE_VOLUME(255),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_NOTE(D7,   SEMIQUAVER),

    RINGTONE_NOTE(REST, SEMIQUAVER),

    RINGTONE_END
};
#endif
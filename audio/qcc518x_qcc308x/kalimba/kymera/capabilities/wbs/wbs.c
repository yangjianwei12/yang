/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  wbs.c
 * \ingroup  operators
 *
 *  WBS_ENC/WBS_DEC operator common code
 *
 */

/****************************************************************************
Include Files
*/
#include "wbs_private.h"
#include "capabilities.h"

/***************************** WBS shared tables ******************************/
/** memory shared between enc and dec */
const share_malloc_t_entry wbs_sbc_shared_malloc_table[WBS_SBC_SHARED_TABLE_LENGTH] =
{
    {80, MALLOC_PREFERENCE_DM1, SBC_SHARED_WIN_COEFS_M8, offsetof(sbc_codec, win_coefs_m8)},
    {48, MALLOC_PREFERENCE_NONE, SBC_SHARED_LOUDNESS_OFFSET, offsetof(sbc_codec, loudness_offset)},
    {66, MALLOC_PREFERENCE_FAST, SBC_SHARED_BITSLICE_LOOKUP, offsetof(sbc_codec, bitslice_lookup)},
};

/** memory shared between encoders */
const share_malloc_t_entry wbs_sbc_enc_shared_malloc_table[WBS_SBC_ENC_SHARED_TABLE_LENGTH] =
{
    {128, MALLOC_PREFERENCE_DM2, SBC_ENC_SHARED_ANALYSIS_COEFS_M8, offsetof(sbc_codec, analysis_coefs_m8)},
    {16, MALLOC_PREFERENCE_NONE, SBC_ENC_SHARED_LEVEL_COEFS, offsetof(sbc_codec, level_coefs)}
};

/** memory shared between decoders */
const share_malloc_t_entry wbs_sbc_dec_shared_malloc_table[WBS_SBC_DEC_SHARED_TABLE_LENGTH] =
{
    {128, MALLOC_PREFERENCE_DM2, SBC_DEC_SHARED_SYNTHESIS_COEFS_M8, offsetof(sbc_codec, synthesis_coefs_m8)},
    {15, MALLOC_PREFERENCE_NONE, SBC_DEC_SHARED_LEVELRECIP_COEFS, offsetof(sbc_codec, levelrecip_coefs)},
    {17, MALLOC_PREFERENCE_NONE, SBC_DEC_SHARED_BITMASK_LOOKUP, offsetof(sbc_codec, bitmask_lookup)}
};

/** Scratch memory used by an encoder and decoder instance */
const scratch_malloc_t_entry wbs_scratch_table_dm1[WBS_DM1_SCRATCH_TABLE_LENGTH] =
{
    {WBS_AUDIO_SAMPLE_BUFF_SIZE, offsetof(sbc_codec, audio_sample)},
    {SBC_SCALE_FACTOR_LENGTH, offsetof(sbc_codec, scale_factor)}
};


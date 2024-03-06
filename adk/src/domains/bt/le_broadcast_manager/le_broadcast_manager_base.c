/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    leabm
    \brief      Handles the Broadcast Audio Source Endpoint (BASE) structure that is present in the periodic advertising.
*/

#if defined(INCLUDE_LE_AUDIO_BROADCAST)

#include "le_broadcast_manager_base.h"

#include "le_broadcast_manager.h"

#include "le_broadcast_manager_bis_selection.h"

#include "ltv_utilities.h"
#include "pacs_utilities.h"
#include "audio_announcement_parser_lib.h"

#include <logging.h>

#include <panic.h>

#define IS_BASE_BIS_INDEX_VALID(bis_index)          ((bis_index >= 1) && (bis_index <= 31))

#define BIS_INDEX_TO_BITFIELD(bis_index)            (1 << ((bis_index) - 1))


#define BROADCAST_MANAGER_BASE_LOG                  DEBUG_LOG

static void leBroadcastManager_StoreBaseLevel3CodecSpecificConfiguration(uint8 config_length,
                                                                         const uint8 *config,
                                                                         uint8 bis,
                                                                         le_broadcast_manager_subgroup_codec_specific_config_t *codec_specific_config);
static uint16 leBroadcastManager_ConvertSampleRateValue(AudioAnnouncementParserSamplingFreqType sampling_frequency_value);
static uint16 leBroadcastManager_ConvertSampleRatFrameDurationValue(AudioAnnouncementParserFrameDurationType frame_duration_value);
static bool leBroadcastManager_DoesAudioLocationMatch(uint32 audio_location_1, uint32 audio_location_2);
#ifndef ENABLE_LE_AUDIO_BIS_SELECTION
static uint8 leBroadcastManager_GetBaseNumberBisWithSpecifiedAudioLocation(le_broadcast_manager_subgroup_codec_specific_config_t *base_specific_config, uint8 num_subgroups,
                                                                           AudioLocationType audio_location);
static bool leBroadcastManager_FindCompatibleBaseBisIndexesAndStoreCodecConfig(broadcast_source_state_t *broadcast_source,
                                                                               le_broadcast_manager_subgroup_codec_specific_config_t *base_codec_config,
                                                                               AudioAnnouncementParserBaseData *base,
                                                                               le_broadcast_manager_bis_index_t *bis_index_l,
                                                                               le_broadcast_manager_bis_index_t *bis_index_r);
#endif
static bool leBroadcastManager_FindAudioLocationOfBaseBisIndexesAndStoreCodecConfig(broadcast_source_state_t *broadcast_source,
                                                                                    AudioAnnouncementParserBaseData *base, le_broadcast_manager_subgroup_codec_specific_config_t *base_codec_config,
                                                                                    uint32 base_bis_indexes,
                                                                                    le_broadcast_manager_bis_index_t *bis_index_l,
                                                                                    le_broadcast_manager_bis_index_t *bis_index_r);
static uint32 leBroadcastManager_GetBasePresentationDelay(AudioAnnouncementParserBaseData *base);
static void leBroadcastManager_GetCodecSpecificConfiguration(uint8 *config,
                                                             uint8 config_length,
                                                             le_broadcast_manager_codec_specific_config_t *base_codec_config);
static PacsSamplingFrequencyType leBroadcastManager_ConvertBaseToPacsSamplingFreqValue(AudioAnnouncementParserSamplingFreqType sampling_frequency);
static PacsFrameDurationType leBroadcastManager_ConvertBaseToPacsFrameDurationValue(AudioAnnouncementParserFrameDurationType frame_duration);
static bool leBroadcastManager_IsBaseCodecConfigSupportedInPacs(AudioAnnouncementParserBaseData *base,
                                                                le_broadcast_manager_codec_specific_config_t *base_codec_config,
                                                                uint8 subgroup);
static void leBroadcastManager_FreeCodecSpecificConfig(le_broadcast_manager_subgroup_codec_specific_config_t *base_codec_config,
                                                       uint8 num_subgroups);

static void leBroadcastManager_StoreBaseLevel3CodecSpecificConfiguration(uint8 config_length,
                                                                         const uint8 *config,
                                                                         uint8 bis,
                                                                         le_broadcast_manager_subgroup_codec_specific_config_t *codec_specific_config)
{
    le_broadcast_manager_codec_specific_config_t codec_specific_config_bis = {0};

    leBroadcastManager_GetCodecSpecificConfiguration((uint8 *)config,
                                                     config_length,
                                                     &codec_specific_config_bis);

    /*  Update codec configuration if set at a higher level.
        Give a warning if the valid higher level value has changed from the valid lower level value,
        in case the source has been configured incorrectly.
    */
    if (leBroadcastManager_IsCodecConfigurationValueSet(codec_specific_config_bis.sample_rate))
    {
        /* Store subgroup sample_rate at level 2, as it should apply to all BIS in this subgroup */
        if (leBroadcastManager_HasHigherLevelCodecConfigChangedFromLowerLevel(codec_specific_config_bis.sample_rate,
                                                                              codec_specific_config->level2_codec_specific_config.sample_rate))
        {
            BROADCAST_MANAGER_BASE_LOG("    Warning! Level 3 change of sample_rate:0x%x->0x%x",
                    codec_specific_config->level2_codec_specific_config.sample_rate,
                    codec_specific_config_bis.sample_rate
                    );
        }
        codec_specific_config->level2_codec_specific_config.sample_rate = codec_specific_config_bis.sample_rate;
        BROADCAST_MANAGER_BASE_LOG("    sample_rate 0x%x", codec_specific_config->level2_codec_specific_config.sample_rate);
    }
    if (leBroadcastManager_IsCodecConfigurationValueSet(codec_specific_config_bis.frame_duration))
    {
        /* Store subgroup frame_duration at level 2, as it should apply to all BIS in this subgroup */
        if (leBroadcastManager_HasHigherLevelCodecConfigChangedFromLowerLevel(codec_specific_config_bis.frame_duration,
                                                                              codec_specific_config->level2_codec_specific_config.frame_duration))
        {
            BROADCAST_MANAGER_BASE_LOG("    Warning! Level 3 change of frame_duration:0x%x->0x%x",
                    codec_specific_config->level2_codec_specific_config.frame_duration,
                    codec_specific_config_bis.frame_duration
                    );
        }
        codec_specific_config->level2_codec_specific_config.frame_duration = codec_specific_config_bis.frame_duration;
        BROADCAST_MANAGER_BASE_LOG("    frame_duration 0x%x", codec_specific_config->level2_codec_specific_config.frame_duration);
    }
    if (leBroadcastManager_IsCodecConfigurationValueSet(codec_specific_config_bis.octets_per_frame))
    {
        /* Store subgroup octets_per_frame at level 2, as it should apply to all BIS in this subgroup */
        if (leBroadcastManager_HasHigherLevelCodecConfigChangedFromLowerLevel(codec_specific_config_bis.octets_per_frame,
                                                                              codec_specific_config->level2_codec_specific_config.octets_per_frame))
        {
            BROADCAST_MANAGER_BASE_LOG("    Warning! Level 3 change of octets_per_frame:0x%x->0x%x",
                    codec_specific_config->level2_codec_specific_config.octets_per_frame,
                    codec_specific_config_bis.octets_per_frame
                    );
        }
        codec_specific_config->level2_codec_specific_config.octets_per_frame = codec_specific_config_bis.octets_per_frame;
        BROADCAST_MANAGER_BASE_LOG("    octets_per_frame 0x%x", codec_specific_config->level2_codec_specific_config.octets_per_frame);
    }
    if (leBroadcastManager_IsCodecConfigurationValueSet(codec_specific_config_bis.audio_channel_allocation))
    {
        /* Store audio_channel at level 3, as it can be set per BIS */
        if (leBroadcastManager_HasHigherLevelCodecConfigChangedFromLowerLevel(codec_specific_config_bis.audio_channel_allocation,
                                                                              codec_specific_config->level2_codec_specific_config.audio_channel_allocation))
        {
            BROADCAST_MANAGER_BASE_LOG("    Level 3 change of audio_channel_allocation:0x%x->0x%x",
                    codec_specific_config->level2_codec_specific_config.audio_channel_allocation,
                    codec_specific_config_bis.audio_channel_allocation
                    );
        }
        codec_specific_config->level3_codec_specific_config[bis].audio_channel_allocation = codec_specific_config_bis.audio_channel_allocation;
        BROADCAST_MANAGER_BASE_LOG("    audio_channel from level 3 0x%x",
                                   codec_specific_config->level3_codec_specific_config[bis].audio_channel_allocation);
    }
    else if (leBroadcastManager_IsCodecConfigurationValueSet(codec_specific_config->level2_codec_specific_config.audio_channel_allocation))
    {
        /* Store audio_channel from level 2 at level 3 if no level 3 value specified */
        codec_specific_config->level3_codec_specific_config[bis].audio_channel_allocation = codec_specific_config->level2_codec_specific_config.audio_channel_allocation;
        BROADCAST_MANAGER_BASE_LOG("    audio_channel from level 2 0x%x",
                                   codec_specific_config->level3_codec_specific_config[bis].audio_channel_allocation);
    }
    if (leBroadcastManager_IsCodecConfigurationValueSet(codec_specific_config_bis.codec_frame_blocks_per_sdu))
    {
        /* Store subgroup codec_frame_blocks_per_sdu at level 2, as it should apply to all BIS in this subgroup */
        if (leBroadcastManager_HasHigherLevelCodecConfigChangedFromLowerLevel(codec_specific_config_bis.codec_frame_blocks_per_sdu,
                                                                              codec_specific_config->level2_codec_specific_config.codec_frame_blocks_per_sdu))
        {
            BROADCAST_MANAGER_BASE_LOG("    Warning! Level 3 change of codec_frame_blocks_per_sdu:0x%x->0x%x",
                    codec_specific_config->level2_codec_specific_config.codec_frame_blocks_per_sdu,
                    codec_specific_config_bis.codec_frame_blocks_per_sdu
                    );
        }
        codec_specific_config->level2_codec_specific_config.codec_frame_blocks_per_sdu = codec_specific_config_bis.codec_frame_blocks_per_sdu;
        BROADCAST_MANAGER_BASE_LOG("    codec_frame_blocks_per_sdu 0x%x", codec_specific_config->level2_codec_specific_config.codec_frame_blocks_per_sdu);
    }
}

static bool leBroadcastManager_DoesAudioLocationMatch(AudioLocationType audio_location_1,
                                                      AudioLocationType audio_location_2)
{
    bool result = FALSE;
    
    if (audio_location_1 & audio_location_2)
    {
        result= TRUE;
    }
    else if ((audio_location_1 == BROADCAST_MANAGER_AUDIO_LOCATION_NOT_SET) &&
             (audio_location_2 == BROADCAST_MANAGER_AUDIO_LOCATION_NOT_SET))
    {
        result= TRUE;
    }
            
    return result;
}

#ifndef ENABLE_LE_AUDIO_BIS_SELECTION
static uint8 leBroadcastManager_GetBaseNumberBisWithSpecifiedAudioLocation(le_broadcast_manager_subgroup_codec_specific_config_t *base_specific_config,
                                                                           uint8 num_subgroups,
                                                                           AudioLocationType audio_location)
{
    uint8 subgroup_count, bis_count;
    uint8 number_bis = 0;
    
    for (subgroup_count=0; subgroup_count<num_subgroups; subgroup_count++)
    {
        for (bis_count=0; bis_count<base_specific_config->num_bis; bis_count++)
        {
            if (leBroadcastManager_DoesAudioLocationMatch(base_specific_config[subgroup_count].level3_codec_specific_config[bis_count].audio_channel_allocation,
                                                          audio_location))
            {
                number_bis++;
            }
        }
    }
    
    return number_bis;
}
#endif

static uint16 leBroadcastManager_ConvertSampleRateValue(AudioAnnouncementParserSamplingFreqType sampling_frequency_value)
{
    uint16 sample_rate = 0;

    switch(sampling_frequency_value)
    {
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_8000:
            sample_rate = 8000;
        break;
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_16000:
            sample_rate = 16000;
        break;
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_24000:
            sample_rate = 24000;
        break;
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_32000:
            sample_rate = 32000;
        break;
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_44100:
            sample_rate = 44100;
        break;
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_48000:
            sample_rate = 48000;
        break;
        default:
            DEBUG_LOG_WARN("leBroadcastManager_ConvertSampleRateValue: sampling_frequency_value not recognised");
        break;
    }

    return sample_rate;
}

static uint16 leBroadcastManager_ConvertSampleRatFrameDurationValue(AudioAnnouncementParserFrameDurationType frame_duration_value)
{
    uint16 frame_duration = 0;

    switch(frame_duration_value)
    {
        case AUDIO_ANNOUNCEMENT_PARSER_FRAME_DURATION_TYPE_7_5:
            frame_duration = 7500;
                break;
        case AUDIO_ANNOUNCEMENT_PARSER_FRAME_DURATION_TYPE_10:
            frame_duration = 10000;
        break;
            DEBUG_LOG_WARN("leBroadcastManager_ConvertSampleRatFrameDurationValue: frame_duration_value not recognised");
        default:
        break;
    }

    return frame_duration;
}

void leBroadcastManager_SetBaseSubgroupCodecSpecificConfiguration(broadcast_source_state_t *broadcast_source,
                                                                  le_broadcast_manager_codec_specific_config_t *base_codec_specific_config)
{
    broadcast_source_codec_config_t codec_config = {0};
    codec_config.sample_rate = leBroadcastManager_ConvertSampleRateValue(base_codec_specific_config->sample_rate);
    codec_config.frame_duration = leBroadcastManager_ConvertSampleRatFrameDurationValue(base_codec_specific_config->frame_duration);
    codec_config.octets_per_frame = base_codec_specific_config->octets_per_frame;
    codec_config.codec_frame_blocks_per_sdu = base_codec_specific_config->codec_frame_blocks_per_sdu;

    leBroadcastManager_SetBroadcastSourceCodecConfig(broadcast_source, &codec_config);
}

#ifndef ENABLE_LE_AUDIO_BIS_SELECTION
static bool leBroadcastManager_FindCompatibleBaseBisIndexesAndStoreCodecConfig(broadcast_source_state_t *broadcast_source,
                                                                               le_broadcast_manager_subgroup_codec_specific_config_t *base_codec_config,
                                                                               AudioAnnouncementParserBaseData *base,
                                                                               le_broadcast_manager_bis_index_t *bis_index_l,
                                                                               le_broadcast_manager_bis_index_t *bis_index_r)
{
    uint8 subgroup_count, bis_count;
    uint8 number_bis = 0;
    AudioLocationType audio_location = LeBapPacsUtilities_GetSinkAudioLocation();
    bool result = FALSE;

    memset(bis_index_l, 0, sizeof(le_broadcast_manager_bis_index_t));
    memset(bis_index_r, 0, sizeof(le_broadcast_manager_bis_index_t));

    /* Need to find the bis_index that this device is interested to sync with from the BASE information */
    number_bis = leBroadcastManager_GetBaseNumberBisWithSpecifiedAudioLocation(base_codec_config,
                                                                               base->numSubgroups,
                                                                               audio_location);

    BROADCAST_MANAGER_BASE_LOG("leBroadcastManager_FindCompatibleBaseBisIndexesAndStoreCodecConfig. audio_location=0x%x number_bis=0x%x", audio_location, number_bis);

    if ((number_bis == 0) && (audio_location != AUDIO_LOCATION_MONO))
    {
        audio_location = AUDIO_LOCATION_MONO;
        number_bis = leBroadcastManager_GetBaseNumberBisWithSpecifiedAudioLocation(base_codec_config,
                                                                                   base->numSubgroups,
                                                                                   audio_location);
        BROADCAST_MANAGER_BASE_LOG("    audio_location=mono number_bis=0x%x", number_bis);
    }

    /* todo Assuming only one mono BIS, or one left BIS and one right BIS sent by the Broadcast Source.
        If more than one BIS is found with say an audio location of left, we currently don't know which one to pick.
        Might need to look at Metadata to find, eg. Language supported. */
    if (number_bis != 0)
    {
        for (subgroup_count=0; subgroup_count<base->numSubgroups; subgroup_count++)
        {
            if (leBroadcastManager_IsBaseCodecConfigSupportedInPacs(base,
                                                                    &(base_codec_config[subgroup_count].level2_codec_specific_config),
                                                                    subgroup_count))
            {
                for (bis_count=0; bis_count<base->subGroupsData[subgroup_count].numBis; bis_count++)
                {
                    if (IS_BASE_BIS_INDEX_VALID(base->subGroupsData[subgroup_count].bisData[bis_count].bisIndex))
                    {
                        BROADCAST_MANAGER_BASE_LOG("    compare audio_location=0x%x 0x%x",
                                                   base_codec_config[subgroup_count].level3_codec_specific_config[bis_count].audio_channel_allocation,
                                                   audio_location);
                        if (leBroadcastManager_DoesAudioLocationMatch(base_codec_config[subgroup_count].level3_codec_specific_config[bis_count].audio_channel_allocation,
                                                                      audio_location))
                        {
                            if (base_codec_config[subgroup_count].level3_codec_specific_config[bis_count].audio_channel_allocation == BROADCAST_MANAGER_AUDIO_LOCATION_NOT_SET)
                            {
                                bis_index_l->index = base->subGroupsData[subgroup_count].bisData[bis_count].bisIndex;
                                bis_index_l->subgroup = subgroup_count;
                                bis_index_l->is_stereo_bis = bis_index_r->is_stereo_bis = FALSE;
                                bis_index_r->index = bis_index_l->index;
                                bis_index_r->subgroup = bis_index_l->subgroup;
                                result = TRUE;
                            }
                            else if (leBroadcastManager_DoesAudioLocationMatch(base_codec_config[subgroup_count].level3_codec_specific_config[bis_count].audio_channel_allocation,
                                                                               PACS_AUDIO_LOCATION_LEFT))
                            {
                                bis_index_l->index = base->subGroupsData[subgroup_count].bisData[bis_count].bisIndex;
                                bis_index_l->subgroup = subgroup_count;
                                bis_index_l->is_stereo_bis = leBroadcastManager_DoesAudioLocationMatch(base_codec_config[subgroup_count].level3_codec_specific_config[bis_count].audio_channel_allocation, PACS_AUDIO_LOCATION_RIGHT);
                                result = TRUE;
                            }
                            else if (leBroadcastManager_DoesAudioLocationMatch(base_codec_config[subgroup_count].level3_codec_specific_config[bis_count].audio_channel_allocation,
                                                                               PACS_AUDIO_LOCATION_RIGHT))
                            {
                                bis_index_r->index = base->subGroupsData[subgroup_count].bisData[bis_count].bisIndex;
                                bis_index_r->subgroup = subgroup_count;
                                bis_index_r->is_stereo_bis = FALSE;
                                result = TRUE;
                            }
                        }
                    }
                }
            }
            if (result)
            {
                /*  Store codec values from the subgroup that the BIS belong to.
                    All BIS must be from the same subgroup.
                */
                leBroadcastManager_SetBaseSubgroupCodecSpecificConfiguration(broadcast_source,
                                                                             &(base_codec_config[subgroup_count].level2_codec_specific_config));
                AudioAnnouncementParserGetStreamAudioContextFromMetadata(base->subGroupsData[subgroup_count].metadataLen,
                                                                         base->subGroupsData[subgroup_count].metadata,
                                                                         (AudioContextType *) &(broadcast_source->streaming_audio_context));
                break;
            }
        }
    }

    BROADCAST_MANAGER_BASE_LOG("    result=0x%x bis_index_l=0x%x bis_index_r=0x%x", result, bis_index_l->index, bis_index_r->index);

    return result;
}
#endif

static bool leBroadcastManager_FindAudioLocationOfBaseBisIndexesAndStoreCodecConfig(broadcast_source_state_t *broadcast_source,
                                                                                    AudioAnnouncementParserBaseData *base,
                                                                                    le_broadcast_manager_subgroup_codec_specific_config_t *base_codec_config,
                                                                                    uint32 base_bis_indexes,
                                                                                    le_broadcast_manager_bis_index_t *bis_index_l,
                                                                                    le_broadcast_manager_bis_index_t *bis_index_r)
{
    uint8 subgroup_count, bis_count;
    bool result = FALSE;
    uint32 base_index = 0;

    memset(bis_index_l, 0, sizeof(le_broadcast_manager_bis_index_t));
    memset(bis_index_r, 0, sizeof(le_broadcast_manager_bis_index_t));

    BROADCAST_MANAGER_BASE_LOG("leBroadcastManager_FindAudioLocationOfBaseBisIndexesAndStoreCodecConfig. base_bis_indexes=0x%x", base_bis_indexes);

    for (subgroup_count=0; subgroup_count<base->numSubgroups; subgroup_count++)
    {
        if (leBroadcastManager_IsBaseCodecConfigSupportedInPacs(base,
                                                                &(base_codec_config[subgroup_count].level2_codec_specific_config),
                                                                subgroup_count))
        {
            for (bis_count=0; bis_count<base->subGroupsData[subgroup_count].numBis; bis_count++)
            {
                if (IS_BASE_BIS_INDEX_VALID(base->subGroupsData[subgroup_count].bisData[bis_count].bisIndex))
                {
                    base_index = base->subGroupsData[subgroup_count].bisData[bis_count].bisIndex;
                    uint32 base_bitfield = BIS_INDEX_TO_BITFIELD(base_index);

                    BROADCAST_MANAGER_BASE_LOG("    Compare bis_indexes base_index=0x%x base_bitfield=0x%x target_bis=0x%x",
                                    base_index,
                                    base_bitfield,
                                    base_bis_indexes);
                    if ((base_bitfield & base_bis_indexes) == base_bitfield)
                    {
                        BROADCAST_MANAGER_BASE_LOG("    bis equal channel=0x%x",
                            base_codec_config[subgroup_count].level3_codec_specific_config[bis_count].audio_channel_allocation);
                        if (base_codec_config[subgroup_count].level3_codec_specific_config[bis_count].audio_channel_allocation & PACS_AUDIO_LOCATION_LEFT)
                        {
                            bis_index_l->index = base_index;
                            bis_index_l->subgroup = subgroup_count;
                            bis_index_l->is_stereo_bis = leBroadcastManager_DoesAudioLocationMatch(base_codec_config[subgroup_count].level3_codec_specific_config[bis_count].audio_channel_allocation, PACS_AUDIO_LOCATION_RIGHT);
                            result = TRUE;
                        }
                        else if (base_codec_config[subgroup_count].level3_codec_specific_config[bis_count].audio_channel_allocation & PACS_AUDIO_LOCATION_RIGHT)
                        {
                            bis_index_r->index = base_index;
                            bis_index_r->subgroup = subgroup_count;
                            bis_index_r->is_stereo_bis = FALSE;
                            result = TRUE;
                        }
                        else if (base_codec_config[subgroup_count].level3_codec_specific_config[bis_count].audio_channel_allocation == BROADCAST_MANAGER_AUDIO_LOCATION_NOT_SET)
                        {
                            bis_index_l->index = base_index;
                            bis_index_l->subgroup = subgroup_count;
                            bis_index_l->is_stereo_bis = bis_index_r->is_stereo_bis = FALSE;
                            bis_index_r->index = bis_index_l->index;
                            bis_index_r->subgroup = bis_index_l->subgroup;
                            result = TRUE;
                        }
                    }
                }
            }
        }
        if (result)
        {
            /*  Store codec configuration values from the subgroup that the BIS belong to.
                All BIS must be from the same subgroup.
            */
            leBroadcastManager_SetBaseSubgroupCodecSpecificConfiguration(broadcast_source,
                                                                         &(base_codec_config[subgroup_count].level2_codec_specific_config));
            break;
        }
    }

    BROADCAST_MANAGER_BASE_LOG("    result=0x%x bis_left[index:0x%x subgroup:0x%x] bis_right[index:0x%x subgroup:0x%x] stereo_bis:0x%x",
                    result,
                    bis_index_l->index,
                    bis_index_l->subgroup,
                    bis_index_r->index,
                    bis_index_r->subgroup,
                    bis_index_r->is_stereo_bis
                    );
    return result;
}

static uint32 leBroadcastManager_GetBasePresentationDelay(AudioAnnouncementParserBaseData *base)
{
    return base->presentationDelay;
}

static uint8 leBroadcastManager_GetBaseNumberOfSubgroups(AudioAnnouncementParserBaseData *base)
{
    return base->numSubgroups;
}

static void leBroadcastManager_GetCodecSpecificConfiguration(uint8 *config,
                                                             uint8 config_length,
                                                             le_broadcast_manager_codec_specific_config_t *base_codec_config)
{
    AudioAnnouncementParserGetSamplingFreqFromCodecCnfg(config_length,
                                                        config,
                                                        &(base_codec_config->sample_rate));
    AudioAnnouncementParserGetFrameDurationFromCodecCnfg(config_length,
                                                         config,
                                                         &(base_codec_config->frame_duration));
    AudioAnnouncementParserGetOctetsPerCodecFrameFromCodecCnfg(config_length,
                                                               config,
                                                               &(base_codec_config->octets_per_frame));
    AudioAnnouncementParserGetAudioChannelAllocationFromCodecCnfg(config_length,
                                                                  config,
                                                                  &(base_codec_config->audio_channel_allocation));
    AudioAnnouncementParserGetCodecFrameBlocksPerSduFromCodecCnfg(config_length,
                                                                  config,
                                                                  &(base_codec_config->codec_frame_blocks_per_sdu));
}

static PacsSamplingFrequencyType leBroadcastManager_ConvertBaseToPacsSamplingFreqValue(AudioAnnouncementParserSamplingFreqType sampling_frequency)
{
    PacsSamplingFrequencyType pacs_frequency = (PacsSamplingFrequencyType)0;
    switch (sampling_frequency)
    {
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_8000:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_8KHZ;
            break;
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_16000:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_16KHZ;
            break;
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_24000:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_24KHZ;
            break;
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_32000:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_32KHZ;
             break;
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_44100:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_44_1KHZ;
            break;
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_48000:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_48KHZ;
            break;
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_11025:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_11_025KHZ;
            break;
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_176400:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_176_4KHZ;
            break;
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_192000:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_192KHZ;
            break;
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_22050:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_22_05KHZ;
            break;
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_384000:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_384KHZ;
            break;
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_88200:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_88_2KHZ;
            break;
        case AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_96000:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_96KHZ;
            break;
        default:
            break;
    }

    return pacs_frequency;
}

static PacsFrameDurationType leBroadcastManager_ConvertBaseToPacsFrameDurationValue(AudioAnnouncementParserFrameDurationType frame_duration)
{
    PacsFrameDurationType pacs_duration = (PacsFrameDurationType)0;
    switch (frame_duration)
    {
        case AUDIO_ANNOUNCEMENT_PARSER_FRAME_DURATION_TYPE_7_5:
            pacs_duration = PACS_SUPPORTED_FRAME_DURATION_7P5MS;
            break;
        case AUDIO_ANNOUNCEMENT_PARSER_FRAME_DURATION_TYPE_10:
            pacs_duration = PACS_SUPPORTED_FRAME_DURATION_10MS;
            break;
        default:
            break;
    }

    return pacs_duration;
}

static bool leBroadcastManager_IsBaseCodecConfigSupportedInPacs(AudioAnnouncementParserBaseData *base,
                                                                le_broadcast_manager_codec_specific_config_t *base_codec_config,
                                                                uint8 subgroup)
{
    bool codec_config_supported = FALSE;

    if (LeBapPacsUtilities_IsCodecIdAndSpecificCapabilitiesSupportedBySink(
                            base->subGroupsData[subgroup].codecId.codingFormat,
                            base->subGroupsData[subgroup].codecId.companyId,
                            base->subGroupsData[subgroup].codecId.vendorSpecificCodecId,
                            leBroadcastManager_ConvertBaseToPacsSamplingFreqValue(base_codec_config->sample_rate),
                            leBroadcastManager_ConvertBaseToPacsFrameDurationValue(base_codec_config->frame_duration),
                            base_codec_config->octets_per_frame))
    {
        BROADCAST_MANAGER_BASE_LOG("leBroadcastManager_IsBaseCodecConfigSupportedInPacs Codec ID and Capabilities supported, subgroup:%u", subgroup);
        codec_config_supported = TRUE;
    }
    else
    {
        BROADCAST_MANAGER_BASE_LOG("leBroadcastManager_IsBaseCodecConfigSupportedInPacs FAILED CODEC CONFIGURATION CHECK, subgroup:%u", subgroup);
    }

    return codec_config_supported;
}

static void leBroadcastManager_FreeCodecSpecificConfig(le_broadcast_manager_subgroup_codec_specific_config_t *base_codec_config,
                                                      uint8 num_subgroups)
{
    uint8 subgroup_count = 0;

    if(base_codec_config)
    {
        for (subgroup_count=0; subgroup_count<num_subgroups; subgroup_count++)
        {
            if(base_codec_config[subgroup_count].level3_codec_specific_config)
                free(base_codec_config[subgroup_count].level3_codec_specific_config);
        }

        free(base_codec_config);
    }
}

#ifdef ENABLE_LE_AUDIO_BIS_SELECTION
static void leBroadcastManager_FreeMetadata(le_broadcast_manager_subgroup_metadata_t **base_metadata,
                                            uint8 num_subgroups)
{
    uint8 subgroup_count = 0;

    if(base_metadata)
    {
        for (subgroup_count=0; subgroup_count<num_subgroups; subgroup_count++)
        {
            if(base_metadata[subgroup_count])
            {
                if(base_metadata[subgroup_count]->language)
                    free(base_metadata[subgroup_count]->language);
                free(base_metadata[subgroup_count]);
            }
        }

        free(base_metadata);
    }
}
#endif

static le_broadcast_manager_subgroup_codec_specific_config_t *leBroadcastManager_GetAndStoreCodecSpecificConfig(AudioAnnouncementParserBaseData *base_data)
{
    size_t size = base_data->numSubgroups * sizeof(le_broadcast_manager_subgroup_codec_specific_config_t);
    le_broadcast_manager_subgroup_codec_specific_config_t *base_codec_config = PanicUnlessMalloc(size);
    uint8 subgroup_count, bis_count = 0;

    memset(base_codec_config, 0, (base_data->numSubgroups * sizeof(le_broadcast_manager_subgroup_codec_specific_config_t)));

    for (subgroup_count=0; subgroup_count<base_data->numSubgroups; subgroup_count++)
    {
        size_t size_bis = base_data->subGroupsData[subgroup_count].numBis * sizeof(le_broadcast_manager_bis_audio_channel_allocation_t);

        //memset(&base_codec_config[subgroup_count], 0, sizeof(le_broadcast_manager_bis_audio_channel_allocation_t));

        if(base_data->subGroupsData[subgroup_count].codecSpecificConfigLen)
        {
            leBroadcastManager_GetCodecSpecificConfiguration(base_data->subGroupsData[subgroup_count].codecSpecificConfig,
                                                             base_data->subGroupsData[subgroup_count].codecSpecificConfigLen,
                                                             &(base_codec_config[subgroup_count].level2_codec_specific_config));
        }

        base_codec_config[subgroup_count].num_bis = base_data->subGroupsData[subgroup_count].numBis;
        base_codec_config[subgroup_count].level3_codec_specific_config = PanicUnlessMalloc(size_bis);
        memset(base_codec_config[subgroup_count].level3_codec_specific_config, 0, size_bis);

        for (bis_count=0; bis_count<base_data->subGroupsData[subgroup_count].numBis; bis_count++)
        {
            base_codec_config[subgroup_count].level3_codec_specific_config[bis_count].bis_index = base_data->subGroupsData[subgroup_count].bisData[bis_count].bisIndex;

            if(base_data->subGroupsData[subgroup_count].bisData[bis_count].codecSpecificConfigLen)
            {
                leBroadcastManager_StoreBaseLevel3CodecSpecificConfiguration(base_data->subGroupsData[subgroup_count].bisData[bis_count].codecSpecificConfigLen,
                                                                             base_data->subGroupsData[subgroup_count].bisData[bis_count].codecSpecificConfig,
                                                                             bis_count,
                                                                             &base_codec_config[subgroup_count]);
            }
        }
    }

    return base_codec_config;
}

#ifdef ENABLE_LE_AUDIO_BIS_SELECTION
static le_broadcast_manager_subgroup_metadata_t **leBroadcastManager_GetMetadata(AudioAnnouncementParserBaseData *base_data)
{
    size_t size = base_data->numSubgroups * sizeof(le_broadcast_manager_subgroup_metadata_t *);
    le_broadcast_manager_subgroup_metadata_t **base_metadata = PanicUnlessMalloc(size);
    uint8 subgroup_count = 0;
    AudioAnnouncementParserAudioActiveState audio_active_state_subgroup = 0 ;

    for (subgroup_count=0; subgroup_count<base_data->numSubgroups; subgroup_count++)
    {
        if(base_data->subGroupsData[subgroup_count].metadata)
        {
           base_metadata[subgroup_count] = PanicUnlessMalloc(sizeof(le_broadcast_manager_subgroup_metadata_t));

           if(AudioAnnouncementParserGetAudioActiveStateFromMetadata(base_data->subGroupsData[subgroup_count].metadataLen,
                                                                     base_data->subGroupsData[subgroup_count].metadata,
                                                                     &audio_active_state_subgroup) == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS)
           {
               base_metadata[subgroup_count]->audio_active_state = audio_active_state_subgroup;
           }
           else
           {
               base_metadata[subgroup_count]->audio_active_state = BROADCAST_MANAGER_AUDIO_ACTIVE_STATE_NO_PRESENT;
           }

           base_metadata[subgroup_count]->language = AudioAnnouncementParserGetLanguageFromMetadata(base_data->subGroupsData[subgroup_count].metadataLen,
                                                                                                    base_data->subGroupsData[subgroup_count].metadata,
                                                                                                    &(base_metadata[subgroup_count]->languageLen));

        }
        else
        {
            base_metadata[subgroup_count] = NULL;
        }
    }

    return base_metadata;
}
#endif

void leBroadcastManager_FreeBaseData(AudioAnnouncementParserBaseData *base)
{
    uint8 subgroup_count, bis_count;

    if(base)
    {
        if(base->subGroupsData)
        {
            for (subgroup_count=0; subgroup_count<base->numSubgroups; subgroup_count++)
            {
                for (bis_count=0; bis_count<base->subGroupsData[subgroup_count].numBis; bis_count++)
                {
                    if(base->subGroupsData[subgroup_count].bisData[bis_count].codecSpecificConfig)
                        free(base->subGroupsData[subgroup_count].bisData[bis_count].codecSpecificConfig);
                }

                if(base->subGroupsData[subgroup_count].bisData)
                    free(base->subGroupsData[subgroup_count].bisData);

                if(base->subGroupsData[subgroup_count].codecSpecificConfig)
                    free(base->subGroupsData[subgroup_count].codecSpecificConfig);

                if(base->subGroupsData[subgroup_count].metadata)
                    free(base->subGroupsData[subgroup_count].metadata);
            }

            free(base->subGroupsData);
        }
    }
}

void leBroadcastManager_ParseBaseReport(broadcast_source_state_t *broadcast_source,
                                        uint16 data_length_adv,
                                        const uint8 *data_adv,
                                        bool check_bis_index,
                                        uint32 target_bis_sync_state
                                        )
{
    le_broadcast_manager_bis_index_t bis_index_l = {0}, bis_index_r = {0};
    bool bis_index_found = FALSE;
    AudioAnnouncementParserBaseData base_data;
    AudioAnnouncementParserStatus status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND;

    status = AudioAnnouncementParserBasicAudioAnnouncementParsing(data_length_adv,
                                                         (uint8 *) data_adv,
                                                         &base_data);

    if(status == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS)
    {
        BROADCAST_MANAGER_BASE_LOG("leBroadcastManager_ParseBaseReport Found Basic Audio Announcement");
        DEBUG_LOG_DATA_VERBOSE(data_adv, data_length_adv);

#ifdef ENABLE_LE_AUDIO_BIS_SELECTION
        le_broadcast_manager_subgroup_metadata_t **metadata = leBroadcastManager_GetMetadata(&base_data);
#endif

        le_broadcast_manager_subgroup_codec_specific_config_t *base_codec_config = leBroadcastManager_GetAndStoreCodecSpecificConfig(&base_data);

        leBroadcastManager_SetBroadcastSourcePresentationDelay(broadcast_source,
                                                               leBroadcastManager_GetBasePresentationDelay(&base_data));
        leBroadcastManager_SetBroadcastSourceNumberOfSubgroups(broadcast_source,
                                                               leBroadcastManager_GetBaseNumberOfSubgroups(&base_data));

        if (check_bis_index)
        {
            /* Need to find which BIS to sync to from BASE information in this case. */
            BROADCAST_MANAGER_BASE_LOG("leBroadcastManager_ParseBaseReport Search for bis_index");

            if(base_codec_config)
            {
#ifdef ENABLE_LE_AUDIO_BIS_SELECTION
                bis_index_found = leBroadcastManager_FindBestBaseBisIndexes(broadcast_source,
                                                                            base_codec_config,
                                                                            &base_data,
                                                                            metadata,
                                                                            &bis_index_l,
                                                                            &bis_index_r);
#else
                /* todo Compare with PACS record to see if compatible */
                bis_index_found = leBroadcastManager_FindCompatibleBaseBisIndexesAndStoreCodecConfig(broadcast_source,
                                                                                                     base_codec_config,
                                                                                                     &base_data,
                                                                                                     &bis_index_l,
                                                                                                     &bis_index_r);
#endif
            }
        }
        else if (target_bis_sync_state)
        {
            /* Have been told which BIS to sync to by an assistant in this case. */
            BROADCAST_MANAGER_BASE_LOG("leBroadcastManager_ParseBaseReport Use assistant bis_index:0x%x", target_bis_sync_state);

            /* If assistant has said to sync to BIS, need to find out the audio location of the BIS indexes,
               and store which index to use for left and right channels.
            */
            bis_index_found = leBroadcastManager_FindAudioLocationOfBaseBisIndexesAndStoreCodecConfig(broadcast_source,
                                                                                                      &base_data,
                                                                                                      base_codec_config,
                                                                                                      target_bis_sync_state,
                                                                                                      &bis_index_l,
                                                                                                      &bis_index_r);
        }

       if (bis_index_found)
       {
                BROADCAST_MANAGER_BASE_LOG("leBroadcastManager_ParseBaseReport Found bis_index_l[index:0x%x subgroup:0x%x] bis_index_r[index:0x%x subgroup:0x%x] in BASE",
                                      bis_index_l.index,
                                      bis_index_l.subgroup,
                                      bis_index_r.index,
                                      bis_index_r.subgroup);
           leBroadcastManager_SetBaseBisIndex(broadcast_source, &bis_index_l, broadcast_manager_bis_location_left_or_stereo);
           leBroadcastManager_SetBaseBisIndex(broadcast_source, &bis_index_r, broadcast_manager_bis_location_right);
        }

        leBroadcastManager_FreeCodecSpecificConfig(base_codec_config, base_data.numSubgroups);
#ifdef ENABLE_LE_AUDIO_BIS_SELECTION
        leBroadcastManager_FreeMetadata(metadata, base_data.numSubgroups);
#endif
        leBroadcastManager_FreeBaseData(&base_data);
    }
}

void leBroadcastManager_SetBaseBisIndex(broadcast_source_state_t *broadcast_source, const le_broadcast_manager_bis_index_t *base_bis_index, uint8 index)
{
    broadcast_source->bis_info[index].base_bis_index = *base_bis_index;
}

#endif /* INCLUDE_LE_AUDIO_BROADCAST */

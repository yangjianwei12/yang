/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    leabm
    \brief      LE Broadcast Manager module to handle the selection of the BIS to syncronize to.
*/

#if defined(INCLUDE_LE_AUDIO_BROADCAST)

#include "le_broadcast_manager_base.h"
#include "le_broadcast_manager_data.h"
#include "le_broadcast_manager_bis_selection.h"

#include "pacs_utilities.h"
#include "device_info.h"

#define BROADCAST_MANAGER_BIS_SELECTION_LOG  DEBUG_LOG
#define IS_BASE_BIS_INDEX_VALID(bis_index)          ((bis_index >= 1) && (bis_index <= 31))

static bool leBroadcastManager_IsBaseCodecConfigSupportedInPacs(AudioAnnouncementParserBaseData *base,
                                                                le_broadcast_manager_codec_specific_config_t *base_codec_config,
                                                                uint8 subgroup);
static PacsFrameDurationType leBroadcastManager_ConvertBaseToPacsFrameDurationValue(AudioAnnouncementParserFrameDurationType frame_duration);
static bool leBroadcastManager_SelectAndSetBisIndexes(AudioAnnouncementParserBaseData *base,
                                                      le_broadcast_manager_subgroup_codec_specific_config_t *base_codec_config,
                                                      uint8 subgroup_index,
                                                      le_broadcast_manager_bis_index_t *bis_index_l,
                                                      le_broadcast_manager_bis_index_t *bis_index_r);

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
        BROADCAST_MANAGER_BIS_SELECTION_LOG("leBroadcastManager_IsBaseCodecConfigSupportedInPacs Codec ID and Capabilities supported, subgroup:%u", subgroup);
        codec_config_supported = TRUE;
    }
    else
    {
        BROADCAST_MANAGER_BIS_SELECTION_LOG("leBroadcastManager_IsBaseCodecConfigSupportedInPacs FAILED CODEC CONFIGURATION CHECK, subgroup:%u", subgroup);
    }

    return codec_config_supported;
}

static bool leBroadcastManager_DoesLanguageMatch(le_broadcast_manager_subgroup_metadata_t *base_metadata)
{
    if(!memcmp(DeviceInfo_GetCurrentLanguage(), "en", strlen(DeviceInfo_GetCurrentLanguage())))
    {
        if(!memcmp(base_metadata->language, "eng", base_metadata->languageLen))
            return TRUE;
    }

    return FALSE;
}

static bool leBroadcastManager_SelectAndSetBisIndexes(AudioAnnouncementParserBaseData *base,
                                                      le_broadcast_manager_subgroup_codec_specific_config_t *base_codec_config,
                                                      uint8 subgroup_index,
                                                      le_broadcast_manager_bis_index_t *bis_index_l,
                                                      le_broadcast_manager_bis_index_t *bis_index_r)
{
    uint8 bis_count = 0;
    bool result = FALSE;

    for (bis_count=0; bis_count<base->subGroupsData[subgroup_index].numBis; bis_count++)
    {
        if (IS_BASE_BIS_INDEX_VALID(base->subGroupsData[subgroup_index].bisData[bis_count].bisIndex))
        {
            if (base_codec_config[subgroup_index].level3_codec_specific_config[bis_count].audio_channel_allocation == BROADCAST_MANAGER_AUDIO_LOCATION_NOT_SET)
            {
                bis_index_l->index = base->subGroupsData[subgroup_index].bisData[bis_count].bisIndex;
                bis_index_l->subgroup = subgroup_index;
                bis_index_l->is_stereo_bis = bis_index_r->is_stereo_bis = FALSE;
                bis_index_r->index = bis_index_l->index;
                bis_index_r->subgroup = bis_index_l->subgroup;
                result = TRUE;
            }
            else if (leBroadcastManager_DoesAudioLocationMatch(base_codec_config[subgroup_index].level3_codec_specific_config[bis_count].audio_channel_allocation,
                                                               PACS_AUDIO_LOCATION_LEFT))
            {
                bis_index_l->index = base->subGroupsData[subgroup_index].bisData[bis_count].bisIndex;
                bis_index_l->subgroup = subgroup_index;
                bis_index_l->is_stereo_bis = leBroadcastManager_DoesAudioLocationMatch(base_codec_config[subgroup_index].level3_codec_specific_config[bis_count].audio_channel_allocation, PACS_AUDIO_LOCATION_RIGHT);
                result = TRUE;
            }
            else if (leBroadcastManager_DoesAudioLocationMatch(base_codec_config[subgroup_index].level3_codec_specific_config[bis_count].audio_channel_allocation,
                                                               PACS_AUDIO_LOCATION_RIGHT))
            {
                bis_index_r->index = base->subGroupsData[subgroup_index].bisData[bis_count].bisIndex;
                bis_index_r->subgroup = subgroup_index;
                bis_index_r->is_stereo_bis = FALSE;
                result = TRUE;
            }
        }
    }

    return result;
}

bool leBroadcastManager_FindBestBaseBisIndexes(broadcast_source_state_t *broadcast_source,
                                               le_broadcast_manager_subgroup_codec_specific_config_t *base_codec_config,
                                               AudioAnnouncementParserBaseData *base,
                                               le_broadcast_manager_subgroup_metadata_t **base_metadata,
                                               le_broadcast_manager_bis_index_t *bis_index_l,
                                               le_broadcast_manager_bis_index_t *bis_index_r)
{
    uint8 subgroup_count = 0;
    bool result = FALSE;
    uint8 selected_subgroups_count = 0;
    uint8 next_selected_subgroups_count = 0;
    uint8 *selected_subgroup_indexes = PanicUnlessMalloc(sizeof(uint8) * base->numSubgroups);
    uint8 *next_selected_subgroup_indexes = NULL;
    uint8 subgroup_index = 0;

    memset(bis_index_l, 0, sizeof(le_broadcast_manager_bis_index_t));
    memset(bis_index_r, 0, sizeof(le_broadcast_manager_bis_index_t));

    for (subgroup_count=0; subgroup_count<base->numSubgroups; subgroup_count++)
    {
        if (leBroadcastManager_IsBaseCodecConfigSupportedInPacs(base,
                                                                &(base_codec_config[subgroup_count].level2_codec_specific_config),
                                                                subgroup_count))
        {
            selected_subgroups_count++;
            selected_subgroup_indexes[selected_subgroups_count-1] = subgroup_count;
        }
    }

    if (!selected_subgroups_count)
    {
        /* We didn't find any subgroup with a supported codec configuration*/
        BROADCAST_MANAGER_BIS_SELECTION_LOG("LeBroadcastManager_FindCompatibleBaseBisIndexesAndStoreCodecConfig no BIS with a supported codec specifi configuration!");
    }
    else if (selected_subgroups_count == 1)
    {
        /* We found only one subgroup with a supported code configuration */
        subgroup_index = *selected_subgroup_indexes;

        result = leBroadcastManager_SelectAndSetBisIndexes(base,
                                                           base_codec_config,
                                                           subgroup_index,
                                                           bis_index_l,
                                                           bis_index_r);
    }
    else
    {
        /* More than one subgroup with a supported codec configuration has been found */
        uint8 index = 0;

        next_selected_subgroup_indexes = PanicUnlessMalloc(sizeof(uint8) * base->numSubgroups);

        for (subgroup_count=0; subgroup_count<selected_subgroups_count; subgroup_count++)
        {
            /* We need to check how many subrgroups have the Audio Active State LTV structure in their metadata set to 1
                 * ("Audio data is being transmitted")
                 */
            index = selected_subgroup_indexes[subgroup_count];

            if (!base_metadata[index] ||
                    base_metadata[index]->audio_active_state == BROADCAST_MANAGER_AUDIO_ACTIVE_STATE_NO_PRESENT ||
                    base_metadata[index]->audio_active_state == AUDIO_ANNOUNCEMENT_PARSER_AUDIO_ACTIVE_STATE_TYPE_AUDIO_TRANSMITTED)
            {
                next_selected_subgroups_count++;
                next_selected_subgroup_indexes[next_selected_subgroups_count-1] = index;
            }
        }

        if(next_selected_subgroups_count == 1)
        {
            /* We found only one subgroup with Audio Active state set to 1 */
            subgroup_index = *next_selected_subgroup_indexes;

            result = leBroadcastManager_SelectAndSetBisIndexes(base,
                                                               base_codec_config,
                                                               subgroup_index,
                                                               bis_index_l,
                                                               bis_index_r);
        }
        else
        {
            /* More than one subgroup with Audio Active State set to 1 have been found */
            selected_subgroups_count = next_selected_subgroups_count;
            next_selected_subgroups_count = 0;
            memcpy(selected_subgroup_indexes, next_selected_subgroup_indexes, selected_subgroups_count);

            for (subgroup_count=0; subgroup_count<selected_subgroups_count; subgroup_count++)
            {
                index = selected_subgroup_indexes[subgroup_count];

                if(!base_metadata[index] ||
                        !base_metadata[index]->language ||
                        leBroadcastManager_DoesLanguageMatch(base_metadata[index]))
                {
                    next_selected_subgroups_count++;
                    next_selected_subgroup_indexes[next_selected_subgroups_count-1] = index;
                }
            }

            if(next_selected_subgroups_count == 1)
            {
                /* We found only one subgroup with language that matches the device language */
                subgroup_index = *next_selected_subgroup_indexes;

                result = leBroadcastManager_SelectAndSetBisIndexes(base,
                                                                   base_codec_config,
                                                                   subgroup_index,
                                                                   bis_index_l,
                                                                   bis_index_r);
            }
            else
            {
                /* More than one subgroup with language that matches the device language have been found:
                     * we will select the subgroup with the best audio quality.
                     */
                AudioAnnouncementParserSamplingFreqType selected_sampling_freq;

                selected_subgroups_count = next_selected_subgroups_count;
                next_selected_subgroups_count = 0;
                memcpy(selected_subgroup_indexes, next_selected_subgroup_indexes, selected_subgroups_count);

                index = selected_subgroup_indexes[0];
                selected_sampling_freq = base_codec_config[index].level2_codec_specific_config.sample_rate;
                next_selected_subgroups_count++;
                next_selected_subgroup_indexes[next_selected_subgroups_count-1] = index;

                for(subgroup_count=1; subgroup_count<selected_subgroups_count; subgroup_count++)
                {
                    index = selected_subgroup_indexes[subgroup_count];
                    if(base_codec_config[index].level2_codec_specific_config.sample_rate == selected_sampling_freq)
                    {
                        next_selected_subgroups_count++;
                        next_selected_subgroup_indexes[next_selected_subgroups_count-1] = index;
                    }
                    else if(base_codec_config[index].level2_codec_specific_config.sample_rate > selected_sampling_freq)
                    {
                        selected_sampling_freq = base_codec_config[index].level2_codec_specific_config.sample_rate;
                        next_selected_subgroups_count = 1;
                        next_selected_subgroup_indexes[next_selected_subgroups_count-1] = index;
                    }
                }

                if(next_selected_subgroups_count == 1)
                {
                    /* We found only the subgroup with the best audio quality */
                    subgroup_index = *next_selected_subgroup_indexes;

                    result = leBroadcastManager_SelectAndSetBisIndexes(base,
                                                                       base_codec_config,
                                                                       subgroup_index,
                                                                       bis_index_l,
                                                                       bis_index_r);
                }
                else
                {
                    AudioAnnouncementParserFrameDurationType selected_frame_duration;

                    selected_subgroups_count = next_selected_subgroups_count;
                    next_selected_subgroups_count = 0;
                    memcpy(selected_subgroup_indexes, next_selected_subgroup_indexes, selected_subgroups_count);

                    index = selected_subgroup_indexes[0];
                    selected_frame_duration = base_codec_config[index].level2_codec_specific_config.frame_duration;
                    next_selected_subgroups_count++;
                    next_selected_subgroup_indexes[next_selected_subgroups_count-1] = index;

                    for(subgroup_count=1; subgroup_count<selected_subgroups_count; subgroup_count++)
                    {
                        index = selected_subgroup_indexes[subgroup_count];
                        if(base_codec_config[index].level2_codec_specific_config.frame_duration == selected_frame_duration)
                        {
                            next_selected_subgroups_count++;
                            next_selected_subgroup_indexes[next_selected_subgroups_count-1] = index;
                        }
                        else if(base_codec_config[index].level2_codec_specific_config.frame_duration > selected_frame_duration)
                        {
                            selected_frame_duration = base_codec_config[index].level2_codec_specific_config.frame_duration;
                            next_selected_subgroups_count = 1;
                            next_selected_subgroup_indexes[next_selected_subgroups_count-1] = index;
                        }
                    }

                    if(next_selected_subgroups_count == 1)
                    {
                        /* We found only the subgroup with the best audio quality */
                        subgroup_index = *next_selected_subgroup_indexes;

                        result = leBroadcastManager_SelectAndSetBisIndexes(base,
                                                                           base_codec_config,
                                                                           subgroup_index,
                                                                           bis_index_l,
                                                                           bis_index_r);
                    }
                    else
                    {
                        /* Since we have no other creteria to satisfy, we will go with the first one we found */
                        subgroup_index = next_selected_subgroup_indexes[0];

                        result = leBroadcastManager_SelectAndSetBisIndexes(base,
                                                                           base_codec_config,
                                                                           subgroup_index,
                                                                           bis_index_l,
                                                                           bis_index_r);
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
                                                                     &(base_codec_config[subgroup_index].level2_codec_specific_config));
        AudioAnnouncementParserGetStreamAudioContextFromMetadata(base->subGroupsData[subgroup_index].metadataLen,
                                                                 base->subGroupsData[subgroup_index].metadata,
                                                                 (AudioContextType *) &(broadcast_source->streaming_audio_context));
    }

    BROADCAST_MANAGER_BIS_SELECTION_LOG("    result=0x%x bis_index_l=0x%x bis_index_r=0x%x", result, bis_index_l->index, bis_index_r->index);

    free(selected_subgroup_indexes);
    free(next_selected_subgroup_indexes);

    return result;
}

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST) */

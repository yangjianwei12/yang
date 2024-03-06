/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup leabm
    \brief      Handles the Broadcast Audio Source Endpoint (BASE) structure that is present in the periodic advertising.
    @{
*/

#ifndef LE_BROADCAST_MANAGER_BASE_H_
#define LE_BROADCAST_MANAGER_BASE_H_

#if defined(INCLUDE_LE_AUDIO_BROADCAST)

#include "le_broadcast_manager_data.h"

#include <audio_announcement_parser_lib.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct
{
    AudioAnnouncementParserSamplingFreqType sample_rate;
    AudioAnnouncementParserFrameDurationType frame_duration;
    uint16 octets_per_frame;
    uint32 audio_channel_allocation;
    uint8 codec_frame_blocks_per_sdu;
}le_broadcast_manager_codec_specific_config_t;

typedef struct
{
    uint8 bis_index;
    uint32 audio_channel_allocation;
}le_broadcast_manager_bis_audio_channel_allocation_t;

typedef struct
{
    le_broadcast_manager_codec_specific_config_t level2_codec_specific_config;
    uint8 num_bis;
    le_broadcast_manager_bis_audio_channel_allocation_t *level3_codec_specific_config;
}le_broadcast_manager_subgroup_codec_specific_config_t;


typedef struct
{
    AudioAnnouncementParserAudioActiveState audio_active_state;
    uint8 languageLen;
    char *language;
}le_broadcast_manager_subgroup_metadata_t;

/*\{*/

#define leBroadcastManager_IsCodecConfigurationValueSet(codec_config_value)        (codec_config_value != 0)
#define leBroadcastManager_HasHigherLevelCodecConfigChangedFromLowerLevel(higher_level_value, lower_level_value) \
            ((leBroadcastManager_IsCodecConfigurationValueSet(lower_level_value)) && \
                (lower_level_value != higher_level_value))

/*! \brief Parse the BASE information in the PA report.

    \param broadcast_source The broadcast source associated with the PA report.
    \param data_length_adv The length of the advert report.
    \param data_adv The advert report.
    \param check_bis_index TRUE if the BIS index in the BASE report should be parsed. FALSE otherwise.
    \param target_bis_sync_state The intended BIS index to sync to (eg. as specified by a Broadcast Assistant). 
*/
void leBroadcastManager_ParseBaseReport(broadcast_source_state_t *broadcast_source, 
                                        uint16 data_length_adv,
                                        const uint8 *data_adv,
                                        bool check_bis_index,
                                        uint32 target_bis_sync_state
                                        );

/*! \brief Sets the BIS index to sync to for the left and right channels. The BIS index are found from the BASE information in the PA report.
    
    \param broadcast_source The broadcast source associated with the PA report.
    \param base_bis_index The BIS index to sync.
    \param bis_loc BIS location.
*/
void leBroadcastManager_SetBaseBisIndex(broadcast_source_state_t *broadcast_source, const le_broadcast_manager_bis_index_t *base_bis_index, uint8 bis_loc);

/*! \brief Sets the the codec specific configuration of the subgroup to synchronize to.

    \param broadcast_source The broadcast source associated with the PA report.
    \param base_codec_specific_config Codec specific configuration to set.
*/
void leBroadcastManager_SetBaseSubgroupCodecSpecificConfiguration(broadcast_source_state_t *broadcast_source,
                                                                  le_broadcast_manager_codec_specific_config_t *base_codec_specific_config);

/*! \brief Frees the memory allocated for BASE data record.

    \param BASE data.
*/
void leBroadcastManager_FreeBaseData(AudioAnnouncementParserBaseData *base);

/*! @} */

#endif /* INCLUDE_LE_AUDIO_BROADCAST */
#endif /* LE_BROADCAST_MANAGER_BASE_H_ */

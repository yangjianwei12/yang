/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup leabm
    \brief      LE Broadcast Manager module to handle the selection of the BIS to syncronize to.
    @{
*/

#ifndef LE_BROADCAST_MANAGER_BIS_SELECTION_H_
#define LE_BROADCAST_MANAGER_BIS_SELECTION_H_

/*! \brief Initialises the LE Broadcast Manager handling of broadcast sources.

    Initialises the LE Broadcast Manager message handlers and the initial state of the stored broadcast sources.
 */
bool leBroadcastManager_FindBestBaseBisIndexes(broadcast_source_state_t *broadcast_source,
                                               le_broadcast_manager_subgroup_codec_specific_config_t *base_codec_config,
                                               AudioAnnouncementParserBaseData *base,
                                               le_broadcast_manager_subgroup_metadata_t **base_metadata,
                                               le_broadcast_manager_bis_index_t *bis_index_l,
                                               le_broadcast_manager_bis_index_t *bis_index_r);

#endif /* LE_BROADCAST_MANAGER_BIS_SELECTION_H_ */

/*! @} */

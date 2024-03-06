/*!
\copyright  Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header for AVRCP Metadata functionality. This file is mainly used to provide functionality
            that is required for PTS testing and not general use cases.
*/

#ifndef AVRCP_PROFILE_METADATA_H_
#define AVRCP_PROFILE_METADATA_H_

#ifdef INCLUDE_AVRCP_METADATA

#include <avrcp.h>
#include <av_instance.h>
#include <avrcp_profile_abstraction.h>

void AvrcpMetadata_SetTrackSelected(bool is_selected);
void AvrcpMetadata_SetPlayStatus(avInstanceTaskData *the_inst, avrcp_play_status play_status);
void AvrcpMetadata_SetLargeMetadata(bool use_large_metadata);
bool AvrcpMetadata_SendTrackChange(const bdaddr *bt_addr, uint32 high_index, uint32 low_index);
void AvrcpMetadata_HandleEventTrackChanged(avInstanceTaskData* the_inst, avrcp_response_type response, uint32 msg_id);
void AvrcpMetadata_HandleGetElementAttributesInd(avInstanceTaskData *the_inst, AVRCP_GET_ELEMENT_ATTRIBUTES_IND_T *ind);
void AvrcpMetadata_HandleGetPlayStatusInd(avInstanceTaskData *the_inst, AVRCP_GET_PLAY_STATUS_IND_T *ind);

#else

#define AvrcpMetadata_HandleEventTrackChanged(the_inst, response, msgid)
#define AvrcpMetadata_HandleGetElementAttributesInd(the_inst, ind)
#define AvrcpMetadata_HandleGetPlayStatusInd(the_inst, ind)
#define AvrcpMetadata_SetTrackSelected(is_selected)
#define AvrcpMetadata_SetPlayStatus(the_inst, play_status)
#define AvrcpMetadata_SendTrackChange(bt_addr, high_index, low_index) FALSE
#define AvrcpMetadata_SetLargeMetadata(use_large_metadata)

#endif /* INCLUDE_AVRCP_METADATA */
#endif /* AVRCP_PROFILE_METADATA_H_ */

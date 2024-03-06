/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\addtogroup select_focus_domains
\brief      Focus Select Config Types API
@{
*/
#ifndef FOCUS_SELECT_CONFIG_TYPES_H
#define FOCUS_SELECT_CONFIG_TYPES_H

typedef enum
{
    focus_select_enable_media_barge_in,
    focus_select_enable_transport_based_ordering,
    focus_select_block_disconnect_with_dfu,
    focus_select_block_disconnect_with_audio
} focus_select_config_option_t;

#endif /* FOCUS_SELECT_CONFIG_TYPES_H */

/*! @} !*/
/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\addtogroup select_focus_domains
\brief      Focus Select Config API
@{
*/
#ifndef FOCUS_SELECT_CONFIG_H
#define FOCUS_SELECT_CONFIG_H

typedef struct
{
    bool media_barge_in_enabled;
    bool transport_based_ordering_enabled;
    bool block_disconnect_with_dfu;
    bool block_disconnect_with_audio;
} focus_select_config_t;

extern focus_select_config_t focus_select_config;

/*! \brief Initialise the configuration callbacks and defaults */
void FocusSelect_ConfigInit(void);

#endif /* FOCUS_SELECT_CONFIG_H */

/*! @} !*/
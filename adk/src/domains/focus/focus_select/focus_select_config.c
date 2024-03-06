/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      This module is an implementation of the focus interface for config
*/

#include "focus_select.h"
#include "focus_select_config.h"
#include "focus_select_config_types.h"
#include "focus_select_ui.h"

#include <focus_plugin.h>

#ifndef ENABLE_MEDIA_BARGE_IN
#define ENABLE_MEDIA_BARGE_IN FALSE
#endif

#ifndef ENABLE_TRANSPORT_BASED_ORDERING_IN_TIE_BREAK
#define ENABLE_TRANSPORT_BASED_ORDERING_IN_TIE_BREAK FALSE
#endif

#ifndef BLOCK_DISCONNECT_OF_CONNECTED_DFU
#define BLOCK_DISCONNECT_OF_CONNECTED_DFU FALSE
#endif

#ifndef BLOCK_DISCONNECT_OF_ACTIVE_AUDIO
#define BLOCK_DISCONNECT_OF_ACTIVE_AUDIO FALSE
#endif

static bool FocusSelect_SetConfig(unsigned config_option, void* config);
static bool FocusSelect_GetConfig(unsigned config_option, void* config);

static const focus_plugin_configure_t focus_select_configure_fns = 
{
    .set_config = FocusSelect_SetConfig,
    .get_config = FocusSelect_GetConfig
};

focus_select_config_t focus_select_config;

void FocusSelect_ConfigInit(void)
{
    focus_select_config.media_barge_in_enabled = ENABLE_MEDIA_BARGE_IN;
    focus_select_config.transport_based_ordering_enabled = ENABLE_TRANSPORT_BASED_ORDERING_IN_TIE_BREAK;
    focus_select_config.block_disconnect_with_dfu = BLOCK_DISCONNECT_OF_CONNECTED_DFU;
    focus_select_config.block_disconnect_with_audio = BLOCK_DISCONNECT_OF_ACTIVE_AUDIO;
    
    Focus_PluginConfigure(&focus_select_configure_fns);
}

static bool FocusSelect_SetConfig(unsigned config_option, void* config)
{
    bool success = FALSE;
    
    switch(config_option)
    {
        case focus_select_enable_media_barge_in:
            focus_select_config.media_barge_in_enabled = *(bool*)config;
            success = TRUE;
        break;

        case focus_select_enable_transport_based_ordering:
            focus_select_config.transport_based_ordering_enabled = *(bool*)config;
            success = TRUE;
        break;

        case focus_select_block_disconnect_with_dfu:
            focus_select_config.block_disconnect_with_dfu = *(bool*)config;
            success = TRUE;
        break;
        
        case focus_select_block_disconnect_with_audio:
            focus_select_config.block_disconnect_with_audio = *(bool*)config;
            success = TRUE;
        break;

        default:
        break;
    }
    
    return success;
}

static bool FocusSelect_GetConfig(unsigned config_option, void* config)
{
    bool success = FALSE;
    
    switch(config_option)
    {
        case focus_select_enable_media_barge_in:
            *(bool*)config = focus_select_config.media_barge_in_enabled;
            success = TRUE;
        break;

        case focus_select_enable_transport_based_ordering:
            *(bool*)config = focus_select_config.transport_based_ordering_enabled;
            success = TRUE;
        break;

        case focus_select_block_disconnect_with_dfu:
            *(bool*)config = focus_select_config.block_disconnect_with_dfu;
            success = TRUE;
        break;
        
        case focus_select_block_disconnect_with_audio:
            *(bool*)config = focus_select_config.block_disconnect_with_audio;
            success = TRUE;
        break;

        default:
        break;
    }
    
    return success;
}

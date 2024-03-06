/*!
\copyright  Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       usb_dongle_ui_config.c
\brief      ui configuration table

    This file contains ui configuration table which maps different logical inputs to
    corresponding ui inputs based upon ui provider contexts.
*/

#include "usb_dongle_ui_config.h"
#include "ui.h"
#include "usb_dongle_sm.h"
#include "usb_dongle_buttons.h"

/* Needed for UI contexts - transitional; when table is code generated these can be anonymised
 * unsigned ints and these includes can be removed. */
#include "bt_device.h"
#include "media_player.h"
#include "sink_service.h"
#include "power_manager.h"
#include <focus_select.h>

static const focus_select_voice_tie_break_t usb_dongle_voice_source_focus_tie_break_order[] =
{
#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
    FOCUS_SELECT_VOICE_HFP,
#endif
#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
    FOCUS_SELECT_VOICE_LEA_UNICAST,
#endif
    FOCUS_SELECT_VOICE_USB
};

/*! \brief ui config table*/
const ui_config_table_content_t usb_dongle_ui_config_table[] =
{
    {LI_MFB_BUTTON_SINGLE_PRESS,           ui_provider_sink_service,    context_sink_connected,         ui_input_disconnect_sink        },
    {LI_MFB_BUTTON_SINGLE_PRESS,           ui_provider_sink_service,    context_sink_disconnected,      ui_input_connect_sink           },
    {LI_MFB_BUTTON_DOUBLE_PRESS,           ui_provider_power,           context_power_on,               ui_input_gaming_mode_toggle     },
    {LI_MFB_BUTTON_RELEASE_1SEC,           ui_provider_sink_service,    context_sink_connected,         ui_input_disconnect_sink        },
    {LI_MFB_BUTTON_RELEASE_1SEC,           ui_provider_sink_service,    context_sink_disconnected,      ui_input_connect_sink           },
    {LI_MFB_BUTTON_RELEASE_3SEC,           ui_provider_sink_service,    context_sink_connected,         ui_input_pair_sink              },
    {LI_MFB_BUTTON_RELEASE_3SEC,           ui_provider_sink_service,    context_sink_disconnected,      ui_input_pair_sink              },
    {LI_MFB_BUTTON_RELEASE_6SEC,           ui_provider_app_sm,          context_app_sm_idle,            ui_input_sm_delete_handsets     },
    {LI_MFB_BUTTON_RELEASE_6SEC,           ui_provider_app_sm,          context_app_sm_connected,       ui_input_sm_delete_handsets     },
    {LI_MFB_BUTTON_RELEASE_6SEC,           ui_provider_app_sm,          context_app_sm_streaming,       ui_input_sm_delete_handsets     },
    {LI_MFB_BUTTON_RELEASE_8SEC,           ui_provider_app_sm,          context_app_sm_idle,            ui_input_factory_reset_request  },
    {LI_MFB_BUTTON_RELEASE_8SEC,           ui_provider_app_sm,          context_app_sm_connected,       ui_input_factory_reset_request  },
    {LI_MFB_BUTTON_RELEASE_8SEC,           ui_provider_app_sm,          context_app_sm_streaming,       ui_input_factory_reset_request  },
};

const ui_config_table_content_t* UsbDongleUi_GetConfigTable(unsigned* table_length)
{
    *table_length = ARRAY_DIM(usb_dongle_ui_config_table);
    return usb_dongle_ui_config_table;
}

void UsbDongleUi_ConfigureFocusSelection(void)
{
    FocusSelect_ConfigureVoiceSourceTieBreakOrder(usb_dongle_voice_source_focus_tie_break_order);
}

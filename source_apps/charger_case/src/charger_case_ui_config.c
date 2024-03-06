/*!
\copyright  Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       charger_case_ui_config.c
\brief      ui configuration table

    This file contains ui configuration table which maps different logical inputs to
    corresponding ui inputs based upon ui provider contexts.
*/

#include "charger_case_ui_config.h"
#include "ui.h"
#include "charger_case_sm.h"
#include "charger_case_buttons.h"

/* Needed for UI contexts - transitional; when table is code generated these can be anonymised
 * unsigned ints and these includes can be removed. */
#include "bt_device.h"
#include "media_player.h"

/*! \brief ui config table*/
const ui_config_table_content_t charger_case_ui_config_table[] =
{
    {LI_MFB_BUTTON_SINGLE_PRESS,           ui_provider_app_sm,          context_app_sm_idle,          ui_input_connect_handset         },
    {LI_MFB_BUTTON_SINGLE_PRESS,           ui_provider_app_sm,          context_app_sm_connected,     ui_input_connect_handset         },
    {LI_MFB_BUTTON_SINGLE_PRESS,           ui_provider_app_sm,          context_app_sm_streaming,     ui_input_connect_handset         },
    {LI_MFB_BUTTON_RELEASE_1SEC,           ui_provider_app_sm,          context_app_sm_idle,          ui_input_connect_handset         },
    {LI_MFB_BUTTON_RELEASE_1SEC,           ui_provider_app_sm,          context_app_sm_connected,     ui_input_connect_handset         },
    {LI_MFB_BUTTON_RELEASE_1SEC,           ui_provider_app_sm,          context_app_sm_streaming,     ui_input_connect_handset         },
    {LI_MFB_BUTTON_RELEASE_3SEC,           ui_provider_app_sm,          context_app_sm_idle,          ui_input_sm_pair_handset         },
    {LI_MFB_BUTTON_RELEASE_3SEC,           ui_provider_app_sm,          context_app_sm_connected,     ui_input_sm_pair_handset         },
    {LI_MFB_BUTTON_RELEASE_3SEC,           ui_provider_app_sm,          context_app_sm_streaming,     ui_input_sm_pair_handset         },
    {LI_MFB_BUTTON_RELEASE_6SEC,           ui_provider_app_sm,          context_app_sm_idle,          ui_input_sm_delete_handsets      },
};

const ui_config_table_content_t* ChargerCaseUi_GetConfigTable(unsigned* table_length)
{
    *table_length = ARRAY_DIM(charger_case_ui_config_table);
    return charger_case_ui_config_table;
}

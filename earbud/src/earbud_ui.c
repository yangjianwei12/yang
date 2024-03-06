/*!
\copyright  Copyright (c) 2008 - 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_ui.c
\brief      Configure the Earbud Application User Interface Indicators
*/

#include "earbud_ui.h"

#include "earbud_prompts_config_table.h"
#include "earbud_tones_config_table.h"
#include "earbud_leds_config_table.h"

#include <ui_indicator_prompts.h>
#include <ui_indicator_tones.h>
#include <ui_indicator_leds.h>

/*! brief Initialise indicator module */
bool AppUi_Init(Task init_task)
{
    UNUSED(init_task);

#ifdef INCLUDE_PROMPTS
    UiPrompts_SetPromptConfiguration(
                app_ui_prompts_table,
                AppPromptsConfigTable_GetSize());
#endif

#ifdef INCLUDE_TONES
    UiTones_SetToneConfiguration(
                app_ui_tones_table,
                AppTonesConfigTable_SingleGetSize(),
                app_ui_repeating_tones_table,
                AppTonesConfigTable_RepeatingGetSize());
#endif

    UiLeds_SetLedConfiguration(
                app_ui_leds_table,
                AppLedsConfigTable_EventsTableGetSize(),
                app_ui_leds_context_indications_table,
                AppLedsConfigTable_ContextsTableGetSize());

    return TRUE;
}

